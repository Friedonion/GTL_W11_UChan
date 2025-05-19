#pragma once
#include "Components/ParticleSystemComponent.h"
#include "GameFramework/Actor.h"

class AEmitterTest : public AActor
{
    DECLARE_CLASS(AEmitterTest, AActor)
    
public:
    AEmitterTest() = default;
    virtual ~AEmitterTest() override = default;

    virtual void PostSpawnInitialize() override;

    virtual void Tick(float DeltaTime) override;

protected:
    UParticleSystemComponent* ParticleSystemComponent;
};
