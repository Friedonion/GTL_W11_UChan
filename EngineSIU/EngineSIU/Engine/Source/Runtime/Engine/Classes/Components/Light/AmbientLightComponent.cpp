#include "AmbientLightComponent.h"

#include "UObject/Casts.h"

UAmbientLightComponent::UAmbientLightComponent()
{
    AmbientLightInfo.AmbientColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

UObject* UAmbientLightComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->AmbientLightInfo = AmbientLightInfo;
    return NewComponent;
}

void UAmbientLightComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("AmbientColor"), AmbientLightInfo.AmbientColor.ToString());
}

void UAmbientLightComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("AmbientColor"));
    if (TempStr)
    {
        AmbientLightInfo.AmbientColor.InitFromString(*TempStr);
    }
}

const FAmbientLightInfo& UAmbientLightComponent::GetAmbientLightInfo() const
{
    return AmbientLightInfo;
}

void UAmbientLightComponent::SetAmbientLightInfo(const FAmbientLightInfo& InAmbient)
{
    AmbientLightInfo = InAmbient;
}

FLinearColor UAmbientLightComponent::GetLightColor() const
{
    return AmbientLightInfo.AmbientColor;
}

void UAmbientLightComponent::SetLightColor(const FLinearColor& InColor)
{
    AmbientLightInfo.AmbientColor = InColor;
}
