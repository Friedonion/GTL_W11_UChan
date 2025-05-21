#pragma once
#include "Particles/ParticleModule.h"

class UParticleModuleSubUVBase : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleSubUVBase, UParticleModule)

    UParticleModuleSubUVBase();
    ~UParticleModuleSubUVBase() = default;

    // Begin UParticleModule Interface
    virtual EModuleType	GetModuleType() const override { return EPMT_SubUV; }
    //End UParticleModule Interface
};
