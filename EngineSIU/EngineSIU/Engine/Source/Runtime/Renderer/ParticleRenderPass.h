
#pragma once
#include "IRenderPass.h"
#include "Math/Color.h"
#include "Math/Matrix.h"
#include "Container/Array.h"

class ID3D11BlendState;
class ID3D11DepthStencilState;
class ID3D11Buffer;
struct FTexture;

struct FParticleInstanceData
{
    FMatrix World;
    FLinearColor Color;
    int SubImageIndexX;
};

class FParticleRenderPass : public IRenderPass
{
public:
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void ClearRenderArr() override;

private:
    void SetupPipeline();
    void RenderParticles(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void CreateBlendState();
    void CreateDepthState();
    void CreateInstanceBuffer();
    void UpdateInstanceBuffer();

    FDXDBufferManager* BufferManager = nullptr;
    FGraphicsDevice* Graphics = nullptr;
    FDXDShaderManager* ShaderManager = nullptr;

    ID3D11BlendState* BlendState = nullptr;
    ID3D11DepthStencilState* DepthState = nullptr;

    ID3D11Buffer* InstanceBuffer = nullptr;
    TArray<FParticleInstanceData> InstanceData;

    // 임시코드
    std::shared_ptr<FTexture> ParticleTexture;
    int SubImageCountX;
    int SubImageCountY;
};
