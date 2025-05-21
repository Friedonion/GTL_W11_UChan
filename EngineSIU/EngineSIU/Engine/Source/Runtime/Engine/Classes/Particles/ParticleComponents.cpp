#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"
#include "ParticleSpriteEmitter.h"
#include "Components/ParticleSystemComponent.h"
#include "Lifetime/ParticleModuleLifetimeBase.h"

#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleSystem.h"
#include "SubUV/ParticleModuleSubUV.h"
#include "TypeData/ParticleModuleTypeDataBase.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"

/*-----------------------------------------------------------------------------
    UParticleLODLevel implementation.
-----------------------------------------------------------------------------*/
UParticleLODLevel::UParticleLODLevel()
{
    bEnabled = true;
    ConvertedModules = true;
    PeakActiveParticles = 0;
}

int32 UParticleLODLevel::GetModuleIndex(UParticleModule* InModule)
{
    if (InModule)
    {
        if (InModule == RequiredModule)
        {
            return INDEX_REQUIREDMODULE;
        }
        else if (InModule == SpawnModule)
        {
            return INDEX_SPAWNMODULE;
        }
        else if (InModule == TypeDataModule)
        {
            return INDEX_TYPEDATAMODULE;
        }
        else
        {
            for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++)
            {
                if (InModule == Modules[ModuleIndex])
                {
                    return ModuleIndex;
                }
            }
        }
    }

    return INDEX_NONE;
}

UParticleModule* UParticleLODLevel::GetModuleAtIndex(int32 InIndex)
{
    // 'Normal' modules
    if (InIndex > INDEX_NONE)
    {
        if (InIndex < Modules.Num())
        {
            return Modules[InIndex];
        }

        return NULL;
    }

    switch (InIndex)
    {
    case INDEX_REQUIREDMODULE:		return RequiredModule;
    case INDEX_SPAWNMODULE:			return SpawnModule;
    case INDEX_TYPEDATAMODULE:		return TypeDataModule;
    }

    return NULL;
}

void UParticleLODLevel::CompileModules( FParticleEmitterBuildInfo& EmitterBuildInfo )
{
    //check( RequiredModule );
    //check( SpawnModule );

    // Store a few special modules.
    EmitterBuildInfo.RequiredModule = RequiredModule;
    EmitterBuildInfo.SpawnModule = SpawnModule;

    // Compile those special modules.
    RequiredModule->CompileModule( EmitterBuildInfo );
    if ( SpawnModule->bEnabled )
    {
        SpawnModule->CompileModule( EmitterBuildInfo );
    }

    // Compile all remaining modules.
    const int32 ModuleCount = Modules.Num();
    for ( int32 ModuleIndex = 0; ModuleIndex < ModuleCount; ++ModuleIndex )
    {
        UParticleModule* Module = Modules[ModuleIndex];
        if ( Module && Module->bEnabled )
        {
            Module->CompileModule( EmitterBuildInfo );
        }
    }

    // Estimate the maximum number of active particles.
    EmitterBuildInfo.EstimatedMaxActiveParticleCount = CalculateMaxActiveParticleCount();
}

void UParticleLODLevel::UpdateModuleLists()
{
	//LLM_SCOPE(ELLMTag::Particles);

	SpawningModules.Empty();
	SpawnModules.Empty();
	UpdateModules.Empty();
	// OrbitModules.Empty();
	// EventReceiverModules.Empty();
	// EventGenerator = NULL;

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
			UParticleModuleSpawnBase* SpawnBase = CastChecked<UParticleModuleSpawnBase>(Module);
			SpawningModules.Add(SpawnBase);
		}
	}

	if (TypeDataModuleIndex != -1)
	{
		Modules.RemoveAt(TypeDataModuleIndex);
	}

	if (TypeDataModule /**&& (Level == 0)**/)
	{
		// UParticleModuleTypeDataMesh* MeshTD = Cast<UParticleModuleTypeDataMesh>(TypeDataModule);
		// if (MeshTD
		// 	&& MeshTD->Mesh
		// 	&& MeshTD->Mesh->HasValidRenderData(false))
		// {
		// 	UParticleSpriteEmitter* SpriteEmitter = Cast<UParticleSpriteEmitter>(GetOuter());
		// 	if (SpriteEmitter && (MeshTD->bOverrideMaterial == false))
		// 	{
		// 		FStaticMeshSection& Section = MeshTD->Mesh->GetRenderData()->LODResources[0].Sections[0];
		// 		UMaterialInterface* Material = MeshTD->Mesh->GetMaterial(Section.MaterialIndex);
		// 		if (Material)
		// 		{
		// 			RequiredModule->Material = Material;
		// 		}
		// 	}
		// }
	}
}

int32	UParticleLODLevel::CalculateMaxActiveParticleCount()
{
	//check(RequiredModule != NULL);

	// Determine the lifetime for particles coming from the emitter
	float ParticleLifetime = 0.0f;
	float MaxSpawnRate = SpawnModule->GetEstimatedSpawnRate();
	int32 MaxBurstCount = SpawnModule->GetMaximumBurstCount();
	for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++)
	{
		UParticleModuleLifetimeBase* LifetimeMod = Cast<UParticleModuleLifetimeBase>(Modules[ModuleIndex]);
		if (LifetimeMod != NULL)
		{
			ParticleLifetime += LifetimeMod->GetMaxLifetime();
		}

		UParticleModuleSpawnBase* SpawnMod = Cast<UParticleModuleSpawnBase>(Modules[ModuleIndex]);
		if (SpawnMod != NULL)
		{
			MaxSpawnRate += SpawnMod->GetEstimatedSpawnRate();
			MaxBurstCount += SpawnMod->GetMaximumBurstCount();
		}
	}

	// Determine the maximum duration for this particle system
	float MaxDuration = 0.0f;
	float TotalDuration = 0.0f;
	int32 TotalLoops = 0;
	if (RequiredModule != NULL)
	{
		// We don't care about delay wrt spawning...
		MaxDuration = FMath::Max<float>(RequiredModule->EmitterDuration, RequiredModule->EmitterDurationLow);
		TotalLoops = RequiredModule->EmitterLoops;
		TotalDuration = MaxDuration * TotalLoops;
	}

	// Determine the max
	int32 MaxAPC = 0;

	if (TotalDuration != 0.0f)
	{
		if (TotalLoops == 1)
		{
			// Special case for one loop... 
			if (ParticleLifetime < MaxDuration)
			{
				MaxAPC += FMath::CeilToInt(ParticleLifetime * MaxSpawnRate);
			}
			else
			{
				MaxAPC += FMath::CeilToInt(MaxDuration * MaxSpawnRate);
			}
			// Safety zone...
			MaxAPC += 1;
			// Add in the bursts...
			MaxAPC += MaxBurstCount;
		}
		else
		{
			if (ParticleLifetime < MaxDuration)
			{
				MaxAPC += FMath::CeilToInt(ParticleLifetime * MaxSpawnRate);
			}
			else
			{
				MaxAPC += (FMath::CeilToInt(FMath::CeilToInt(MaxDuration * MaxSpawnRate) * ParticleLifetime));
			}
			// Safety zone...
			MaxAPC += 1;
			// Add in the bursts...
			MaxAPC += MaxBurstCount;
			if (ParticleLifetime > MaxDuration)
			{
				MaxAPC += MaxBurstCount * FMath::CeilToInt(ParticleLifetime - MaxDuration);
			}
		}
	}
	else
	{
		// We are infinite looping... 
		// Single loop case is all we will worry about. Safer base estimate - but not ideal.
		if (ParticleLifetime < MaxDuration)
		{
			MaxAPC += FMath::CeilToInt(ParticleLifetime * FMath::CeilToInt(MaxSpawnRate));
		}
		else
		{
			if (ParticleLifetime != 0.0f)
			{
				if (ParticleLifetime <= MaxDuration)
				{
					MaxAPC += FMath::CeilToInt(MaxDuration * MaxSpawnRate);
				}
				else //if (ParticleLifetime > MaxDuration)
				{
					MaxAPC += FMath::CeilToInt(MaxDuration * MaxSpawnRate) * ParticleLifetime;
				}
			}
			else
			{
				// No lifetime, no duration...
				MaxAPC += FMath::CeilToInt(MaxSpawnRate);
			}
		}
		// Safety zone...
		MaxAPC += FMath::Max<int32>(FMath::CeilToInt(MaxSpawnRate * 0.032f), 2);
		// Burst
		MaxAPC += MaxBurstCount;
	}

	PeakActiveParticles = MaxAPC;

	return MaxAPC;
}

/*-----------------------------------------------------------------------------
    UParticleEmitter implementation.
-----------------------------------------------------------------------------*/
UParticleEmitter::UParticleEmitter()
    : bDisabledLODsKeepEmitterAlive(false)
    , QualityLevelSpawnRateScale(1.0f)
    , ReqInstanceBytes(0)
    , ParticleSize(sizeof(FBaseParticle))
    , DetailModeBitmask(0xFFFF) //PDM_DefaultValue
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

FParticleEmitterInstance* UParticleEmitter::CreateInstance(UParticleSystemComponent* InComponent)
{
    return NULL; 
}

void UParticleEmitter::UpdateModuleLists()
{
    for (int32 LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
    {
        UParticleLODLevel* LODLevel = LODLevels[LODIndex];
        if (LODLevel)
        {
            LODLevel->UpdateModuleLists();
        }
    }
    Build();
    //USubUVAnimation* ModuleSubUVAnimation = Cast<UParticleModuleSubUV>(ParticleModule)->Animation;
}

void UParticleEmitter::SetEmitterName(FName Name)
{
    EmitterName = Name;
}

FName& UParticleEmitter::GetEmitterName()
{
    return EmitterName;
}

void UParticleEmitter::SetLODCount(int32 LODCount)
{
    // 
}

int32 UParticleEmitter::CreateLODLevel(int32 LODLevel, bool bGenerateModuleData)
{
	int32					LevelIndex		= -1;
	UParticleLODLevel*	CreatedLODLevel	= NULL;

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
	UParticleLODLevel*	NextHighestLODLevel	= NULL;
	int32 NextHighIndex = -1;
	// NextLowestLODLevel is the one (and all ones lower than it) that will have their LOD indices updated
	UParticleLODLevel*	NextLowestLODLevel	= NULL;
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
	if (NextLowestLODLevel)
	{
		//NextLowestLODLevel->ConditionalPostLoad();
		for (int32 LowIndex = LODLevels.Num() - 1; LowIndex >= NextLowIndex; LowIndex--)
		{
			UParticleLODLevel* LowRemapLevel = LODLevels[LowIndex];
			if (LowRemapLevel)
			{
				//LowRemapLevel->SetLevelIndex(LowIndex + 1);
			}
		}
	}

	// Create a ParticleLODLevel
	CreatedLODLevel = FObjectFactory::ConstructObject<UParticleLODLevel>(this);
	//check(CreatedLODLevel);

	CreatedLODLevel->Level = LODLevel;
	CreatedLODLevel->bEnabled = true;
	CreatedLODLevel->ConvertedModules = true;
	CreatedLODLevel->PeakActiveParticles = 0;

	// Determine where to place it...
	if (LODLevels.Num() == 0)
	{
		//LODLevels.InsertZeroed(0, 1);
		LODLevels.Add(CreatedLODLevel);
		CreatedLODLevel->Level	= 0;
	}
	else
	{
		//LODLevels.InsertZeroed(LODLevel, 1);
        if (LODLevels.Num() < LODLevel) 
        {
            LODLevels.SetNum(LODLevel + 1);
        }
		LODLevels[LODLevel] = CreatedLODLevel;
		CreatedLODLevel->Level = LODLevel;
	}

	if (NextHighestLODLevel)
	{
		//NextHighestLODLevel->ConditionalPostLoad();

		// Generate from the higher LOD level
		// if (CreatedLODLevel->GenerateFromLODLevel(NextHighestLODLevel, 100.0, bGenerateModuleData) == false)
		// {
		// 	UE_LOG(LogParticles, Warning, TEXT("Failed to generate LOD level %d from level %d"), LODLevel, NextHighestLODLevel->Level);
		// }
	}
	else
	{
		// Create the RequiredModule
		UParticleModuleRequired* RequiredModule = FObjectFactory::ConstructObject<UParticleModuleRequired>(GetOuter());
		//check(RequiredModule);
		RequiredModule->SetToSensibleDefaults(this);
		CreatedLODLevel->RequiredModule	= RequiredModule;
	    CreatedLODLevel->Modules.Add(RequiredModule);
		// The SpawnRate for the required module
		RequiredModule->bUseLocalSpace			= false;
		RequiredModule->bKillOnDeactivate		= false;
		RequiredModule->bKillOnCompleted		= false;
		RequiredModule->EmitterDuration			= 1.0f;
		RequiredModule->EmitterLoops			= 0;
		RequiredModule->ParticleBurstMethod		= EParticleBurstMethod::EPBM_Instant;
#if WITH_EDITORONLY_DATA
		RequiredModule->ModuleEditorColor		= FColor::MakeRandomColor();
#endif // WITH_EDITORONLY_DATA
		RequiredModule->InterpolationMethod		= EParticleSubUVInterpMethod::PSUVIM_Linear;
		RequiredModule->SubImages_Horizontal	= 1;
		RequiredModule->SubImages_Vertical		= 1;
		RequiredModule->bScaleUV				= false;
		RequiredModule->RandomImageTime			= 0.0f;
		RequiredModule->RandomImageChanges		= 0;
		RequiredModule->bEnabled				= true;
	    
		RequiredModule->LODValidity = (1 << LODLevel);

		// There must be a spawn module as well...
		UParticleModuleSpawn* SpawnModule = FObjectFactory::ConstructObject<UParticleModuleSpawn>(GetOuter());
		//check(SpawnModule);
		CreatedLODLevel->SpawnModule = SpawnModule;
	    CreatedLODLevel->Modules.Add(SpawnModule);
		SpawnModule->LODValidity = (1 << LODLevel);
	    SpawnModule->Rate = 10;
		//ConstantSpawn->bIsDirty					= true;
		SpawnModule->BurstList.Empty();

		// Copy the TypeData module
		CreatedLODLevel->TypeDataModule			= NULL;
	}

	LevelIndex	= CreatedLODLevel->Level;

	//MarkPackageDirty();

	return LevelIndex;
}

bool UParticleEmitter::IsLODLevelValid(int32 LODLevel)
{
    for (int32 LODIndex = 0; LODIndex < LODLevels.Num(); LODIndex++)
    {
        UParticleLODLevel* CheckLODLevel	= LODLevels[LODIndex];
        if (CheckLODLevel->Level == LODLevel)
        {
            return true;
        }
    }

    return false;
}

UParticleLODLevel* UParticleEmitter::GetCurrentLODLevel(FParticleEmitterInstance* Instance)
{
    return Instance->CurrentLODLevel;
}

UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 LODLevel)
{
    if (LODLevel >= LODLevels.Num())
    {
        return NULL;
    }
    return LODLevels[LODLevel];
}

void UParticleEmitter::Build()
{
    const int32 LODCount = LODLevels.Num();
    if ( LODCount > 0 )
    {
        UParticleLODLevel* HighLODLevel = LODLevels[0];
        //check(HighLODLevel);
        if (HighLODLevel->TypeDataModule != nullptr)
        {
            FParticleEmitterBuildInfo EmitterBuildInfo;
#if WITH_EDITOR
            HighLODLevel->CompileModules( EmitterBuildInfo );
#endif
            if(HighLODLevel->TypeDataModule->RequiresBuild())
            {
                FParticleEmitterBuildInfo EmitterBuildInfo;
                HighLODLevel->TypeDataModule->Build( EmitterBuildInfo );
            }

            // Allow TypeData module to cache pointers to modules
            HighLODLevel->TypeDataModule->CacheModuleInfo(this);
        }

        // Cache particle size/offset data for all LOD Levels
        CacheEmitterModuleInfo();
    }
}

void UParticleEmitter::CacheEmitterModuleInfo()
{
	// This assert makes sure that packing is as expected.
	// Added FBaseColor...
	// Linear color change
	// Added Flags field

	bRequiresLoopNotification = false;
	bAxisLockEnabled = false;
	bMeshRotationActive = false;
	LockAxisFlags = EParticleAxisLock::EPAL_NONE;
	ModuleOffsetMap.Empty();
	ModuleInstanceOffsetMap.Empty();
	ModuleRandomSeedInstanceOffsetMap.Empty();
	ModulesNeedingInstanceData.Empty();
	ModulesNeedingRandomSeedInstanceData.Empty();
	MeshMaterials.Empty();
	DynamicParameterDataOffset = 0;
	LightDataOffset = 0;
	LightVolumetricScatteringIntensity = 0;
	CameraPayloadOffset = 0;
	ParticleSize = sizeof(FBaseParticle);
	ReqInstanceBytes = 0;
	PivotOffset = FVector2D(-0.5f, -0.5f);
	TypeDataOffset = 0;
	TypeDataInstanceOffset = -1;
	SubUVAnimation = nullptr;

	UParticleLODLevel* HighLODLevel = GetLODLevel(0);
	//check(HighLODLevel);

	UParticleModuleTypeDataBase* HighTypeData = HighLODLevel->TypeDataModule;
	if (HighTypeData)
	{
		int32 ReqBytes = HighTypeData->RequiredBytes(static_cast<UParticleModuleTypeDataBase*>(nullptr));
		if (ReqBytes)
		{
			TypeDataOffset = ParticleSize;
			ParticleSize += ReqBytes;
		}

		int32 TempInstanceBytes = HighTypeData->RequiredBytesPerInstance();
		if (TempInstanceBytes)
		{
			TypeDataInstanceOffset = ReqInstanceBytes;
			ReqInstanceBytes += TempInstanceBytes;
		}
	}

	// Grab required module
	UParticleModuleRequired* RequiredModule = HighLODLevel->RequiredModule;
	//check(RequiredModule);
	// mesh rotation active if alignment is set
	//bMeshRotationActive = (RequiredModule->ScreenAlignment == PSA_Velocity || RequiredModule->ScreenAlignment == PSA_AwayFromCenter);

	// NOTE: This code assumes that the same module order occurs in all LOD levels

	for (int32 ModuleIdx = 0; ModuleIdx < HighLODLevel->Modules.Num(); ModuleIdx++)
	{
		UParticleModule* ParticleModule = HighLODLevel->Modules[ModuleIdx];
		//check(ParticleModule);

		// Loop notification?
		bRequiresLoopNotification |= (ParticleModule->bEnabled && ParticleModule->RequiresLoopingNotification());

		if (ParticleModule->IsA(UParticleModuleTypeDataBase::StaticClass()) == false)
		{
			int32 ReqBytes = ParticleModule->RequiredBytes(HighTypeData);
			if (ReqBytes)
			{
				ModuleOffsetMap.Add(ParticleModule, ParticleSize);
				// if (ParticleModule->IsA(UParticleModuleParameterDynamic::StaticClass()) && (DynamicParameterDataOffset == 0))
				// {
				// 	DynamicParameterDataOffset = ParticleSize;
				// }
				// if (ParticleModule->IsA(UParticleModuleLight::StaticClass()) && (LightDataOffset == 0))
				// {
				// 	UParticleModuleLight* ParticleModuleLight = Cast<UParticleModuleLight>(ParticleModule);
				// 	LightVolumetricScatteringIntensity = ParticleModuleLight->VolumetricScatteringIntensity;
				// 	LightDataOffset = ParticleSize;
				// }
				// if (ParticleModule->IsA(UParticleModuleCameraOffset::StaticClass()) && (CameraPayloadOffset == 0))
				// {
				// 	CameraPayloadOffset = ParticleSize;
				// }
				ParticleSize += ReqBytes;
			}

			int32 TempInstanceBytes = ParticleModule->RequiredBytesPerInstance();
			if (TempInstanceBytes > 0)
			{
				// Add the high-lodlevel offset to the lookup map
				ModuleInstanceOffsetMap.Add(ParticleModule, ReqInstanceBytes);
				// Remember that this module has emitter-instance data
				ModulesNeedingInstanceData.Add(ParticleModule);

				// Add all the other LODLevel modules, using the same offset.
				// This removes the need to always also grab the HighestLODLevel pointer.
				for (int32 LODIdx = 1; LODIdx < LODLevels.Num(); LODIdx++)
				{
					UParticleLODLevel* CurLODLevel = LODLevels[LODIdx];
					ModuleInstanceOffsetMap.Add(CurLODLevel->Modules[ModuleIdx], ReqInstanceBytes);
				}
				ReqInstanceBytes += TempInstanceBytes;
			}

			// Add space for per instance random seed value if required
			if (ParticleModule->bSupportsRandomSeed)
			{
				// Add the high-lodlevel offset to the lookup map
				ModuleRandomSeedInstanceOffsetMap.Add(ParticleModule, ReqInstanceBytes);
				// Remember that this module has emitter-instance data
				ModulesNeedingRandomSeedInstanceData.Add(ParticleModule);

				// Add all the other LODLevel modules, using the same offset.
				// This removes the need to always also grab the HighestLODLevel pointer.
				for (int32 LODIdx = 1; LODIdx < LODLevels.Num(); LODIdx++)
				{
					UParticleLODLevel* CurLODLevel = LODLevels[LODIdx];
					ModuleRandomSeedInstanceOffsetMap.Add(CurLODLevel->Modules[ModuleIdx], ReqInstanceBytes);
				}

				ReqInstanceBytes += sizeof(FParticleRandomSeedInstancePayload);
			}
		}

		// if (ParticleModule->IsA(UParticleModuleOrientationAxisLock::StaticClass()))
		// {
		// 	UParticleModuleOrientationAxisLock* Module_AxisLock = CastChecked<UParticleModuleOrientationAxisLock>(ParticleModule);
		// 	bAxisLockEnabled = Module_AxisLock->bEnabled;
		// 	LockAxisFlags = Module_AxisLock->LockAxisFlags;
		// }
		// else if (ParticleModule->IsA(UParticleModulePivotOffset::StaticClass()))
		// {
		// 	PivotOffset += Cast<UParticleModulePivotOffset>(ParticleModule)->PivotOffset;
		// }
		// else if (ParticleModule->IsA(UParticleModuleMeshMaterial::StaticClass()))
		// {
		// 	UParticleModuleMeshMaterial* MeshMaterialModule = CastChecked<UParticleModuleMeshMaterial>(ParticleModule);
		// 	if (MeshMaterialModule->bEnabled)
		// 	{
		// 		MeshMaterials = MeshMaterialModule->MeshMaterials;
		// 	}
		// }
		// else if (ParticleModule->IsA(UParticleModuleSubUV::StaticClass()))
		// {
		// 	USubUVAnimation* ModuleSubUVAnimation = Cast<UParticleModuleSubUV>(ParticleModule)->Animation;
		// 	SubUVAnimation = ModuleSubUVAnimation && ModuleSubUVAnimation->SubUVTexture && ModuleSubUVAnimation->IsBoundingGeometryValid()
		// 		? ModuleSubUVAnimation
		// 		: NULL;
		// }
		// // Perform validation / fixup on some modules that can cause crashes if LODs / Modules are out of sync
		// // This should only be applied on uncooked builds to avoid wasting cycles
		// else if ( !FPlatformProperties::RequiresCookedData() )
		// {
		// 	if (ParticleModule->IsA(UParticleModuleLocationBoneSocket::StaticClass()))
		// 	{
		// 		UParticleModuleLocationBoneSocket::ValidateLODLevels(this, ModuleIdx);
		// 	}
		// }

		// Set bMeshRotationActive if module says so
		if(!bMeshRotationActive && ParticleModule->TouchesMeshRotation())
		{
			bMeshRotationActive = true;
		}
	}
}

/*-----------------------------------------------------------------------------
    UParticleSpriteEmitter implementation.
-----------------------------------------------------------------------------*/
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

    UParticleLODLevel* LODLevel	= GetLODLevel(0);
    //check(LODLevel);

    if (LODLevel->TypeDataModule)
    {
        //@todo. This will NOT work for trails/beams!
        Instance = LODLevel->TypeDataModule->CreateInstance(this, InComponent);
    }
    else
    {
        //check(InComponent);
        Instance = new FParticleSpriteEmitterInstance();
        //check(Instance);
        Instance->InitParameters(this, InComponent);
    }

    if (Instance)
    {
        Instance->CurrentLODLevelIndex	= 0;
        Instance->CurrentLODLevel		= LODLevels[Instance->CurrentLODLevelIndex];
        Instance->Init();
    }

    return Instance;
}

// void UParticleSpriteEmitter::SetToSensibleDefaults()
// {
//     UParticleLODLevel* LODLevel = LODLevels[0];
//
// 	// Spawn rate
// 	LODLevel->SpawnModule->LODValidity = 1;
// 	UDistributionFloatConstant* SpawnRateDist = Cast<UDistributionFloatConstant>(LODLevel->SpawnModule->Rate.Distribution);
// 	if (SpawnRateDist)
// 	{
// 		SpawnRateDist->Constant = 20.f;
// 	}
//
// 	// Create basic set of modules
//
// 	// Lifetime module
// 	UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>(GetOuter());
// 	UDistributionFloatUniform* LifetimeDist = Cast<UDistributionFloatUniform>(LifetimeModule->Lifetime.Distribution);
// 	if (LifetimeDist)
// 	{
// 		LifetimeDist->Min = 1.0f;
// 		LifetimeDist->Max = 1.0f;
// 		LifetimeDist->bIsDirty = true;
// 	}
// 	LifetimeModule->LODValidity = 1;
// 	LODLevel->Modules.Add(LifetimeModule);
//
// 	// Size module
// 	UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>(GetOuter());
// 	UDistributionVectorUniform* SizeDist = Cast<UDistributionVectorUniform>(SizeModule->StartSize.Distribution);
// 	if (SizeDist)
// 	{
// 		SizeDist->Min = FVector(25.f, 25.f, 25.f);
// 		SizeDist->Max = FVector(25.f, 25.f, 25.f);
// 		SizeDist->bIsDirty = true;
// 	}
// 	SizeModule->LODValidity = 1;
// 	LODLevel->Modules.Add(SizeModule);
//
// 	// Initial velocity module
// 	UParticleModuleVelocity* VelModule = NewObject<UParticleModuleVelocity>(GetOuter());
// 	UDistributionVectorUniform* VelDist = Cast<UDistributionVectorUniform>(VelModule->StartVelocity.Distribution);
// 	if (VelDist)
// 	{
// 		VelDist->Min = FVector(-10.f, -10.f, 50.f);
// 		VelDist->Max = FVector(10.f, 10.f, 100.f);
// 		VelDist->bIsDirty = true;
// 	}
// 	VelModule->LODValidity = 1;
// 	LODLevel->Modules.Add(VelModule);
//
// 	// Color over life module
// 	UParticleModuleColorOverLife* ColorModule = NewObject<UParticleModuleColorOverLife>(GetOuter());
// 	UDistributionVectorConstantCurve* ColorCurveDist = Cast<UDistributionVectorConstantCurve>(ColorModule->ColorOverLife.Distribution);
// 	if (ColorCurveDist)
// 	{
// 		// Add two points, one at time 0.0f and one at 1.0f
// 		for (int32 Key = 0; Key < 2; Key++)
// 		{
// 			int32	KeyIndex = ColorCurveDist->CreateNewKey(Key * 1.0f);
// 			for (int32 SubIndex = 0; SubIndex < 3; SubIndex++)
// 			{
// 				ColorCurveDist->SetKeyOut(SubIndex, KeyIndex, 1.0f);
// 			}
// 		}
// 		ColorCurveDist->bIsDirty = true;
// 	}
// 	ColorModule->AlphaOverLife.Distribution = NewObject<UDistributionFloatConstantCurve>(ColorModule);
// 	UDistributionFloatConstantCurve* AlphaCurveDist = Cast<UDistributionFloatConstantCurve>(ColorModule->AlphaOverLife.Distribution);
// 	if (AlphaCurveDist)
// 	{
// 		// Add two points, one at time 0.0f and one at 1.0f
// 		for (int32 Key = 0; Key < 2; Key++)
// 		{
// 			int32	KeyIndex = AlphaCurveDist->CreateNewKey(Key * 1.0f);
// 			if (Key == 0)
// 			{
// 				AlphaCurveDist->SetKeyOut(0, KeyIndex, 1.0f);
// 			}
// 			else
// 			{
// 				AlphaCurveDist->SetKeyOut(0, KeyIndex, 0.0f);
// 			}
// 		}
// 		AlphaCurveDist->bIsDirty = true;
// 	}
// 	ColorModule->LODValidity = 1;
// 	LODLevel->Modules.Add(ColorModule);
// }

/*-----------------------------------------------------------------------------
    UParticleSystem implementation.
-----------------------------------------------------------------------------*/

UParticleSystem::UParticleSystem()
    : bAnyEmitterLoopsForever(false)
    , bIsImmortal(false)
    , bWillBecomeZombie(false)
{
#if WITH_EDITORONLY_DATA
    ThumbnailDistance = 200.0;
    ThumbnailWarmup = 1.0;
#endif // WITH_EDITORONLY_DATA
    UpdateTime_FPS = 60.0f;
    UpdateTime_Delta = 1.0f/60.0f;
    WarmupTime = 0.0f;
    WarmupTickRate = 0.0f;
#if WITH_EDITORONLY_DATA
    EditorLODSetting = 0;
#endif // WITH_EDITORONLY_DATA
    FixedRelativeBoundingBox.MinLocation = FVector(-1.0f, -1.0f, -1.0f);
    FixedRelativeBoundingBox.MaxLocation = FVector(1.0f, 1.0f, 1.0f);
    
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

ParticleSystemLODMethod UParticleSystem::GetCurrentLODMethod()
{
    return ParticleSystemLODMethod(LODMethod);
}

int32 UParticleSystem::GetLODLevelCount()
{
    return LODDistances.Num();
}

float UParticleSystem::GetLODDistance(int32 LODLevelIndex)
{
    if (LODLevelIndex >= LODDistances.Num())
    {
        return -1.0f;
    }

    return LODDistances[LODLevelIndex];
}

void UParticleSystem::SetCurrentLODMethod(ParticleSystemLODMethod InMethod)
{
    LODMethod = InMethod;
}

bool UParticleSystem::SetLODDistance(int32 LODLevelIndex, float InDistance)
{
    if (LODLevelIndex >= LODDistances.Num())
    {
        return false;
    }

    LODDistances[LODLevelIndex] = InDistance;

    return true;
}

void UParticleSystem::BuildEmitters()
{
    const int32 EmitterCount = Emitters.Num();
    for ( int32 EmitterIndex = 0; EmitterIndex < EmitterCount; ++EmitterIndex )
    {
        if (UParticleEmitter* Emitter = Emitters[EmitterIndex])
        {
            Emitter->Build();
        }
    }
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

void UParticleSystem::SetDelay(float InDelay)
{
    Delay = InDelay;
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
