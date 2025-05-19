#pragma once

#include "UObject/ObjectMacros.h"
#include "Particles/Lifetime/ParticleModuleLifetime.h"

struct FParticleEmitterInstance;

class UParticleModuleLifetime_Seeded : public UParticleModuleLifetime
{
    DECLARE_CLASS(UParticleModuleLifetime_Seeded, UParticleModuleLifetime)

    UParticleModuleLifetime_Seeded();
    ~UParticleModuleLifetime_Seeded() = default;
    /** The random seed(s) to use for looking up values in StartLocation */
    //UPROPERTY(EditAnywhere, Category=RandomSeed)
    struct FParticleRandomSeedInfo RandomSeedInfo;


    //Begin UParticleModule Interface
    virtual FParticleRandomSeedInfo* GetRandomSeedInfo() override
    {
        return &RandomSeedInfo;
    }
    virtual void EmitterLoopingNotify(FParticleEmitterInstance* Owner) override;
    //End UParticleModule Interface

    //~ Begin UParticleModuleLifetimeBase Interface
    virtual float	GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data = NULL) override;
    //~ End UParticleModuleLifetimeBase Interface
};
