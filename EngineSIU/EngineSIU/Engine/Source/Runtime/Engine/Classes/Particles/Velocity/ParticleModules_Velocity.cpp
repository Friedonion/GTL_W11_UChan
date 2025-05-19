#include "ParticleEmitterInstances.h"
#include "ParticleModuleVelocity.h"
#include "Components/ParticleSystemComponent.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"

UParticleModuleVelocityBase::UParticleModuleVelocityBase()
{
}

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	UParticleModuleVelocity implementation.
-----------------------------------------------------------------------------*/

UParticleModuleVelocity::UParticleModuleVelocity()
{
	bSpawnModule = true;
    InitializeDefaults();
}

void UParticleModuleVelocity::InitializeDefaults()
{
	// if (!StartVelocity.IsCreated())
	// {
	// 	StartVelocity.Distribution = NewObject<UDistributionVectorUniform>(this, TEXT("DistributionStartVelocity"));
	// }
	//
	// if (!StartVelocityRadial.IsCreated())
	// {
	// 	StartVelocityRadial.Distribution = NewObject<UDistributionFloatUniform>(this, TEXT("DistributionStartVelocityRadial"));
	// }
    StartVelocity = FVector(0.0f, 0.0f, 0.0f);
    StartVelocityRadial = 0.0f;
}

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleVelocity::SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase)
{
	SPAWN_INIT
	{
		FVector Vel = StartVelocity; //.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
		FVector FromOrigin = (Particle.Location - Owner->EmitterToSimulation.GetOrigin()).GetSafeNormal();

		FVector OwnerScale(1.0f);
		if ((bApplyOwnerScale == true) && Owner->Component)
		{
			OwnerScale = Owner->Component->GetComponentTransform().GetScale3D();
		}

		UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
		//check(LODLevel);
		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			if (bInWorldSpace == true)
			{
				Vel = Owner->SimulationToWorld.InverseTransformVector(Vel);
			}
			else
			{
				Vel = FMatrix::TransformVector(Vel, Owner->EmitterToSimulation);
			}
		}
		else if (bInWorldSpace == false)
		{
			Vel = FMatrix::TransformVector(Vel, Owner->EmitterToSimulation);
		}
		Vel *= OwnerScale;
		Vel += FromOrigin * StartVelocityRadial; //.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream) * OwnerScale;
		Particle.Velocity		+= Vel;
		Particle.BaseVelocity	+= Vel;
	}
}
