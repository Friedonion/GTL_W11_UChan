#pragma once
#include "ParticleModuleColorBase.h"
#include "Distributions/DistributionFloat.h"
#include "Distributions/DistributionVector.h"

struct FBaseParticle;
struct FParticleEmitterInstance;

class UParticleModuleColorOverLife : public UParticleModuleColorBase
{
    DECLARE_CLASS(UParticleModuleColorOverLife, UParticleModuleColorBase)

public:
    UParticleModuleColorOverLife();
    ~UParticleModuleColorOverLife() = default;

    UPROPERTY_WITH_FLAGS(EditAnywhere, FVector, StartColor)
    UPROPERTY_WITH_FLAGS(EditAnywhere, FVector, EndColor)
    UPROPERTY_WITH_FLAGS(EditAnywhere, float, AlphaStart)
    UPROPERTY_WITH_FLAGS(EditAnywhere, float, AlphaEnd)
    UPROPERTY(EditAnywhere, uint8, bClampAlpha, = 1)

    virtual void InitializeDefaults();
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    virtual void SetToSensibleDefaults(UParticleEmitter* Owner);
    virtual void CompileModule(FParticleEmitterBuildInfo& EmitterInfo);
};

