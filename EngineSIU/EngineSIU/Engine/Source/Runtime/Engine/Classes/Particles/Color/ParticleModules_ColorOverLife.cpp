#include "ParticleModuleColorOverLife.h"
#include "ParticleEmitterInstances.h"
#include "UObject/ObjectFactory.h"
#include "UObject/Casts.h"

UParticleModuleColorOverLife::UParticleModuleColorOverLife()
{
    bUpdateModule = true;
    bSpawnModule = false;
    bClampAlpha = true;
    bCurvesAsColor = true;

    InitializeDefaults();
}


void UParticleModuleColorOverLife::InitializeDefaults()
{
    if (!ColorOverLife.IsCreated())
    {
        ColorOverLife.Distribution = FObjectFactory::ConstructObject<UDistributionVector>(this);
        ColorOverLife.Distribution->Constant = FVector(1.0f, 1.0f, 1.0f);
    }

    if (!AlphaOverLife.IsCreated())
    {
        AlphaOverLife.Distribution = FObjectFactory::ConstructObject<UDistributionFloat>(this);
        AlphaOverLife.Distribution->Constant = 1.0f;
    }
}


void UParticleModuleColorOverLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    BEGIN_UPDATE_LOOP
    for (int32 i = 0; i < Owner->ActiveParticles; ++i)
    {
        DECLARE_PARTICLE(Particle, Owner->ParticleData + Owner->ParticleStride * Owner->ParticleIndices[i]);
        float LifeRatio = Particle.RelativeTime;

        FVector ColorVec = ColorOverLife.GetValue(LifeRatio); 
        float Alpha = AlphaOverLife.GetValue(LifeRatio);

        if (bClampAlpha)
        {
            Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
        }

        Particle.Color.R = Particle.BaseColor.R * ColorVec.X;
        Particle.Color.G = Particle.BaseColor.G * ColorVec.Y;
        Particle.Color.B = Particle.BaseColor.B * ColorVec.Z;
        Particle.Color.A = Particle.BaseColor.A * Alpha;
    }
    END_UPDATE_LOOP
}

void UParticleModuleColorOverLife::SetToSensibleDefaults(UParticleEmitter* Owner)
{
    if (UDistributionVector* DistVec = Cast<UDistributionVector>(ColorOverLife.Distribution))
    {
        DistVec->Constant = FVector(1.0f, 1.0f, 1.0f);
    }

    if (UDistributionFloat* DistFloat = Cast<UDistributionFloat>(AlphaOverLife.Distribution))
    {
        DistFloat->Constant = 1.0f;
    }

}

void UParticleModuleColorOverLife::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
    FRandomStream RandomStream(GetName().ToAnsiString().c_str());

    FVector SampleColor = ColorOverLife.GetValue(0.0f, 0, &RandomStream);
    float SampleAlpha = AlphaOverLife.GetValue(0.0f, &RandomStream);

    EmitterInfo.ColorScale = SampleColor;
    EmitterInfo.AlphaScale = SampleAlpha;
}

