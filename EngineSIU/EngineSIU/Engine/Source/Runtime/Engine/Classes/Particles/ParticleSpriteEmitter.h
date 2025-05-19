#pragma once

#include "UObject/ObjectMacros.h"
#include "Particles/ParticleEmitter.h"

class UParticleSystemComponent;

enum EParticleScreenAlignment : int
{
    PSA_FacingCameraPosition,
    PSA_Square,
    PSA_Rectangle,
    PSA_Velocity,
    PSA_AwayFromCenter,
    PSA_TypeSpecific,
    PSA_FacingCameraDistanceBlend,
    PSA_MAX,
};

//UCLASS(collapsecategories, hidecategories = Object, editinlinenew, MinimalAPI)
class UParticleSpriteEmitter : public UParticleEmitter
{
    DECLARE_CLASS(UParticleSpriteEmitter, UParticleEmitter)
public:
    UParticleSpriteEmitter();
    //~UParticleSpriteEmitter();

    //~ Begin UObject Interface
    //virtual void PostLoad() override;
    //~ End UObject Interface

    //~ Begin UParticleEmitter Interface
    virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InComponent) override;
    //virtual void SetToSensibleDefaults() override;
    //~ End UParticleEmitter Interface
};

