#pragma once

#include "Particles/Lifetime/ParticleModuleLifetimeBase.h"
#include "UObject/ObjectMacros.h"

class UParticleEmitter;
struct FParticleEmitterInstance;

class UParticleModuleLifetime : public UParticleModuleLifetimeBase
{
    DECLARE_CLASS(UParticleModuleLifetime, UParticleModuleLifetimeBase)

public:
    UParticleModuleLifetime();
    ~UParticleModuleLifetime() = default;
    /** The lifetime of the particle, in seconds. Retrieved using the EmitterTime at the spawn of the particle. */
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, float, Lifetime)

    /** Initializes the default values for this property */
    void InitializeDefaults();

    //Begin UParticleModule Interface
    virtual void CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo ) override;
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    virtual void SetToSensibleDefaults(UParticleEmitter* Owner) override;
    //End UParticleModule Interface

    //~ Begin UParticleModuleLifetimeBase Interface
    virtual float GetMaxLifetime() override;
    virtual float GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data = NULL) override;
    //~ End UParticleModuleLifetimeBase Interface

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    /**
     *	Extended version of spawn, allows for using a random stream for distribution value retrieval
     *
     *	@param	Owner				The particle emitter instance that is spawning
     *	@param	Offset				The offset to the modules payload data
     *	@param	SpawnTime			The time of the spawn
     *	@param	InRandomStream		The random stream to use for retrieving random values
     */
    void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};
