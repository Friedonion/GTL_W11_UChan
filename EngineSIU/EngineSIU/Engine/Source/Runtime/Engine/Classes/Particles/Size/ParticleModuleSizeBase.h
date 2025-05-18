#pragma once

#include "UObject/ObjectMacros.h"
#include "Particles/ParticleModule.h"

//UCLASS(editinlinenew, hidecategories = Object, abstract, meta = (DisplayName = "Size"))
class UParticleModuleSizeBase : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleSizeBase, UParticleModule)
public:
    UParticleModuleSizeBase() = default;
    ~UParticleModuleSizeBase() = default;
};

