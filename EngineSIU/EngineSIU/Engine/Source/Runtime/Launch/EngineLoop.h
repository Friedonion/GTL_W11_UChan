#pragma once
#include "Core/HAL/PlatformType.h"
#include "Engine/ResourceMgr.h"
#include "LevelEditor/SlateAppMessageHandler.h"
#include "Renderer/Renderer.h"
#include "UnrealEd/PrimitiveDrawBatch.h"
#include "Stats/ProfilerStatsManager.h"
#include "Stats/GPUTimingManager.h"


class FSlateAppMessageHandler;
class UnrealEd;
class UImGuiManager;
class UWorld;
class FEditorViewportClient;
class SSplitterV;
class SSplitterH;
class FGraphicDevice;
class SLevelEditor;
class FDXDBufferManager;

class FEngineLoop
{
public:
    FEngineLoop();

    int32 PreInit();
    int32 Init(HINSTANCE hInstance);
    void Render() const;
    void Tick();
    void Exit() const;

    void GetClientSize(uint32& OutWidth, uint32& OutHeight) const;

private:
    HWND CreateEngineWnd(const HINSTANCE hInstance, WCHAR WindowClass[], WCHAR Title[]);
    void DestroyEngineWnd(HWND AppWnd, HINSTANCE hInstance, WCHAR WindowClass[]);
    void UpdateUI(HWND AppWnd) const;

    static LRESULT CALLBACK AppWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

public:
    static FGraphicsDevice GraphicDevice;
    static FRenderer Renderer;
    static UPrimitiveDrawBatch PrimitiveDrawBatch;
    static FResourceManager ResourceManager;
    static uint32 TotalAllocationBytes;
    static uint32 TotalAllocationCount;

    FGPUTimingManager GPUTimingManager;
    FEngineProfiler EngineProfiler;

    HWND GetMainWnd() const { return MainWnd; }
    FSlateAppMessageHandler* GetAppMessageHandler() const { return AppMessageHandler.get(); }

private:
    TArray<HWND> AppWnds;
    HWND MainWnd;
    std::unique_ptr<FSlateAppMessageHandler> AppMessageHandler;

    FDXDBufferManager* BufferManager; //TODO: UEngine으로 옮겨야함.

    bool bIsExit = false;
    int32 TargetFPS = 999;
};
