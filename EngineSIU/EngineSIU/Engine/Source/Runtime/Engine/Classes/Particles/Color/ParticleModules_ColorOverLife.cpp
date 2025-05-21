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
    /*if (!ColorOverLife.IsCreated())
    {
        ColorOverLife.Distribution = FObjectFactory::ConstructObject<UDistributionVector>(this);
        ColorOverLife.Distribution->Constant = FVector(1.0f, 1.0f, 1.0f);
    }

    if (!AlphaOverLife.IsCreated())
    {
        AlphaOverLife.Distribution = FObjectFactory::ConstructObject<UDistributionFloat>(this);
        AlphaOverLife.Distribution->Constant = 1.0f;
    }*/
}


void UParticleModuleColorOverLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    BEGIN_UPDATE_LOOP
        float LerpFactor = Particle.RelativeTime;
        FVector CurrentColorVec = FMath::Lerp(StartColor, EndColor, LerpFactor);
        float CurrentAlpha = FMath::Lerp(AlphaStart, AlphaEnd, LerpFactor);

        Particle.Color.R = CurrentColorVec.X;
        Particle.Color.G = CurrentColorVec.Y;
        Particle.Color.B = CurrentColorVec.Z;
        Particle.Color.A = CurrentAlpha;
    END_UPDATE_LOOP
}

void UParticleModuleColorOverLife::SetToSensibleDefaults(UParticleEmitter* Owner)
{
    /*if (UDistributionVector* DistVec = Cast<UDistributionVector>(ColorOverLife.Distribution))
    {
        DistVec->Constant = FVector(1.0f, 1.0f, 1.0f);
    }

    if (UDistributionFloat* DistFloat = Cast<UDistributionFloat>(AlphaOverLife.Distribution))
    {
        DistFloat->Constant = 1.0f;
    }*/

}

void UParticleModuleColorOverLife::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
    FRandomStream RandomStream(GetName().ToAnsiString().c_str());

    //FVector SampleColor = ColorOverLife.GetValue(0.0f, 0, &RandomStream);
    //float SampleAlpha = AlphaOverLife.GetValue(0.0f, &RandomStream);

    //EmitterInfo.ColorScale = SampleColor;
    //EmitterInfo.AlphaScale = SampleAlpha;
}

