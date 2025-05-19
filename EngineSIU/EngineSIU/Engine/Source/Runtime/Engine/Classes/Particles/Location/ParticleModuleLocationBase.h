#pragma once

#include "Particles/ParticleModule.h"
#include "UObject/ObjectMacros.h"

class UParticleModuleLocationBase : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleLocationBase, UParticleModule)

    UParticleModuleLocationBase();
    ~UParticleModuleLocationBase() = default;
};
