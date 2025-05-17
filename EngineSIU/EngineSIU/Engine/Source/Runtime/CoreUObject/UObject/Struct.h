#pragma once
#include "UObject/Object.h"

struct FProperty;


/**
 * UClass와 UScriptStruct의 기본 클래스입니다.
 * 구조체 또는 클래스의 멤버 변수(프로퍼티) 목록, 크기, 정렬 등의 메타데이터를 관리합니다.
 */
class UStruct : public UObject
{
public:
    UStruct(
        const char* InName,
        uint32 InStructSize,
        uint32 InAlignment,
        UStruct* InSuperStruct
    );

    virtual ~UStruct() override;

    UStruct(const UStruct&) = delete;
    UStruct& operator=(const UStruct&) = delete;
    UStruct(UStruct&&) = delete;
    UStruct& operator=(UStruct&&) = delete;

    using Super = UObject;
    using ThisClass = UStruct;

public:
    /** 현재 구조체의 크기를 반환합니다. */
    [[nodiscard]] uint32 GetStructSize() const { return StructSize; }

    /** 현재 구조체의 Property 총 크기를 반환합니다. */
    [[nodiscard]] uint32 GetPropertiesSize() const { return PropertiesSize; }

    /** 현재 구조체의 최소 메모리 정렬 크기를 반환합니다. */
    [[nodiscard]] uint32 GetMinAlignment() const { return MinAlignment; }

    /** 부모의 UStruct를 가져옵니다. */
    [[nodiscard]] UStruct* GetSuperStruct() const { return SuperStruct; }

    /** SomeBase의 자식 Struct인지 확인합니다. */
    bool IsChildOf(const UStruct* SomeBase) const;

    template <typename T>
    requires
        std::derived_from<T, UObject>
    [[nodiscard]] bool IsChildOf() const;

    [[nodiscard]] const TArray<FProperty*>& GetProperties() const { return Properties; }

    /**
     * UStruct에 Property를 추가합니다.
     * @param Prop 추가할 Property
     */
    void AddProperty(FProperty* Prop);

    /** 이름으로 Property를 찾습니다. 부모 Struct의 Property도 검색합니다. */
    [[nodiscard]] FProperty* FindPropertyByName(const FName& InName) const;

    /**
     * 이 Struct 타입의 인스턴스 데이터를 직렬화합니다.
     * @param Ar 직렬화 아카이브
     * @param Data 직렬화할 Struct 데이터의 시작 포인터
     */
    virtual void SerializeBin(FArchive& Ar, void* Data);

    /** 컴파일 타임에 알 수 없는 프로퍼티 타입을 런타임에 검사합니다.*/
    static void ResolvePendingProperties();

private:
    /** 컴파일 타임에 알 수 없는 프로퍼티 목록들*/
    static TArray<FProperty*>& GetUnresolvedProperties();

protected:
    uint32 StructSize;
    uint32 PropertiesSize;
    uint32 MinAlignment;
    UStruct* SuperStruct;

    // 이 Struct/Class에 직접 정의된 프로퍼티들
    TArray<FProperty*> Properties;

    // 이름으로 프로퍼티를 빠르게 찾기 위한 맵
    TMap<FName, FProperty*> PropertyMap;
};

template <typename T>
requires
    std::derived_from<T, UObject>
bool UStruct::IsChildOf() const
{
    return IsChildOf(T::StaticClass());
}


/**
 * T가 UClass인지 UScriptStruct인지에 따라 각각의 StaticClass() 또는 StaticStruct()를 호출합니다.
 * @tparam T UClass 또는 UScriptStruct 타입
 * @return T의 StaticClass() 또는 StaticStruct()의 반환값
 */
template <typename T>
requires
    requires { T::StaticClass(); }
    || requires { T::StaticStruct(); }
[[nodiscard]] static UStruct* GetStructHelper()
{
    // UClass
    if constexpr (requires { T::StaticClass(); })
    {
        return T::StaticClass();
    }

    // UScriptStruct
    else if constexpr (requires { T::StaticStruct(); })
    {
        return T::StaticStruct();
    }

    else
    {
        // DECLARE_CLASS, DECLARE_STRUCT 없이 UPROPERTY만 선언한 경우
        static_assert(TAlwaysFalse<T>, "T is neither UClass nor UScriptStruct");
    }

    std::unreachable();
}
