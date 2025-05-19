#pragma once
#include "Container/Map.h"
#include "Core/HAL/PlatformType.h"

class UImGuiManager
{
public:
    //void Initialize(HWND hWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
    static UImGuiManager& Get();

    void AddWnd(HWND AppWnd, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
    void RemoveWnd(HWND AppWnd);

    void BeginFrame(HWND AppWnd) const;
    void EndFrame(HWND AppWnd) const;
    void Shutdown();

    bool GetWantCaptureMouse(HWND AppWnd);
    bool GetWantCaptureKeyboard(HWND AppWnd);
    ImGuiContext* GetImGuiContext(HWND AppWnd);

private:
    void InitializeWnd();
    void PreferenceStyle() const;

private:
    TMap<HWND, ImGuiContext*> WndUImGuiContextMap;
};

