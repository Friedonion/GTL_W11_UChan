#include "ParticleSystemActor.h"
#include "Components/ParticleSystemComponent.h"
#include "Components/CubeComp.h"

AParticleSystemActor::AParticleSystemActor()
{
    // [TEMP] For test
    SetActorTickInEditor(true);
    ParticleSystemComponent = AddComponent<UParticleSystemComponent>();
}

void AParticleSystemActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ParticleSystemComponent) return;

    ParticleSystemComponent->TickComponent(DeltaTime);
}

UObject* AParticleSystemActor::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->ParticleSystemComponent = ParticleSystemComponent;

    return NewActor;
}
