#include "Console.h"
#include <cstdarg>
#include <cstdio>

#include "Actors/PointLightActor.h"
#include "Actors/SpotLightActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/Light/LightComponent.h"
#include "Engine/Engine.h"
#include "Renderer/UpdateLightBufferPass.h"
#include "Stats/GPUTimingManager.h"
#include "Stats/ProfilerStatsManager.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"


void FStatOverlay::ToggleStat(const std::string& Command)
{
    if (Command == "stat fps")
    {
        bShowFps = true;
        bShowRender = true;
    }
    else if (Command == "stat memory")
    {
        bShowMemory = true;
        bShowRender = true;
    }
    else if (Command == "stat light")
    {
        bShowLight = true;
        bShowRender = true;
    }
    else if (Command == "stat all")
    {
        StatFlags = 0xFF;
    }
    else if (Command == "stat none")
    {
        // 모든 Flag 끄기
        StatFlags = 0x00;
    }
}

void FStatOverlay::Render(ID3D11DeviceContext* Context, UINT Width, UINT Height) const
{
    if (!bShowRender)
    {
        return;
    }

    const ImVec2 DisplaySize = ImGui::GetIO().DisplaySize;
    // 창 크기를 화면의 50%로 설정합니다.
    const ImVec2 WindowSize(DisplaySize.x * 0.5f, DisplaySize.y * 0.5f);
    // 창을 중앙에 배치하기 위해 위치를 계산합니다.
    const ImVec2 WindowPos((DisplaySize.x - WindowSize.x) * 0.5f, (DisplaySize.y - WindowSize.y) * 0.5f);

    ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
    ImGui::Begin(
        "Stat Overlay", nullptr,
        ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoInputs
        | ImGuiWindowFlags_AlwaysAutoResize
    );

    if (bShowFps)
    {
        // FPS 정보 출력
        const float Fps = ImGui::GetIO().Framerate;
        ImGui::Text("FPS: %.1f (%.1f ms)", Fps, 1000.0f / Fps);
    }

    if (bShowMemory)
    {
        ImGui::SeparatorText("Memory Usage");
        ImGui::Text("Allocated Object Count: %llu", FPlatformMemory::GetAllocationCount<EAT_Object>());
        ImGui::Text("Allocated Object Memory: %llu Byte", FPlatformMemory::GetAllocationBytes<EAT_Object>());
        ImGui::Text("Allocated Container Count: %llu", FPlatformMemory::GetAllocationCount<EAT_Container>());
        ImGui::Text("Allocated Container Memory: %llu Byte", FPlatformMemory::GetAllocationBytes<EAT_Container>());
    }

    if (bShowLight)
    {
        ImGui::SeparatorText("[ Light Counters ]\n");
        ImGui::Text("Point Light: %d", GetNumOfObjectsByClass(APointLight::StaticClass()));
        ImGui::Text("Spot Light: %d", GetNumOfObjectsByClass(ASpotLight::StaticClass()));
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::End();
}

void FEngineProfiler::SetGPUTimingManager(FGPUTimingManager* InGPUTimingManager)
{
    GPUTimingManager = InGPUTimingManager;
}

void FEngineProfiler::Render(ID3D11DeviceContext* Context, UINT Width, UINT Height)
{
    if (!bShowWindow) return;

    // Example positioning: Top-left corner
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Engine Profiler", &bShowWindow))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTable("ProfilerTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("CPU (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("GPU (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (const auto& [DisplayName, CPUStatName, GPUStatName] : TrackedScopes)
        {
            const double CPUTimeMs = FProfilerStatsManager::GetCpuStatMs(CPUStatName);
            double GPUTimeMs = GPUTimingManager->GetElapsedTimeMs(TStatId(GPUStatName));

            FString CPUText = (CPUTimeMs >= 0.0) ? FString::Printf(TEXT("%.3f"), CPUTimeMs) : TEXT("---");
            FString GPUText;

            if (GPUTimeMs == -1.0) GPUText = TEXT("Disjoint");
            else if (GPUTimeMs == -2.0) GPUText = TEXT("Waiting");
            else if (GPUTimeMs < 0.0) GPUText = TEXT("---");
            else GPUText = FString::Printf(TEXT("%.3f"), GPUTimeMs);

            ImGui::TableNextRow();

            // Scope 열
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", *DisplayName);

            // CPU (ms) 열 - 우측 정렬
            ImGui::TableSetColumnIndex(1);
            float CPUTextWidth = ImGui::CalcTextSize(*CPUText).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - CPUTextWidth);
            ImGui::TextUnformatted(*CPUText);

            // GPU (ms) 열 - 우측 정렬
            ImGui::TableSetColumnIndex(2);
            float GPUTextWidth = ImGui::CalcTextSize(*GPUText).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - GPUTextWidth);
            ImGui::TextUnformatted(*GPUText);
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void FEngineProfiler::RegisterStatScope(const FString& DisplayName, const FName& CPUStatName, const FName& GPUStatName)
{
    TrackedScopes.Add({ DisplayName, CPUStatName, GPUStatName });
}

// 싱글톤 인스턴스 반환
FConsole& FConsole::GetInstance() {
    static FConsole Instance;
    return Instance;
}

// 로그 초기화
void FConsole::Clear() {
    Items.Empty();
}

// 로그 추가
void FConsole::AddLog(ELogLevel Level, const ANSICHAR* Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    // AddLog(Level, Fmt, Args);

    char Buf[1024];
    vsnprintf_s(Buf, sizeof(Buf), _TRUNCATE, Fmt, Args);

    Items.Emplace(Level, std::string(Buf));
    va_end(Args);

    ScrollToBottom = true;
}

void FConsole::AddLog(ELogLevel Level, const WIDECHAR* Fmt, ...)
{
    va_list Args;
    va_start(Args, Fmt);
    // AddLog(Level, Fmt, Args);

    wchar_t Buf[1024];
    _vsnwprintf_s(Buf, sizeof(Buf), _TRUNCATE, Fmt, Args);

    Items.Emplace(Level, FString(Buf).ToAnsiString());
    va_end(Args);

    ScrollToBottom = true;
}

void FConsole::AddLog(ELogLevel Level, const FString& Message)
{
    Items.Emplace(Level, Message);
    ScrollToBottom = true;
}

// 콘솔 창 렌더링
void FConsole::Draw() {
    if (!bWasOpen)
    {
        return;
    }

    // 창 크기 및 위치 계산
    const ImVec2 DisplaySize = ImGui::GetIO().DisplaySize;

    // 콘솔 창의 크기와 위치 설정
    const float ExpandedHeight = DisplaySize.y * 0.4f; // 확장된 상태일 때 높이 (예: 화면의 40%)
    constexpr float CollapsedHeight = 30.0f;               // 축소된 상태일 때 높이
    const float CurrentHeight = bExpand ? ExpandedHeight : CollapsedHeight;

    // 왼쪽 하단에 위치하도록 계산 (창의 좌측 하단이 화면의 좌측 하단에 위치)
    const ImVec2 WindowSize(DisplaySize.x * 0.5f, CurrentHeight); // 폭은 화면의 40%
    const ImVec2 WindowPos(0, DisplaySize.y - CurrentHeight);

    // 창 위치와 크기를 고정
    ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);

    // 창을 표시하고 닫힘 여부 확인
    Overlay.Render(FEngineLoop::GraphicDevice.DeviceContext, Width, Height);

    bExpand = ImGui::Begin("Console", &bWasOpen);
    if (!bExpand)
    {
        // 창을 접었을 경우 UI를 표시하지 않음
        ImGui::End();
        return;
    }

    // 버튼 UI (로그 수준별 추가)
    if (ImGui::Button("Clear"))
    {
        Clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy"))
    {
        ImGui::LogToClipboard();
    }

    ImGui::Separator();

    // 필터 입력 창
    ImGui::Text("Filter:");
    ImGui::SameLine();
    Filter.Draw("##Filter", 100);
    ImGui::SameLine();
    // 로그 수준을 선택할 체크박스
    ImGui::Checkbox("Show Display", &ShowLogTemp);
    ImGui::SameLine();
    ImGui::Checkbox("Show Warning", &ShowWarning);
    ImGui::SameLine();
    ImGui::Checkbox("Show Error", &ShowError);

    ImGui::Separator();

    // 로그 출력 (필터 적용)
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& [Level, Message] : Items)
    {
        if (!Filter.PassFilter(*Message))
        {
            continue;
        }

        // 로그 수준에 맞는 필터링
        if ((Level == ELogLevel::Display && !ShowLogTemp) ||
            (Level == ELogLevel::Warning && !ShowWarning) ||
            (Level == ELogLevel::Error && !ShowError))
        {
            continue;
        }

        // 색상 지정
        ImVec4 Color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        switch (Level)
        {
        case ELogLevel::Display:
            Color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 기본 흰색
            break;
        case ELogLevel::Warning:
            Color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // 노란색
            break;
        case ELogLevel::Error:
            Color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // 빨간색
            break;
        }

        ImGui::TextColored(Color, "%s", *Message);
    }

    if (ScrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        ScrollToBottom = false;
    }
    ImGui::EndChild();

    ImGui::Separator();

    // 입력창
    bool ReclaimFocus = false;
    if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (InputBuf[0])
        {
            AddLog(ELogLevel::Display, ">> %s", InputBuf);
            const std::string Command(InputBuf);
            ExecuteCommand(Command);
            History.Add(std::string(InputBuf));
            HistoryPos = -1;
            ScrollToBottom = true; // 자동 스크롤
        }
        InputBuf[0] = '\0';
        ReclaimFocus = true;
    }

    // 입력 필드에 자동 포커스
    if (ReclaimFocus)
    {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

void FConsole::ExecuteCommand(const std::string& Command)
{
    AddLog(ELogLevel::Display, "Executing command: %s", Command.c_str());

    if (Command == "clear")
    {
        Clear();
    }
    else if (Command == "help")
    {
        AddLog(ELogLevel::Display, "Available commands:");
        AddLog(ELogLevel::Display, " - clear: Clears the console");
        AddLog(ELogLevel::Display, " - help: Shows available commands");
        AddLog(ELogLevel::Display, " - stat fps: Toggle FPS display");
        AddLog(ELogLevel::Display, " - stat memory: Toggle Memory display");
        AddLog(ELogLevel::Display, " - stat none: Hide all stat overlays");
    }
    else if (Command.starts_with("stat "))
    {
        Overlay.ToggleStat(Command);
    }
    else if (Command == "Toggle Skinning")
    {
        USkeletalMeshComponent::SetCPUSkinning(!(USkeletalMeshComponent::GetCPUSkinning()));
    }
    else
    {
        AddLog(ELogLevel::Error, "Unknown command: %s", Command.c_str());
    }
}

void FConsole::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

