#include "ParticleEmitterInstances.h"
#include "ParticleModuleLocation.h"
#include "Particles/ParticleEmitter.h"

UParticleModuleLocationBase::UParticleModuleLocationBase()
{
}

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	UParticleModuleLocation implementation.
-----------------------------------------------------------------------------*/

UParticleModuleLocation::UParticleModuleLocation()
{
	bSpawnModule = true;
	bSupported3DDrawMode = true;
	DistributeOverNPoints = 0.0f;
}

void UParticleModuleLocation::InitializeDefaults()
{
	// if (!StartLocation.IsCreated())
	// {
	// 	StartLocation.Distribution = NewObject<UDistributionVectorUniform>(this, TEXT("DistributionStartLocation"));
	// }
    StartLocation = FVector(0.0f, 0.0f, 0.0f);
}

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleLocation::SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase)
{
	SPAWN_INIT
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	//check(LODLevel);
	FVector LocationOffset;

	// Avoid divide by zero.
	if ((DistributeOverNPoints != 0.0f) && (DistributeOverNPoints != 1.f))
	{
		float RandomNum = InRandomStream->FRand() * FMath::Fractional(Owner->EmitterTime);

		if(RandomNum > DistributeThreshold)
		{
			LocationOffset = StartLocation; //.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
		}
		else
		{
			FVector Min, Max;
			//StartLocation.GetRange(Min, Max);
			FVector Lerped = FMath::Lerp(Min, Max, FMath::TruncToFloat((InRandomStream->FRand() * (DistributeOverNPoints - 1.0f)) + 0.5f)/(DistributeOverNPoints - 1.0f));
			LocationOffset = FVector(Lerped.X, Lerped.Y, Lerped.Z);
		}
	}
	else
	{
		LocationOffset = StartLocation; //.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
	}

	LocationOffset = FMatrix::TransformVector(LocationOffset, Owner->EmitterToSimulation);
	Particle.Location += LocationOffset;
	//ensureMsgf(!Particle.Location.ContainsNaN(), TEXT("NaN in Particle Location. Template: %s, Component: %s"), Owner->Component ? *GetNameSafe(Owner->Component->Template) : TEXT("UNKNOWN"), *GetPathNameSafe(Owner->Component));
}
