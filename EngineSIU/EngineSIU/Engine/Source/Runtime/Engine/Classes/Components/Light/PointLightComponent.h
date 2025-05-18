#pragma once
#include "LightComponent.h"

class UPointLightComponent :public ULightComponentBase
{
    DECLARE_CLASS(UPointLightComponent, ULightComponentBase)

public:
    UPointLightComponent();
    virtual ~UPointLightComponent() override = default;

    void InitShadowDebugView();

    virtual UObject* Duplicate(UObject* InOuter) override;
    
    virtual void GetProperties(TMap<FString, FString>& OutProperties) const override;
    virtual void SetProperties(const TMap<FString, FString>& InProperties) override;

    FPointLightInfo& GetPointLightInfo();
    void SetPointLightInfo(const FPointLightInfo& InPointLightInfo);

    float GetRadius() const;
    void SetRadius(float InRadius);

    bool GetCastShadows() const { return PointLightInfo.CastShadows; }
    void SetCastShadows(bool InCastShadows) { PointLightInfo.CastShadows = InCastShadows; }

    FLinearColor GetLightColor() const;
    void SetLightColor(const FLinearColor& InColor);
    
    float GetIntensity() const;
    void SetIntensity(float InIntensity);

    int GetType() const;
    void SetType(int InType);

    virtual void UpdateViewMatrix() override;
    virtual void UpdateProjectionMatrix() override;

private:
    FPointLightInfo PointLightInfo;

    TArray<ID3D11Texture2D*> OutputTextures = {};
    TArray<ID3D11ShaderResourceView*> OutputSRVs = {};
    ID3D11ShaderResourceView* SliceSRVs[6] = { nullptr };
};


