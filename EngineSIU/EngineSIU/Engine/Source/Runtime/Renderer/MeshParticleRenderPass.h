#pragma once
#include "IRenderPass.h"
#include "Container/Array.h"
#include "Math/Color.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"

// 전방 선언
class ID3D11BlendState;
class ID3D11DepthStencilState;
class ID3D11Buffer;
class UStaticMesh;
class FDXDBufferManager;
class FGraphicsDevice;
class FDXDShaderManager;
struct FMatrix;
struct FLinearColor;
struct FVector;


// 메쉬 파티클 인스턴스 데이터 구조체
struct FMeshParticleInstanceData
{
    FMatrix World;           // 월드 변환 행렬
    FLinearColor Color;      // 색상 정보
    uint32 MeshIndex;        // 사용할 메쉬의 인덱스
};

class FMeshParticleRenderPass : public IRenderPass
{
public:
    FMeshParticleRenderPass();
    virtual ~FMeshParticleRenderPass();

    // IRenderPass 인터페이스 구현
    virtual void Initialize(FDXDBufferManager* BufferManager, FGraphicsDevice* Graphics, FDXDShaderManager* ShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void ClearRenderArr() override;

private:
    TArray<UStaticMesh*> RandomMeshes;
    TArray<FMeshParticleInstanceData> InstanceData;
    ID3D11Buffer* InstanceBuffer;
    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;
    ID3D11BlendState* BlendState;
    ID3D11DepthStencilState* DepthState;
    
    // 파이프라인 설정
    void SetupPipeline();
    void RenderParticles(const std::shared_ptr<FEditorViewportClient>& Viewport);
    
    // 리소스 생성
    void CreateBlendState();
    void CreateDepthState();
    void CreateInstanceBuffer();
    void UpdateInstanceBuffer();
    void LoadRandomMeshes();
};
