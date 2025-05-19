#pragma once

#include "UObject/ObjectMacros.h"
#include "Core/Container/Array.h"
#include "Components/PrimitiveComponent.h"
#include "Math/RandomStream.h"

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

    virtual int32 GetLODLevel();

    virtual void TickComponent(float DeltaTime) override;

    void ComputeTickComponent_Concurrent();

    void FinalizeTickComponent();

public:
    // 업데이트는 진행하지만 Spawning를 막는 용도로 보임, Fadeout 등
    uint8 bSuppressSpawning : 1;

    /** Stream of random values to use with this component */
    FRandomStream RandomStream;

    /** This is created at start up and then added to each emitter */
    float EmitterDelay;

    /** Indicates that the component has not been ticked since being registered. */
    uint8 bJustRegistered:1;
private:
    int32 LODLevel;

    float DeltaTimeTick;

    int32 TotalActiveParticles;

    uint32 TimeSinceLastTick;

    uint8 bNeedsFinalize : 1;
};

