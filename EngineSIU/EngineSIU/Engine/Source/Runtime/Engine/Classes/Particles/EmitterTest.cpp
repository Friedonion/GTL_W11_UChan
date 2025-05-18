#include "EmitterTest.h"

#include "ParticleSystem.h"
#include "UObject/ObjectFactory.h"
#include "Particles/ParticleEmitter.h"

void AEmitterTest::PostSpawnInitialize()
{
   // AActor::PostSpawnInitialize();

    ParticleSystemComponent = AddComponent<UParticleSystemComponent>();
    RootComponent = ParticleSystemComponent;

    UParticleEmitter* Emitter = FObjectFactory::ConstructObject<UParticleEmitter>(this);
    ParticleSystemComponent->Template->Emitters.Add(Emitter);
    
    
}

void AEmitterTest::Tick(float DeltaTime)
{
    AActor::Tick(DeltaTime);
}
