#include "ParticleEmitterInstances.h"
#include "ParticleModuleColor.h"
#include "UObject/ObjectFactory.h"

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
	bUpdateModule = true;
	bCurvesAsColor = true;
	bClampAlpha = true;

    InitializeDefaults();
}

void UParticleModuleColor::InitializeDefaults() 
{
	 if (!StartColor.IsCreated())
	 {
	 	StartColor.Distribution = FObjectFactory::ConstructObject<UDistributionVector>(nullptr);
        StartColor.Distribution->Constant = FVector(1.0f, 0.0f, 0.0f);
        StartColor.Op = ERawDistributionOperation::RDO_Random;
	 }
	
	 if (!StartAlpha.IsCreated())
	 {
	 	UDistributionFloat* DistributionStartAlpha = FObjectFactory::ConstructObject<UDistributionFloat>(nullptr);
	 	DistributionStartAlpha->Constant = 1.0f;
        StartAlpha.Op = ERawDistributionOperation::RDO_Random;
	 	StartAlpha.Distribution = DistributionStartAlpha;
	 }
    //StartColor = FVector(1.0f, 0.0f, 0.0f);
    //StartAlpha = 1.0f;
}

void UParticleModuleColor::CompileModule( FParticleEmitterBuildInfo& EmitterInfo )
{
	FVector InitialColor;
	float InitialAlpha;

	// Use a self-contained random number stream for compiling the module, so it doesn't differ between cooks.
	FRandomStream RandomStream(GetName().ToAnsiString().c_str());
	InitialColor = StartColor.GetValue(0.0f, 0, &RandomStream);
	InitialAlpha = StartAlpha.GetValue(0.0f, &RandomStream);

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
        FVector ColorVec	= StartColor.GetValue(Owner->EmitterTime, 0, InRandomStream);
        //FVector ColorVec = FVector(0);
        float	Alpha		= StartAlpha.GetValue(Owner->EmitterTime, InRandomStream);
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
