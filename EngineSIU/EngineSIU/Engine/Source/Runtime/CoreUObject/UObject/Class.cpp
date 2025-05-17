#include "Class.h"
#include <cassert>

#include "EngineStatics.h"
#include "UObjectArray.h"
#include "Serialization/Archive.h"


UClass::UClass(
    const char* InClassName,
    uint32 InClassSize,
    uint32 InAlignment,
    UClass* InSuperClass,
    ClassConstructorType InCTOR
)
    : UStruct(InClassName, InClassSize, InAlignment, InSuperClass)
    , ClassCTOR(InCTOR)
{
}

UClass::~UClass()
{
    delete ClassDefaultObject;
    ClassDefaultObject = nullptr;
}

TMap<FName, UClass*>& UClass::GetClassMap()
{
    static TMap<FName, UClass*> ClassMap;
    return ClassMap;
}

UClass* UClass::FindClass(const FName& ClassName)
{
    if (UClass** It = GetClassMap().Find(ClassName))
    {
        return *It;
    }
    return nullptr;
}

bool UClass::IsChildOf(const UClass* SomeBase) const
{
    return Super::IsChildOf(SomeBase);
}

UObject* UClass::GetDefaultObject() const
{
    if (!ClassDefaultObject)
    {
        const_cast<UClass*>(this)->CreateDefaultObject();
    }
    return ClassDefaultObject;
}

UObject* UClass::CreateDefaultObject()
{
    if (!ClassDefaultObject)
    {
        ClassDefaultObject = ClassCTOR();
        if (!ClassDefaultObject)
        {
            return nullptr;
        }

        ClassDefaultObject->ClassPrivate = this;
        ClassDefaultObject->NamePrivate = GetName() + "_CDO";
        ClassDefaultObject->UUID = UEngineStatics::GenUUID();
        GUObjectArray.AddObject(ClassDefaultObject);
    }
    return ClassDefaultObject;
}
