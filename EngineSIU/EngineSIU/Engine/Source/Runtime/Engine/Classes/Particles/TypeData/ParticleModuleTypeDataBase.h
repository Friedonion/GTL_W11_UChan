#pragma once

#include "UObject/ObjectMacros.h"
#include "Particles/ParticleModule.h"

class UParticleEmitter;
class UParticleSystemComponent;
struct FParticleEmitterInstance;

class UParticleModuleTypeDataBase : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleTypeDataBase, UParticleModule)
public:
    UParticleModuleTypeDataBase() = default;
    ~UParticleModuleTypeDataBase() = default;
    
    //~ Begin UParticleModule Interface
    virtual EModuleType	GetModuleType() const override { return EPMT_TypeData; }
    //~ End UParticleModule Interface

    /**
     * Build any resources required for simulating the emitter.
     * @param EmitterBuildInfo - Information compiled for the emitter.
     */
    virtual void Build(struct FParticleEmitterBuildInfo& EmitterBuildInfo) {}

    /**
     * Return whether the type data module requires a build step.
     */
    virtual bool RequiresBuild() const { return false; }

    // @todo document
    virtual FParticleEmitterInstance* CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent);

    //virtual const FVertexFactoryType* GetVertexFactoryType() const { return nullptr; }
    virtual EPrimitiveType GetPrimitiveType() const { return PT_TriangleList; }

    /** Cache any desired module pointers inside this type data */
    virtual void CacheModuleInfo(UParticleEmitter* Emitter) {}

    // @todo document
    virtual bool		SupportsSpecificScreenAlignmentFlags() const { return false; }
    // @todo document
    virtual bool		SupportsSubUV() const { return false; }
    // @todo document
    virtual bool		IsAMeshEmitter() const { return false; }

    /** Determine if motion blur is enabled for the owning emitter. */
    virtual bool		IsMotionBlurEnabled() const { return false; }
};



