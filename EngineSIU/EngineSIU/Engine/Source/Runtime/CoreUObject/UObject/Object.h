#pragma once
#include "EngineLoop.h"
#include "NameTypes.h"
#include "Misc/CoreMiscDefines.h"

extern FEngineLoop GEngineLoop;

class UClass;
class UWorld;
class AActor;

class UObject
{
    friend class AActor;
private:
    UObject(const UObject&) = delete;
    UObject& operator=(const UObject&) = delete;
    UObject(UObject&&) = delete;
    UObject& operator=(UObject&&) = delete;

public:
    using Super = UObject;
    using ThisClass = UObject;

    static UClass* StaticClass();

private:
    friend class FObjectFactory;
    friend class FSceneMgr;
    friend class UClass;

    uint32 UUID;
    uint32 InternalIndex; // Index of GUObjectArray

    FName NamePrivate;
    UClass* ClassPrivate = nullptr;
    UObject* OuterPrivate = nullptr;

    
    // FName을 키값으로 넣어주는 컨테이너를 모두 업데이트 해야합니다.
    void SetFName(const FName& InName) { NamePrivate = InName; }

public:
    UObject();
    virtual ~UObject() = default;

    virtual UObject* Duplicate(UObject* InOuter);

    UObject* GetOuter() const { return OuterPrivate; }
    virtual UWorld* GetWorld() const;
    virtual void Serialize(FArchive& Ar);

    FName GetFName() const { return NamePrivate; }
    FString GetName() const { return NamePrivate.ToString(); }


    uint32 GetUUID() const { return UUID; }
    uint32 GetInternalIndex() const { return InternalIndex; }

    UClass* GetClass() const { return ClassPrivate; }

    /** this가 SomeBase인지, SomeBase의 자식 클래스인지 확인합니다. */
    bool IsA(const UClass* SomeBase) const;

    template <typename T>
        // requires std::derived_from<T, UObject>
    bool IsA() const
    {
        return IsA(T::StaticClass());
    }


    // UObjectBaseUtility

    /** 이 Object를 삭제 대기열에 추가합니다. */
    void MarkAsGarbage();

    // ~UObjectBaseUtility

public:
    FVector4 EncodeUUID() const
    {
        FVector4 Result;

        Result.X = static_cast<float>(UUID % 0xFF);
        Result.Y = static_cast<float>(UUID >> 8 & 0xFF);
        Result.Z = static_cast<float>(UUID >> 16 & 0xFF);
        Result.W = static_cast<float>(UUID >> 24 & 0xFF);

        return Result;
    }

    virtual void SerializeAsset(FArchive& Ar) {}
};
