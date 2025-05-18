#pragma once
#include <format>
#include "Container/Array.h"
#include "D3D11RHI/GraphicDevice.h"
#include "HAL/PlatformType.h"
#include "UObject/NameTypes.h"
#include "ImGui/imgui.h"
#include "PropertyEditor/IWindowToggleable.h"

static consteval std::string_view GetFileName(std::string_view PathView)
{
    const size_t LastSlash = PathView.find_last_of("/\\");
    if (LastSlash == std::string_view::npos)
    {
        return PathView;
    }
    return PathView.substr(LastSlash + 1);
}

#define FILENAME GetFileName(__FILE__)
#define UE_LOG(Level, Fmt, ...) FConsole::GetInstance().AddLog(Level, "[%s:%d] " Fmt, FILENAME.data(), __LINE__, __VA_ARGS__)

// TODO: 테스트 해야함
#define UE_LOGFMT(Level, Fmt, ...) FConsole::GetInstance().AddLogFmt(Level, "[{}:{}] " Fmt, FILENAME, __LINE__, __VA_ARGS__)


enum class ELogLevel : uint8
{
    Display,
    Warning,
    Error
};

class FStatOverlay
{
public:
    // @todo Stat-FPS Default 설정 복구 (showFPS = false, showRender = false)
    // TODO: 추후에 enum 비트연산으로 바꿔도 좋을듯
    union
    {
        struct  // NOLINT(clang-diagnostic-nested-anon-types)
        {
            uint8 bShowFps : 1;
            uint8 bShowMemory : 1;
            uint8 bShowLight : 1;
            uint8 bShowRender : 1;
        };
        uint8 StatFlags = 0; // 기본적으로 다 끄기
    };

    void ToggleStat(const std::string& Command);
    void Render(ID3D11DeviceContext* Context, UINT Width, UINT Height) const;
};

struct FProfiledScope
{
    FString DisplayName;
    FName CPUStatName;
    FName GPUStatName;
};

class FGPUTimingManager;

class FEngineProfiler
{
public:
    FEngineProfiler() = default;
    ~FEngineProfiler() = default;

    void SetGPUTimingManager(FGPUTimingManager* InGPUTimingManager);
    void Render(ID3D11DeviceContext* Context, UINT Width, UINT Height);
    void RegisterStatScope(const FString& DisplayName, const FName& CPUStatName, const FName& GPUStatName);

private:
    FGPUTimingManager* GPUTimingManager = nullptr;
    TArray<FProfiledScope> TrackedScopes;
    bool bShowWindow = true;
};

class FConsole : public IWindowToggleable
{
private:
    FConsole() = default;
    virtual ~FConsole() override = default;

public:
    // 복사 방지
    FConsole(const FConsole&) = delete;
    FConsole& operator=(const FConsole&) = delete;
    FConsole(FConsole&&) = delete;
    FConsole& operator=(FConsole&&) = delete;

public:
    static FConsole& GetInstance(); // 참조 반환으로 변경

    void Clear();
    void AddLog(ELogLevel Level, const ANSICHAR* Fmt, ...);
    void AddLog(ELogLevel Level, const WIDECHAR* Fmt, ...);
    void AddLog(ELogLevel Level, const FString& Message);

    template <typename... Args>
    void AddLogFmt(ELogLevel Level, std::string_view Fmt, Args&&... Arguments)
    {
        // 1. 인자들을 변환하여 튜플에 저장
        //    FString인 경우 ToAnsiString()의 결과(std::string)가 저장됨
        //    변환된 std::string 객체들은 이 튜플 내에 실제 값으로 존재하게 되어 수명 문제 방지
        std::tuple TransformedArgsTuple(ConvertIfFString(std::forward<Args>(Arguments))...);

        // 2. 튜플에 저장된 인자들을 std::make_format_args 전달
        //    std::apply를 사용하여 튜플의 요소들을 함수의 인자로 풀어줌
        const auto FormatArgs = std::apply(
            []<typename... InnerArgs>(InnerArgs&&... TransformedArgs)
            {
                return std::make_format_args(std::forward<InnerArgs>(TransformedArgs)...);
            },
            TransformedArgsTuple
        );

        AddLog(Level, std::vformat(Fmt, FormatArgs));
    }

    void Draw();
    void ExecuteCommand(const std::string& Command);
    void OnResize(HWND hWnd);

    virtual void Toggle() override
    {
        if (bWasOpen)
        {
            bWasOpen = false;
        }
        else
        {
            bWasOpen = true;
        }
    }

private:
    /**
     * 인자의 타입이 FString일 경우 ANSI 문자열(std::string)로 변환하고,
     * 그 외의 경우에는 인자를 그대로 전달합니다.
     *
     * 이 메서드는 컴파일 타임에 인자의 타입이 FString과 일치하는지 검사합니다.
     * 일치하는 경우 FString의 ToAnsiString 메서드를 호출하고,
     * 그렇지 않은 경우 원본 인자를 수정 없이 전달합니다.
     *
     * @tparam T 전달되어 처리될 인자의 타입
     * @param Arg FString 타입일 경우 변환될 인자, 그 외의 경우 그대로 전달될 인자
     * @return 인자가 FString 타입인 경우 ANSI 문자열(std::string)을 반환하고,
     *         그 외의 타입인 경우 원본 인자를 그대로 반환합니다.
     */
    template <typename T>
    auto ConvertIfFString(T&& Arg)
    {
        if constexpr (std::same_as<std::decay_t<T>, FString>)
        {
            return Arg.ToAnsiString();
        }
        else
        {
            return std::forward<T>(Arg);
        }
    }

public:
    struct LogEntry
    {
        ELogLevel Level;
        FString Message;
    };

    TArray<LogEntry> Items;
    TArray<FString> History;
    int32 HistoryPos = -1;
    char InputBuf[256] = "";
    bool ScrollToBottom = false;

    ImGuiTextFilter Filter; // 필터링을 위한 ImGuiTextFilter

    bool ShowLogTemp = true; // LogTemp 체크박스
    bool ShowWarning = true; // Warning 체크박스
    bool ShowError = true;   // Error 체크박스

    bool bWasOpen = true;

    FStatOverlay Overlay;

private:
    bool bExpand = true;
    UINT Width;
    UINT Height;
};
