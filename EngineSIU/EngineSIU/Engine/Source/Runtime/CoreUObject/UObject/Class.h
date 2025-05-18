#pragma once
#include <concepts>
#include "Struct.h"
#include "Property.h"

class FArchive;


/**
 * UObject의 RTTI를 가지고 있는 클래스
 */
class UClass : public UStruct
{
    using ClassConstructorType = UObject*(*)();

public:
    UClass(
        const char* InClassName,
        uint32 InClassSize,
        uint32 InAlignment,
        UClass* InSuperClass,
        ClassConstructorType InCTOR
    );

    virtual ~UClass() override;

    // 복사 & 이동 생성자 제거
    UClass(const UClass&) = delete;
    UClass& operator=(const UClass&) = delete;
    UClass(UClass&&) = delete;
    UClass& operator=(UClass&&) = delete;

    using Super = UStruct;
    using ThisClass = UClass;

public:
    static TMap<FName, UClass*>& GetClassMap();
    static UClass* FindClass(const FName& ClassName);

public:
    /** SomeBase의 자식 클래스인지 확인합니다. */
    bool IsChildOf(const UClass* SomeBase) const;

    template <typename T>
    requires
        std::derived_from<T, UObject>
    bool IsChildOf() const;

    /**
     * 부모의 UClass를 가져옵니다.
     *
     * @note AActor::StaticClass()->GetSuperClass() == UObject::StaticClass()
     */
    FORCEINLINE UClass* GetSuperClass() const
    {
        return static_cast<UClass*>(SuperStruct);  // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    }

    UObject* GetDefaultObject() const;

    template <typename T>
        requires std::derived_from<T, UObject>
    T* GetDefaultObject() const;

protected:
    virtual UObject* CreateDefaultObject();

public:
    ClassConstructorType ClassCTOR;

private:
    UObject* ClassDefaultObject = nullptr;
};

template <typename T>
    requires std::derived_from<T, UObject>
bool UClass::IsChildOf() const
{
    return IsChildOf(T::StaticClass());
}

template <typename T>
    requires std::derived_from<T, UObject>
T* UClass::GetDefaultObject() const
{
    UObject* Ret = GetDefaultObject();
    assert(Ret->IsA<T>());
    return static_cast<T*>(Ret);
}
