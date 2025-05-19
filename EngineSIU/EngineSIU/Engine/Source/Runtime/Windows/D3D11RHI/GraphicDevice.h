#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#define SAFE_RELEASE(p) if(p) { p->Release(); p = nullptr; }
#include <d3d11.h>

#include "EngineBaseTypes.h"
#include "Container/Array.h"
#include "Container/Map.h"
#include "Container/String.h"

struct FWindowData
{
    IDXGISwapChain* SwapChain = nullptr;

    UINT ScreenWidth = 0;
    UINT ScreenHeight = 0;

    ID3D11Texture2D* BackBufferTexture = nullptr;
    ID3D11RenderTargetView* BackBufferRTV = nullptr;

    float GetAspectRatio() const
    {
        return static_cast<float>(ScreenWidth) / static_cast<float>(ScreenHeight);
    }
};

class FEditorViewportClient;

class FGraphicsDevice
{
public:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    TMap<HWND, FWindowData> SwapChains;

    HWND CurrentAppWnd = nullptr;
    
    //IDXGISwapChain* SwapChain = nullptr;
    
    //ID3D11Texture2D* BackBufferTexture = nullptr;
    //ID3D11RenderTargetView* BackBufferRTV = nullptr;
    
    ID3D11RasterizerState* RasterizerSolidBack = nullptr;
    ID3D11RasterizerState* RasterizerSolidFront = nullptr;
    ID3D11RasterizerState* RasterizerWireframeBack = nullptr;
    ID3D11RasterizerState* RasterizerShadow = nullptr;

    ID3D11DepthStencilState* DepthStencilState = nullptr;
    
    ID3D11BlendState* AlphaBlendState = nullptr;
    
    DXGI_SWAP_CHAIN_DESC SwapchainDesc;
    
    UINT ScreenWidth = 0;
    UINT ScreenHeight = 0;

    D3D11_VIEWPORT Viewport;
    
    FLOAT ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 화면을 초기화(clear) 할 때 사용할 색상(RGBA)

    void Initialize(HWND AppWnd);
    void AddWnd(HWND AppWnd);
    void RemoveWnd(HWND AppWnd);
    
    void ChangeRasterizer(EViewModeIndex ViewModeIndex);
    void CreateRTV(ID3D11Texture2D*& OutTexture, ID3D11RenderTargetView*& OutRTV);
    ID3D11Texture2D* CreateTexture2D(const D3D11_TEXTURE2D_DESC& Description, const void* InitialData);
    
    void Release();
    
    void Prepare(HWND AppWnd);

    void SwapBuffer(HWND AppWnd) const;
    
    void Resize(HWND AppWnd);
    
    ID3D11RasterizerState* GetCurrentRasterizer() const { return CurrentRasterizer; }

    /*
    uint32 GetPixelUUID(POINT pt) const;
    uint32 DecodeUUIDColor(FVector4 UUIDColor) const;
    */
    
private:
    void CreateDeviceAndSwapChain(HWND AppWnd);
    void CreateSwapChain(HWND AppWnd);
    void CreateBackBuffer(HWND AppWnd);
    void CreateDepthStencilState();
    void CreateRasterizerState();
    void CreateAlphaBlendState();
    
    void ReleaseDevice();
    void ReleaseSwapChain(HWND AppWnd);
    void ReleaseBackBuffer(HWND AppWnd);
    void ReleaseRasterizerState();
    void ReleaseDepthStencilResources();
    
    ID3D11RasterizerState* CurrentRasterizer = nullptr;

    const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT BackBufferRTVFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
};

