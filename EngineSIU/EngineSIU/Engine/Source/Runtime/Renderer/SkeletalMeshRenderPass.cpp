#include "SkeletalMeshRenderPass.h"

#include "ShadowManager.h"
#include "UnrealClient.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "UnrealEd/EditorViewportClient.h"

void FSkeletalMeshRenderPass::CreateResource()
{
    FSkeletalMeshRenderPassBase::CreateResource();

    const HRESULT Result = ShaderManager->AddPixelShader(L"SkeletalMeshPixelShader", L"Shaders/StaticMeshPixelShader.hlsl", "mainPS");
    if (FAILED(Result))
    {
        return;
    }
}

void FSkeletalMeshRenderPass::ReleaseResource()
{
    FSkeletalMeshRenderPassBase::ReleaseResource();
}

void FSkeletalMeshRenderPass::PrepareRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const EViewModeIndex ViewMode = Viewport->GetViewMode();

    ChangeViewMode(ViewMode);
    
    Graphics->DeviceContext->RSSetViewports(1, &Viewport->GetViewportResource()->GetD3DViewport());

    constexpr EResourceType ResourceType = EResourceType::ERT_Scene;
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    const FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    const FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(ResourceType);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);

    Graphics->DeviceContext->VSSetShaderResources(1, 1, &BoneSRV);

    const TArray<FString> PSBufferKeys = {
        TEXT("FLightInfoBuffer"),
        TEXT("FMaterialConstants"),
        TEXT("FLitUnlitConstants"),
        TEXT("FSubMeshConstants"),
        TEXT("FTextureConstants"),
        TEXT("FIsShadowConstants"),
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);

    BufferManager->BindConstantBuffer(TEXT("FLightInfoBuffer"), 0, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FCPUSkinningConstants"), 1, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FObjectConstantBuffer"), 12, EShaderStage::Vertex);

    ShadowManager->BindResourcesForSampling();
}

void FSkeletalMeshRenderPass::CleanUpRenderPass(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    ID3D11ShaderResourceView* NullSRV[1] = { nullptr };
    Graphics->DeviceContext->VSSetShaderResources(1, 1, NullSRV);
    
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    // Release ShadowManager Buffers
    Graphics->DeviceContext->PSSetShaderResources(static_cast<int>(EShaderSRVSlot::SRV_PointLight), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<int>(EShaderSRVSlot::SRV_DirectionalLight), 1, NullSRV);
    Graphics->DeviceContext->PSSetShaderResources(static_cast<int>(EShaderSRVSlot::SRV_SpotLight), 1, NullSRV);

    // 머티리얼 리소스 해제
    constexpr UINT NumViews = static_cast<UINT>(EMaterialTextureSlots::MTS_MAX);
    
    ID3D11ShaderResourceView* NullSRVs[NumViews] = { nullptr };
    ID3D11SamplerState* NullSamplers[NumViews] = { nullptr};
    
    Graphics->DeviceContext->PSSetShaderResources(0, NumViews, NullSRVs);
    Graphics->DeviceContext->PSSetSamplers(0, NumViews, NullSamplers);

    // for Gouraud shading
    ID3D11SamplerState* NullSampler[1] = { nullptr};
    Graphics->DeviceContext->VSSetShaderResources(0, 1, NullSRV);
    Graphics->DeviceContext->VSSetSamplers(0, 1, NullSampler);

    // 상수버퍼 해제
    ID3D11Buffer* NullPSBuffer[9] = { nullptr };
    Graphics->DeviceContext->PSSetConstantBuffers(0, 9, NullPSBuffer);
    ID3D11Buffer* NullVSBuffer[2] = { nullptr };
    Graphics->DeviceContext->VSSetConstantBuffers(0, 2, NullVSBuffer);
}

void FSkeletalMeshRenderPass::ChangeViewMode(EViewModeIndex ViewMode)
{
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    
    switch (ViewMode)
    {
    case EViewModeIndex::VMI_Lit_Gouraud:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"GOURAUD_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Lit_Lambert:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"LAMBERT_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Lit_BlinnPhong:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"PHONG_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_LIT_PBR:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"PBR_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Wireframe:
    case EViewModeIndex::VMI_Unlit:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"LAMBERT_StaticMeshPixelShader");
        UpdateLitUnlitConstant(0);
        break;
    case EViewModeIndex::VMI_SceneDepth:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderDepth");
        UpdateLitUnlitConstant(0);
        break;
    case EViewModeIndex::VMI_WorldNormal:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderWorldNormal");
        UpdateLitUnlitConstant(0);
        break;
    case EViewModeIndex::VMI_WorldTangent:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderWorldTangent");
        UpdateLitUnlitConstant(0);
        break;
    // HeatMap ViewMode 등
    default:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"LAMBERT_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    }

    // Rasterizer
    Graphics->ChangeRasterizer(ViewMode);

    // Setup
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
}

void FSkeletalMeshRenderPass::UpdateLitUnlitConstant(int32 IsLit) const
{
    FLitUnlitConstants Data;
    Data.bIsLit = IsLit;
    BufferManager->UpdateConstantBuffer(TEXT("FLitUnlitConstants"), Data);
}
