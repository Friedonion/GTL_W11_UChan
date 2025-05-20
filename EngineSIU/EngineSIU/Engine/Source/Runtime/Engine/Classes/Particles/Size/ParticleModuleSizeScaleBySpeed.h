#pragma once

#include "UObject/ObjectMacros.h"
#include "Particles/Size/ParticleModuleSizeBase.h"

struct FParticleEmitterInstance;

//UCLASS(editinlinenew, hidecategories = Object, meta = (DisplayName = "Size By Speed"), MinimalAPI)
class UParticleModuleSizeScaleBySpeed : public UParticleModuleSizeBase
{
    DECLARE_CLASS(UParticleModuleSizeScaleBySpeed, UParticleModuleSizeBase)
public:
    UParticleModuleSizeScaleBySpeed();
    ~UParticleModuleSizeScaleBySpeed() = default;
    
    /** By how much speed affects the size of the particle in each dimension. */
    //UPROPERTY(EditAnywhere, Category = ParticleModuleSizeScaleBySpeed)
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, FVector2D, SpeedScale)

    /** The maximum amount by which to scale a particle in each dimension. */
    //UPROPERTY(EditAnywhere, Category = ParticleModuleSizeScaleBySpeed)
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, FVector2D, MaxScale)

    //~ Begin UParticleModule Interface
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo);
    // End UParticleModule Interface
};



