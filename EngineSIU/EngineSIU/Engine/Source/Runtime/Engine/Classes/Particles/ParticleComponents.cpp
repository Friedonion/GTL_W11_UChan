#include "Components/ParticleSystemComponent.h"

#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleSystem.h"

UParticleLODLevel* UParticleEmitter::GetCurrentLODLevel(FParticleEmitterInstance* Instance)
{
    return Instance->CurrentLODLevel;
}

bool UParticleSystem::CanBePooled()const
{
    if (MaxPoolSize == 0)
    {
        return false;
    }

    return true;
}

bool UParticleSystem::CalculateMaxActiveParticleCounts()
{
    bool bSuccess = true;

    for (int32 EmitterIndex = 0; EmitterIndex < Emitters.Num(); EmitterIndex++)
    {
        UParticleEmitter* Emitter = Emitters[EmitterIndex];
        if (Emitter)
        {
            /*if (Emitter->CalculateMaxActiveParticleCount() == false)
            {
                bSuccess = false;
            }*/
        }
    }

    return bSuccess;
}
