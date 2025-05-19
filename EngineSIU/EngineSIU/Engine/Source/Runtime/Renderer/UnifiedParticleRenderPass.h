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
struct FTexture;

// 파티클 시스템 타입 열거형
enum class EParticleSystemType : uint8
{
    Sprite = 0,
    Mesh = 1
};

// 메쉬 파티클 인스턴스 데이터 구조체
struct FMeshParticleInstanceData
{
    FMatrix World;           // 월드 변환 행렬
    FLinearColor Color;      // 색상 정보
    uint32 MeshIndex;        // 사용할 메쉬의 인덱스
};

// 스프라이트 파티클 인스턴스 데이터 구조체
struct FSpriteParticleInstanceData
{
    FMatrix World;           // 월드 변환 행렬
    FLinearColor Color;      // 색상 정보
    int SubImageIndex;       // 서브 이미지 인덱스
};

class FUnifiedParticleRenderPass : public IRenderPass
{
public:
    FUnifiedParticleRenderPass();
    virtual ~FUnifiedParticleRenderPass();

    // IRenderPass 인터페이스 구현
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void ClearRenderArr() override;
    
    // 활성화/비활성화 제어 메서드
    void SetParticleTypeActive(EParticleSystemType Type, bool bActive);

private:
    // 그래픽스 리소스
    FDXDBufferManager* BufferManager;
    FGraphicsDevice* Graphics;
    FDXDShaderManager* ShaderManager;
    
    // 공통 렌더 상태
    ID3D11BlendState* BlendState;
    ID3D11DepthStencilState* DepthState;
    
    // 메쉬 파티클 관련 리소스
    TArray<UStaticMesh*> MeshArray;
    TArray<FMeshParticleInstanceData> MeshInstanceData;
    ID3D11Buffer* MeshInstanceBuffer;
    
    // 스프라이트 파티클 관련 리소스
    std::shared_ptr<FTexture> ParticleTexture;
    TArray<FSpriteParticleInstanceData> SpriteInstanceData;
    ID3D11Buffer* SpriteInstanceBuffer;
    int SubImageCountX;
    int SubImageCountY;
    
    // 활성화된 파티클 타입 플래그
    uint32 ActiveTypes;
    
    // 공통 메서드
    void CreateBlendState();
    void CreateDepthState();
    void SetupPipeline();
    
    // 타입별 메서드
    void PrepareMeshParticles();
    void PrepareSpriteParticles();
    void RenderMeshParticles(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderSpriteParticles(const std::shared_ptr<FEditorViewportClient>& Viewport);
    
    // 리소스 관리
    void CreateMeshInstanceBuffer();
    void CreateSpriteInstanceBuffer();
    void UpdateMeshInstanceBuffer();
    void UpdateSpriteInstanceBuffer();
    void LoadMeshes();
    void LoadTexture();
};