#include "Components/ParticleSystemComponent.h"

#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/TypeData/ParticleModuleTypeDataBase.h"

#include "UObject/ObjectFactory.h"
#include "UObject/Casts.h"

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

UParticleEmitter::UParticleEmitter()
    : bDisabledLODsKeepEmitterAlive(false)
    , QualityLevelSpawnRateScale(1.0f)
{
    // Structure to hold one-time initialization
    struct FConstructorStatics
    {
        FName NAME_Particle_Emitter;
        FConstructorStatics()
            : NAME_Particle_Emitter(TEXT("Particle Emitter"))
        {
        }
    };
    static FConstructorStatics ConstructorStatics;

    EmitterName = ConstructorStatics.NAME_Particle_Emitter;
    ConvertedModules = true;
    PeakActiveParticles = 0;
}

UParticleSystem::UParticleSystem()
    : bAnyEmitterLoopsForever(false)
    , bIsImmortal(false)
    , bWillBecomeZombie(false)
{
    UpdateTime_FPS = 60.0f;
    UpdateTime_Delta = 1.0f / 60.0f;
    WarmupTime = 0.0f;
    WarmupTickRate = 0.0f;

    LODMethod = PARTICLESYSTEMLODMETHOD_Automatic;
    LODDistanceCheckTime = 0.25f;
    bRegenerateLODDuplicate = false;
    ThumbnailImageOutOfDate = true;

    MacroUVPosition = FVector(0.0f, 0.0f, 0.0f);

    MacroUVRadius = 200.0f;
    bAutoDeactivate = true;
    MinTimeBetweenTicks = 0;
    MaxPoolSize = 32;

    bAllowManagedTicking = true;
}

FParticleEmitterInstance* UParticleEmitter::CreateInstance(UParticleSystemComponent* InComponent)
{
    return NULL;
}

UParticleSpriteEmitter::UParticleSpriteEmitter()
{
}

FParticleEmitterInstance* UParticleSpriteEmitter::CreateInstance(UParticleSystemComponent* InComponent)
{
    // If this emitter was cooked out or has no valid LOD levels don't create an instance for it.
    if ((bCookedOut == true) || (LODLevels.Num() == 0))
    {
        return NULL;
    }

    FParticleEmitterInstance* Instance = 0;

    UParticleLODLevel* LODLevel = GetLODLevel(0);
    
    if (!LODLevel) return nullptr;
    if (!InComponent) return nullptr;

    //if (LODLevel->TypeDataModule)
    //{
    //    //@todo. This will NOT work for trails/beams!
    //    Instance = LODLevel->TypeDataModule->CreateInstance(this, InComponent);
    //}
    //else
    {
        Instance = new FParticleSpriteEmitterInstance();
        Instance->InitParameters(this, InComponent);
    }

    if (Instance)
    {
        Instance->CurrentLODLevelIndex = 0;
        Instance->CurrentLODLevel = LODLevels[Instance->CurrentLODLevelIndex];
        Instance->Init();
    }

    return Instance;
}

int32 UParticleEmitter::CreateLODLevel(int32 LODLevel, bool bGenerateModuleData)
{
    int32 LevelIndex = -1;
    UParticleLODLevel* CreatedLODLevel = NULL;

    if (LODLevels.Num() == 0)
    {
        LODLevel = 0;
    }

    // Is the requested index outside a viable range?
    if ((LODLevel < 0) || (LODLevel > LODLevels.Num()))
    {
        return -1;
    }

    // NextHighestLODLevel is the one that will be 'copied'
    UParticleLODLevel* NextHighestLODLevel = NULL;
    int32 NextHighIndex = -1;
    // NextLowestLODLevel is the one (and all ones lower than it) that will have their LOD indices updated
    UParticleLODLevel* NextLowestLODLevel = NULL;
    int32 NextLowIndex = -1;

    // Grab the two surrounding LOD levels...
    if (LODLevel == 0)
    {
        // It is being added at the front of the list... (highest)
        if (LODLevels.Num() > 0)
        {
            NextHighestLODLevel = LODLevels[0];
            NextHighIndex = 0;
            NextLowestLODLevel = NextHighestLODLevel;
            NextLowIndex = 0;
        }
    }
    else
        if (LODLevel > 0)
        {
            NextHighestLODLevel = LODLevels[LODLevel - 1];
            NextHighIndex = LODLevel - 1;
            if (LODLevel < LODLevels.Num())
            {
                NextLowestLODLevel = LODLevels[LODLevel];
                NextLowIndex = LODLevel;
            }
        }

    // Update the LODLevel index for the lower levels and
    // offset the LOD validity flags for the modules...
   /* if (NextLowestLODLevel)
    {
        NextLowestLODLevel->ConditionalPostLoad();
        for (int32 LowIndex = LODLevels.Num() - 1; LowIndex >= NextLowIndex; LowIndex--)
        {
            UParticleLODLevel* LowRemapLevel = LODLevels[LowIndex];
            if (LowRemapLevel)
            {
                LowRemapLevel->SetLevelIndex(LowIndex + 1);
            }
        }
    }*/
    // Create a ParticleLODLevel
    CreatedLODLevel = FObjectFactory::ConstructObject<UParticleLODLevel>(nullptr);

    CreatedLODLevel->Level = LODLevel;
    CreatedLODLevel->bEnabled = true;
    CreatedLODLevel->ConvertedModules = true;
    CreatedLODLevel->PeakActiveParticles = 0;

    // Determine where to place it...
    if (LODLevels.Num() == 0)
    {
        //LODLevels.InsertZeroed(0, 1);
        LODLevels.Add(CreatedLODLevel);
        CreatedLODLevel->Level = 0;
    }
    else
    {
        //LODLevels.InsertZeroed(LODLevel, 1);
        if (LODLevels.Num() < LODLevel) 
        {
            LODLevels.Reserve(LODLevel + 1);
        }

        LODLevels[LODLevel] = CreatedLODLevel;
        CreatedLODLevel->Level = LODLevel;
    }

    if (NextHighestLODLevel)
    {
        //NextHighestLODLevel->ConditionalPostLoad();

        // Generate from the higher LOD level
        /*if (CreatedLODLevel->GenerateFromLODLevel(NextHighestLODLevel, 100.0, bGenerateModuleData) == false)
        {
            UE_LOG(LogParticles, Warning, TEXT("Failed to generate LOD level %d from level %d"), LODLevel, NextHighestLODLevel->Level);
        }*/
    }
    else
    {
        // Create the RequiredModule
        UParticleModuleRequired* RequiredModule = FObjectFactory::ConstructObject<UParticleModuleRequired>(GetOuter());
        //check(RequiredModule);
        //RequiredModule->SetToSensibleDefaults(this);
        CreatedLODLevel->RequiredModule = RequiredModule;

        // The SpawnRate for the required module
        RequiredModule->bUseLocalSpace = false;
        RequiredModule->bKillOnDeactivate = false;
        RequiredModule->bKillOnCompleted = false;
        RequiredModule->EmitterDuration = 1.0f;
        RequiredModule->EmitterLoops = 0;
        RequiredModule->ParticleBurstMethod = EPBM_Instant;
        RequiredModule->InterpolationMethod = PSUVIM_None;
        RequiredModule->SubImages_Horizontal = 1;
        RequiredModule->SubImages_Vertical = 1;
        RequiredModule->bScaleUV = false;
        RequiredModule->RandomImageTime = 0.0f;
        RequiredModule->RandomImageChanges = 0;
        RequiredModule->bEnabled = true;

        RequiredModule->LODValidity = (1 << LODLevel);

        // There must be a spawn module as well...
        UParticleModuleSpawn* SpawnModule = FObjectFactory::ConstructObject<UParticleModuleSpawn>(GetOuter());
        //check(SpawnModule);
        CreatedLODLevel->SpawnModule = SpawnModule;
        SpawnModule->LODValidity = (1 << LODLevel);
        //UDistributionFloatConstant* ConstantSpawn = Cast<UDistributionFloatConstant>(SpawnModule->Rate.Distribution);
        //ConstantSpawn->Constant = 10;
        //ConstantSpawn->bIsDirty = true;
        SpawnModule->BurstList.Empty();

        // Copy the TypeData module
        CreatedLODLevel->TypeDataModule = NULL;
    }

    LevelIndex = CreatedLODLevel->Level;

    //MarkPackageDirty();

    return LevelIndex;
}

UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 LODLevel)
{
    if (LODLevel >= LODLevels.Num())
    {
        return NULL;
    }

    return LODLevels[LODLevel];
}

void UParticleLODLevel::UpdateModuleLists()
{
    SpawningModules.Empty();
    SpawnModules.Empty();
    UpdateModules.Empty();

    UParticleModule* Module;
    int32 TypeDataModuleIndex = -1;

    for (int32 i = 0; i < Modules.Num(); i++)
    {
        Module = Modules[i];
        if (!Module)
        {
            continue;
        }

        if (Module->bSpawnModule)
        {
            SpawnModules.Add(Module);
        }
        if (Module->bUpdateModule || Module->bFinalUpdateModule)
        {
            UpdateModules.Add(Module);
        }

        if (Module->IsA(UParticleModuleTypeDataBase::StaticClass()))
        {
            TypeDataModule = CastChecked<UParticleModuleTypeDataBase>(Module);
            if (!Module->bSpawnModule && !Module->bUpdateModule)
            {
                // For now, remove it from the list and set it as the TypeDataModule
                TypeDataModuleIndex = i;
            }
        }
        else
        if (Module->IsA(UParticleModuleSpawnBase::StaticClass()))
        {
            UParticleModuleSpawnBase* SpawnBase = Cast<UParticleModuleSpawnBase>(Module);
            SpawningModules.Add(SpawnBase);
        }
    }

    if (TypeDataModuleIndex != -1)
    {
        Modules.RemoveAt(TypeDataModuleIndex);
    }

    //if (TypeDataModule /**&& (Level == 0)**/)
    //{
    //    UParticleModuleTypeDataMesh* MeshTD = Cast<UParticleModuleTypeDataMesh>(TypeDataModule);
    //    if (MeshTD
    //        && MeshTD->Mesh
    //        && MeshTD->Mesh->HasValidRenderData(false))
    //    {
    //        UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(GetOuter());
    //        if (SpriteEmitter && (MeshTD->bOverrideMaterial == false))
    //        {
    //            FStaticMeshSection& Section = MeshTD->Mesh->GetRenderData()->LODResources[0].Sections[0];
    //            UMaterialInterface* Material = MeshTD->Mesh->GetMaterial(Section.MaterialIndex);
    //            if (Material)
    //            {
    //                RequiredModule->Material = Material;
    //            }
    //        }
    //    }
    //}
}

void UParticleSystem::UpdateAllModuleLists()
{
    for (int32 EmitterIdx = 0; EmitterIdx < Emitters.Num(); EmitterIdx++)
    {
        UParticleEmitter* Emitter = Emitters[EmitterIdx];
        if (Emitter != NULL)
        {
            for (int32 LODIdx = 0; LODIdx < Emitter->LODLevels.Num(); LODIdx++)
            {
                UParticleLODLevel* LODLevel = Emitter->LODLevels[LODIdx];
                if (LODLevel != NULL)
                {
                    LODLevel->UpdateModuleLists();
                }
            }

            // Allow type data module to cache any module info
            if (Emitter->LODLevels.Num() > 0)
            {
                UParticleLODLevel* HighLODLevel = Emitter->LODLevels[0];
                if (HighLODLevel != nullptr && HighLODLevel->TypeDataModule != nullptr)
                {
                    // Allow TypeData module to cache pointers to modules
                    HighLODLevel->TypeDataModule->CacheModuleInfo(Emitter);
                }
            }

            // Update any cached info from modules on the emitter
            //Emitter->CacheEmitterModuleInfo();
        }
    }
}
