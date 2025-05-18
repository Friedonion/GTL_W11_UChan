#pragma once

#include "UObject/ObjectMacros.h"
#include "Core/Container/Array.h"
#include "Components/PrimitiveComponent.h"

class UParticleSystem;
class FDynamicEmitterDataBase;

class UParticleSystemComponent : public UPrimitiveComponent
{
    DECLARE_CLASS(UParticleSystemComponent, UPrimitiveComponent)
public:
    UParticleSystemComponent();
    ~UParticleSystemComponent();

    TArray<struct FParticleEmitterInstance*> EmitterInstances;
    // UMaterialInterface 미구현 - UMaterial로 대체?
    TArray<UMaterial*> EmitterMaterials;
    UParticleSystem* Template;

    TArray<FDynamicEmitterDataBase*> EmitterRenderData;

    virtual void TickComponent(float DeltaTime) override;

    void ComputeTickComponent_Concurrent();

    void FinalizeTickComponent();

public:
    // 업데이트는 진행하지만 Spawning를 막는 용도로 보임, Fadeout 등
    uint8 bSuppressSpawning : 1;

private:
    float DeltaTimeTick;

    int32 TotalActiveParticles;

    uint32 TimeSinceLastTick;

    uint8 bNeedsFinalize : 1;
};

