#include "ParticleLODLevel.h"
#include "SubUVAnimation.h"
#include "Components/ParticleSystemComponent.h"
#include "Lifetime/ParticleModuleLifetime.h"
#include "Lifetime/ParticleModuleLifetime_Seeded.h"

#include "Particles/TypeData/ParticleModuleTypeDataBase.h"
#include "Particles/TypeData/ParticleModuleTypeDataMesh.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleEmitter.h"
#include "SubUV/ParticleModuleSubUV.h"
#include "SubUV/ParticleModuleSubUVBase.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"

UParticleModule::UParticleModule()
{
    bSupported3DDrawMode = false;
    b3DDrawMode = false;
    bEnabled = true;
    bEditable = true;
    LODDuplicate = true;
    bSupportsRandomSeed = false;
    bRequiresLoopingNotification = false;
    bUpdateForGPUEmitter = false;
    bUpdateModule = true;
}

FParticleEmitterInstance* UParticleModuleTypeDataBase::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
    return NULL;
}

void UParticleModule::CompileModule( FParticleEmitterBuildInfo& EmitterInfo )
{
    if ( bSpawnModule )
    {
        EmitterInfo.SpawnModules.Add( this );
    }
}

void UParticleModule::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
}

void UParticleModule::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
}

void UParticleModule::FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
}

uint32 UParticleModule::RequiredBytes(UParticleModuleTypeDataBase* TypeData)
{
    return 0;
}

uint32 UParticleModule::RequiredBytesPerInstance()
{
    return 0;
}

uint32 UParticleModule::PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData)
{
    return 0xffffffff;
}

void UParticleModule::SetToSensibleDefaults(UParticleEmitter* Owner)
{
    // The default implementation does nothing...
}

bool UParticleModule::GenerateLODModuleValues(UParticleModule* SourceModule, float Percentage, UParticleLODLevel* LODLevel)
{
    return true;
}

FRandomStream& UParticleModule::GetRandomStream(FParticleEmitterInstance* Owner)
{
    FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
    FRandomStream& RandomStream = (Payload != nullptr) ? Payload->RandomStream : Owner->Component->RandomStream;
    return RandomStream;
}

/*-----------------------------------------------------------------------------
    UParticleModuleRequired implementation.
-----------------------------------------------------------------------------*/
UParticleModuleRequired::UParticleModuleRequired()
{
    bSpawnModule = true;
    bUpdateModule = true;
    EmitterDuration = 1.0f;
    EmitterDurationLow = 0.0f;
    bEmitterDurationUseRange = false;
    EmitterDelay = 0.0f;
    EmitterDelayLow = 0.0f;
    bEmitterDelayUseRange = false;
    EmitterLoops = 0;
    SubImages_Horizontal = 1;
    SubImages_Vertical = 1;
    bUseMaxDrawCount = true;
    MaxDrawCount = 500;
    LODDuplicate = true;
    NormalsSphereCenter = FVector(0.0f, 0.0f, 100.0f);
    NormalsCylinderDirection = FVector(0.0f, 0.0f, 1.0f);
    bUseLegacyEmitterTime = true;
    bSupportLargeWorldCoordinates = true;
    UVFlippingMode = EParticleUVFlipMode::None;
    AlphaThreshold = 0.1f;
    InitializeDefaults();
}

void UParticleModuleRequired::InitializeDefaults()
{
    SpawnRate = 10.f;
}

void UParticleModuleRequired::SetToSensibleDefaults(UParticleEmitter* Owner)
{
    Super::SetToSensibleDefaults(Owner);
    bUseLegacyEmitterTime = false;
}

bool UParticleModuleRequired::GenerateLODModuleValues(UParticleModule* SourceModule, float Percentage, UParticleLODLevel* LODLevel)
{
    // Convert the module values
    UParticleModuleRequired*	RequiredSource	= Cast<UParticleModuleRequired>(SourceModule);
    if (!RequiredSource)
    {
        return false;
    }

    bool bResult	= true;

    Material = RequiredSource->Material;
    //ScreenAlignment = RequiredSource->ScreenAlignment;

    //bUseLocalSpace
    //bKillOnDeactivate
    //bKillOnCompleted
    //EmitterDuration
    //EmitterLoops
    //SpawnRate
    //InterpolationMethod
    //SubImages_Horizontal
    //SubImages_Vertical
    //bScaleUV
    //RandomImageTime
    //RandomImageChanges
    //SubUVDataOffset
    //EmitterRenderMode
    //EmitterEditorColor

    return bResult;
}

/*-----------------------------------------------------------------------------
    UParticleModuleLifetime implementation.
-----------------------------------------------------------------------------*/

UParticleModuleLifetime::UParticleModuleLifetime()
    : Super()
{
    bSpawnModule = true;
    InitializeDefaults();
}

void UParticleModuleLifetime::InitializeDefaults()
{
    // if(!Lifetime.IsCreated())
    // {
    //     Lifetime.Distribution = NewObject<UDistributionFloatUniform>(this, TEXT("DistributionLifetime"));
    // }
    Lifetime = 10.0f;
}

void UParticleModuleLifetime::CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo )
{
    // float MinLifetime;
    // float MaxLifetime;
    //
    // // Call GetValue once to ensure the distribution has been initialized.
    // Lifetime.GetValue();
    // Lifetime.GetOutRange( MinLifetime, MaxLifetime );
    EmitterInfo.MaxLifetime = Lifetime;
    EmitterInfo.SpawnModules.Add( this );
}

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    SpawnEx(Owner, Offset, SpawnTime, &GetRandomStream(Owner), ParticleBase);
}

void UParticleModuleLifetime::SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, struct FRandomStream* InRandomStream, FBaseParticle* ParticleBase)
{
    SPAWN_INIT
    {
        float MaxLifetime = Lifetime; //.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
        if(Particle.OneOverMaxLifetime > 0.f)
        {
            // Another module already modified lifetime.
            Particle.OneOverMaxLifetime = 1.f / (MaxLifetime + 1.f / Particle.OneOverMaxLifetime);
        }
        else
        {
            // First module to modify lifetime.
            Particle.OneOverMaxLifetime = MaxLifetime > 0.f ? 1.f / MaxLifetime : 0.f;
        }
        //If the relative time is already > 1.0f then we don't want to be setting it. Some modules use this to mark a particle as dead during spawn.
        Particle.RelativeTime = Particle.RelativeTime > 1.0f ? Particle.RelativeTime : SpawnTime * Particle.OneOverMaxLifetime;
    }
}

void UParticleModuleLifetime::SetToSensibleDefaults(UParticleEmitter* Owner)
{
    // UDistributionFloatUniform* LifetimeDist = Cast<UDistributionFloatUniform>(Lifetime.Distribution);
    // if (LifetimeDist)
    // {
    //     LifetimeDist->Min = 1.0f;
    //     LifetimeDist->Max = 1.0f;
    //     LifetimeDist->bIsDirty = true;
    // }
}

float UParticleModuleLifetime::GetMaxLifetime()
{
    // Check the distribution for the max value
    float Min, Max;
    // Lifetime.GetOutRange(Min, Max);
    return Lifetime;
}

float UParticleModuleLifetime::GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data)
{
    return Lifetime;//.GetValue(InTime, Data);
}

void UParticleModuleLifetime::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    UParticleModuleLifetimeBase::Update(Owner, Offset, DeltaTime);
    BEGIN_UPDATE_LOOP
    Particle.RelativeTime += DeltaTime * Particle.OneOverMaxLifetime;
    END_UPDATE_LOOP
}

/*-----------------------------------------------------------------------------
    UParticleModuleLifetime_Seeded implementation.
-----------------------------------------------------------------------------*/
UParticleModuleLifetime_Seeded::UParticleModuleLifetime_Seeded()
{
    bSpawnModule = true;
    bSupportsRandomSeed = true;
    bRequiresLoopingNotification = true;
}

void UParticleModuleLifetime_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner)
{
    if (RandomSeedInfo.bResetSeedOnEmitterLooping == true)
    {
        FParticleRandomSeedInstancePayload* Payload = Owner->GetModuleRandomSeedInstanceData(this);
        //PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
    }
}

float UParticleModuleLifetime_Seeded::GetLifetimeValue(FParticleEmitterInstance* Owner, float InTime, UObject* Data )
{
    return Lifetime;//.GetValue(InTime, Data, &GetRandomStream(Owner));
}

/*-----------------------------------------------------------------------------
    UParticleModuleSubUVBase implementation.
-----------------------------------------------------------------------------*/
UParticleModuleSubUVBase::UParticleModuleSubUVBase()
{
}

/*-----------------------------------------------------------------------------
    UParticleModuleSubUV implementation.
-----------------------------------------------------------------------------*/
UParticleModuleSubUV::UParticleModuleSubUV()
{
    bSpawnModule = true;
    bUpdateModule = true;
    InitializeDefaults();
}

void UParticleModuleSubUV::InitializeDefaults()
{
    if (!SubImageIndex.IsCreated())
    {
        SubImageIndex.Distribution = FObjectFactory::ConstructObject<UDistributionFloat>(nullptr);
    }
}

void UParticleModuleSubUV::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	//check(Owner->SpriteTemplate);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	//check(LODLevel);
	// Grab the interpolation method...
	EParticleSubUVInterpMethod InterpMethod = (EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
	const int32 PayloadOffset = Owner->SubUVDataOffset;
	if ((InterpMethod == EParticleSubUVInterpMethod::PSUVIM_None) || (PayloadOffset == 0))
	{
		return;
	}

	if (!LODLevel->TypeDataModule || LODLevel->TypeDataModule->SupportsSubUV())
	{
		SPAWN_INIT
		{
			int32	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			PARTICLE_ELEMENT(FFullSubUVPayload, SubUVPayload);
			CurrentOffset	= TempOffset;

			SubUVPayload.ImageIndex = DetermineImageIndex(Owner, Offset, &Particle, InterpMethod, SubUVPayload, SpawnTime);
		}
	}
}

void UParticleModuleSubUV::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	//check(Owner->SpriteTemplate);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	// Grab the interpolation method...
	EParticleSubUVInterpMethod InterpMethod = 
		(EParticleSubUVInterpMethod)(LODLevel->RequiredModule->InterpolationMethod);
	const int32 PayloadOffset = Owner->SubUVDataOffset;
	if ((InterpMethod == EParticleSubUVInterpMethod::PSUVIM_None) || (PayloadOffset == 0))
	{
		return;
	}

	// Quick-out in case of Random that only uses a single image for the whole lifetime...
	if ((InterpMethod == EParticleSubUVInterpMethod::PSUVIM_Random) || (InterpMethod == EParticleSubUVInterpMethod::PSUVIM_Random_Blend))
	{
		if (LODLevel->RequiredModule->RandomImageChanges == 0)
		{
			// Never change the random image...
			return;
		}
	}

	if (!LODLevel->TypeDataModule || LODLevel->TypeDataModule->SupportsSubUV())
	{
		BEGIN_UPDATE_LOOP
			if (Particle.RelativeTime > 1.0f)
			{
				CONTINUE_UPDATE_LOOP
			}

			int32	TempOffset	= CurrentOffset;
			CurrentOffset	= PayloadOffset;
			PARTICLE_ELEMENT(FFullSubUVPayload, SubUVPayload)
			CurrentOffset	= TempOffset;

			SubUVPayload.ImageIndex = DetermineImageIndex(Owner, Offset, &Particle, InterpMethod, SubUVPayload, DeltaTime);
		END_UPDATE_LOOP
	}
}

float UParticleModuleSubUV::DetermineImageIndex(FParticleEmitterInstance* Owner, int32 Offset, FBaseParticle* Particle, 
	EParticleSubUVInterpMethod InterpMethod, FFullSubUVPayload& SubUVPayload, float DeltaTime)
{
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	//check(LODLevel);

	USubUVAnimation* __restrict SubUVAnimation = Owner->SpriteTemplate->SubUVAnimation;

	const int32 TotalSubImages = SubUVAnimation 
		? SubUVAnimation->SubImages_Horizontal * SubUVAnimation->SubImages_Vertical
		: LODLevel->RequiredModule->SubImages_Horizontal * LODLevel->RequiredModule->SubImages_Vertical;

	float ImageIndex = SubUVPayload.ImageIndex;

	if ((InterpMethod == EParticleSubUVInterpMethod::PSUVIM_Linear) || (InterpMethod == EParticleSubUVInterpMethod::PSUVIM_Linear_Blend))
	{
		if (bUseRealTime == false)
		{
			ImageIndex = SubImageIndex.GetValue(Particle->RelativeTime);
		}
		else
		{
			UWorld* World = Owner->Component->GetWorld();
			// if ((World != NULL))
			// {
			// 	ImageIndex = SubImageIndex.GetValue(Particle->RelativeTime / World->GetWorldSettings()->GetEffectiveTimeDilation(), Owner->Component);
			// }
			// else
			// {
			// ImageIndex = SubImageIndex.GetValue(Particle->RelativeTime);
			// }
		    ImageIndex = (SubUVAnimation->SubImages_Horizontal) * (SubUVAnimation->SubImages_Vertical) * (Particle->RelativeTime);
		}

		if (InterpMethod == EParticleSubUVInterpMethod::PSUVIM_Linear)
		{
			ImageIndex = FMath::TruncToFloat(ImageIndex);
		}
	}
	else if ((InterpMethod == EParticleSubUVInterpMethod::PSUVIM_Random) || (InterpMethod == EParticleSubUVInterpMethod::PSUVIM_Random_Blend))
	{
		if ((LODLevel->RequiredModule->RandomImageTime == 0.0f) ||
			((Particle->RelativeTime - SubUVPayload.RandomImageTime) > LODLevel->RequiredModule->RandomImageTime) ||
			(SubUVPayload.RandomImageTime == 0.0f))
		{
			ImageIndex = GetRandomStream(Owner).RandHelper(TotalSubImages);
			SubUVPayload.RandomImageTime	= Particle->RelativeTime;
		}

		if (InterpMethod == EParticleSubUVInterpMethod::PSUVIM_Random)
		{
			ImageIndex = FMath::TruncToFloat(ImageIndex);
		}
	}
	else
	{
		ImageIndex	= 0;
	}

	return ImageIndex;
}

UParticleModuleTypeDataMesh::UParticleModuleTypeDataMesh()
{
    CastShadows = false;
    DoCollisions = false;
    MeshAlignment = EMeshScreenAlignment::PSMA_MeshFaceCameraWithRoll;
    AxisLockOption = EParticleAxisLock::EPAL_NONE;
    CameraFacingUpAxisOption_DEPRECATED = CameraFacing_NoneUP;
    CameraFacingOption = EMeshCameraFacingOptions::XAxisFacing_NoUp;
    bCollisionsConsiderPartilceSize = true;
    bUseStaticMeshLODs = true;
    LODSizeScale = 1.0f;
}

FParticleEmitterInstance* UParticleModuleTypeDataMesh::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
    //SetToSensibleDefaults(InEmitterParent);
    FParticleEmitterInstance* Instance = new FParticleMeshEmitterInstance();

    Instance->InitParameters(InEmitterParent, InComponent);

    CreateDistribution();

    return Instance;
}

void UParticleModuleTypeDataMesh::CreateDistribution()
{
    if (!RollPitchYawRange.IsCreated())
    {
        RollPitchYawRange.Distribution = FObjectFactory::ConstructObject<UDistributionVector>(nullptr);
    }
}
