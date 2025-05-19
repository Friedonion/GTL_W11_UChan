#include "Components/ParticleSystemComponent.h"
#include "Lifetime/ParticleModuleLifetime.h"
#include "Lifetime/ParticleModuleLifetime_Seeded.h"

#include "Particles/TypeData/ParticleModuleTypeDataBase.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleEmitter.h"
#include "UObject/Casts.h"

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
}

void UParticleModuleLifetime::InitializeDefaults()
{
    // if(!Lifetime.IsCreated())
    // {
    //     Lifetime.Distribution = NewObject<UDistributionFloatUniform>(this, TEXT("DistributionLifetime"));
    // }
    Lifetime = 1.0f;
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

//TO DO : Frawdistributionfloat인가 뭔가 어떻게 해야할듯.

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
