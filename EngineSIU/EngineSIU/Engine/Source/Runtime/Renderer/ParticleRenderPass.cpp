
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

FParticleRenderPass::FParticleRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
    , BlendState(nullptr)
    , DepthState(nullptr)
    , MeshInstanceBuffer(nullptr)
    , SpriteInstanceBuffer(nullptr)
    , SubImageCountX(6)
    , SubImageCountY(6)
    , ActiveTypes(0xFF) // 기본적으로 모든 타입 활성화
{
}

FParticleRenderPass::~FParticleRenderPass()
{
    if (BlendState) BlendState->Release();
    if (DepthState) DepthState->Release();
    if (MeshInstanceBuffer) MeshInstanceBuffer->Release();
    if (SpriteInstanceBuffer) SpriteInstanceBuffer->Release();
}

void FParticleRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    // 공통 상태 생성
    CreateBlendState();
    CreateDepthState();

    // 메쉬 파티클 초기화
    LoadMeshes();
    CreateMeshInstanceBuffer();

    // 스프라이트 파티클 초기화
    LoadTexture();
    CreateSpriteInstanceBuffer();
}

void FParticleRenderPass::PrepareRenderArr()
{
    // 각 타입별 파티클 준비
    if (ActiveTypes & (1 << static_cast<uint32>(EParticleSystemType::Mesh)))
    {
        PrepareMeshParticles();
    }

    if (ActiveTypes & (1 << static_cast<uint32>(EParticleSystemType::Sprite)))
    {
        PrepareSpriteParticles();
    }
}

void FParticleRenderPass::ClearRenderArr()
{
    MeshInstanceData.Empty();
    SpriteInstanceData.Empty();
}

void FParticleRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    const FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);
    Graphics->DeviceContext->RSSetViewports(1, &ViewportResource->GetD3DViewport());

    SetupPipeline();

    // 깊이 기반으로 렌더링 순서를 정하여 알파 블렌딩이 올바르게 작동하도록 함
    // Z값이 작은(카메라에 가까운) 파티클이 나중에 렌더링되어야 함
    
    // 메쉬 파티클 렌더링
    if (ActiveTypes & (1 << static_cast<uint32>(EParticleSystemType::Mesh)) && MeshInstanceData.Num() > 0)
    {
        RenderMeshParticles(Viewport);
    }

    // 스프라이트 파티클 렌더링
    if (ActiveTypes & (1 << static_cast<uint32>(EParticleSystemType::Sprite)) && SpriteInstanceData.Num() > 0)
    {
        RenderSpriteParticles(Viewport);
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

void FParticleRenderPass::SetupPipeline()
{
    Graphics->DeviceContext->OMSetBlendState(BlendState, nullptr, 0xFFFFFFFF);
    Graphics->DeviceContext->OMSetDepthStencilState(DepthState, 0);
}

void FParticleRenderPass::CreateBlendState()
{
    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Graphics->Device->CreateBlendState(&desc, &BlendState);
}

void FParticleRenderPass::CreateDepthState()
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;  // 알파 블렌딩을 위해 깊이 쓰기 비활성화
    desc.DepthFunc = D3D11_COMPARISON_LESS;

    Graphics->Device->CreateDepthStencilState(&desc, &DepthState);
}

void FParticleRenderPass::LoadMeshes()
{
    // 기존 메쉬 데이터 초기화
    MeshArray.Empty();
    
    // 방법 1: StaticMeshComponent에서 고유한 StaticMesh 얻기
    TSet<UStaticMesh*> UniqueMeshes;
    
    for (const auto Iter : TObjectRange<UStaticMeshComponent>())
    {
        if (Iter && Iter->GetStaticMesh() && Iter->GetStaticMesh()->GetRenderData())
        {
            UniqueMeshes.Add(Iter->GetStaticMesh());
        }
    }
    
    // Set에서 배열로 변환
    for (UStaticMesh* Mesh : UniqueMeshes)
    {
        MeshArray.Add(Mesh);
    }
    
    // 방법 2: AssetManager 사용 (GEngine이 유효할 때만)
    if (MeshArray.Num() == 0 && GEngine)
    {
            const TMap<FName, UStaticMesh*>& StaticMeshMap = UAssetManager::Get().GetStaticMeshMap();
            
            for (const auto& Pair : StaticMeshMap)
            {
                UStaticMesh* Mesh = Pair.Value;
                if (Mesh && Mesh->GetRenderData())
                {
                    MeshArray.Add(Mesh);
                }
            }
    }
}

void FParticleRenderPass::LoadTexture()
{
    ParticleTexture = FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/T_Explosion_SubUV.png");
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

void FParticleRenderPass::UpdateMeshInstanceBuffer()
{
    if (MeshInstanceData.Num() == 0)
        return;
        
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(Graphics->DeviceContext->Map(MeshInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, MeshInstanceData.GetData(), sizeof(FMeshParticleInstanceData) * MeshInstanceData.Num());
        Graphics->DeviceContext->Unmap(MeshInstanceBuffer, 0);
    }
}

void FParticleRenderPass::UpdateSpriteInstanceBuffer()
{
    if (SpriteInstanceData.Num() == 0)
        return;
        
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(Graphics->DeviceContext->Map(SpriteInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, SpriteInstanceData.GetData(), sizeof(FSpriteParticleInstanceData) * SpriteInstanceData.Num());
        Graphics->DeviceContext->Unmap(SpriteInstanceBuffer, 0);
    }
}

void FParticleRenderPass::PrepareMeshParticles()
{
    LoadMeshes();
    if (MeshArray.Num() == 0)
        return;
        
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> rotDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> scaleDist(0.5f, 2.0f);
    std::uniform_int_distribution<int> meshDist(0, MeshArray.Num() - 1);

    // 스프라이트 파티클 시스템처럼 더 많은 파티클 생성
    const int ParticleCount = 0; 
    MeshInstanceData.SetNum(ParticleCount);
    
    // 원점 주변에 랜덤하게 배치 (-100 ~ 100 범위)
    std::uniform_real_distribution<float> posDist(-100.0f, 100.0f);
    
    for (int i = 0; i < ParticleCount; ++i)
    {
        // 원점 주변에 랜덤 위치 생성
        FVector Position = FVector(posDist(rng), posDist(rng), posDist(rng));
        
        // 랜덤 회전 생성
        float Pitch = rotDist(rng);
        float Yaw = rotDist(rng);
        float Roll = rotDist(rng);
        
        FRotator Rotator(FMath::RadiansToDegrees(Pitch), FMath::RadiansToDegrees(Yaw), FMath::RadiansToDegrees(Roll));
        FMatrix RotationMatrix = FMatrix(FMatrix::CreateRotationMatrix(Rotator));
        
        // 랜덤 크기
        float randomScale = scaleDist(rng) * 0.8f;
        FVector Scale(randomScale, randomScale, randomScale);
        
        // 월드 변환 행렬 구성
        FMatrix TranslationMatrix = FMatrix::CreateTranslationMatrix(Position);
        
        // 랜덤 메쉬 선택
        int MeshIndex = meshDist(rng);
        
        // 인스턴스 데이터 설정
        FMeshParticleInstanceData& Instance = MeshInstanceData[i];
        FMatrix ScaleMatrix = FMatrix::CreateScaleMatrix(Scale);

        // 셰이더에서 World 하나만 사용하도록 통합
        Instance.World = ScaleMatrix * RotationMatrix * TranslationMatrix;
        Instance.Color = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f); // 빨간색
        Instance.MeshIndex = MeshIndex;
    }
    
    UpdateMeshInstanceBuffer();
}

void FParticleRenderPass::PrepareSpriteParticles()
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);

    const int ParticleCount = 0;
    SpriteInstanceData.SetNum(ParticleCount);
    
    for (int i = 0; i < ParticleCount; ++i)
    {
        FVector Pos = FVector(dist(rng), dist(rng), dist(rng));
        FMatrix World = FMatrix::CreateTranslationMatrix(Pos);

        FSpriteParticleInstanceData& Inst = SpriteInstanceData[i];
        Inst.World = World;
        Inst.Color = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f);
        
        int TotalFrames = SubImageCountX * SubImageCountY;
        Inst.SubImageIndex = FMath::RandRange(0, TotalFrames - 1);
    }
    
    UpdateSpriteInstanceBuffer();
}

void FParticleRenderPass::RenderMeshParticles(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (MeshArray.Num() == 0 || MeshInstanceData.Num() == 0)
        return;

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
    
    // 샘플러 생성 (없으면)
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
    rasterizerDesc.CullMode = D3D11_CULL_NONE; // 양면 렌더링을 위해 컬링 없음
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthClipEnable = TRUE;
    
    static ID3D11RasterizerState* rasterizerState = nullptr;
    if (!rasterizerState) {
        Graphics->Device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
    }
    
    Graphics->DeviceContext->RSSetState(rasterizerState);
    
    // 각 메쉬 별로 렌더링
    for (int i = 0; i < MeshArray.Num(); ++i)
    {
        UStaticMesh* Mesh = MeshArray[i];
        
        if (!Mesh || !Mesh->GetRenderData())
            continue;
            
        FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
        
        // 메쉬의 인스턴스만 필터링
        TArray<FMeshParticleInstanceData> FilteredInstances;
        for (const auto& Instance : MeshInstanceData)
        {
            if (Instance.MeshIndex == i)
            {
                FilteredInstances.Add(Instance);
            }
        }
        
        if (FilteredInstances.Num() == 0)
            continue;
        
        // 인스턴스 버퍼 업데이트
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(Graphics->DeviceContext->Map(MeshInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, FilteredInstances.GetData(), sizeof(FMeshParticleInstanceData) * FilteredInstances.Num());
            Graphics->DeviceContext->Unmap(MeshInstanceBuffer, 0);
        }
        
        // 인스턴스 버퍼 바인딩
        Graphics->DeviceContext->IASetVertexBuffers(1, 1, &MeshInstanceBuffer, &instanceStride, &instanceOffset);
        
        // 머티리얼 설정
        TArray<FStaticMaterial*> Materials = Mesh->GetMaterials();
        TArray<UMaterial*> OverrideMaterials; // 비어있음
        
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
            
            // 정점 버퍼 바인딩
            UINT vertexStride = sizeof(FStaticMeshVertex);
            UINT vertexOffset = 0;
            
            FVertexInfo VertexInfo;
            BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);
            Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &vertexStride, &vertexOffset);
            
            // 인덱스 버퍼 바인딩
            FIndexInfo IndexInfo;
            BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
            if (IndexInfo.IndexBuffer)
            {
                Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
            }
            
            // 인스턴스드 렌더링
            uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
            uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
            Graphics->DeviceContext->DrawIndexedInstanced(IndexCount, FilteredInstances.Num(), StartIndex, 0, 0);
        }
    }
}

void FParticleRenderPass::RenderSpriteParticles(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    LoadTexture();
    if (!ParticleTexture || SpriteInstanceData.Num() == 0)
        return;

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

    Graphics->DeviceContext->PSSetShaderResources(0, 1, &ParticleTexture->TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &ParticleTexture->SamplerState);
    
    Graphics->DeviceContext->DrawIndexedInstanced(QuadIB.NumIndices, SpriteInstanceData.Num(), 0, 0, 0);
}
