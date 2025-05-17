#include "Struct.h"
#include "Property.h"
#include "Serialization/Archive.h"


// 정적 멤버 변수 정의
TArray<FProperty*>& UStruct::GetUnresolvedProperties()
{
    static TArray<FProperty*> GUnresolvedProperties;
    return GUnresolvedProperties;
}

void UStruct::ResolvePendingProperties()
{
    for (FProperty* Prop : GetUnresolvedProperties())
    {
        if (Prop)
        {
            Prop->Resolve();
        }
    }
    GetUnresolvedProperties().Empty();
}

UStruct::UStruct(
    const char* InName,
    uint32 InStructSize,
    uint32 InAlignment,
    UStruct* InSuperStruct
)
    : StructSize(InStructSize)
    , MinAlignment(InAlignment)
    , SuperStruct(InSuperStruct)
{
    NamePrivate = InName;
}

UStruct::~UStruct()
{
    // Properties 배열에 있는 FProperty 객체들의 메모리 해제
    for (const FProperty* Prop : Properties)
    {
        delete Prop;
    }
    Properties.Empty();
    PropertyMap.Empty();
}

bool UStruct::IsChildOf(const UStruct* SomeBase) const
{
    if (SomeBase == nullptr)
    {
        return false;
    }
    for (const UStruct* TempStruct = this; TempStruct != nullptr; TempStruct = TempStruct->GetSuperStruct())
    {
        if (TempStruct == SomeBase)
        {
            return true;
        }
    }
    return false;
}

void UStruct::AddProperty(FProperty* Prop)
{
    if (!Prop)
    {
        return;
    }

    if (Prop->Type == EPropertyType::UnresolvedPointer)
    {
        GetUnresolvedProperties().Add(Prop);
    }

    PropertiesSize += Prop->Size;
    Properties.Add(Prop);
    PropertyMap.Add(FName(Prop->Name), Prop);
}

FProperty* UStruct::FindPropertyByName(const FName& InName) const
{
    if (FProperty* const* FoundProperty = PropertyMap.Find(InName))
    {
        return *FoundProperty;
    }

    if (SuperStruct)
    {
        return SuperStruct->FindPropertyByName(InName);
    }

    return nullptr;
}

void UStruct::SerializeBin(FArchive& Ar, void* Data)
{
    // 부모 Struct의 프로퍼티 먼저 직렬화
    if (SuperStruct)
    {
        SuperStruct->SerializeBin(Ar, Data);
    }

    // 이 Struct에 직접 정의된 프로퍼티들 직렬화
    for (const FProperty* Prop : Properties)
    {
        if (Prop)
        {
            // 프로퍼티가 실제로 데이터 컨테이너(Data) 내에 유효한 오프셋을 가지는지 확인 필요
            // 예를 들어, 에디터 전용 프로퍼티인데 런타임 데이터에는 없을 수 있음 (그런 경우 Flags로 구분)
            void* PropData = static_cast<std::byte*>(Data) + Prop->Offset;

            // TODO: FProperty 자체에 SerializeValue(FArchive& Ar, void* ValueData) 같은 가상 함수를 만들고 호출하는 것이 더 좋음
            // 현재는 UClass의 SerializeBin과 유사하게 크기만큼 직접 직렬화
            // 이는 FProperty의 실제 타입(FIntProperty, FObjectProperty 등)에 따른 특화된 직렬화 로직이 없음을 의미.
            // 예를 들어 FObjectProperty는 포인터 자체를 직렬화하는게 아니라, 참조하는 UObject를 찾아 직렬화해야 함.
            // 이 부분은 FProperty 파생 클래스에서 SerializeValue를 오버라이드하여 구현해야 합니다.
            // 여기서는 임시로 단순 메모리 복사 형태로 가정. (실제로는 매우 위험)
            Ar.Serialize(PropData, Prop->Size);
        }
    }
}
