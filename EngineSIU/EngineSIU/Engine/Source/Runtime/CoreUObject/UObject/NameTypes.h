#pragma once
#include "Core/HAL/PlatformType.h"

class FString;

enum ENameNone : uint8 { NAME_None = 0 };

/** Maximum size of name, including the null terminator. */
enum : uint16 { NAME_SIZE = 256 };

class FName
{
    friend struct FNameHelper;

    uint32 DisplayIndex;    // 원본 문자열의 Hash
    uint32 ComparisonIndex; // 비교시 사용되는 Hash

    #if defined(_DEBUG)
    const void* DebugEntryPtr = nullptr;
    #endif

public:
    FName() : DisplayIndex(NAME_None), ComparisonIndex(NAME_None) {}
    FName(ENameNone) : DisplayIndex(NAME_None), ComparisonIndex(NAME_None) {}
    FName(const WIDECHAR* Name);
    FName(const WIDECHAR* Name, uint32 Len);
    FName(const ANSICHAR* Name);
    FName(const ANSICHAR* Name, uint32 Len);
    FName(const FString& Name);

    FString ToString() const;
    uint32 GetDisplayIndex() const { return DisplayIndex; }
    uint32 GetComparisonIndex() const { return ComparisonIndex; }

    bool operator==(const FName& Other) const;
    bool operator==(ENameNone) const;
    bool operator!=(const FName& Other) const;
    bool operator!=(ENameNone) const;
};

template<>
struct std::hash<FName>
{
    size_t operator()(const FName& Key) const noexcept
    {
        // TODO: 나중에 FName에 Number 추가하면 살짝 수정
        return hash<uint32>()(Key.GetComparisonIndex());
    }
};
