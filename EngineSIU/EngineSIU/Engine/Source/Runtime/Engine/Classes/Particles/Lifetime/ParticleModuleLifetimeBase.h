#pragma once

#include "Particles/ParticleModule.h"
#include "UObject/ObjectMacros.h"

struct FParticleEmitterInstance;

class UParticleModuleLifetimeBase : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleLifetimeBase, UParticleModule)

public:
    UParticleModuleLifetimeBase() = default;
    ~UParticleModuleLifetimeBase() = default;
    /** Return the maximum lifetime this module would return. */
    virtual float	GetMaxLifetime()
    {
        return 0.0f;
    }

    /**
     *	Return the lifetime value at the given time.
     *
     *	@param	Owner		The emitter instance that owns this module
     *	@param	InTime		The time input for retrieving the lifetime value
     *	@param	Data		The data associated with the distribution
     *
     *	@return	float		The Lifetime value
     */
    virtual float	GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data = NULL);
};

inline float UParticleModuleLifetimeBase::GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data)
{
    return 0.f;
}
