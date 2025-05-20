#include "Engine/ParticleHelper.h"
#include "Engine/ParticleEmitterInstances.h"
#include "Particles/Size/ParticleModuleSizeScaleBySpeed.h"

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
