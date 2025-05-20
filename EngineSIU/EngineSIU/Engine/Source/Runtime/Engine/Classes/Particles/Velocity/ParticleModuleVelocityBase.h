#pragma once

#include "Particles/ParticleModule.h"

class UParticleModuleVelocityBase : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleVelocityBase, UParticleModule)

    UParticleModuleVelocityBase();
    ~UParticleModuleVelocityBase() = default;

    /**
     *	If true, then treat the velocity as world-space defined.
     *	NOTE: LocalSpace emitters that are moving will see strange results...
     */
    //UPROPERTY(EditAnywhere, Category=Velocity)
    UPROPERTY
    (EditAnywhere, uint8, bInWorldSpace, = 1)

    /** If true, then apply the particle system components scale to the velocity value. */
    //UPROPERTY(EditAnywhere, Category=Velocity)
    UPROPERTY
    (EditAnywhere, uint8, bApplyOwnerScale, = 1)

};
