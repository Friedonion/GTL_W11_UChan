#include "ParticleEmitterInstances.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleSystem.h"
#include "Particles/Spawn/ParticleModuleSpawnBase.h"
#include "Components/ParticleSystemComponent.h"
#include "Userinterface/Console.h"

// [Particle Sequece] 03 - ParticleEmitterInstance 업데이트, 파티클 Pool관리 및 ParticleModule update
void FParticleEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning)
{
    if (!SpriteTemplate || !(SpriteTemplate->LODLevels.Num() > 0)) return;

    bool bFirstTime = (SecondsSinceCreation > 0.0f) ? false : true;

    // Grab the current LOD level
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();

    // Handle EmitterTime setup, looping, etc.
    //float EmitterDelay = Tick_EmitterTimeSetup(DeltaTime, LODLevel);

    if (bEnabled)
    {
        // Kill off any dead particles
        KillParticles();

        // Reset particle parameters.
        ResetParticleParameters(DeltaTime);

        // Update the particles
        CurrentMaterial = LODLevel->RequiredModule->Material;
        Tick_ModuleUpdate(DeltaTime, LODLevel);

        // Spawn new particles.
        SpawnFraction = Tick_SpawnParticles(DeltaTime, LODLevel, bSuppressSpawning, bFirstTime);

        // PostUpdate (beams only)
        //Tick_ModulePostUpdate(DeltaTime, LODLevel);

        //if (ActiveParticles > 0)
        //{
            // Update the orbit data...
            // UpdateOrbitData(DeltaTime);
            // Calculate bounding box and simulate velocity.
            //UpdateBoundingBox(DeltaTime);
        //}

        //Tick_ModuleFinalUpdate(DeltaTime, LODLevel);

        //CheckEmitterFinished();

        // Invalidate the contents of the vertex/index buffer.
        IsRenderDataDirty = 1;
    }
    else
    {
        FakeBursts();
    }

    // 'Reset' the emitter time so that the delay functions correctly
    //EmitterTime += EmitterDelay;

    // Store the last delta time.
    LastDeltaTime = DeltaTime;

    // Reset particles position offset
    PositionOffsetThisTick = FVector::ZeroVector;
}

void FParticleEmitterInstance::Tick_MaterialOverrides(int32 EmitterIndex)
{
    UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
    bool bOverridden = false;
    if (LODLevel && LODLevel->RequiredModule && Component && Component->Template)
    {
        TArray<FName>& NamedOverrides = LODLevel->RequiredModule->NamedMaterialOverrides;
        TArray<FNamedEmitterMaterial>& Slots = Component->Template->NamedMaterialSlots;
        //TArray<UMaterialInterface*>& EmitterMaterials = Component->EmitterMaterials;
        if (NamedOverrides.Num() > 0)
        {
            //If we have named material overrides then get it's index into the emitter materials array.
            //Only check for [0] in in the named overrides as most emitter types only need one material. Mesh emitters might use more but they override this function.		
            for (int32 CheckIdx = 0; CheckIdx < Slots.Num(); ++CheckIdx)
            {
                if (NamedOverrides[0] == Slots[CheckIdx].Name)
                {
                    //Default to the default material for that slot.
                    CurrentMaterial = Slots[CheckIdx].Material;
                    //if (EmitterMaterials.IsValidIndex(CheckIdx) && nullptr != EmitterMaterials[CheckIdx])
                    //{
                    //    //This material has been overridden externally, e.g. from a BP so use that one.
                    //    CurrentMaterial = EmitterMaterials[CheckIdx];
                    //}

                    bOverridden = true;
                    break;
                }
            }
        }
    }

    if (bOverridden == false && Component)
    {
        /*if (Component->EmitterMaterials.IsValidIndex(EmitterIndex))
        {
            if (Component->EmitterMaterials[EmitterIndex])
            {
                CurrentMaterial = Component->EmitterMaterials[EmitterIndex];
            }
        }*/
    }
}

void FParticleEmitterInstance::KillParticles()
{
    if (ActiveParticles > 0)
    {
        UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
        /*FParticleEventInstancePayload* EventPayload = NULL;
        if (LODLevel->EventGenerator)
        {
            EventPayload = (FParticleEventInstancePayload*)GetModuleInstanceData(LODLevel->EventGenerator);
            if (EventPayload && (EventPayload->bDeathEventsPresent == false))
            {
                EventPayload = NULL;
            }
        }*/

        bool bFoundCorruptIndices = false;
        // Loop over the active particles... If their RelativeTime is > 1.0f (indicating they are dead),
        // move them to the 'end' of the active particle list.
        for (int32 i = ActiveParticles - 1; i >= 0; i--)
        {
            const int32	CurrentIndex = ParticleIndices[i];
            if (CurrentIndex < MaxActiveParticles)
            {
                const uint8* ParticleBase = ParticleData + CurrentIndex * ParticleStride;
                FBaseParticle& Particle = *((FBaseParticle*)ParticleBase);

                if (Particle.RelativeTime > 1.0f)
                {
                    /*if (EventPayload)
                    {
                        LODLevel->EventGenerator->HandleParticleKilled(this, EventPayload, &Particle);
                    }*/
                    // Move it to the 'back' of the list
                    ParticleIndices[i] = ParticleIndices[ActiveParticles - 1];
                    ParticleIndices[ActiveParticles - 1] = CurrentIndex;
                    ActiveParticles--;
                }
            }
            else
            {
                bFoundCorruptIndices = true;
            }
        }

        if (bFoundCorruptIndices)
        {
            UE_LOG(ELogLevel::Error, TEXT("Detected corrupted particle indices."));
            //FixupParticleIndices();
        }
    }
}

void FParticleEmitterInstance::ResetParticleParameters(float DeltaTime)
{
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    
    if (!HighestLODLevel) return;

    for (int32 ParticleIndex = 0; ParticleIndex < ActiveParticles; ParticleIndex++)
    {
        DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
        Particle.Velocity = Particle.BaseVelocity;
        Particle.Size = GetParticleBaseSize(Particle);
        Particle.RotationRate = Particle.BaseRotationRate;
        Particle.Color = Particle.BaseColor;

        bool bJustSpawned = (Particle.Flags & STATE_Particle_JustSpawned) != 0;

        //Don't update position for newly spawned particles. They already have a partial update applied during spawn.
        bool bSkipUpdate = bJustSpawned;

        Particle.RelativeTime += bSkipUpdate ? 0.0f : Particle.OneOverMaxLifetime * DeltaTime;

        /*if (CameraPayloadOffset > 0)
        {
            int32 CurrentOffset = CameraPayloadOffset;
            const uint8* ParticleBase = (const uint8*)&Particle;
            PARTICLE_ELEMENT(FCameraOffsetParticlePayload, CameraOffsetPayload);
            CameraOffsetPayload.Offset = CameraOffsetPayload.BaseOffset;
        }*/
    }
}

void FParticleEmitterInstance::Tick_ModuleUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel)
{
    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    
    if (!HighestLODLevel) return;

    for (int32 ModuleIndex = 0; ModuleIndex < InCurrentLODLevel->UpdateModules.Num(); ModuleIndex++)
    {
        UParticleModule* CurrentModule = InCurrentLODLevel->UpdateModules[ModuleIndex];
        if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bUpdateModule)
        {
            // 하위 클래스에서 구현된 각각의 ParticleModule Update 함수 호출
            CurrentModule->Update(this, GetModuleDataOffset(HighestLODLevel->UpdateModules[ModuleIndex]), DeltaTime);
        }
    }
}

float FParticleEmitterInstance::Tick_SpawnParticles(float DeltaTime, UParticleLODLevel* InCurrentLODLevel, bool bSuppressSpawning, bool bFirstTime)
{
    if (!bHaltSpawning && !bHaltSpawningExternal && !bSuppressSpawning && (EmitterTime >= 0.0f))
    {
        // If emitter is not done - spawn at current rate.
        // If EmitterLoops is 0, then we loop forever, so always spawn.
        if ((InCurrentLODLevel->RequiredModule->EmitterLoops == 0) ||
            (LoopCount < InCurrentLODLevel->RequiredModule->EmitterLoops) ||
            (SecondsSinceCreation < (EmitterDuration * InCurrentLODLevel->RequiredModule->EmitterLoops)) ||
            bFirstTime)
        {
            bFirstTime = false;
            SpawnFraction = Spawn(DeltaTime);
        }
    }
    else if (bFakeBurstsWhenSpawningSupressed)
    {
        FakeBursts();
    }

    return SpawnFraction;
}

void FParticleEmitterInstance::Tick_ModuleFinalUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel)
{
    /*UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
    
    if (!HighestLODLevel) return;

    for (int32 ModuleIndex = 0; ModuleIndex < InCurrentLODLevel->UpdateModules.Num(); ModuleIndex++)
    {
        UParticleModule* CurrentModule = InCurrentLODLevel->UpdateModules[ModuleIndex];
        if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bFinalUpdateModule)
        {
            CurrentModule->FinalUpdate(this, GetModuleDataOffset(HighestLODLevel->UpdateModules[ModuleIndex]), DeltaTime);
        }
    }

    if (InCurrentLODLevel->TypeDataModule && InCurrentLODLevel->TypeDataModule->bEnabled && InCurrentLODLevel->TypeDataModule->bFinalUpdateModule)
    {
        InCurrentLODLevel->TypeDataModule->FinalUpdate(this, GetModuleDataOffset(HighestLODLevel->TypeDataModule), DeltaTime);
    }*/
}

void FParticleEmitterInstance::CheckEmitterFinished()
{
    // Grab the current LOD level
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();

    // figure out if this emitter will no longer spawn particles
    //
    if (this->ActiveParticles == 0)
    {
        //UParticleModuleSpawn* SpawnModule = LODLevel->SpawnModule;
        
        //if (!SpawnModule) return;

        FParticleBurst* LastBurst = nullptr;
        /*if (SpawnModule->BurstList.Num())
        {
            LastBurst = &SpawnModule->BurstList.Last();
        }*/

        if (!LastBurst || LastBurst->Time < this->EmitterTime)
        {
            const UParticleModuleRequired* RequiredModule = LODLevel->RequiredModule;
            
            if (!RequiredModule) return;

            /*if (HasCompleted() ||
                (SpawnModule->GetMaximumSpawnRate() == 0
                    && RequiredModule->EmitterDuration == 0
                    && RequiredModule->EmitterLoops == 0)
                )
            {
                bEmitterIsDone = true;
            }*/
        }
    }
}

UParticleLODLevel* FParticleEmitterInstance::GetCurrentLODLevelChecked()
{
    if (SpriteTemplate == NULL) return nullptr;
    UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
    if (LODLevel == NULL) return nullptr;
    if (LODLevel->RequiredModule == NULL) return nullptr;
    return LODLevel;
}

void FParticleEmitterInstance::FakeBursts()
{
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    //if (LODLevel->SpawnModule->BurstList.Num() > 0)
    //{
    //    // For each burst in the list
    //    for (int32 BurstIdx = 0; BurstIdx < LODLevel->SpawnModule->BurstList.Num(); BurstIdx++)
    //    {
    //        FParticleBurst* BurstEntry = &(LODLevel->SpawnModule->BurstList[BurstIdx]);
    //        // If it hasn't been fired
    //        if (LODLevel->Level < BurstFired.Num())
    //        {
    //            FLODBurstFired& LocalBurstFired = BurstFired[LODLevel->Level];
    //            if (BurstIdx < LocalBurstFired.Fired.Num())
    //            {
    //                if (EmitterTime >= BurstEntry->Time)
    //                {
    //                    LocalBurstFired.Fired[BurstIdx] = true;
    //                }
    //            }
    //        }
    //    }
    //}
}

uint32 FParticleEmitterInstance::GetModuleDataOffset(UParticleModule* Module)
{
    if (!SpriteTemplate) return 0;

    uint32* Offset = SpriteTemplate->ModuleOffsetMap.Find(Module);
    return (Offset != nullptr) ? *Offset : 0;
}

float FParticleEmitterInstance::Spawn(float DeltaTime)
{
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();

    // For beams, we probably want to ignore the SpawnRate distribution,
    // and focus strictly on the BurstList...
    float SpawnRate = 0.0f;
    int32 SpawnCount = 0;
    int32 BurstCount = 0;
    float SpawnRateDivisor = 0.0f;
    float OldLeftover = SpawnFraction;

    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];

    bool bProcessSpawnRate = true;
    bool bProcessBurstList = true;

    if (SpriteTemplate->QualityLevelSpawnRateScale > 0.0f)
    {
        // Process all Spawning modules that are present in the emitter.
        for (int32 SpawnModIndex = 0; SpawnModIndex < LODLevel->SpawningModules.Num(); SpawnModIndex++)
        {
            UParticleModuleSpawnBase* SpawnModule = LODLevel->SpawningModules[SpawnModIndex];
            if (SpawnModule && SpawnModule->bEnabled)
            {
                UParticleModule* OffsetModule = HighestLODLevel->SpawningModules[SpawnModIndex];
                uint32 Offset = GetModuleDataOffset(OffsetModule);

                // Update the spawn rate
                int32 Number = 0;
                float Rate = 0.0f;
                if (SpawnModule->GetSpawnAmount(this, Offset, OldLeftover, DeltaTime, Number, Rate) == false)
                {
                    bProcessSpawnRate = false;
                }

                Number = FMath::Max<int32>(0, Number);
                Rate = FMath::Max<float>(0.0f, Rate);

                SpawnCount += Number;
                SpawnRate += Rate;
                // Update the burst list
                int32 BurstNumber = 0;
                if (SpawnModule->GetBurstCount(this, Offset, OldLeftover, DeltaTime, BurstNumber) == false)
                {
                    bProcessBurstList = false;
                }

                BurstCount += BurstNumber;
            }
        }

        // Figure out spawn rate for this tick.
        if (bProcessSpawnRate)
        {
            //float RateScale = LODLevel->SpawnModule->RateScale.GetValue(EmitterTime, Component) * LODLevel->SpawnModule->GetGlobalRateScale();
            float RateScale = LODLevel->SpawnModule->RateScale;

            SpawnRate += LODLevel->SpawnModule->Rate * RateScale;
            SpawnRate = FMath::Max<float>(0.0f, SpawnRate);
        }

        // Take Bursts into account as well...
        if (bProcessBurstList)
        {
            int32 Burst = 0;
            float BurstTime = GetCurrentBurstRateOffset(DeltaTime, Burst);
            BurstCount += Burst;
        }

        //float QualityMult = SpriteTemplate->GetQualityLevelSpawnRateMult();
        //SpawnRate = FMath::Max<float>(0.0f, SpawnRate * QualityMult);
        //BurstCount = FMath::CeilToInt(BurstCount * QualityMult);
    }
    else
    {
        // Disable any spawning if MediumDetailSpawnRateScale is 0 and we are not in high detail mode
        SpawnRate = 0.0f;
        SpawnCount = 0;
        BurstCount = 0;
    }

    // Spawn new particles...
    if ((SpawnRate > 0.f) || (BurstCount > 0))
    {
        float SafetyLeftover = OldLeftover;
        // Ensure continuous spawning... lots of fiddling.
        float	NewLeftover = OldLeftover + DeltaTime * SpawnRate;
        int32		Number = FMath::FloorToInt(NewLeftover);
        float	Increment = (SpawnRate > 0.0f) ? (1.f / SpawnRate) : 0.0f;
        float	StartTime = DeltaTime + OldLeftover * Increment - Increment;
        NewLeftover = NewLeftover - Number;

        // Handle growing arrays.
        bool bProcessSpawn = true;
        int32 NewCount = ActiveParticles + Number + BurstCount;

        //float	BurstIncrement = SpriteTemplate->bUseLegacySpawningBehavior ? (BurstCount > 0.0f) ? (1.f / BurstCount) : 0.0f : 0.0f;
        //float	BurstStartTime = SpriteTemplate->bUseLegacySpawningBehavior ? DeltaTime * BurstIncrement : 0.0f;
        // [TEMP]
        float	BurstIncrement = 0.0f;
        float	BurstStartTime = 0.0f;

        if (NewCount >= MaxActiveParticles)
        {
            if (DeltaTime < PeakActiveParticleUpdateDelta)
            {
                bProcessSpawn = Resize(NewCount + FMath::TruncToInt(FMath::Sqrt(FMath::Sqrt((float)NewCount)) + 1));
            }
            else
            {
                bProcessSpawn = Resize((NewCount + FMath::TruncToInt(FMath::Sqrt(FMath::Sqrt((float)NewCount)) + 1)), false);
            }
        }

        if (bProcessSpawn == true)
        {
            FParticleEventInstancePayload* EventPayload = NULL;
            /*if (LODLevel->EventGenerator)
            {
                EventPayload = (FParticleEventInstancePayload*)GetModuleInstanceData(LODLevel->EventGenerator);
                if (EventPayload && !EventPayload->bSpawnEventsPresent && !EventPayload->bBurstEventsPresent)
                {
                    EventPayload = NULL;
                }
            }*/

            const FVector InitialLocation = EmitterToSimulation.GetOrigin();

            // Spawn particles.
            SpawnParticles(Number, StartTime, Increment, InitialLocation, FVector::ZeroVector, EventPayload);

            // Burst particles.
            SpawnParticles(BurstCount, BurstStartTime, BurstIncrement, InitialLocation, FVector::ZeroVector, EventPayload);

            return NewLeftover;
        }
        return SafetyLeftover;
    }

    return SpawnFraction;
}

void FParticleEmitterInstance::SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity, struct FParticleEventInstancePayload* EventPayload)
{
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();

    if (ActiveParticles > MaxActiveParticles) return;
    //check(LODLevel->EventGenerator != NULL || EventPayload == NULL);

    // Ensure we don't access particle beyond what is allocated.
    //ensure(ActiveParticles + Count <= MaxActiveParticles);
    Count = FMath::Min<int32>(Count, MaxActiveParticles - ActiveParticles);

   /* if (EventPayload && EventPayload->bBurstEventsPresent && Count > 0)
    {
        LODLevel->EventGenerator->HandleParticleBurst(this, EventPayload, Count);
    }*/

    auto SpawnInternal = [&](bool bLegacySpawnBehavior)
        {
            UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
            float SpawnTime = StartTime;
            float Interp = 1.0f;
            const float InterpIncrement = (Count > 0 && Increment > 0.0f) ? (1.0f / (float)Count) : 0.0f;
            for (int32 i = 0; i < Count; i++)
            {
                // Workaround to released data.
                if (!(ParticleData && ParticleIndices))
                {
                    static bool bErrorReported = false;
                    if (!bErrorReported)
                    {
                        UE_LOG(ELogLevel::Error, TEXT("Detected null particles. Template : %s, Component %s"));
                        bErrorReported = true;
                    }
                    ActiveParticles = 0;
                    Count = 0;
                    continue;
                }

                // Workaround to corrupted indices.
                uint16 NextFreeIndex = ParticleIndices[ActiveParticles];
                if (!(NextFreeIndex < MaxActiveParticles))
                {
                    UE_LOG(ELogLevel::Error, TEXT("Detected corrupted particle indices. Template : %s, Component %s"));
                    FixupParticleIndices();
                    NextFreeIndex = ParticleIndices[ActiveParticles];
                }

                DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * NextFreeIndex);
                const uint32 CurrentParticleIndex = ActiveParticles++;

                if (bLegacySpawnBehavior)
                {
                    StartTime -= Increment;
                    Interp -= InterpIncrement;
                }

                PreSpawn(Particle, InitialLocation, InitialVelocity);
                for (int32 ModuleIndex = 0; ModuleIndex < LODLevel->SpawnModules.Num(); ModuleIndex++)
                {
                    UParticleModule* SpawnModule = LODLevel->SpawnModules[ModuleIndex];
                    if (SpawnModule->bEnabled)
                    {
                        UParticleModule* OffsetModule = HighestLODLevel->SpawnModules[ModuleIndex];
                        SpawnModule->Spawn(this, GetModuleDataOffset(OffsetModule), SpawnTime, Particle);

                        //ensureMsgf(!Particle->Location.ContainsNaN(), TEXT("NaN in Particle Location. Template: %s, Component: %s"), Component ? *GetNameSafe(Component->Template) : TEXT("UNKNOWN"), *GetPathNameSafe(Component));
                    }
                }
                PostSpawn(Particle, Interp, SpawnTime);

                // Spawn modules may set a relative time greater than 1.0f to indicate that a particle should not be spawned. We kill these particles.
                if (Particle->RelativeTime > 1.0f)
                {
                    KillParticle(CurrentParticleIndex);

                    // Process next particle
                    continue;
                }

               /* if (EventPayload)
                {
                    if (EventPayload->bSpawnEventsPresent)
                    {
                        LODLevel->EventGenerator->HandleParticleSpawned(this, EventPayload, Particle);
                    }
                }*/

                if (!bLegacySpawnBehavior)
                {
                    SpawnTime -= Increment;
                    Interp -= InterpIncrement;
                }
            }
        };

    // 한쪽중에 고르면 될듯
    /*if (SpriteTemplate->bUseLegacySpawningBehavior)
    {
        SpawnInternal(true);
    }
    else
    {
        SpawnInternal(false);
    }*/
}
