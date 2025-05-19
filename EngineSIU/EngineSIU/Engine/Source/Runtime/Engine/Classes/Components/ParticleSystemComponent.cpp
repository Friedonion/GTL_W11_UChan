#include "ParticleSystemComponent.h"

#include "UObject/ObjectFactory.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"

#include "Engine/ParticleEmitterInstances.h"
#include "UObject/ObjectFactory.h"

UParticleSystemComponent::UParticleSystemComponent()
    : DeltaTimeTick(0.0f)
    , TimeSinceLastTick(0)
    , bSuppressSpawning(false)
    , bNeedsFinalize(false)
{
    // 템플릿 만들기 테스트
    // 일단 여기에 다 때려넣고 테스트 함
    Template = FObjectFactory::ConstructObject<UParticleSystem>(nullptr);

    UParticleSpriteEmitter* SampleEmitter = FObjectFactory::ConstructObject<UParticleSpriteEmitter>(nullptr);
    SampleEmitter->CreateLODLevel(0);

    UParticleModuleSpawn* SampleSpawnModule = FObjectFactory::ConstructObject<UParticleModuleSpawn>(nullptr);

    SampleEmitter->GetLODLevel(0)->Modules.Add(SampleSpawnModule);

    Template->Emitters.Add(SampleEmitter);
    
    FParticleEmitterInstance* SampleInstance = SampleEmitter->CreateInstance(this);
    EmitterInstances.Add(SampleInstance);

    bSuppressSpawning = false;

    DeltaTimeTick = 0.016f;

    TotalActiveParticles = 0;

    TimeSinceLastTick = 0;

    bNeedsFinalize = false;
}

UParticleSystemComponent::~UParticleSystemComponent()
{
}

int32 UParticleSystemComponent::GetLODLevel()
{
    return LODLevel;
}

// [Particle Sequece] 01 - ComponentTick 
void UParticleSystemComponent::TickComponent(float DeltaTime)
{
    if (Template == nullptr || Template->Emitters.Num() == 0)
    {
        // Disable our tick here, will be enabled when activating
        //SetComponentTickEnabled(false);
        return;
    }

    // control tick rate
    // don't tick if enough time hasn't passed
    if (TimeSinceLastTick + static_cast<uint32>(DeltaTime * 1000.0f) < Template->MinTimeBetweenTicks)
    {
        TimeSinceLastTick += static_cast<uint32>(DeltaTime * 1000.0f);
        return;
    }

    // if enough time has passed, and some of it in previous frames, need to take that into account for DeltaTime
    DeltaTime += TimeSinceLastTick / 1000.0f;
    TimeSinceLastTick = 0;

    DeltaTimeTick = DeltaTime;

    // Orient the Z axis toward the camera
    //if (Template->bOrientZAxisTowardCamera)
    //{
        //OrientZAxisTowardCamera();
    //}

    if (Template->SystemUpdateMode == EPSUM_FixedTime)
    {
        // Use the fixed delta time!
        DeltaTime = Template->UpdateTime_Delta;
    }

    TotalActiveParticles = 0;
    bNeedsFinalize = true;

    Template->UpdateAllModuleLists();
    // Emitter Update
    ComputeTickComponent_Concurrent();
    // 현재 역할 없음
    FinalizeTickComponent();
}

// [Particle Sequece] 02 - Update Emitters, Async update 무시하도록 함
// Original code -> ComputeTickComponent_Concurrent
void UParticleSystemComponent::ComputeTickComponent_Concurrent()
{
    // Tick Subemitters.
    int32 EmitterIndex;

    for (EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
    {
        FParticleEmitterInstance* Instance = EmitterInstances[EmitterIndex];

        if (EmitterIndex + 1 < EmitterInstances.Num())
        {
            FParticleEmitterInstance* NextInstance = EmitterInstances[EmitterIndex + 1];
        }

        if (Instance && Instance->SpriteTemplate)
        {
            if (!(Instance->SpriteTemplate->LODLevels.Num() > 0)) return;

            UParticleLODLevel* SpriteLODLevel = Instance->SpriteTemplate->GetCurrentLODLevel(Instance);
            if (SpriteLODLevel && SpriteLODLevel->bEnabled)
            {
                Instance->Tick(DeltaTimeTick, bSuppressSpawning);
                // 일단 Runtime Material변경 등 Override Material 신경쓰지 않도록 함
                //Instance->Tick_MaterialOverrides(EmitterIndex);
                TotalActiveParticles += Instance->ActiveParticles;
            }
        }
    }
}

// Emitter업데이트 후 정리 작업 추가 작업 등 나중에 추가
void UParticleSystemComponent::FinalizeTickComponent()
{
   
}
