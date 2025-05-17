#include "Components/ParticleSystemComponent.h"
#include "Particles/TypeData/ParticleModuleTypeDataBase.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleEmitter.h"

FParticleEmitterInstance* UParticleModuleTypeDataBase::CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent)
{
    return NULL;
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

bool UParticleModule::GenerateLODModuleValues(UParticleModule* SourceModule, float Percentage, UParticleLODLevel* LODLevel)
{
    return true;
}

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
