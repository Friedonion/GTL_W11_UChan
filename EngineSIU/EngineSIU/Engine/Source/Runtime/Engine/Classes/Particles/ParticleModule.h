#pragma once

#include "UObject/Object.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "Core/Container/String.h"
#include "Engine/ParticleHelper.h"

class UDistribution;
class UDistributionFloat;
class UDistributionVector;
class UInterpCurveEdSetup;
class UParticleEmitter;
class UParticleLODLevel;
class UParticleModuleTypeDataBase;
class UParticleSystemComponent;
struct FCurveEdEntry;
struct FParticleEmitterInstance;

/** ModuleType
 *	Indicates the kind of emitter the module can be applied to.
 *	ie, EPMT_Beam - only applies to beam emitters.
 *
 *	The TypeData field is present to speed up finding the TypeData module.
 */

enum EModuleType : int
{
    /** General - all emitter types can use it			*/
    EPMT_General,
    /** TypeData - TypeData modules						*/
    EPMT_TypeData,
    /** Beam - only applied to beam emitters			*/
    EPMT_Beam,
    /** Trail - only applied to trail emitters			*/
    EPMT_Trail,
    /** Spawn - all emitter types REQUIRE it			*/
    EPMT_Spawn,
    /** Required - all emitter types REQUIRE it			*/
    EPMT_Required,
    /** Event - event related modules					*/
    EPMT_Event,
    /** Light related modules							*/
    EPMT_Light,
    /** SubUV related modules							*/
    EPMT_SubUV,
    EPMT_MAX,
};

/**
 *	Particle Selection Method, for any emitters that utilize particles
 *	as the source points.
 */
enum EParticleSourceSelectionMethod : int
{
    /** Random		- select a particle at random		*/
    EPSSM_Random,
    /** Sequential	- select a particle in order		*/
    EPSSM_Sequential,
    EPSSM_MAX,
};

struct FParticleCurvePair
{
    FString CurveName;

    UObject* CurveObject;

    FParticleCurvePair()
        : CurveObject(NULL)
    {
    }

};

struct FParticleRandomSeedInfo
{
    /** The name to expose to the placed instances for setting this seed */
    FName ParameterName;

    /**
     *	If true, the module will attempt to get the seed from the owner
     *	instance. If that fails, it will fall back to getting it from
     *	the RandomSeeds array.
     */
    //UPROPERTY(EditAnywhere, uint8, bGetSeedFromInstance, 1);
    uint8 bGetSeedFromInstance : 1;

    /**
     *	If true, the seed value retrieved from the instance will be an
     *	index into the array of seeds.
     */
    //UPROPERTY(EditAnywhere, Category = ParticleRandomSeedInfo)
    uint8 bInstanceSeedIsIndex : 1;

    /**
     *	If true, then reset the seed upon the emitter looping.
     *	For looping environmental effects this should likely be set to false to avoid
     *	a repeating pattern.
     */
    //UPROPERTY(EditAnywhere, Category = ParticleRandomSeedInfo)
    uint8 bResetSeedOnEmitterLooping : 1;

    /**
    *	If true, then randomly select a seed entry from the RandomSeeds array
    */
    //UPROPERTY(EditAnywhere, Category = ParticleRandomSeedInfo)
    uint8 bRandomlySelectSeedArray : 1;

    /**
     *	The random seed values to utilize for the module.
     *	More than 1 means the instance will randomly select one.
     */
    //UPROPERTY(EditAnywhere, Category = ParticleRandomSeedInfo)
    TArray<int32> RandomSeeds;



    FParticleRandomSeedInfo()
        : bGetSeedFromInstance(false)
        , bInstanceSeedIsIndex(false)
        , bResetSeedOnEmitterLooping(true)
        , bRandomlySelectSeedArray(false)
    {
    }
    FORCEINLINE int32 GetInstancePayloadSize() const
    {
        return ((RandomSeeds.Num() > 0) ? sizeof(FParticleRandomSeedInstancePayload) : 0);
    }

};

//UCLASS(editinlinenew, hidecategories = Object, abstract, Within = ParticleSystem, MinimalAPI)
class UParticleModule : public UObject
{
    DECLARE_CLASS(UParticleModule, UObject)
public:
    UParticleModule();
    ~UParticleModule() = default;
    
    /** If true, the module performs operations on particles during Spawning		*/
    // UPROPERTY()
    uint8 bSpawnModule : 1;

    /** If true, the module performs operations on particles during Updating		*/
    // UPROPERTY()
    uint8 bUpdateModule : 1;

    /** If true, the module performs operations on particles during final update	*/
    // UPROPERTY()
    uint8 bFinalUpdateModule : 1;

    /** If true, the module performs operations on particles during update and/or final update for GPU emitters*/
    // UPROPERTY()
    uint8 bUpdateForGPUEmitter : 1;

    /** If true, the module displays FVector curves as colors						*/
    // UPROPERTY()
    uint8 bCurvesAsColor : 1;

    /** If true, the module should render its 3D visualization helper				*/
    //UPROPERTY(EditAnywhere, Category = Cascade)
    uint8 b3DDrawMode : 1;

    /** If true, the module supports rendering a 3D visualization helper			*/
    // UPROPERTY()
    uint8 bSupported3DDrawMode : 1;

    /** If true, the module is enabled												*/
    // UPROPERTY()
    uint8 bEnabled : 1;

    /** If true, the module has had editing enabled on it							*/
    // UPROPERTY()
    uint8 bEditable : 1;

    /**
    *	If true, this flag indicates that auto-generation for LOD will result in
    *	an exact duplicate of the module, regardless of the percentage.
    *	If false, it will result in a module with different settings.
    */
    // UPROPERTY()
    uint8 LODDuplicate : 1;

    /** If true, the module supports RandomSeed setting */
    // UPROPERTY()
    uint8 bSupportsRandomSeed : 1;

    /** If true, the module should be told when looping */
    // UPROPERTY()
    uint8 bRequiresLoopingNotification : 1;

    /**
     *	The LOD levels this module is present in.
     *	Bit-flags are used to indicate validity for a given LOD level.
     *	For example, if
     *		((1 << Level) & LODValidity) != 0
     *	then the module is used in that LOD.
     */
    // UPROPERTY()
    uint8 LODValidity;

    /**
     * Called once to compile the effects of this module on runtime simulation.
     * @param EmitterInfo - Information needed for runtime simulation.
     */
    virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo);

    /**
     *	Called on a particle that is freshly spawned by the emitter.
     *
     *	@param	Owner		The FParticleEmitterInstance that spawned the particle.
     *	@param	Offset		The modules offset into the data payload of the particle.
     *	@param	SpawnTime	The time of the spawn.
     */
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase);
    /**
     *	Called on a particle that is being updated by its emitter.
     *
     *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
     *	@param	Offset		The modules offset into the data payload of the particle.
     *	@param	DeltaTime	The time since the last update.
     */
    virtual void	Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime);
    /**
     *	Called on an emitter when all other update operations have taken place
     *	INCLUDING bounding box cacluations!
     *
     *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
     *	@param	Offset		The modules offset into the data payload of the particle.
     *	@param	DeltaTime	The time since the last update.
     */
    virtual void	FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime);

    /**
     *	Returns the number of bytes that the module requires in the particle payload block.
     *
     *	@param	TypeData	The UParticleModuleTypeDataBase for the emitter that contains this module
     *
     *	@return	uint32		The number of bytes the module needs per particle.
     */
    virtual uint32	RequiredBytes(UParticleModuleTypeDataBase* TypeData);
    /**
     *	Returns the number of bytes the module requires in the emitters 'per-instance' data block.
     *
     *	@return uint32		The number of bytes the module needs per emitter instance.
     */
    virtual uint32	RequiredBytesPerInstance();

    /**
     *	Allows the module to prep its 'per-instance' data block.
     *
     *	@param	Owner		The FParticleEmitterInstance that 'owns' the particle.
     *	@param	InstData	Pointer to the data block for this module.
     */
    virtual uint32	PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData);

    // For Cascade
    /**
     *	Called when the module is created, this function allows for setting values that make
     *	sense for the type of emitter they are being used in.
     *
     *	@param	Owner			The UParticleEmitter that the module is being added to.
     */
    virtual void SetToSensibleDefaults(UParticleEmitter* Owner);

    /**
     *	Fill an array with each Object property that fulfills the FCurveEdInterface interface.
     *
     *	@param	OutCurve	The array that should be filled in.
     */
    //virtual void	GetCurveObjects(TArray<FParticleCurvePair>& OutCurves);
    /**
     *	Render the modules 3D visualization helper primitive.
     *
     *	@param	Owner		The FParticleEmitterInstance that 'owns' the module.
     *	@param	View		The scene view that is being rendered.
     *	@param	PDI			The FPrimitiveDrawInterface to use for rendering.
     */
    //virtual void Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View, FPrimitiveDrawInterface* PDI) {};

    /**
     *	Retrieve the ModuleType of this module.
     *
     *	@return	EModuleType		The type of module this is.
     */
    virtual EModuleType	GetModuleType() const { return EPMT_General; }

    /**
     *	Helper function used by the editor to auto-populate a placed AEmitter with any
     *	instance parameters that are utilized.
     *
     *	@param	PSysComp		The particle system component to be populated.
     */
    //virtual void	AutoPopulateInstanceProperties(UParticleSystemComponent* PSysComp);

    /**
     *	Helper function used by the editor to auto-generate LOD values from a source module
     *	and a percentage value used to scale its values.
     *
     *	@param	SourceModule		The module to use as the source for values
     *	@param	Percentage			The percentage of the source values to set
     *	@param	LODLevel			The LOD level being generated
     *
     *	@return	true	if successful
     *			false	if failed
     */
    virtual bool	GenerateLODModuleValues(UParticleModule* SourceModule, float Percentage, UParticleLODLevel* LODLevel);

    /**
     *	Store the given percentage of the SourceFloat distribution in the FloatDist
     *
     *	@param	FloatDist			The distribution to put the result into.
     *	@param	SourceFloatDist		The distribution of use as the source.
     *	@param	Percentage			The percentage of the source value to use [0..100]
     *
     *	@return	bool				true if successful, false if not.
     */
    //virtual bool	ConvertFloatDistribution(UDistributionFloat* FloatDist, UDistributionFloat* SourceFloatDist, float Percentage);
    /**
     *	Store the given percentage of the SourceVector distribution in the VectorDist
     *
     *	@param	VectorDist			The distribution to put the result into.
     *	@param	SourceVectorDist	The distribution of use as the source.
     *	@param	Percentage			The percentage of the source value to use [0..100]
     *
     *	@return	bool				true if successful, false if not.
     */
    //virtual bool	ConvertVectorDistribution(UDistributionVector* VectorDist, UDistributionVector* SourceVectorDist, float Percentage);
    /**
     *	Returns whether the module is SizeMultipleLife or not.
     *
     *	@return	bool	true if the module is a UParticleModuleSizeMultipleLife
     *					false if not
     */
    virtual bool   IsSizeMultiplyLife() { return false; };

    /**
     *	Returns whether the module supports the RandomSeed functionality
     *
     *	@return	bool	true if it supports it; false if not
     */
    bool SupportsRandomSeed() const
    {
        return bSupportsRandomSeed;
    }

    /**
     *	Returns whether the module requires notification when an emitter loops.
     *
     *	@return	bool	true if the module required looping notification
     */
    bool RequiresLoopingNotification() const
    {
        return bRequiresLoopingNotification;
    }

    /**
     *	Called when an emitter instance is looping...
     *
     *	@param	Owner					The emitter instance that owns this module
     */
    virtual void EmitterLoopingNotify(FParticleEmitterInstance* Owner)
    {
    }

    /**
     *	Generates a new module for LOD levels, setting the values appropriately.
     *	Note that the module returned could simply be the module it was called on.
     *
     *	@param	SourceLODLevel		The source LODLevel
     *	@param	DestLODLevel		The destination LODLevel
     *	@param	Percentage			The percentage value that should be used when setting values
     *
     *	@return	UParticleModule*	The generated module, or this if percentage == 100.
     */
    /*virtual UParticleModule* GenerateLODModule(UParticleLODLevel* SourceLODLevel, UParticleLODLevel* DestLODLevel, float Percentage,
        bool bGenerateModuleData, bool bForceModuleConstruction = false);*/

    /**
     *	Returns true if the results of LOD generation for the given percentage will result in a
     *	duplicate of the module.
     *
     *	@param	SourceLODLevel		The source LODLevel
     *	@param	DestLODLevel		The destination LODLevel
     *	@param	Percentage			The percentage value that should be used when setting values
     *
     *	@return	bool				true if the generated module will be a duplicate.
     *								false if not.
     */
    virtual bool WillGeneratedModuleBeIdentical(UParticleLODLevel* SourceLODLevel, UParticleLODLevel* DestLODLevel, float Percentage)
    {
        // The assumption is that at 100%, ANY module will be identical...
        // (Although this is virtual to allow over-riding that assumption on a case-by-case basis!)

        if (Percentage != 100.0f)
        {
            return LODDuplicate;
        }

        return true;
    }

    /**
     *	Returns true if the module validiy flags indicate this module is used in the given LOD level.
     *
     *	@param	SourceLODIndex		The index of the source LODLevel
     *
     *	@return	bool				true if the generated module is used, false if not.
     */
    //virtual bool IsUsedInLODLevel(int32 SourceLODIndex) const;

    /**
     *	Retrieve the ParticleSysParams associated with this module.
     *
     *	@param	ParticleSysParamList	The list of FParticleSysParams to add to
     */
    //virtual void GetParticleSysParamsUtilized(TArray<FString>& ParticleSysParamList);

    /**
     *	Retrieve the distributions that use ParticleParameters in this module.
     *
     *	@param	ParticleParameterList	The list of ParticleParameter distributions to add to
     */
    //virtual void GetParticleParametersUtilized(TArray<FString>& ParticleParameterList);

    /**
     *	Refresh the module...
     */
    virtual void RefreshModule(UInterpCurveEdSetup* EdSetup, UParticleEmitter* InEmitter, int32 InLODLevel) {}

    /**
     *	Return true if this module impacts rotation of Mesh emitters
     *	@return	bool		true if the module impacts mesh emitter rotation
     */
    virtual bool	TouchesMeshRotation() const { return false; }

    /**
     *	Prepare a random seed instance payload...
     *
     *	@param	Owner					The emitter instance that owns this module
     *	@param	InRandSeedPayload		The random seed instance payload to initialize
     *	@param	InRandSeedInfo			The random seed info of the module
     *
     *	@return	uint32					0xffffffff is failed
     */
    //virtual uint32	PrepRandomSeedInstancePayload(FParticleEmitterInstance* Owner, FParticleRandomSeedInstancePayload* InRandSeedPayload, const FParticleRandomSeedInfo& InRandSeedInfo);

    /**
     *	Retrieve the random seed info for this module.
     *
     *	@return	FParticleRandomSeedInfo*	The random seed info; NULL if not supported
     */
    virtual FParticleRandomSeedInfo* GetRandomSeedInfo()
    {
        return NULL;
    }

    /**
     *	Set the random seed info entry at the given index to the given seed
     *
     *	@param	InIndex			The index of the entry to set
     *	@param	InRandomSeed	The seed to set the entry to
     *
     *	@return	bool			true if successful; false if not (not found, etc.)
     */
    //virtual bool SetRandomSeedEntry(int32 InIndex, int32 InRandomSeed);

    /** Return false if this emitter requires a game thread tick **/
    virtual bool CanTickInAnyThread()
    {
        return true;
    }

    /**
     * Retreive the random stream that should be used for the provided instance.
     *
     * @param	Owner			The emitter instance that owns this module
     *
     * @return FRandomStream&	The random stream to use for the provided instance.
     */
    //FRandomStream& GetRandomStream(FParticleEmitterInstance* Owner);
};



