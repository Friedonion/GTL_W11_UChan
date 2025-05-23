#include "ParticleRenderPass.h"
#include "Engine/EditorEngine.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealClient.h"
#include "Engine/ResourceMgr.h"
#include "UObject/UObjectIterator.h"
#include "RendererHelpers.h"
#include <random>

#include "Math/Rotator.h"
#include "Define.h"
#include "Container/Map.h"
#include "Container/Pair.h"
#include "Engine/Asset/StaticMeshAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ParticleSystemComponent.h"
#include "Engine/ParticleEmitterInstances.h"
#include "Editor/LevelEditor/SLevelEditor.h"
#include "ParticleHelper.h"

#include "Particles/ParticleLODLevel.h"
#include "Particles/TypeData/ParticleModuleTypeDataMesh.h"
#include "Particles/ParticleModuleRequired.h"

FParticleRenderPass::FParticleRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
    , OpaqueBlendState(nullptr)
    , TransparentBlendState(nullptr)
    , OpaqueDepthState(nullptr)
    , TransparentDepthState(nullptr)
    , MeshInstanceBuffer(nullptr)
    , SpriteInstanceBuffer(nullptr)
    , SubImageCountX(1)
    , SubImageCountY(1)
    , ActiveTypes(0xFF) 
{
}

FParticleRenderPass::~FParticleRenderPass()
{
    if (OpaqueBlendState) OpaqueBlendState->Release();
    if (TransparentBlendState) TransparentBlendState->Release();
    if (OpaqueDepthState) OpaqueDepthState->Release();
    if (TransparentDepthState) TransparentDepthState->Release();
    if (MeshInstanceBuffer) MeshInstanceBuffer->Release();
    if (SpriteInstanceBuffer) SpriteInstanceBuffer->Release();
}

void FParticleRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    // 공통 상태 생성
    CreateBlendStates();
    CreateDepthStates();
    CreateMeshInstanceBuffer();
    CreateSpriteInstanceBuffer();
}

void FParticleRenderPass::PrepareRenderArr()
{
    // TObjectRange를 사용하여 월드의 모든 파티클 시스템 컴포넌트 수집
    ParticleSystemComponents.Empty();
    
    for (const auto Iter : TObjectRange<UParticleSystemComponent>())
    {
        // 현재 활성 월드에 있는 컴포넌트만 추가
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            if (Iter->GetOwner() && !Iter->GetOwner()->IsHidden())
            {
                ParticleSystemComponents.Add(Iter);
            }
        }
    }
}

void FParticleRenderPass::ClearRenderArr()
{
    OpaqueMeshInstanceData.Empty();
    TransparentMeshInstanceData.Empty();
    OpaqueSpriteInstanceData.Empty();
    TransparentSpriteInstanceData.Empty();
}

void FParticleRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    const FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);
    Graphics->DeviceContext->RSSetViewports(1, &ViewportResource->GetD3DViewport());

    for (UParticleSystemComponent* Comp : ParticleSystemComponents)
    {
        for (FParticleEmitterInstance* EmitterInstance : Comp->EmitterInstances)
        {
            SubImageCountX = EmitterInstance->CurrentLODLevel->RequiredModule->SubImages_Horizontal;
            SubImageCountY = EmitterInstance->CurrentLODLevel->RequiredModule->SubImages_Vertical;
            if (EmitterInstance->CurrentLODLevel->TypeDataModule && CastChecked<UParticleModuleTypeDataMesh>(EmitterInstance->CurrentLODLevel->TypeDataModule))
            {
                // 메쉬
                if (ActiveTypes & (1 << static_cast<uint32>(EParticleSystemType::Mesh)))
                {
                    PrepareMeshParticles(EmitterInstance);

                    SortParticlesByDistance(TransparentMeshInstanceData);
                    UParticleModuleTypeDataMesh* TypeDataMesh = (UParticleModuleTypeDataMesh*)(EmitterInstance->CurrentLODLevel->TypeDataModule);
                    if (!TypeDataMesh->Mesh) return;
                    UpdateMeshInstanceBuffer(true);
                    RenderMeshParticles(Viewport, false, TypeDataMesh->Mesh);

                    UpdateMeshInstanceBuffer(false);
                    RenderMeshParticles(Viewport, true, TypeDataMesh->Mesh);
                }
            }
            else
            {
                if (!EmitterInstance->CurrentLODLevel->RequiredModule->Material) return;
                if (EmitterInstance->CurrentLODLevel->RequiredModule->Material->GetMaterialInfo().TextureInfos.IsEmpty()) return;

                std::shared_ptr<FTexture> SpriteTexture = FEngineLoop::ResourceManager.GetTexture(EmitterInstance->CurrentLODLevel->RequiredModule->Material->GetMaterialInfo().TextureInfos[0].TexturePath);
                
                if (!SpriteTexture) return;
                // 일반적인 스프라이트
                if (ActiveTypes & (1 << static_cast<uint32>(EParticleSystemType::Sprite)))
                {
                    PrepareSpriteParticles(EmitterInstance);

                    SortParticlesByDistance(TransparentSpriteInstanceData);

                    UpdateSpriteInstanceBuffer(true);
                    RenderSpriteParticles(Viewport, false, SpriteTexture);

                    UpdateSpriteInstanceBuffer(false);
                    RenderSpriteParticles(Viewport, true, SpriteTexture);
                }
            }
        }
    }

    // 렌더 타겟 및 상태 해제
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    Graphics->DeviceContext->OMSetDepthStencilState(nullptr, 0);

    // 셰이더 리소스 해제
    ID3D11ShaderResourceView* NullSRVs[2] = { nullptr, nullptr };
    Graphics->DeviceContext->PSSetShaderResources(0, 2, NullSRVs);
    ID3D11SamplerState* NullSamplers[1] = { nullptr };
    Graphics->DeviceContext->PSSetSamplers(0, 1, NullSamplers);
}

void FParticleRenderPass::SetParticleTypeActive(EParticleSystemType Type, bool bActive)
{
    uint32 TypeBit = 1 << static_cast<uint32>(Type);
    
    if (bActive)
    {
        ActiveTypes |= TypeBit;
    }
    else
    {
        ActiveTypes &= ~TypeBit;
    }
}

void FParticleRenderPass::SetupPipeline(bool bIsTransparent)
{
    if (bIsTransparent)
    {
        Graphics->DeviceContext->OMSetBlendState(TransparentBlendState, nullptr, 0xFFFFFFFF);
        Graphics->DeviceContext->OMSetDepthStencilState(TransparentDepthState, 0);
    }
    else
    {
        Graphics->DeviceContext->OMSetBlendState(OpaqueBlendState, nullptr, 0xFFFFFFFF);
        Graphics->DeviceContext->OMSetDepthStencilState(OpaqueDepthState, 0);
    }
}

void FParticleRenderPass::CreateBlendStates()
{
    // 불투명 객체용 블렌드 상태
    D3D11_BLEND_DESC opaqueDesc = {};
    opaqueDesc.RenderTarget[0].BlendEnable = FALSE;
    opaqueDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    Graphics->Device->CreateBlendState(&opaqueDesc, &OpaqueBlendState);
    
    // 반투명 객체용 블렌드 상태
    D3D11_BLEND_DESC transparentDesc = {};
    transparentDesc.RenderTarget[0].BlendEnable = TRUE;
    transparentDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    transparentDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    transparentDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    transparentDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    transparentDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    Graphics->Device->CreateBlendState(&transparentDesc, &TransparentBlendState);
}

void FParticleRenderPass::CreateDepthStates()
{
    // 불투명 객체용 깊이 상태 (깊이 쓰기 활성화)
    D3D11_DEPTH_STENCIL_DESC opaqueDesc = {};
    opaqueDesc.DepthEnable = TRUE;
    opaqueDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;  // 깊이 쓰기 활성화
    opaqueDesc.DepthFunc = D3D11_COMPARISON_LESS;
    Graphics->Device->CreateDepthStencilState(&opaqueDesc, &OpaqueDepthState);
    
    // 반투명 객체용 깊이 상태 (깊이 쓰기 비활성화)
    D3D11_DEPTH_STENCIL_DESC transparentDesc = {};
    transparentDesc.DepthEnable = TRUE;
    transparentDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // 깊이 쓰기 비활성화
    transparentDesc.DepthFunc = D3D11_COMPARISON_LESS;
    Graphics->Device->CreateDepthStencilState(&transparentDesc, &TransparentDepthState);
}

void FParticleRenderPass::CreateMeshInstanceBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(FMeshParticleInstanceData) * 11000; // 10000개 + 여유 공간
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Graphics->Device->CreateBuffer(&desc, nullptr, &MeshInstanceBuffer);
}

void FParticleRenderPass::CreateSpriteInstanceBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(FSpriteParticleInstanceData) * 11000; // 10000개 + 여유 공간
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Graphics->Device->CreateBuffer(&desc, nullptr, &SpriteInstanceBuffer);
}

void FParticleRenderPass::UpdateMeshInstanceBuffer(bool bIsOpaque)
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(Graphics->DeviceContext->Map(MeshInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        if (bIsOpaque = true) 
        {
        memcpy(mapped.pData, OpaqueMeshInstanceData.GetData(), sizeof(FMeshParticleInstanceData) * OpaqueMeshInstanceData.Num());
        }
        else
        {
        memcpy(mapped.pData, TransparentMeshInstanceData.GetData(), sizeof(FMeshParticleInstanceData) * TransparentMeshInstanceData.Num());
        }
        Graphics->DeviceContext->Unmap(MeshInstanceBuffer, 0);
    }
}

void FParticleRenderPass::UpdateSpriteInstanceBuffer(bool bIsOpaque)
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(Graphics->DeviceContext->Map(SpriteInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        if(bIsOpaque)
        {
            memcpy(mapped.pData, OpaqueSpriteInstanceData.GetData(), sizeof(FSpriteParticleInstanceData) * OpaqueSpriteInstanceData.Num());
        }
        else
        {
            memcpy(mapped.pData, TransparentSpriteInstanceData.GetData(), sizeof(FSpriteParticleInstanceData) * TransparentSpriteInstanceData.Num());
        }
        Graphics->DeviceContext->Unmap(SpriteInstanceBuffer, 0);
    }
}

void FParticleRenderPass::PrepareMeshParticles(FParticleEmitterInstance* EmitterInstance)
{
    // 파티클 시스템 컴포넌트에서 데이터 수집
    OpaqueMeshInstanceData.Empty();
    TransparentMeshInstanceData.Empty();
    
    // 뷰포트 클라이언트를 통해 카메라 위치 가져오기
    FVector CameraLocation = FVector::ZeroVector;
    if (auto EditorEngine = Cast<UEditorEngine>(GEngine))
    {
        const std::shared_ptr<FEditorViewportClient> ViewportClient = EditorEngine->GetLevelEditor()->GetActiveViewportClient();
        if (ViewportClient)
        {
            CameraLocation = ViewportClient->GetCameraLocation();
        }
    }
    
    
    if (!EmitterInstance) return;
            
    // 임시 데이터 컨테이너
    TArray<FMeshParticleInstanceData> tempMeshData;
    TArray<FSpriteParticleInstanceData> tempSpriteData; // 사용하지 않지만 GatherParticleInstanceData 함수의 인자로 필요
        
    // 메시 파티클만 수집합니다
    GatherParticleInstanceData(EmitterInstance, tempMeshData, tempSpriteData);
        
    // 투명도에 따라 분류
    for (const auto& Instance : tempMeshData)
    {
        // 파티클 위치 추출 (월드 행렬의 이동 벡터)
        FVector ParticleLocation = FVector(Instance.World.M[3][0], Instance.World.M[3][1], Instance.World.M[3][2]);
            
        // 불투명/반투명 판단 및 분류
        FMeshParticleInstanceData NewInstance = Instance;
        NewInstance.DistanceToCamera = CalculateDistanceToCamera(ParticleLocation, CameraLocation);
            
        if (IsTransparent(Instance.Color))
        {
            TransparentMeshInstanceData.Add(NewInstance);
        }
        else
        {
            OpaqueMeshInstanceData.Add(NewInstance);
        }
    }
}

void FParticleRenderPass::PrepareSpriteParticles(FParticleEmitterInstance* EmitterInstance)
{
    // 파티클 시스템 컴포넌트에서 데이터 수집
    OpaqueSpriteInstanceData.Empty();
    TransparentSpriteInstanceData.Empty();
    
    // 뷰포트 클라이언트를 통해 카메라 위치 가져오기
    FVector CameraLocation = FVector::ZeroVector;
    if (auto EditorEngine = Cast<UEditorEngine>(GEngine))
    {
        const std::shared_ptr<FEditorViewportClient> ViewportClient = EditorEngine->GetLevelEditor()->GetActiveViewportClient();
        if (ViewportClient)
        {
            CameraLocation = ViewportClient->GetCameraLocation();
        }
    }
    
    if (!EmitterInstance) return;
            
    // 임시 데이터 컨테이너
    TArray<FMeshParticleInstanceData> tempMeshData; // 사용하지 않지만 GatherParticleInstanceData 함수의 인자로 필요
    TArray<FSpriteParticleInstanceData> tempSpriteData;
        
    // 스프라이트 파티클만 수집합니다
    GatherParticleInstanceData(EmitterInstance, tempMeshData, tempSpriteData);
        
    // 투명도에 따라 분류
    for (const auto& Instance : tempSpriteData)
    {
        // 파티클 위치 추출 (월드 행렬의 이동 벡터)
        FVector ParticleLocation = FVector(Instance.World.M[3][0], Instance.World.M[3][1], Instance.World.M[3][2]);
            
        // 불투명/반투명 판단 및 분류
        FSpriteParticleInstanceData NewInstance = Instance;
        NewInstance.DistanceToCamera = CalculateDistanceToCamera(ParticleLocation, CameraLocation);
            
        if (IsTransparent(Instance.Color))
        {
            TransparentSpriteInstanceData.Add(NewInstance);
        }
        else
        {
            OpaqueSpriteInstanceData.Add(NewInstance);
        }
    }
}

void FParticleRenderPass::RenderMeshParticles(const std::shared_ptr<FEditorViewportClient>& Viewport, bool bIsTransparent, UStaticMesh* Mesh)
{
    // 렌더링할 파티클 데이터 선택
    TArray<FMeshParticleInstanceData>& InstanceData = bIsTransparent ? TransparentMeshInstanceData : OpaqueMeshInstanceData;
        
    // 투명도에 따른 렌더 상태 설정
    SetupPipeline(bIsTransparent);

    // 인스턴스 버퍼 설정
    UINT instanceStride = sizeof(FMeshParticleInstanceData);
    UINT instanceOffset = 0;
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // 셰이더 설정
    Graphics->DeviceContext->IASetInputLayout(ShaderManager->GetInputLayoutByKey(L"MeshParticleVS"));
    Graphics->DeviceContext->VSSetShader(ShaderManager->GetVertexShaderByKey(L"MeshParticleVS"), nullptr, 0);
    Graphics->DeviceContext->PSSetShader(ShaderManager->GetPixelShaderByKey(L"MeshParticlePS"), nullptr, 0);
    
    // 샘플러 상태 설정
    ID3D11SamplerState* samplerState;
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    static ID3D11SamplerState* defaultSampler = nullptr;
    if (!defaultSampler) {
        Graphics->Device->CreateSamplerState(&sampDesc, &defaultSampler);
    }
    
    // 샘플러 설정
    Graphics->DeviceContext->PSSetSamplers(0, 1, &defaultSampler);
    
    // 카메라 상수 버퍼 설정
    BufferManager->BindConstantBuffer(TEXT("FCameraConstantBuffer"), 13, EShaderStage::Vertex);
    
    // 레스터라이저 상태 설정
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthClipEnable = TRUE;
    
    static ID3D11RasterizerState* rasterizerState = nullptr;
    if (!rasterizerState) {
        Graphics->Device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
    }
    
    Graphics->DeviceContext->RSSetState(rasterizerState);
    
    if (!Mesh || !Mesh->GetRenderData()) return;
            
    FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
        
    TArray<FMeshParticleInstanceData> FilteredInstances;
    for (const auto& Instance : InstanceData)
    {
        FilteredInstances.Add(Instance);
    }
        
    if (FilteredInstances.Num() == 0) return;
        
    // 인스턴스 버퍼 바인딩
    Graphics->DeviceContext->IASetVertexBuffers(1, 1, &MeshInstanceBuffer, &instanceStride, &instanceOffset);
        
    // 머티리얼 설정
    TArray<FStaticMaterial*> Materials = Mesh->GetMaterials();
        
    // 각 서브메쉬 렌더링
    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        uint32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;
            
        // 서브메쉬 상수 업데이트
        FSubMeshConstants SubMeshData = FSubMeshConstants(false);
        BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);
            
        if (!Materials.IsEmpty() && Materials.Num() > MaterialIndex && Materials[MaterialIndex] != nullptr)
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, Materials[MaterialIndex]->Material->GetMaterialInfo());
        }
            
        UINT vertexStride = sizeof(FStaticMeshVertex);
        UINT vertexOffset = 0;
            
        FVertexInfo VertexInfo;
        BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);
        Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &vertexStride, &vertexOffset);
            
        FIndexInfo IndexInfo;
        BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
        if (IndexInfo.IndexBuffer)
        {
            Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        }
            
        uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexedInstanced(IndexCount, FilteredInstances.Num(), StartIndex, 0, 0);
    }
}

void FParticleRenderPass::RenderSpriteParticles(const std::shared_ptr<FEditorViewportClient>& Viewport, bool bIsTransparent, std::shared_ptr<FTexture> SpriteTexture)
{
    TArray<FSpriteParticleInstanceData>& InstanceData = bIsTransparent ? TransparentSpriteInstanceData : OpaqueSpriteInstanceData;
    
    // 투명도에 따른 렌더 상태 설정
    SetupPipeline(bIsTransparent);

    FVertexInfo QuadVB;
    FIndexInfo QuadIB;
    BufferManager->GetQuadBuffer(QuadVB, QuadIB);

    if (!QuadVB.VertexBuffer || !QuadIB.IndexBuffer) 
        return;

    UINT stride = QuadVB.Stride;
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &QuadVB.VertexBuffer, &stride, &offset);

    UINT instanceStride = sizeof(FSpriteParticleInstanceData);
    UINT instanceOffset = 0;
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    
    Graphics->DeviceContext->IASetVertexBuffers(1, 1, &SpriteInstanceBuffer, &instanceStride, &instanceOffset);
    Graphics->DeviceContext->IASetIndexBuffer(QuadIB.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    Graphics->DeviceContext->IASetInputLayout(
        ShaderManager->GetInputLayoutByKey(L"ParticleSpriteVS"));

    Graphics->DeviceContext->VSSetShader(
        ShaderManager->GetVertexShaderByKey(L"ParticleSpriteVS"), nullptr, 0);

    Graphics->DeviceContext->PSSetShader(
        ShaderManager->GetPixelShaderByKey(L"ParticleSpritePS"), nullptr, 0);

    FParticleConstant SubUVData = { SubImageCountX, SubImageCountY };
    BufferManager->UpdateConstantBuffer(TEXT("FParticleConstant"), SubUVData);
    
    BufferManager->BindConstantBuffer(TEXT("FParticleConstant"), 7, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FCameraConstantBuffer"), 13, EShaderStage::Vertex);

    Graphics->DeviceContext->PSSetShaderResources(0, 1, &SpriteTexture->TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &SpriteTexture->SamplerState);
    
    Graphics->DeviceContext->DrawIndexedInstanced(QuadIB.NumIndices, InstanceData.Num(), 0, 0, 0);
}

bool FParticleRenderPass::GatherParticleInstanceData(FParticleEmitterInstance* EmitterInstance,
                                                       TArray<FMeshParticleInstanceData>& OutMeshInstanceData,
                                                       TArray<FSpriteParticleInstanceData>& OutSpriteInstanceData)
{
    if (!EmitterInstance || !EmitterInstance->ActiveParticles)
        return false;

        // 데이터를 적절한 출력 배열에 수집하도록 매개변수 전달
    ProcessParticleEmitter(EmitterInstance, OutMeshInstanceData, OutSpriteInstanceData);

    return OutMeshInstanceData.Num() > 0 || OutSpriteInstanceData.Num() > 0;
}

void FParticleRenderPass::ProcessParticleEmitter(FParticleEmitterInstance* EmitterInstance,
                                                TArray<FMeshParticleInstanceData>& OutMeshInstanceData,
                                                TArray<FSpriteParticleInstanceData>& OutSpriteInstanceData)
{
    if (!EmitterInstance)
        return;

    // 이미터 인스턴스에 활성화된 파티클이 없으면 건너뛰기
    int32 ActiveParticles = EmitterInstance->ActiveParticles;
    if (ActiveParticles <= 0)
        return;

    uint8* ParticleData = EmitterInstance->ParticleData;
    uint32 ParticleStride = EmitterInstance->ParticleStride;
    uint16* ParticleIndices = EmitterInstance->ParticleIndices;

    // @todo TypeDataModule의 종류에 따라 처리 가능하도록 수정 필요 (지금은 TypeDataModule이 있는 경우 Mesh로 처리중임)
    // 이미터 타입에 따라 처리
    bool bIsMesh = false;
    if (EmitterInstance->CurrentLODLevel->TypeDataModule && CastChecked<UParticleModuleTypeDataMesh>(EmitterInstance->CurrentLODLevel->TypeDataModule))
    {
        bIsMesh = true;
    }

    // 각 파티클 처리
    for (int32 i = 0; i < ActiveParticles; i++)
    {
        int32 CurrentIndex = ParticleIndices[i];
        FBaseParticle* Particle = (FBaseParticle*)(ParticleData + CurrentIndex * ParticleStride);
        
        if (!Particle)
            continue;

        // 파티클 변환 행렬 계산
        FMatrix ParticleWorldMatrix = CalculateParticleTransform(Particle, bIsMesh);

        if (bIsMesh)
        {
            // 메시 파티클 인스턴스 데이터 생성
            FMeshParticleInstanceData MeshInstance;
            MeshInstance.World = ParticleWorldMatrix;
            MeshInstance.Color = Particle->Color;
            MeshInstance.MeshIndex =0;

            OutMeshInstanceData.Add(MeshInstance);
        }
        else
        {
            // 스프라이트 파티클 인스턴스 데이터 생성
            FSpriteParticleInstanceData SpriteInstance;
            SpriteInstance.World = ParticleWorldMatrix;
            SpriteInstance.Color = Particle->Color;
            
            // SubUV계산
            const uint8* ParticleBase	= ParticleData + CurrentIndex * ParticleStride;
            uint32 CurrentOffset = sizeof(FBaseParticle);
            PARTICLE_ELEMENT(FFullSubUVPayload, SubUVPayload)
            SpriteInstance.SubImageIndex = SubUVPayload.ImageIndex;
            
            // 간단한 구현을 위해 SubImageIndex를 상대적 시간을 기반으로 설정
            // int TotalFrames = SubImageCountX * SubImageCountY;
            // SpriteInstance.SubImageIndex = static_cast<int>(Particle->RelativeTime * TotalFrames) % TotalFrames;

            // 수정: 멤버 변수 대신 출력 매개변수에 추가
            OutSpriteInstanceData.Add(SpriteInstance);
        }
    }
}

FMatrix FParticleRenderPass::CalculateParticleTransform(const FBaseParticle* Particle, bool bIsMesh)
{
    if (!Particle)
        return FMatrix::Identity;

    // 파티클 위치, 크기, 회전을 기반으로 변환 행렬 계산
    FVector Scale = Particle->Size;
    
    // 회전 행렬 계산
    FRotator Rotator(0.0f, 0.0f, FMath::RadiansToDegrees(Particle->Rotation));
    FMatrix RotationMatrix = FMatrix::CreateRotationMatrix(Rotator);
    
    // 위치 행렬 계산
    FMatrix TranslationMatrix = FMatrix::CreateTranslationMatrix(Particle->Location);
    
    // 크기 행렬 계산
    FMatrix ScaleMatrix = FMatrix::CreateScaleMatrix(Scale);

    // 최종 변환 행렬 계산: Scale * Rotation * Translation
    return ScaleMatrix * RotationMatrix * TranslationMatrix;
}

// 파티클의 투명도를 확인하는 함수
bool FParticleRenderPass::IsTransparent(const FLinearColor& Color) const
{
    constexpr float AlphaThreshold = 0.99f;
    return Color.A < AlphaThreshold;
}

// 파티클을 카메라와의 거리에 따라 정렬하는 함수 (메쉬 파티클)
void FParticleRenderPass::SortParticlesByDistance(TArray<FMeshParticleInstanceData>& Particles)
{
    // 내림차순 정렬 (카메라에서 멀리 있는 파티클이 먼저 렌더링되도록)
    Particles.Sort([](const FMeshParticleInstanceData& A, const FMeshParticleInstanceData& B) -> bool {
        return A.DistanceToCamera > B.DistanceToCamera;
    });
}

// 파티클을 카메라와의 거리에 따라 정렬하는 함수 (스프라이트 파티클)
void FParticleRenderPass::SortParticlesByDistance(TArray<FSpriteParticleInstanceData>& Particles)
{
    // 내림차순 정렬 (카메라에서 멀리 있는 파티클이 먼저 렌더링되도록)
    Particles.Sort([](const FSpriteParticleInstanceData& A, const FSpriteParticleInstanceData& B) -> bool {
        return A.DistanceToCamera > B.DistanceToCamera;
    });
}

// 파티클과 카메라 사이의 거리를 계산하는 함수
float FParticleRenderPass::CalculateDistanceToCamera(const FVector& ParticleLocation, const FVector& CameraLocation)
{
    // 두 점 사이의 거리 계산
    return FVector::Distance(ParticleLocation, CameraLocation);
}
