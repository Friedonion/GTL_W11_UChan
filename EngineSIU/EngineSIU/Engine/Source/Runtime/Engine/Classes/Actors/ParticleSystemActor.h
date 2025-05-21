#pragma once
#include "GameFramework/Actor.h"
#include "UObject/ObjectMacros.h"

class UParticleSystemComponent;

class AParticleSystemActor : public AActor
{
    DECLARE_CLASS(AParticleSystemActor, AActor)
public:
    AParticleSystemActor();
    
    virtual void Tick(float DeltaTime) override;
    virtual UObject* Duplicate(UObject* InOuter) override;

public:
    UParticleSystemComponent* ParticleSystemComponent;

protected:

};

