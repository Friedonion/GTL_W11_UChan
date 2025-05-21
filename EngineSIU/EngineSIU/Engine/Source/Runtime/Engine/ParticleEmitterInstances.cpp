#include "ParticleEmitterInstances.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleSystem.h"
#include "Particles/Spawn/ParticleModuleSpawnBase.h"
#include "Components/ParticleSystemComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Particles/ParticleModuleRequired.h"
#include "Userinterface/Console.h"
#include "World/World.h"

void FParticleDataContainer::Alloc(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts)
{
    // check(InParticleDataNumBytes > 0 && ParticleIndicesNumShorts >= 0
    //     && InParticleDataNumBytes % sizeof(uint16) == 0); // we assume that the particle storage has reasonable alignment below
    ParticleDataNumBytes = InParticleDataNumBytes;
    ParticleIndicesNumShorts = InParticleIndicesNumShorts;

    MemBlockSize = ParticleDataNumBytes + ParticleIndicesNumShorts * sizeof(uint16);

    // ParticleData = (uint8*)FastParticleSmallBlockAlloc(MemBlockSize);
    ParticleIndices = (uint16*)(ParticleData + ParticleDataNumBytes);
}

void FParticleDataContainer::Free()
{
    if (ParticleData)
    {
        // check(MemBlockSize > 0);
        // FastParticleSmallBlockFree(ParticleData, MemBlockSize);
    }
    MemBlockSize = 0;
    ParticleDataNumBytes = 0;
    ParticleIndicesNumShorts = 0;
    ParticleData = nullptr;
    ParticleIndices = nullptr;
}

/*-----------------------------------------------------------------------------
    Information compiled from modules to build runtime emitter data.
-----------------------------------------------------------------------------*/

FParticleEmitterBuildInfo::FParticleEmitterBuildInfo()
    : RequiredModule(NULL)
    , SpawnModule(NULL)
    , SpawnPerUnitModule(NULL)
    , MaxSize(1.0f, 1.0f)
    , SizeScaleBySpeed(FVector2D::ZeroVector)
    , MaxSizeScaleBySpeed(1.0f, 1.0f)
    , bEnableCollision(false)
    // , CollisionResponse(EParticleCollisionResponse::Bounce)
    // , CollisionMode(EParticleCollisionMode::SceneDepth)
    , CollisionRadiusScale(1.0f)
    , CollisionRadiusBias(0.0f)
    , CollisionRandomSpread(0.0f)
    , CollisionRandomDistribution(1.0f)
    , Friction(0.0f)
    , PointAttractorPosition(FVector::ZeroVector)
    , PointAttractorRadius(0.0f)
    , GlobalVectorFieldScale(0.0f)
    , GlobalVectorFieldTightness(-1)
    , LocalVectorField(NULL)
    , LocalVectorFieldTransform(FTransform::Identity)
    , LocalVectorFieldIntensity(0.0f)
    , LocalVectorFieldTightness(0.0f)
    , LocalVectorFieldMinInitialRotation(FVector::ZeroVector)
    , LocalVectorFieldMaxInitialRotation(FVector::ZeroVector)
    , LocalVectorFieldRotationRate(FVector::ZeroVector)
    , ConstantAcceleration(0.0f)
    , MaxLifetime(1.0f)
    , MaxRotationRate(1.0f)
    , EstimatedMaxActiveParticleCount(0)
    // , ScreenAlignment(PSA_Square)
    , PivotOffset(-0.5,-0.5)
    , bLocalVectorFieldIgnoreComponentTransform(false)
    , bLocalVectorFieldTileX(false)
    , bLocalVectorFieldTileY(false)
    , bLocalVectorFieldTileZ(false)
    , bLocalVectorFieldUseFixDT(false)
    , bUseVelocityForMotionBlur(0)
    , bRemoveHMDRoll(0)
    , MinFacingCameraBlendDistance(0.0f)
    , MaxFacingCameraBlendDistance(0.0f)
{
//     DragScale.InitializeWithConstant(1.0f);
//     VectorFieldScale.InitializeWithConstant(1.0f);
//     VectorFieldScaleOverLife.InitializeWithConstant(1.0f);
// #if WITH_EDITOR
//     DynamicColorScale.Initialize();
//     DynamicAlphaScale.Initialize();
// #endif
}


/*-----------------------------------------------------------------------------
    FParticleEmitterInstance
-----------------------------------------------------------------------------*/
// Only update the PeakActiveParticles if the frame rate is 20 or better
const float FParticleEmitterInstance::PeakActiveParticleUpdateDelta = 0.05f;

FParticleEmitterInstance::FParticleEmitterInstance() :
      SpriteTemplate(NULL)
    , Component(NULL)
    , CurrentLODLevel(NULL)
    , CurrentLODLevelIndex(0)
    , TypeDataOffset(0)
    , TypeDataInstanceOffset(-1)
    , SubUVDataOffset(0)
    , DynamicParameterDataOffset(0)
    , LightDataOffset(0)
    , LightVolumetricScatteringIntensity(0)
    , OrbitModuleOffset(0)
    , CameraPayloadOffset(0)
    , PayloadOffset(0)
    , bEnabled(1)
    , bKillOnDeactivate(0)
    , bKillOnCompleted(0)
    , bHaltSpawning(0)
    , bHaltSpawningExternal(0)
    , bRequiresLoopNotification(0)
    , bIgnoreComponentScale(0)
    , bIsBeam(0)
    , bAxisLockEnabled(0)
    , bFakeBurstsWhenSpawningSupressed(0)
    , LockAxisFlags(EPAL_NONE)
    , SortMode(PSORTMODE_None)
    , ParticleData(NULL)
    , ParticleIndices(NULL)
    , InstanceData(NULL)
    , InstancePayloadSize(0)
    , ParticleSize(0)
    , ParticleStride(0)
    , ActiveParticles(0)
    , ParticleCounter(0)
    , MaxActiveParticles(0)
    , SpawnFraction(0.0f)
    , SecondsSinceCreation(0.0f)
    , EmitterTime(0.0f)
    , LoopCount(0)
    , IsRenderDataDirty(0)
    , EmitterDuration(0.0f)
    , TrianglesToRender(0)
    , MaxVertexIndex(0)
    , CurrentMaterial(NULL)
    , PositionOffsetThisTick(0)
    , PivotOffset(-0.5f,-0.5f)
{
}

FParticleEmitterInstance::~FParticleEmitterInstance()
{
    for (int32 i = 0; i < HighQualityLights.Num(); ++i)
    {
        UPointLightComponent* PointLightComponent = HighQualityLights[i];
        {
            PointLightComponent->DestroyComponent(false);
        }
    }
    HighQualityLights.SetNum(0);

    delete ParticleData;
    delete ParticleIndices;
    delete InstanceData;
    BurstFired.Empty();
}

void FParticleEmitterInstance::InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent)
{
    SpriteTemplate = InTemplate;
    Component = InComponent;
    SetupEmitterDuration();
}

void FParticleEmitterInstance::Init()
{
    //SCOPE_CYCLE_COUNTER(STAT_ParticleEmitterInstance_Init);

	//check(SpriteTemplate != nullptr);

	// Use highest LOD level for init'ing data, will contain all module types.
	UParticleLODLevel* HighLODLevel = SpriteTemplate->LODLevels[0];

	// Set the current material
	//check(HighLODLevel->RequiredModule);
	CurrentMaterial = HighLODLevel->RequiredModule->Material;

	// If we already have a non-zero ParticleSize, don't need to do most allocation work again
	bool bNeedsInit = (ParticleSize == 0);

	if(bNeedsInit)
	{
		//SCOPE_CYCLE_COUNTER(STAT_ParticleEmitterInstance_InitSize);

		// Copy pre-calculated info
		bRequiresLoopNotification = SpriteTemplate->bRequiresLoopNotification;
		bAxisLockEnabled = SpriteTemplate->bAxisLockEnabled;
		LockAxisFlags = SpriteTemplate->LockAxisFlags;
		DynamicParameterDataOffset = SpriteTemplate->DynamicParameterDataOffset;
		LightDataOffset = SpriteTemplate->LightDataOffset;
		LightVolumetricScatteringIntensity = SpriteTemplate->LightVolumetricScatteringIntensity;
		CameraPayloadOffset = SpriteTemplate->CameraPayloadOffset;
		ParticleSize = SpriteTemplate->ParticleSize;
		PivotOffset = SpriteTemplate->PivotOffset;
		TypeDataOffset = SpriteTemplate->TypeDataOffset;
		TypeDataInstanceOffset = SpriteTemplate->TypeDataInstanceOffset;

	    if ((InstanceData == NULL) || (SpriteTemplate->ReqInstanceBytes > InstancePayloadSize))
	    {
			    InstanceData = (uint8*)(realloc(InstanceData, SpriteTemplate->ReqInstanceBytes));
			    InstancePayloadSize = SpriteTemplate->ReqInstanceBytes;
	    }
    
	    memset(InstanceData, 0, InstancePayloadSize);
    
	    for (UParticleModule* ParticleModule : SpriteTemplate->ModulesNeedingInstanceData)
	    {
			//check(ParticleModule);
			uint8* PrepInstData = GetModuleInstanceData(ParticleModule);
			//check(PrepInstData != nullptr); // Shouldn't be in the list if it doesn't have data
			ParticleModule->PrepPerInstanceBlock(this, (void*)PrepInstData);
	    }

		for (UParticleModule* ParticleModule : SpriteTemplate->ModulesNeedingRandomSeedInstanceData)
		{
			//check(ParticleModule);
			FParticleRandomSeedInstancePayload* SeedInstancePayload = GetModuleRandomSeedInstanceData(ParticleModule);
			//check(SeedInstancePayload != nullptr); // Shouldn't be in the list if it doesn't have data
			FParticleRandomSeedInfo* RandomSeedInfo = ParticleModule->GetRandomSeedInfo();
			//ParticleModule->PrepRandomSeedInstancePayload(this, SeedInstancePayload, RandomSeedInfo ? *RandomSeedInfo : FParticleRandomSeedInfo());
		}

	    // Offset into emitter specific payload (e.g. TrailComponent requires extra bytes).
	    PayloadOffset = ParticleSize;
	    
	    // Update size with emitter specific size requirements.
	    ParticleSize += RequiredBytes();
    
	    // Make sure everything is at least 16 byte aligned so we can use SSE for FVector.
	    //ParticleSize = Align(ParticleSize, 16);
    
	    // E.g. trail emitters store trailing particles directly after leading one.
	    ParticleStride = CalculateParticleStride(ParticleSize);
	}

	// Setup the emitter instance material array...
	SetMeshMaterials(SpriteTemplate->MeshMaterials);

	// Set initial values.
	SpawnFraction			= 0;
	SecondsSinceCreation	= 0;
	EmitterTime				= 0;
	ParticleCounter			= 0;

	UpdateTransforms();	
	Location				= Component->GetComponentLocation();
	OldLocation				= Location;
	
	TrianglesToRender		= 0;
	MaxVertexIndex			= 0;

	if (ParticleData == NULL)
	{
		MaxActiveParticles	= 0;
		ActiveParticles		= 0;
	}

	//ParticleBoundingBox.Init();
	if (HighLODLevel->RequiredModule->RandomImageChanges == 0)
	{
		HighLODLevel->RequiredModule->RandomImageTime	= 1.0f;
	}
	else
	{
		HighLODLevel->RequiredModule->RandomImageTime	= 0.99f / (HighLODLevel->RequiredModule->RandomImageChanges + 1);
	}

	// Resize to sensible default.
	if (bNeedsInit &&
		// Only presize if any particles will be spawned 
		SpriteTemplate->QualityLevelSpawnRateScale > 0)
	{
		if ((HighLODLevel->PeakActiveParticles > 0) || (SpriteTemplate->InitialAllocationCount > 0))
		{
			// In-game... we assume the editor has set this properly, but still clamp at 100 to avoid wasting
			// memory.
			if (SpriteTemplate->InitialAllocationCount > 0)
			{
				Resize(FMath::Min( SpriteTemplate->InitialAllocationCount, 100 ));
			}
			else
			{
				Resize(FMath::Min( HighLODLevel->PeakActiveParticles, 100 ));
			}
		}
		else
		{
			// This is to force the editor to 'select' a value
			Resize(10);
		}
	}

	LoopCount = 0;

	if(bNeedsInit)
	{
		//QUICK_SCOPE_CYCLE_COUNTER(STAT_AllocateBurstLists);
	// Propagate killon flags
		bKillOnDeactivate = HighLODLevel->RequiredModule->bKillOnDeactivate;
		bKillOnCompleted = HighLODLevel->RequiredModule->bKillOnCompleted;

	// Propagate sorting flag.
		//SortMode = HighLODLevel->RequiredModule->SortMode;

	// Reset the burst lists
	if (BurstFired.Num() < SpriteTemplate->LODLevels.Num())
	{
		BurstFired.AddZeroed(SpriteTemplate->LODLevels.Num() - BurstFired.Num());
	}

	for (int32 LODIndex = 0; LODIndex < SpriteTemplate->LODLevels.Num(); LODIndex++)
	{
			UParticleLODLevel* LODLevel = SpriteTemplate->LODLevels[LODIndex];
		//check(LODLevel);
		FLODBurstFired& LocalBurstFired = BurstFired[LODIndex];
		if (LocalBurstFired.Fired.Num() < LODLevel->SpawnModule->BurstList.Num())
		{
			LocalBurstFired.Fired.AddZeroed(LODLevel->SpawnModule->BurstList.Num() - LocalBurstFired.Fired.Num());
		}
	}
	}

	ResetBurstList();

#if WITH_EDITORONLY_DATA
	//Check for SubUV module to see if it has SubUVAnimation to move data to required module
	for (auto CurrModule : HighLODLevel->Modules)
	{
		if (CurrModule->IsA(UParticleModuleSubUV::StaticClass()))
		{
			UParticleModuleSubUV* SubUVModule = (UParticleModuleSubUV*)CurrModule;

			if (SubUVModule->Animation)
			{
				HighLODLevel->RequiredModule->AlphaThreshold = SubUVModule->Animation->AlphaThreshold;
				HighLODLevel->RequiredModule->BoundingMode = SubUVModule->Animation->BoundingMode;
				HighLODLevel->RequiredModule->OpacitySourceMode = SubUVModule->Animation->OpacitySourceMode;
				HighLODLevel->RequiredModule->CutoutTexture = SubUVModule->Animation->SubUVTexture;

				SubUVModule->Animation = nullptr;

				HighLODLevel->RequiredModule->CacheDerivedData();
				HighLODLevel->RequiredModule->InitBoundingGeometryBuffer();
			}
		}
	}
#endif //WITH_EDITORONLY_DATA

	// Tag it as dirty w.r.t. the renderer
	IsRenderDataDirty	= 1;

	bEmitterIsDone = false;
}

UWorld* FParticleEmitterInstance::GetWorld() const
{
    return Component->GetWorld();
}

void FParticleEmitterInstance::UpdateTransforms()
{
    //QUICK_SCOPE_CYCLE_COUNTER(STAT_EmitterInstance_UpdateTransforms);

    //check(SpriteTemplate != NULL);

    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    FMatrix ComponentToWorld = Component != NULL ?
        Component->GetComponentTransform().ToMatrixNoScale() : FMatrix::Identity;
    FMatrix EmitterToComponent = FMatrix::CreateRotationMatrix(LODLevel->RequiredModule->EmitterRotation) * FMatrix::CreateTranslationMatrix(LODLevel->RequiredModule->EmitterOrigin);

    if (LODLevel->RequiredModule->bUseLocalSpace)
    {
        EmitterToSimulation = EmitterToComponent;
        SimulationToWorld = ComponentToWorld;
#if ENABLE_NAN_DIAGNOSTIC
        if (SimulationToWorld.ContainsNaN())
        {
            logOrEnsureNanError(TEXT("FParticleEmitterInstance::UpdateTransforms() - SimulationToWorld contains NaN!"));
            SimulationToWorld = FMatrix::Identity;
        }
#endif
    }
    else
    {
        EmitterToSimulation = EmitterToComponent * ComponentToWorld;
        SimulationToWorld = FMatrix::Identity;
    }
}

bool FParticleEmitterInstance::Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount)
{
//SCOPE_CYCLE_COUNTER(STAT_ParticleEmitterInstance_Resize);
    
	if (NewMaxActiveParticles > MaxActiveParticles)
	{
		// Alloc (or realloc) the data array
		// Allocations > 16 byte are always 16 byte aligned so ParticleData can be used with SSE.
		// NOTE: We don't have to zero the memory here... It gets zeroed when grabbed later.

		{
			ParticleData = (uint8*)realloc(ParticleData, ParticleStride * NewMaxActiveParticles);
			//check(ParticleData);

			// Allocate memory for indices.
			if (ParticleIndices == NULL)
			{
				// Make sure that we clear all when it is the first alloc
				MaxActiveParticles = 0;
			}
			ParticleIndices	= (uint16*)realloc(ParticleIndices, sizeof(uint16) * (NewMaxActiveParticles + 1));
		}

		// Fill in default 1:1 mapping.
		for (int32 i=MaxActiveParticles; i<NewMaxActiveParticles; i++)
		{
			ParticleIndices[i] = i;
		}

		// Set the max count
		MaxActiveParticles = NewMaxActiveParticles;
	}

	// Set the PeakActiveParticles
	if (bSetMaxActiveCount)
	{
		UParticleLODLevel* LODLevel	= SpriteTemplate->GetLODLevel(0);
		//check(LODLevel);
		if (MaxActiveParticles > LODLevel->PeakActiveParticles)
		{
			LODLevel->PeakActiveParticles = MaxActiveParticles;
		}
	}

	return true;
}

// [Particle Sequece] 03 - ParticleEmitterInstance 업데이트, 파티클 Pool관리 및 ParticleModule update
void FParticleEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning)
{
    if (!SpriteTemplate || !(SpriteTemplate->LODLevels.Num() > 0)) return;

    bool bFirstTime = (SecondsSinceCreation > 0.0f) ? false : true;

    // Grab the current LOD level
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();

    // Handle EmitterTime setup, looping, etc.
    float EmitterDelay = Tick_EmitterTimeSetup(DeltaTime, LODLevel);

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
        Tick_ModulePostUpdate(DeltaTime, LODLevel);

        if (ActiveParticles > 0)
        {
            // Update the orbit data...
            // UpdateOrbitData(DeltaTime);
            // Calculate bounding box and simulate velocity.
            //UpdateBoundingBox(DeltaTime);
        }

        Tick_ModuleFinalUpdate(DeltaTime, LODLevel);

        CheckEmitterFinished();

        // Invalidate the contents of the vertex/index buffer.
        IsRenderDataDirty = 1;
    }
    else
    {
        FakeBursts();
    }

    // 'Reset' the emitter time so that the delay functions correctly
    EmitterTime += EmitterDelay;

    // Store the last delta time.
    LastDeltaTime = DeltaTime;

    // Reset particles position offset
    PositionOffsetThisTick = FVector::ZeroVector;
}

void FParticleEmitterInstance::CheckEmitterFinished()
{
    // Grab the current LOD level
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();

    // figure out if this emitter will no longer spawn particles
    //
    if (this->ActiveParticles == 0)
    {
        UParticleModuleSpawn* SpawnModule = LODLevel->SpawnModule;
        
        if (!SpawnModule) return;

        FParticleBurst* LastBurst = nullptr;
        if (SpawnModule->BurstList.Num())
        {
            LastBurst = &SpawnModule->BurstList.Last();
        }

        if (!LastBurst || LastBurst->Time < this->EmitterTime)
        {
            const UParticleModuleRequired* RequiredModule = LODLevel->RequiredModule;
            
            if (!RequiredModule) return;

            if (HasCompleted() ||
                (SpawnModule->GetMaximumSpawnRate() == 0
                    && RequiredModule->EmitterDuration == 0
                    && RequiredModule->EmitterLoops == 0)
                )
            {
                bEmitterIsDone = true;
            }
        }
    }
}

float FParticleEmitterInstance::Tick_EmitterTimeSetup(float DeltaTime, UParticleLODLevel* InCurrentLODLevel)
{
// Make sure we don't try and do any interpolation on the first frame we are attached (OldLocation is not valid in this circumstance)
	if (Component->bJustRegistered)
	{
		Location	= Component->GetComponentLocation();
		OldLocation	= Location;
	}
	else
	{
		// Keep track of location for world- space interpolation and other effects.
		OldLocation	= Location;
		Location	= Component->GetComponentLocation();
	}

	UpdateTransforms();
	SecondsSinceCreation += DeltaTime;

	// Update time within emitter loop.
	bool bLooped = false;
	if (InCurrentLODLevel->RequiredModule->bUseLegacyEmitterTime == false)
	{
		EmitterTime += DeltaTime;
		bLooped = (EmitterDuration > 0.0f) && (EmitterTime >= EmitterDuration);
	}
	else
	{
		EmitterTime = SecondsSinceCreation;
		if (EmitterDuration > DBL_EPSILON)
		{
			EmitterTime = FMath::Fmod(SecondsSinceCreation, EmitterDuration);
			bLooped = ((SecondsSinceCreation - (EmitterDuration * LoopCount)) >= EmitterDuration);
		}
	}

	// Get the emitter delay time
	float EmitterDelay = CurrentDelay;

	// Determine if the emitter has looped
	if (bLooped)
	{
		LoopCount++;
		ResetBurstList();
	    
		if (InCurrentLODLevel->RequiredModule->bUseLegacyEmitterTime == false)
		{
			EmitterTime -= EmitterDuration;
		}

		if ((InCurrentLODLevel->RequiredModule->bDurationRecalcEachLoop == true)
			|| ((InCurrentLODLevel->RequiredModule->bDelayFirstLoopOnly == true) && (LoopCount == 1))
			)
		{
			SetupEmitterDuration();
		}

		if (bRequiresLoopNotification == true)
		{
			for (int32 ModuleIdx = -3; ModuleIdx < InCurrentLODLevel->Modules.Num(); ModuleIdx++)
			{
				int32 ModuleFetchIdx;
				switch (ModuleIdx)
				{
				case -3:	ModuleFetchIdx = INDEX_REQUIREDMODULE;	break;
				case -2:	ModuleFetchIdx = INDEX_SPAWNMODULE;		break;
				case -1:	ModuleFetchIdx = INDEX_TYPEDATAMODULE;	break;
				default:	ModuleFetchIdx = ModuleIdx;				break;
				}

				UParticleModule* Module = InCurrentLODLevel->GetModuleAtIndex(ModuleFetchIdx);
				if (Module != NULL)
				{
					if (Module->RequiresLoopingNotification() == true)
					{
						Module->EmitterLoopingNotify(this);
					}
				}
			}
		}
	}

	// Don't delay unless required
	if ((InCurrentLODLevel->RequiredModule->bDelayFirstLoopOnly == true) && (LoopCount > 0))
	{
		EmitterDelay = 0;
	}

	// 'Reset' the emitter time so that the modules function correctly
	EmitterTime -= EmitterDelay;

	return EmitterDelay;
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

void FParticleEmitterInstance::Tick_ModulePostUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel)
{
}

void FParticleEmitterInstance::Tick_ModuleFinalUpdate(float DeltaTime, UParticleLODLevel* InCurrentLODLevel)
{
     // UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
     //
     // if (!HighestLODLevel) return;
     //
     // for (int32 ModuleIndex = 0; ModuleIndex < InCurrentLODLevel->UpdateModules.Num(); ModuleIndex++)
     // {
     //     UParticleModule* CurrentModule = InCurrentLODLevel->UpdateModules[ModuleIndex];
     //     if (CurrentModule && CurrentModule->bEnabled && CurrentModule->bFinalUpdateModule)
     //     {
     //         CurrentModule->FinalUpdate(this, GetModuleDataOffset(HighestLODLevel->UpdateModules[ModuleIndex]), DeltaTime);
     //     }
     // }
     //
     // if (InCurrentLODLevel->TypeDataModule && InCurrentLODLevel->TypeDataModule->bEnabled && InCurrentLODLevel->TypeDataModule->bFinalUpdateModule)
     // {
     //     InCurrentLODLevel->TypeDataModule->FinalUpdate(this, GetModuleDataOffset(HighestLODLevel->TypeDataModule), DeltaTime);
     // }
}
void FParticleEmitterInstance::SetCurrentLODIndex(int32 InLODIndex, bool bInFullyProcess)
{
}

void FParticleEmitterInstance::Rewind()
{
}

FBoundingBox FParticleEmitterInstance::GetBoundingBox()
{
    return ParticleBoundingBox;
}

void FParticleEmitterInstance::UpdateBoundingBox(float DeltaTime)
{
}

void FParticleEmitterInstance::ForceUpdateBoundingBox()
{
}

uint32 FParticleEmitterInstance::RequiredBytes()
{
    //SubUV Particle있으면 구현필요할듯
    return 0;
}

uint32 FParticleEmitterInstance::GetModuleDataOffset(UParticleModule* Module)
{
    if (!SpriteTemplate) return 0;

    uint32* Offset = SpriteTemplate->ModuleOffsetMap.Find(Module);
    return (Offset != nullptr) ? *Offset : 0;
}

uint8* FParticleEmitterInstance::GetModuleInstanceData(UParticleModule* Module)
{
    return nullptr;
}

/** Get pointer to emitter instance random seed payload data for a particular module */
FParticleRandomSeedInstancePayload* FParticleEmitterInstance::GetModuleRandomSeedInstanceData(UParticleModule* Module)
{
    // If there is instance data present, look up the modules offset
    if (InstanceData)
    {
        uint32* Offset = SpriteTemplate->ModuleRandomSeedInstanceOffsetMap.Find(Module);
        if (Offset)
        {
            //check(*Offset < (uint32)InstancePayloadSize);
            return (FParticleRandomSeedInstancePayload*)&(InstanceData[*Offset]);
        }
    }
    return NULL;
}

uint8* FParticleEmitterInstance::GetTypeDataModuleInstanceData()
{
    return nullptr;
}

uint32 FParticleEmitterInstance::CalculateParticleStride(uint32 InParticleSize)
{
    return InParticleSize;
}


void FParticleEmitterInstance::ResetBurstList()
{
}

float FParticleEmitterInstance::GetCurrentBurstRateOffset(float& DeltaTime, int32& Burst)
{
    return 0.f;
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

void FParticleEmitterInstance::ParticlePrefetch()
{
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

/**
* Fixup particle indices to only have valid entries.
*/
void FParticleEmitterInstance::FixupParticleIndices()
{
    // // Something is wrong and particle data are be invalid. Try to fix-up things.
    // TBitArray<> UsedIndices(false, MaxActiveParticles);
    //
    // for (int32 i = 0; i < ActiveParticles; ++i)
    // {
    //     const uint16 UsedIndex = ParticleIndices[i];
    //     if (UsedIndex < MaxActiveParticles && UsedIndices[UsedIndex] == 0)
    //     {
    //         UsedIndices[UsedIndex] = 1;
    //     }
    //     else
    //     {
    //         if (i != ActiveParticles - 1)
    //         {
    //             // Remove this bad or duplicated index
    //             ParticleIndices[i] = ParticleIndices[ActiveParticles - 1];
    //         }
    //         // Decrease particle count.
    //         --ActiveParticles;
    //
    //         // Retry the new index.
    //         --i;
    //     }
    // }
    //
    // for (int32 i = ActiveParticles; i < MaxActiveParticles; ++i)
    // {
    //     const int32 FreeIndex = UsedIndices.FindAndSetFirstZeroBit();
    //     if (ensure(FreeIndex != INDEX_NONE))
    //     {
    //         ParticleIndices[i] =  (uint16)FreeIndex;
    //     }
    //     else // Can't really handle that.
    //     {
    //         ParticleIndices[i] = (uint16)i;
    //     }
    // }
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
                        //UE_LOG(ELogLevel::Error, TEXT("Detected null particles. Template : %s, Component %s"));
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

    SpawnInternal(true);
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

UParticleLODLevel* FParticleEmitterInstance::GetCurrentLODLevelChecked()
{
    if (SpriteTemplate == NULL) return nullptr;
    UParticleLODLevel* LODLevel = SpriteTemplate->GetCurrentLODLevel(this);
    if (LODLevel == NULL) return nullptr;
    if (LODLevel->RequiredModule == NULL) return nullptr;
    return LODLevel;
}

void FParticleEmitterInstance::ForceSpawn(float DeltaTime, int32 InSpawnCount, int32 InBurstCount, FVector& InLocation, FVector& InVelocity)
{
}

void FParticleEmitterInstance::PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity)
{
    //check(Particle);
    // This isn't a problem w/ the FMemory::Memzero call - it's a problem in general!
    //check(ParticleSize > 0);

    // By default, just clear out the particle
    memset(Particle, 0, ParticleSize);

    // Initialize the particle location.
    Particle->Location = InitialLocation;
    Particle->BaseVelocity = FVector(InitialVelocity);
    Particle->Velocity = FVector(InitialVelocity);

    // New particles has already updated spawn location
    // Subtract offset here, so deferred location offset in UpdateBoundingBox will return this particle back
    Particle->Location-= PositionOffsetThisTick;
}

bool FParticleEmitterInstance::HasCompleted()
{
    // Validity check
    if (SpriteTemplate == NULL)
    {
        return true;
    }

    // If it hasn't finished looping or if it loops forever, not completed.
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    if ((LODLevel->RequiredModule->EmitterLoops == 0) || 
        (SecondsSinceCreation < (EmitterDuration * LODLevel->RequiredModule->EmitterLoops)))
    {
        return false;
    }

    // If there are active particles, not completed
    if (ActiveParticles > 0)
    {
        return false;
    }

    return true;
}

void FParticleEmitterInstance::PostSpawn(FBaseParticle* Particle, float InterpolationPercentage, float SpawnTime)
{
    // Interpolate position if using world space.
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    if (LODLevel->RequiredModule->bUseLocalSpace == false)
    {
        if (FVector::DistSquared(OldLocation, Location) > 1.f)
        {
            Particle->Location += (OldLocation - Location) * InterpolationPercentage;	
        }
    }

    // Offset caused by any velocity
    Particle->OldLocation = Particle->Location;
    Particle->Location   += FVector(Particle->Velocity) * SpawnTime;

    // Store a sequence counter.
    Particle->Flags |= ((ParticleCounter++) & STATE_CounterMask);
    Particle->Flags |= STATE_Particle_JustSpawned;
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

void FParticleEmitterInstance::KillParticle(int32 Index)
{
    if (Index < ActiveParticles)
    {
        UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
        FParticleEventInstancePayload* EventPayload = NULL;
        // if (LODLevel->EventGenerator)
        // {
        //     EventPayload = (FParticleEventInstancePayload*)GetModuleInstanceData(LODLevel->EventGenerator);
        //     if (EventPayload && (EventPayload->bDeathEventsPresent == false))
        //     {
        //         EventPayload = NULL;
        //     }
        // }

        int32 KillIndex = ParticleIndices[Index];

        // Handle the kill event, if needed
        if (EventPayload)
        {
            const uint8* ParticleBase	= ParticleData + KillIndex * ParticleStride;
            FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);
            // LODLevel->EventGenerator->HandleParticleKilled(this, EventPayload, &Particle);
        }

        // Move it to the 'back' of the list
        for (int32 i=Index; i < ActiveParticles - 1; i++)
        {
            ParticleIndices[i] = ParticleIndices[i+1];
        }
        ParticleIndices[ActiveParticles-1] = KillIndex;
        ActiveParticles--;

        // INC_DWORD_STAT(STAT_SpriteParticlesKilled);
    }
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

void FParticleEmitterInstance::KillParticlesForced(bool bFireEvents)
{
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    FParticleEventInstancePayload* EventPayload = NULL;
    // if (bFireEvents == true)
    // {
    //     if (LODLevel->EventGenerator)
    //     {
    //         EventPayload = (FParticleEventInstancePayload*)GetModuleInstanceData(LODLevel->EventGenerator);
    //         if (EventPayload && (EventPayload->bDeathEventsPresent == false))
    //         {
    //             EventPayload = NULL;
    //         }
    //     }
    // }

    // Loop over the active particles and kill them.
    // Move them to the 'end' of the active particle list.
    for (int32 KillIdx = ActiveParticles - 1; KillIdx >= 0; KillIdx--)
    {
        const int32 CurrentIndex = ParticleIndices[KillIdx];
        // Handle the kill event, if needed
        if (EventPayload)
        {
            const uint8* ParticleBase = ParticleData + CurrentIndex * ParticleStride;
            FBaseParticle& Particle = *((FBaseParticle*) ParticleBase);
            // LODLevel->EventGenerator->HandleParticleKilled(this, EventPayload, &Particle);
        }
        ParticleIndices[KillIdx] = ParticleIndices[ActiveParticles - 1];
        ParticleIndices[ActiveParticles - 1] = CurrentIndex;
        ActiveParticles--;

        // INC_DWORD_STAT(STAT_SpriteParticlesKilled);
    }

    ParticleCounter = 0;
}

FBaseParticle* FParticleEmitterInstance::GetParticle(int32 Index)
{
    // See if the index is valid. If not, return NULL
    if ((Index >= ActiveParticles) || (Index < 0))
    {
        return NULL;
    }

    // Grab and return the particle
    DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * ParticleIndices[Index]);
    return Particle;
}

FBaseParticle* FParticleEmitterInstance::GetParticleDirect(int32 InDirectIndex)
{
    if ((ActiveParticles > 0) && (InDirectIndex < MaxActiveParticles))
    {
        DECLARE_PARTICLE_PTR(Particle, ParticleData + ParticleStride * InDirectIndex);
        return Particle;
    }
    return NULL;
}

void FParticleEmitterInstance::SetupEmitterDuration()
{
    // Validity check
    if (!SpriteTemplate || !Component)
    {
        return;
    }

    // Set up the array for each LOD level with zeros
    int32 EDCount = EmitterDurations.Num();
    const int32 NumLODLevels = SpriteTemplate->LODLevels.Num();
    if ((EDCount == 0) || (EDCount != NumLODLevels))
    {
        EmitterDurations.Empty();
        EmitterDurations.Init(0.0f, NumLODLevels);
    }

    // Calculate the duration for each LOD level
    for (int32 LODIndex = 0; LODIndex < NumLODLevels; LODIndex++)
    {
        UParticleLODLevel* TempLOD = SpriteTemplate->LODLevels[LODIndex];
        if (!TempLOD || !TempLOD->RequiredModule)
        {
            continue;
        }

        UParticleModuleRequired* RequiredModule = TempLOD->RequiredModule;
        FRandomStream& RandomStream = RequiredModule->GetRandomStream(this);

        // Calculate base delay
        CurrentDelay = RequiredModule->EmitterDelay;
        
        // Add component delay if valid
        if (Component)
        {
            CurrentDelay += Component->EmitterDelay;
        }

        // Apply random range if enabled
        if (RequiredModule->bEmitterDelayUseRange)
        {
            const float Rand = RandomStream.FRand();
            CurrentDelay = FMath::Lerp(RequiredModule->EmitterDelayLow, RequiredModule->EmitterDelay, Rand);
            if (Component)
            {
                CurrentDelay += Component->EmitterDelay;
            }
        }

        // Calculate final duration
        float FinalDuration;
        if (RequiredModule->bEmitterDurationUseRange)
        {
            const float Rand = RandomStream.FRand();
            FinalDuration = FMath::Lerp(RequiredModule->EmitterDurationLow, RequiredModule->EmitterDuration, Rand);
        }
        else
        {
            FinalDuration = RequiredModule->EmitterDuration;
        }

        // Store final duration with delay
        EmitterDurations[TempLOD->Level] = FinalDuration + CurrentDelay;

        // Handle first loop only delay
        if ((LoopCount == 1) && (RequiredModule->bDelayFirstLoopOnly) && 
            ((RequiredModule->EmitterLoops == 0) || (RequiredModule->EmitterLoops > 1)))
        {
            EmitterDurations[TempLOD->Level] -= CurrentDelay;
        }
    }

    // Set the current duration if index is valid
    if (CurrentLODLevelIndex >= 0 && CurrentLODLevelIndex < NumLODLevels)
    {
        EmitterDuration = EmitterDurations[CurrentLODLevelIndex];
    }
}

/**
 *	Process received events.
 */
void FParticleEmitterInstance::ProcessParticleEvents(float DeltaTime, bool bSuppressSpawning)
{
    UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    // if (LODLevel->EventReceiverModules.Num() > 0)
    // {
    //     for (int32 EventModIndex = 0; EventModIndex < LODLevel->EventReceiverModules.Num(); EventModIndex++)
    //     {
    //         int32 EventIndex;
    //         UParticleModuleEventReceiverBase* EventRcvr = LODLevel->EventReceiverModules[EventModIndex];
    //         check(EventRcvr);
    //
    //         if (EventRcvr->WillProcessParticleEvent(EPET_Spawn) && (Component->SpawnEvents.Num() > 0))
    //         {
    //             for (EventIndex = 0; EventIndex < Component->SpawnEvents.Num(); EventIndex++)
    //             {
    //                 EventRcvr->ProcessParticleEvent(this, Component->SpawnEvents[EventIndex], DeltaTime);
    //             }
    //         }
    //
    //         if (EventRcvr->WillProcessParticleEvent(EPET_Death) && (Component->DeathEvents.Num() > 0))
    //         {
    //             for (EventIndex = 0; EventIndex < Component->DeathEvents.Num(); EventIndex++)
    //             {
    //                 EventRcvr->ProcessParticleEvent(this, Component->DeathEvents[EventIndex], DeltaTime);
    //             }
    //         }
    //
    //         if (EventRcvr->WillProcessParticleEvent(EPET_Collision) && (Component->CollisionEvents.Num() > 0))
    //         {
    //             for (EventIndex = 0; EventIndex < Component->CollisionEvents.Num(); EventIndex++)
    //             {
    //                 EventRcvr->ProcessParticleEvent(this, Component->CollisionEvents[EventIndex], DeltaTime);
    //             }
    //         }
    //
    //         if (EventRcvr->WillProcessParticleEvent(EPET_Burst) && (Component->BurstEvents.Num() > 0))
    //         {
    //             for (EventIndex = 0; EventIndex < Component->BurstEvents.Num(); EventIndex++)
    //             {
    //                 EventRcvr->ProcessParticleEvent(this, Component->BurstEvents[EventIndex], DeltaTime);
    //             }
    //         }
    //
    //         if (EventRcvr->WillProcessParticleEvent(EPET_Blueprint) && (Component->KismetEvents.Num() > 0))
    //         {
    //             for (EventIndex = 0; EventIndex < Component->KismetEvents.Num(); EventIndex++)
    //             {
    //                 EventRcvr->ProcessParticleEvent(this, Component->KismetEvents[EventIndex], DeltaTime);
    //             }
    //         }
    //     }
    // }
}

void FParticleEmitterInstance::ApplyWorldOffset(FVector InOffset, bool bWorldShift)
{
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

bool FParticleEmitterInstance::UseLocalSpace()
{
    const UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    return LODLevel->RequiredModule->bUseLocalSpace;
}

void FParticleEmitterInstance::GetScreenAlignmentAndScale(int32& OutScreenAlign, FVector& OutScale)
{
    const UParticleLODLevel* LODLevel = GetCurrentLODLevelChecked();
    //OutScreenAlign = (int32)LODLevel->RequiredModule->ScreenAlignment;

    OutScale = FVector(1.0f, 1.0f, 1.0f);
    if (Component)
    {
        OutScale = Component->GetComponentTransform().GetScale3D();
    }
}

/*-----------------------------------------------------------------------------
    ParticleSpriteEmitterInstance
-----------------------------------------------------------------------------*/
/**
 *	ParticleSpriteEmitterInstance
 *	The structure for a standard sprite emitter instance.
 */
/** Constructor	*/
FParticleSpriteEmitterInstance::FParticleSpriteEmitterInstance() :
    FParticleEmitterInstance()
{
}

/** Destructor	*/
FParticleSpriteEmitterInstance::~FParticleSpriteEmitterInstance()
{
}

UMaterial* FParticleEmitterInstance::GetCurrentMaterial()
{
    UMaterial* RenderMaterial = CurrentMaterial;
    if ((RenderMaterial == NULL))
    {
        //RenderMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
    }
    CurrentMaterial = RenderMaterial;
    return RenderMaterial;
}

/*-----------------------------------------------------------------------------
    ParticleMeshEmitterInstance
-----------------------------------------------------------------------------*/
/**
 *	Structure for mesh emitter instances
 */

/** Constructor	*/

