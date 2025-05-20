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
    // [TEMP] 임시 초기 속도 설정
    StartVelocity = FVector(0.f, 3.f, 0.f);
    StartVelocityRadial = 5.0f;
}

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleVelocity::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    UParticleModuleVelocityBase::Update(Owner, Offset, DeltaTime);
    
    BEGIN_UPDATE_LOOP
    Particle.Location += Particle.Velocity * DeltaTime;
    END_UPDATE_LOOP
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
