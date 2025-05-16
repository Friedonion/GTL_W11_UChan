#include "SkeletalMeshRenderPassBase.h"

#include "Animation/Skeleton.h"
#include "UObject/UObjectIterator.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/Casts.h"
#include "Editor/PropertyEditor/ShowFlags.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "Engine/AssetManager.h"
#include "RendererHelpers.h"

class UEditorEngine;

FSkeletalMeshRenderPassBase::FSkeletalMeshRenderPassBase()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FSkeletalMeshRenderPassBase::~FSkeletalMeshRenderPassBase()
{
    ReleaseResource();
}

void FSkeletalMeshRenderPassBase::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateResource();
}

void FSkeletalMeshRenderPassBase::InitializeShadowManager(class FShadowManager* InShadowManager)
{
    ShadowManager = InShadowManager;
}

void FSkeletalMeshRenderPassBase::PrepareRenderArr()
{
    for (const auto Iter : TObjectRange<USkeletalMeshComponent>())
    {
        if (Iter->GetWorld() != GEngine->ActiveWorld)
        {
            continue;
        }
        SkeletalMeshComponents.Add(Iter);
    }
}

void FSkeletalMeshRenderPassBase::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRenderPass(Viewport);

    Render_Internal(Viewport);

    CleanUpRenderPass(Viewport);
}

void FSkeletalMeshRenderPassBase::ClearRenderArr()
{
    SkeletalMeshComponents.Empty();
}

void FSkeletalMeshRenderPassBase::CreateResource()
{
    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = sizeof(FMatrix) * MaxBoneNum;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(FMatrix);
    
    // 6. 버퍼 생성 및 쉐이더 리소스 뷰 생성
    // BufferManager를 통해 버퍼 생성 요청
    HRESULT Result = Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &BoneBuffer);
    if (FAILED(Result))
    {
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SrvDesc.Buffer.FirstElement = 0;
    SrvDesc.Buffer.NumElements = MaxBoneNum;
    
    Result = Graphics->Device->CreateShaderResourceView(BoneBuffer, nullptr, &BoneSRV);
    if (FAILED(Result))
    {
        return;
    }
}

void FSkeletalMeshRenderPassBase::ReleaseResource()
{
    if (BoneSRV)
    {
        BoneSRV->Release();
        BoneSRV = nullptr;
    }

    if (BoneBuffer)
    {
        BoneBuffer->Release();
        BoneBuffer = nullptr;
    }
}

void FSkeletalMeshRenderPassBase::Render_Internal(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    RenderAllSkeletalMeshes(Viewport);
}

void FSkeletalMeshRenderPassBase::RenderAllSkeletalMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    for (const USkeletalMeshComponent* Comp : SkeletalMeshComponents)
    {
        if (!Comp || !Comp->GetSkeletalMeshAsset())
        {
            continue;
        }
        const FSkeletalMeshRenderData* RenderData = Comp->GetCPUSkinning() ? Comp->GetCPURenderData() : Comp->GetSkeletalMeshAsset()->GetRenderData();
        if (RenderData == nullptr)
        {
            continue;
        }

        const UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        FVector4 UUIDColor = Comp->EncodeUUID() / 255.0f;
        const bool bIsSelected = (Engine && Engine->GetSelectedActor() == Comp->GetOwner());

        UpdateObjectConstant(WorldMatrix, UUIDColor, bIsSelected);

        UpdateBone(Comp);

        RenderSkeletalMesh(RenderData);

        if (Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_AABB))
        {
            FEngineLoop::PrimitiveDrawBatch.AddAABBToBatch(Comp->GetBoundingBox(), Comp->GetComponentLocation(), WorldMatrix);
        }
    }
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(const FSkeletalMeshRenderData* RenderData) const
{
    constexpr UINT Stride = sizeof(FSkeletalMeshVertex);
    constexpr UINT Offset = 0;

    FCPUSkinningConstants CPUSkinningData;
    CPUSkinningData.bCPUSKinning = USkeletalMeshComponent::GetCPUSkinning();
    BufferManager->UpdateConstantBuffer(TEXT("FCPUSkinningConstants"), CPUSkinningData);
    
    FVertexInfo VertexInfo;
    if (CPUSkinningData.bCPUSKinning)
    {
        BufferManager->CreateDynamicVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);
        BufferManager->UpdateDynamicVertexBuffer(RenderData->ObjectName, RenderData->Vertices);
    }
    else
    {
        BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);
    }
    
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);
    
    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }
    else
    {
        Graphics->DeviceContext->Draw(RenderData->Vertices.Num(), 0);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        FName MaterialName = RenderData->MaterialSubsets[SubMeshIndex].MaterialName;
        UMaterial* Material = UAssetManager::Get().GetMaterial(MaterialName);
        FMaterialInfo MaterialInfo = Material->GetMaterialInfo();
        MaterialUtils::UpdateMaterial(BufferManager, Graphics, MaterialInfo);

        const uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        const uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount; 
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(ID3D11Buffer* Buffer, UINT VerticesNum) const
{
}

void FSkeletalMeshRenderPassBase::RenderSkeletalMesh(ID3D11Buffer* VertexBuffer, ID3D11Buffer* IndexBuffer, UINT IndicesNum) const
{
}

void FSkeletalMeshRenderPassBase::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;

    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FSkeletalMeshRenderPassBase::UpdateBone(const USkeletalMeshComponent* SkeletalMeshComponent)
{
    if (!SkeletalMeshComponent ||
        !SkeletalMeshComponent->GetSkeletalMeshAsset() ||
        !SkeletalMeshComponent->GetSkeletalMeshAsset()->GetSkeleton() ||
        USkeletalMeshComponent::GetCPUSkinning())
    {
        return;
    }

    // Skeleton 정보 가져오기
    const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
    const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetSkeleton()->GetReferenceSkeleton();
    const int32 BoneNum = RefSkeleton.RawRefBoneInfo.Num();

    // 현재 애니메이션 본 행렬 계산
    TArray<FMatrix> CurrentGlobalBoneMatrices;
    SkeletalMeshComponent->GetCurrentGlobalBoneMatrices(CurrentGlobalBoneMatrices);
    
    // 최종 스키닝 행렬 계산
    TArray<FMatrix> FinalBoneMatrices;
    FinalBoneMatrices.SetNum(BoneNum);
    
    for (int32 BoneIndex = 0; BoneIndex < BoneNum; ++BoneIndex)
    {
        FinalBoneMatrices[BoneIndex] = RefSkeleton.InverseBindPoseMatrices[BoneIndex] * CurrentGlobalBoneMatrices[BoneIndex];
        FinalBoneMatrices[BoneIndex] = FMatrix::Transpose(FinalBoneMatrices[BoneIndex]);
    }
    
    // Update
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    const HRESULT Result = Graphics->DeviceContext->Map(BoneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Buffer Map 실패, HRESULT: 0x%X"), Result);
        return;
    }
    
    ZeroMemory(MappedResource.pData, sizeof(FMatrix) * MaxBoneNum);
    memcpy(MappedResource.pData, FinalBoneMatrices.GetData(), sizeof(FMatrix) * BoneNum);
    Graphics->DeviceContext->Unmap(BoneBuffer, 0); 
}
