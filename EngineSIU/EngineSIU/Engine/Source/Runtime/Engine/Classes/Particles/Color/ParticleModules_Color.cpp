#include "ParticleEmitterInstances.h"
#include "ParticleModuleColor.h"

UParticleModuleColorBase::UParticleModuleColorBase()
{
}

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
	UParticleModuleColor implementation.
-----------------------------------------------------------------------------*/
UParticleModuleColor::UParticleModuleColor()
{
	bSpawnModule = true;
	bUpdateModule = false;
	bCurvesAsColor = true;
	bClampAlpha = true;
}

void UParticleModuleColor::InitializeDefaults() 
{
	// if (!StartColor.IsCreated())
	// {
	// 	StartColor.Distribution = NewObject<UDistributionVectorConstant>(this, TEXT("DistributionStartColor"));
	// }
	//
	// if (!StartAlpha.IsCreated())
	// {
	// 	UDistributionFloatConstant* DistributionStartAlpha = NewObject<UDistributionFloatConstant>(this, TEXT("DistributionStartAlpha"));
	// 	DistributionStartAlpha->Constant = 1.0f;
	// 	StartAlpha.Distribution = DistributionStartAlpha;
	// }
    StartColor = FVector(1.0f, 1.0f, 1.0f);
    StartAlpha = 1.0f;
}

void UParticleModuleColor::CompileModule( FParticleEmitterBuildInfo& EmitterInfo )
{
	FVector InitialColor;
	float InitialAlpha;

	// Use a self-contained random number stream for compiling the module, so it doesn't differ between cooks.
	FRandomStream RandomStream(GetName().ToAnsiString().c_str());
	InitialColor = StartColor; //.GetValue(0.0f, nullptr, 0, &RandomStream);
	InitialAlpha = StartAlpha; //.GetValue(0.0f, nullptr, &RandomStream);

	EmitterInfo.ColorScale = InitialColor;
	EmitterInfo.AlphaScale = InitialAlpha;
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}


void UParticleModuleColor::SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase)
{
	SPAWN_INIT
	{
		FVector ColorVec	= StartColor; //GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
		float	Alpha		= StartAlpha; //GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
		Particle_SetColorFromVector(ColorVec, Alpha, Particle.Color);
		Particle.BaseColor	= Particle.Color;
	}
}

void UParticleModuleColor::SetToSensibleDefaults(UParticleEmitter* Owner)
{
// 	UDistributionVectorConstant* StartColorDist = Cast<UDistributionVectorConstant>(StartColor.Distribution);
// 	if (StartColorDist)
// 	{
// 		StartColorDist->Constant = FVector(1.0f,1.0f,1.0f);
// 		StartColorDist->bIsDirty = true;
// 	}
//
}
