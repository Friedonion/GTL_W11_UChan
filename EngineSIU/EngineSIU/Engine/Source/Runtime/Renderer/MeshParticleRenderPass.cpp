// MeshParticleRenderPass.cpp

#include "MeshParticleRenderPass.h"
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

FMeshParticleRenderPass::FMeshParticleRenderPass()
    : InstanceBuffer(nullptr)
    , BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
    , BlendState(nullptr)
    , DepthState(nullptr)
{
}

FMeshParticleRenderPass::~FMeshParticleRenderPass()
{
    if (BlendState) BlendState->Release();
    if (DepthState) DepthState->Release();
    if (InstanceBuffer) InstanceBuffer->Release();
}

void FMeshParticleRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    LoadRandomMeshes();
    CreateBlendState();
    CreateDepthState();
    CreateInstanceBuffer();
}

void FMeshParticleRenderPass::PrepareRenderArr()
{
        LoadRandomMeshes();
    
    // 여전히 메쉬가 없다면 렌더링 스킵
    if (RandomMeshes.Num() == 0)
        return;
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> rotDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> scaleDist(0.5f, 2.0f);
    std::uniform_int_distribution<int> meshDist(0, RandomMeshes.Num() - 1);

    // 스프라이트 파티클 시스템처럼 더 많은 파티클 생성
    const int ParticleCount = 10000; // 보통 더 많이 생성하지만 디버깅을 위해 100개로 제한
    InstanceData.SetNum(ParticleCount);
    
    // 원점 주변에 랜덤하게 배치 (-25 ~ 25 범위)
    std::uniform_real_distribution<float> posSmallerDist(-100.0f, 100.0f);
    
    for (int i = 0; i < ParticleCount; ++i)
    {
        // 원점 주변에 랜덤 위치 생성 (스프라이트 파티클과 유사하게)
        FVector Position = FVector(posSmallerDist(rng), posSmallerDist(rng), posSmallerDist(rng));
        
        // 랜덤 회전 생성 (완전 랜덤)
        float Pitch = rotDist(rng);     







        float Yaw = rotDist(rng);
        float Roll = rotDist(rng);
        
        FRotator Rotator(FMath::RadiansToDegrees(Pitch), FMath::RadiansToDegrees(Yaw), FMath::RadiansToDegrees(Roll));
        FMatrix RotationMatrix = FMatrix(FMatrix::CreateRotationMatrix(Rotator));
        
        // 랜덤 크기 (조금 더 작게)
        float randomScale = scaleDist(rng) * 0.8f;
        FVector Scale(randomScale, randomScale, randomScale);
        
        // 월드 변환 행렬 구성
        FMatrix TranslationMatrix = FMatrix::CreateTranslationMatrix(Position);
        
        // 랜덤 메쉬 선택
        int MeshIndex = meshDist(rng);
        
        // 인스턴스 데이터 설정
        FMeshParticleInstanceData& Instance = InstanceData[i];
        FMatrix ScaleMatrix = FMatrix::CreateScaleMatrix(Scale);

        // 셰이더에서 World 하나만 사용하도록 통합
        Instance.World = ScaleMatrix * RotationMatrix * TranslationMatrix;

        // 매우 선명한 색상 사용 (빨간색으로 변경)
        Instance.Color = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
        Instance.MeshIndex = MeshIndex;
    }
    
    UpdateInstanceBuffer();
}

void FMeshParticleRenderPass::ClearRenderArr()
{
    InstanceData.Empty();
}

void FMeshParticleRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (RandomMeshes.Num() == 0 || InstanceData.Num() == 0)
        return;
    
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    const FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);
    Graphics->DeviceContext->RSSetViewports(1, &ViewportResource->GetD3DViewport());

    SetupPipeline();
    RenderParticles(Viewport);

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

void FMeshParticleRenderPass::SetupPipeline()
{
    Graphics->DeviceContext->OMSetBlendState(BlendState, nullptr, 0xFFFFFFFF);
    Graphics->DeviceContext->OMSetDepthStencilState(DepthState, 0);
}

void FMeshParticleRenderPass::CreateBlendState()
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

void FMeshParticleRenderPass::CreateDepthState()
{
    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_LESS;

    Graphics->Device->CreateDepthStencilState(&desc, &DepthState);
}

void FMeshParticleRenderPass::CreateInstanceBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(FMeshParticleInstanceData) * 11000; // 10000개 + 여유 공간
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Graphics->Device->CreateBuffer(&desc, nullptr, &InstanceBuffer);
}

void FMeshParticleRenderPass::UpdateInstanceBuffer()
{
    if (InstanceData.Num() == 0)
        return;
        
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(Graphics->DeviceContext->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, InstanceData.GetData(), sizeof(FMeshParticleInstanceData) * InstanceData.Num());
        Graphics->DeviceContext->Unmap(InstanceBuffer, 0);
    }
}

void FMeshParticleRenderPass::LoadRandomMeshes()
{
    // 기존 메쉬 데이터 초기화
    RandomMeshes.Empty();
    
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
        RandomMeshes.Add(Mesh);
    }
    
    // 방법 2: AssetManager 사용 (GEngine이 유효할 때만)
    if (RandomMeshes.Num() == 0 && GEngine)
    {
        try
        {
            const TMap<FName, UStaticMesh*>& StaticMeshMap = UAssetManager::Get().GetStaticMeshMap();
            
            for (const auto& Pair : StaticMeshMap)
            {
                UStaticMesh* Mesh = Pair.Value;
                if (Mesh && Mesh->GetRenderData())
                {
                    RandomMeshes.Add(Mesh);
                }
            }
        }
        catch (...)
        {
            // AssetManager 접근 예외 무시
        }
    }
    
    // 방법 3: 테스트용 큐브 메쉬 생성
    // 메쉬를 찾지 못한 경우에도 진행할 수 있도록 처리
    if (RandomMeshes.Num() == 0)
    {
        // 실제 메쉬를 생성하는 방법이 없으므로, 일단 비어있는 상태로 둡니다.
        // 메쉬가 없으면 PrepareRenderArr()에서 일찍 리턴되어 렌더링이 되지 않을 것입니다.
        // UE_LOG(LogRenderer, Warning, TEXT("No meshes found for mesh particle system!"));
    }
    
    // 디버깅: 로드된 메쉬 확인
    if (RandomMeshes.Num() > 0)
    {
        // 로그 출력 코드 (있다면)
        // UE_LOG(LogRenderer, Log, TEXT("Loaded %d meshes for mesh particle system"), RandomMeshes.Num());
    }
}


void FMeshParticleRenderPass::RenderParticles(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (RandomMeshes.Num() == 0 || InstanceData.Num() == 0)
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
    
    // 디폴트 텍스처 설정 (흰색 텍스처)
    static ID3D11ShaderResourceView* defaultTextureSRV = nullptr;
    // TODO: 디폴트 텍스처 로드 또는 생성 로직 추가
    
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
    for (int i = 0; i < RandomMeshes.Num(); ++i)
    {
        UStaticMesh* Mesh = RandomMeshes[i];

        if (!Mesh || !Mesh->GetRenderData())
            continue;
            
        FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
        
        // 메쉬의 인스턴스만 필터링
        TArray<FMeshParticleInstanceData> FilteredInstances;
        for (const auto& Instance : InstanceData)
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
        if (SUCCEEDED(Graphics->DeviceContext->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, FilteredInstances.GetData(), sizeof(FMeshParticleInstanceData) * FilteredInstances.Num());
            Graphics->DeviceContext->Unmap(InstanceBuffer, 0);
        }
        
        // 인스턴스 버퍼 바인딩
        Graphics->DeviceContext->IASetVertexBuffers(1, 1, &InstanceBuffer, &instanceStride, &instanceOffset);
        
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
