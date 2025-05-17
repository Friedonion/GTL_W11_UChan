#include "Components/ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/Spawn/ParticleModuleSpawnBase.h"
#include "Particles/ParticleModule.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"

//UParticleModuleSpawnBase::UParticleModuleSpawnBase(const FObjectInitializer& ObjectInitializer)
//    : Super(ObjectInitializer)
//{
//    bProcessSpawnRate = true;
//    bProcessBurstList = true;
//}

//UParticleModuleSpawn::UParticleModuleSpawn(const FObjectInitializer& ObjectInitializer)
//    : Super(ObjectInitializer)
//{
//    bProcessSpawnRate = true;
//    LODDuplicate = false;
//    bApplyGlobalSpawnRateScale = true;
//}

void UParticleModuleSpawn::InitializeDefaults()
{
    // Initialize Values
}

bool UParticleModuleSpawn::GetSpawnAmount(FParticleEmitterInstance* Owner,
    int32 Offset, float OldLeftover, float DeltaTime, int32& Number, float& InRate)
{
    if (Owner) return true;
    return false;
}

bool UParticleModuleSpawn::GenerateLODModuleValues(UParticleModule* SourceModule, float Percentage, UParticleLODLevel* LODLevel)
{
    return true;
}

float UParticleModuleSpawn::GetMaximumSpawnRate()
{
    /*float MinSpawn, MaxSpawn;
    float MinScale, MaxScale;

    Rate.GetOutRange(MinSpawn, MaxSpawn);
    RateScale.GetOutRange(MinScale, MaxScale);*/

    //return (MaxSpawn * MaxScale);
    return Rate * RateScale;
}

float UParticleModuleSpawn::GetEstimatedSpawnRate()
{
    return Rate * RateScale;
}

int32 UParticleModuleSpawn::GetMaximumBurstCount()
{
    int32 MaxBurst = 0;
    for (int32 BurstIndex = 0; BurstIndex < BurstList.Num(); BurstIndex++)
    {
        MaxBurst += BurstList[BurstIndex].Count;
    }
    return MaxBurst;
}
