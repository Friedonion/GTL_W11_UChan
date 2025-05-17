#include "ScriptStruct.h"


UScriptStruct::UScriptStruct(
    const char* InName,
    uint32 InStructSize,
    uint32 InAlignment,
    UScriptStruct* InSuperScriptStruct
)
    : UStruct(InName, InStructSize, InAlignment, InSuperScriptStruct)
{
}

TMap<FName, UScriptStruct*>& UScriptStruct::GetScriptStructMap()
{
    static TMap<FName, UScriptStruct*> GScriptStructMap;
    return GScriptStructMap;
}

UScriptStruct* UScriptStruct::FindScriptStruct(const FName& StructName)
{
    if (UScriptStruct*const* FoundStruct = GetScriptStructMap().Find(StructName))
    {
        return *FoundStruct;
    }
    return nullptr;
}
