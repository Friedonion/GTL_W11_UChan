#pragma once

#include "Core/Container/Array.h"
#include "UObject/ObjectMacros.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/Spawn/ParticleModuleSpawnBase.h"

class UParticleLODLevel;

//UCLASS(editinlinenew, hidecategories = Object, hidecategories = ParticleModuleSpawnBase, MinimalAPI, meta = (DisplayName = "Spawn"))
class UParticleModuleSpawn : public UParticleModuleSpawnBase
{
    DECLARE_CLASS(UParticleModuleSpawn, UParticleModuleSpawnBase)
public:
    UParticleModuleSpawn();
    ~UParticleModuleSpawn() = default;

    /** The rate at which to spawn particles. */
    //UPROPERTY(EditAnywhere, Category = Spawn)
    //struct FRawDistributionFloat Rate;
    float Rate;

    /** The scalar to apply to the rate. */
    //UPROPERTY(EditAnywhere, Category = Spawn)
    //struct FRawDistributionFloat RateScale;
    float RateScale;
    /** The array of burst entries. */
    //UPROPERTY(EditAnywhere, export, noclear, Category = Burst)
    TArray<FParticleBurst> BurstList;

    /** Scale all burst entries by this amount. */
    //UPROPERTY(EditAnywhere, Category = Burst)
    //struct FRawDistributionFloat BurstScale;
    float BurstScale;

    /** The method to utilize when burst-emitting particles. */
    //UPROPERTY(EditAnywhere, Category = Burst)
    EParticleBurstMethod ParticleBurstMethod;

    /**	If true, the SpawnRate will be scaled by the global CVar r.EmitterSpawnRateScale */
    //UPROPERTY(EditAnywhere, Category = Spawn)
    uint32 bApplyGlobalSpawnRateScale : 1;

    /** Initializes the default values for this property */
    void InitializeDefaults();

    //~ Begin UObject Interface
    //virtual void	PostInitProperties() override;
    //virtual void	PostLoad() override;
    //~ End UObject Interface

    //~ Begin UParticleModule Interface
    virtual bool	GenerateLODModuleValues(UParticleModule* SourceModule, float Percentage, UParticleLODLevel* LODLevel) override;
    //~ End UParticleModule Interface

    //~ Begin UParticleModuleSpawnBase Interface
    virtual bool GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover,
        float DeltaTime, int32& Number, float& Rate) override;
    virtual float GetMaximumSpawnRate() override;
    virtual float GetEstimatedSpawnRate() override;
    virtual int32 GetMaximumBurstCount() override;
};



