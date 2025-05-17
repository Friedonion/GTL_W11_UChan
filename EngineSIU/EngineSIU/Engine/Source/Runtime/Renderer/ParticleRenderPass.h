#pragma once
#include "IRenderPass.h"
#include "DirectXTK/BufferHelpers.h"

class ParticleRenderPass : public IRenderPass
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
    
    ID3D11Buffer* InstanceBuffer = nullptr;
    ID3D11BlendState* BlendState = nullptr;
    ID3D11DepthStencilState* DepthState = nullptr;
};
