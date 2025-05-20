#include "ParticleModuleSize.h"
#include "Engine/ParticleHelper.h"
#include "Engine/ParticleEmitterInstances.h"
#include "Particles/Size/ParticleModuleSizeScaleBySpeed.h"

UParticleModuleSizeBase::UParticleModuleSizeBase()
{
}

/*-----------------------------------------------------------------------------
    UParticleModuleSize implementation.
-----------------------------------------------------------------------------*/

UParticleModuleSize::UParticleModuleSize()
{
    bSpawnModule = true;
    bUpdateModule = true;
    InitializeDefaults();
}

void UParticleModuleSize::InitializeDefaults()
{
    // if (!StartSize.IsCreated())
    // {
    //     UDistributionVectorUniform* DistributionStartSize = NewObject<UDistributionVectorUniform>(this, TEXT("DistributionStartSize"));
    //     DistributionStartSize->Min = FVector(1.0f, 1.0f, 1.0f);
    //     DistributionStartSize->Max = FVector(1.0f, 1.0f, 1.0f);
    //     StartSize.Distribution = DistributionStartSize;
    // }
    StartSize = FVector(1.0f, 1.0f, 1.0f);
}

void UParticleModuleSize::CompileModule( FParticleEmitterBuildInfo& EmitterInfo )
{
    // float MinSize = 0.0f;
    // float MaxSize = 0.0f;
    // StartSize.GetValue();
    // StartSize.GetOutRange( MinSize, MaxSize );
    // EmitterInfo.MaxSize.X *= MaxSize;
    // EmitterInfo.MaxSize.Y *= MaxSize;
    EmitterInfo.SpawnModules.Add( this );
    EmitterInfo.SizeScale =  FVector( 1.0f, 1.0f, 1.0f );
}

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleSize::SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase)
{
    SPAWN_INIT;
    FVector Size		 = StartSize; //.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
    Particle.Size	+= Size;

    //AdjustParticleBaseSizeForUVFlipping(Size, Owner->CurrentLODLevel->RequiredModule->UVFlippingMode, *InRandomStream);
    Particle.BaseSize += Size;
}

/*------------------------------------------------------------------------------
    Scale size by speed module.
------------------------------------------------------------------------------*/
UParticleModuleSizeScaleBySpeed::UParticleModuleSizeScaleBySpeed()
{
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleSizeScaleBySpeed::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    FVector Scale(SpeedScale.X, SpeedScale.Y, 1.0f);
    FVector ScaleMax(MaxScale.X, MaxScale.Y, 1.0f);

    BEGIN_UPDATE_LOOP
    FVector Size = Scale * Particle.Velocity.Size();
    Size = Size.ComponentMax(FVector(1.0f));
    Size = Size.ComponentMin(ScaleMax);
    Particle.Size = GetParticleBaseSize(Particle) * (FVector)Size;
    Particle.Size = (FVector(1.0f, 1.0f, 1.0f)) * (FVector)Size;
    END_UPDATE_LOOP
}

/**
 * Compile the effects of this module on runtime simulation.
 * @param EmitterInfo - Information needed for runtime simulation.
 */
void UParticleModuleSizeScaleBySpeed::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
    EmitterInfo.SizeScaleBySpeed = SpeedScale;
    EmitterInfo.MaxSizeScaleBySpeed = MaxScale;
}
