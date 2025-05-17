#include "Property.h"
#include "Class.h"
#include "ScriptStruct.h"


void FProperty::Resolve()
{
}

void FUnresolvedPtrProperty::Resolve()
{
    FProperty::Resolve();

    if (Type == EPropertyType::Object)
    {
        return;
    }

    if (Type == EPropertyType::UnresolvedPointer)
    {
        if (std::holds_alternative<FName>(TypeSpecificData))
        {
            const FName& TypeName = std::get<FName>(TypeSpecificData);

            // ClassMap에서 먼저 검사
            if (UClass* FoundClass = UClass::FindClass(TypeName))
            {
                Type = EPropertyType::Object;
                TypeSpecificData = FoundClass;
                ResolvedProperty = new FObjectProperty{OwnerStruct, Name, Size, Offset, Flags};
                return;
            }

            // StructMap에서 검사
            if (UScriptStruct* FoundStruct = UScriptStruct::FindScriptStruct(TypeName))
            {
                Type = EPropertyType::Struct;
                TypeSpecificData = FoundStruct;
                ResolvedProperty = new FStructProperty{OwnerStruct, Name, Size, Offset, Flags};
                return;
            }
        }
    }

    Type = EPropertyType::Unknown;
    TypeSpecificData = std::monostate{};
    UE_LOGFMT(ELogLevel::Error, "Unknown Property Type: {}", Name);
}
