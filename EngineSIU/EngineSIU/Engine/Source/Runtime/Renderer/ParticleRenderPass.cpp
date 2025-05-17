// ParticleRenderPass.cpp

#include "ParticleRenderPass.h"
#include "Engine/EditorEngine.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UnrealClient.h"
#include <random>

void FParticleRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;

    CreateBlendState();
    CreateDepthState();
    CreateInstanceBuffer();
}

void FParticleRenderPass::PrepareRenderArr()
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);

    InstanceData.SetNum(10000);
    for (int i = 0; i < 10000; ++i)
    {
        FVector Pos = FVector(dist(rng), dist(rng), dist(rng));
        FMatrix World = FMatrix::CreateTranslationMatrix(Pos);


        FParticleInstanceData Inst;
        Inst.World = World;
        Inst.Color = FLinearColor(1, 1, 1, 1);
        InstanceData[i] = Inst;
    }
    UpdateInstanceBuffer();
}

void FParticleRenderPass::ClearRenderArr()
{
    InstanceData.Empty();
}
void FParticleRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    const FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);
    Graphics->DeviceContext->RSSetViewports(1, &ViewportResource->GetD3DViewport());

    SetupPipeline();
    RenderParticles(Viewport);

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    Graphics->DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    Graphics->DeviceContext->OMSetDepthStencilState(nullptr, 0);
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
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_LESS;

    Graphics->Device->CreateDepthStencilState(&desc, &DepthState);
}

void FParticleRenderPass::CreateInstanceBuffer()
{
    D3D11_BUFFER_DESC desc = {};
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = sizeof(FParticleInstanceData) * 11000;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Graphics->Device->CreateBuffer(&desc, nullptr, &InstanceBuffer);
}

void FParticleRenderPass::UpdateInstanceBuffer()
{
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (SUCCEEDED(Graphics->DeviceContext->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, InstanceData.GetData(), sizeof(FParticleInstanceData) * InstanceData.Num());
        Graphics->DeviceContext->Unmap(InstanceBuffer, 0);
    }
}

void FParticleRenderPass::RenderParticles(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    FVertexInfo QuadVB;
    FIndexInfo QuadIB;
    BufferManager->GetQuadBuffer(QuadVB, QuadIB);

    if (!QuadVB.VertexBuffer || !QuadIB.IndexBuffer) return;

    UINT stride = QuadVB.Stride;
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &QuadVB.VertexBuffer, &stride, &offset);

    UINT instanceStride = sizeof(FParticleInstanceData);
    UINT instanceOffset = 0;
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Graphics->DeviceContext->IASetVertexBuffers(1, 1, &InstanceBuffer, &instanceStride, &instanceOffset);
    Graphics->DeviceContext->IASetIndexBuffer(QuadIB.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    Graphics->DeviceContext->IASetInputLayout(
    ShaderManager->GetInputLayoutByKey(L"ParticleSpriteVS"));

    Graphics->DeviceContext->VSSetShader(
        ShaderManager->GetVertexShaderByKey(L"ParticleSpriteVS"), nullptr, 0);

    Graphics->DeviceContext->PSSetShader(
        ShaderManager->GetPixelShaderByKey(L"ParticleSpritePS"), nullptr, 0);

    BufferManager->BindConstantBuffer(TEXT("FCameraConstantBuffer"), 13, EShaderStage::Vertex);
    
    Graphics->DeviceContext->DrawIndexedInstanced(QuadIB.NumIndices, InstanceData.Num(), 0, 0, 0);
}
