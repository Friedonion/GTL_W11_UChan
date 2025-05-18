#pragma once
#include "IRenderPass.h"
#include "Container/Array.h"
#include "D3D11RHI/DXDShaderManager.h"

class FShadowManager;
class USkeletalMesh;
class UMaterial;
struct FSkeletalMeshRenderData;
class USkeletalMeshComponent;

struct FMatrix;
struct FVector4;
struct FStaticMaterial;
struct FStaticMeshRenderData;
struct ID3D11Buffer;

class FSkeletalMeshRenderPassBase : public IRenderPass
{
public:
    FSkeletalMeshRenderPassBase();
    virtual ~FSkeletalMeshRenderPassBase() override;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;

    void InitializeShadowManager(FShadowManager* InShadowManager);

    virtual void PrepareRenderArr() override;

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

protected:
    virtual void CreateResource();

    virtual void ReleaseResource();

    virtual void PrepareRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport) = 0;

    virtual void CleanUpRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport) = 0;

    virtual void Render_Internal(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void RenderAllSkeletalMeshes(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void RenderSkeletalMesh(const FSkeletalMeshRenderData* RenderData) const;

    void RenderSkeletalMesh(ID3D11Buffer* Buffer, UINT VerticesNum) const;

    void RenderSkeletalMesh(ID3D11Buffer* VertexBuffer, ID3D11Buffer* IndexBuffer, UINT IndicesNum) const;

    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;

    void UpdateBone(const USkeletalMeshComponent* SkeletalMeshComponent);

    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;

    FShadowManager* ShadowManager;

    TArray<USkeletalMeshComponent*> SkeletalMeshComponents;

    // 일단 렌더패스에서 직접 관리
    ID3D11Buffer* BoneBuffer;
    ID3D11ShaderResourceView* BoneSRV;

    const int32 MaxBoneNum = 1024;
};
