#pragma once
#include "EngineStatics.h"
#include "Object.h"
#include "Class.h"
#include "UObjectArray.h"
#include "UserInterface/Console.h"

class FObjectFactory
{
public:
    static UObject* ConstructObject(UClass* InClass, UObject* InOuter, FName InName = NAME_None)
    {
        const uint32 Id = UEngineStatics::GenUUID();
        FName Name = FString::Printf(TEXT("%s_%d"), *InClass->GetName(), Id);

        if (InName != NAME_None)
        {
            Name = InName;
        }

        UObject* Obj = InClass->ClassCTOR();
        Obj->UUID = Id;
        Obj->NamePrivate = Name;
        Obj->ClassPrivate = InClass;
        Obj->OuterPrivate = InOuter;

        GUObjectArray.AddObject(Obj);

        UE_LOGFMT(ELogLevel::Display, "Created Object: {}, Size: {}", Obj->GetName(), InClass->GetStructSize());

        return Obj;
    }

    template<typename T>
        requires std::derived_from<T, UObject>
    static T* ConstructObject(UObject* InOuter)
    {
        return static_cast<T*>(ConstructObject(T::StaticClass(), InOuter));
    }

    template<typename T>
        requires std::derived_from<T, UObject>
    static T* ConstructObject(UObject* InOuter, FName InName)
    {
        return static_cast<T*>(ConstructObject(T::StaticClass(), InOuter, InName));
    }
};
