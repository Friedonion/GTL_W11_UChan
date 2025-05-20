#pragma once

#include "Engine/ParticleHelper.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "ParticleEmitterInstances.h"

class UInterpCurveEdSetup;
class UParticleLODLevel;
class UParticleSystemComponent;
class UParticleModule;

enum EDetailMode : int;

//~=============================================================================
//	Burst emissions
//~=============================================================================
enum class EParticleBurstMethod : int
{
    EPBM_Instant,
    EPBM_Interpolated,
    EPBM_MAX,
};

//~=============================================================================
//	SubUV-related
//~=============================================================================
enum class EParticleSubUVInterpMethod : int
{
    PSUVIM_None ,
    PSUVIM_Linear,
    PSUVIM_Linear_Blend,
    PSUVIM_Random,
    PSUVIM_Random_Blend,
    PSUVIM_MAX,
};

//~=============================================================================
//	Cascade-related
//~=============================================================================
enum class EEmitterRenderMode : int
{
    ERM_Normal,
    ERM_Point,
    ERM_Cross,
    ERM_LightsOnly,
    ERM_None,
    ERM_MAX,
};

//USTRUCT()
struct FParticleBurst
{
    DECLARE_STRUCT(FParticleBurst)

    /** The number of particles to burst */
    //UPROPERTY(EditAnywhere, Category = ParticleBurst)
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, int32, Count)

    /** If >= 0, use as a range [CountLow..Count] */
    //UPROPERTY(EditAnywhere, Category = ParticleBurst)
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, int32, CountLow)

    /** The time at which to burst them (0..1: emitter lifetime) */
    //UPROPERTY(EditAnywhere, Category = ParticleBurst)
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, float, Time)

    FParticleBurst()
        : Count(0)
        , CountLow(-1)		// Disabled by default...
        , Time(0.0f)
    {
    }
};

//UCLASS(hidecategories = Object, editinlinenew, abstract, MinimalAPI)
class UParticleEmitter : public UObject
{
    DECLARE_CLASS(UParticleEmitter, UObject)
public:
    UParticleEmitter();
    ~UParticleEmitter() = default;
    //~=============================================================================
    //	General variables
    //~=============================================================================
    /** The name of the emitter. */
    //UPROPERTY(EditAnywhere, Category = Particle)
    FName EmitterName;

    //UPROPERTY(transient)
    int32 SubUVDataOffset;

    /**
     *	How to render the emitter particles. Can be one of the following:
     *		ERM_Normal	- As the intended sprite/mesh
     *		ERM_Point	- As a 2x2 pixel block with no scaling and the color set in EmitterEditorColor
     *		ERM_Cross	- As a cross of lines, scaled to the size of the particle in EmitterEditorColor
     *		ERM_None	- Do not render
     */
    //UPROPERTY(EditAnywhere, Category = Cascade)
    EEmitterRenderMode EmitterRenderMode;

    EParticleAxisLock LockAxisFlags;

    //////////////////////////////////////////////////////////////////////////
    // Below is information udpated by calling CacheEmitterModuleInfo
    uint8 bRequiresLoopNotification : 1;
    uint8 bAxisLockEnabled : 1;
    uint8 bMeshRotationActive : 1;

    //UPROPERTY()
    uint8 ConvertedModules : 1;

    /** If true, then show only this emitter in the editor */
    //UPROPERTY(transient)
    uint8 bIsSoloing : 1;

    /**
     *	If true, then this emitter was 'cooked out' by the cooker.
     *	This means it was completely disabled, but to preserve any
     *	indexing schemes, it is left in place.
     */
    // UPROPERTY()
    uint8 bCookedOut : 1;

    /** When true, if the current LOD is disabled the emitter will be kept alive. Otherwise, the emitter will be considered complete if the current LOD is disabled. */
    //UPROPERTY(EditAnywhere, Category = Particle)
    uint8 bDisabledLODsKeepEmitterAlive : 1;

    //~=============================================================================
    //	'Private' data - not required by the editor
    //~=============================================================================
    //UPROPERTY(instanced)
    TArray<UParticleLODLevel*> LODLevels;

    // UPROPERTY()
    int32 PeakActiveParticles;

    //~=============================================================================
    //	Performance/LOD Data
    //~=============================================================================

    /**
     *	Initial allocation count - overrides calculated peak count if > 0
     */
    //UPROPERTY(EditAnywhere, Category = Particle)
    int32 InitialAllocationCount;

    //UPROPERTY(EditAnywhere, Category = Particle)
    float QualityLevelSpawnRateScale;

    /** Detail mode: Set flags reflecting which system detail mode you want the emitter to be ticked and rendered in */
    //UPROPERTY(EditAnywhere, Category = Particle, meta = (Bitmask, BitmaskEnum = "/Script/Engine.EParticleDetailMode"))
    uint32 DetailModeBitmask;

    /** Map module pointers to their offset into the particle data.		*/
    TMap<UParticleModule*, uint32> ModuleOffsetMap;

    /** Map module pointers to their offset into the instance data.		*/
    TMap<UParticleModule*, uint32> ModuleInstanceOffsetMap;

    /** Map module pointers to their offset into the instance data.		*/
    TMap<UParticleModule*, uint32> ModuleRandomSeedInstanceOffsetMap;

    /** Materials collected from any MeshMaterial modules */
    TArray<class UMaterial*> MeshMaterials;

    int32 DynamicParameterDataOffset;
    int32 LightDataOffset;
    float LightVolumetricScatteringIntensity;
    int32 CameraPayloadOffset;
    int32 ParticleSize;
    int32 ReqInstanceBytes;
    FVector2D PivotOffset;
    int32 TypeDataOffset;
    int32 TypeDataInstanceOffset;

    float MinFacingCameraBlendDistance;
    float MaxFacingCameraBlendDistance;

    // Array of modules that want emitter instance data
    TArray<UParticleModule*> ModulesNeedingInstanceData;

    // Array of modules that want emitter random seed instance data
    TArray<UParticleModule*> ModulesNeedingRandomSeedInstanceData;

    /** SubUV animation asset to use for cutout geometry. */
    class USubUVAnimation* __restrict SubUVAnimation;

    // UObject Interface
    //virtual void Serialize(FArchive& Ar)override;
    //virtual void PostLoad() override;

    // @todo document
    virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InComponent);
    
    // Sets up this emitter with sensible defaults so we can see some particles as soon as its created.
    //virtual void SetToSensibleDefaults() {}
    
    virtual void UpdateModuleLists();

    void SetEmitterName(FName Name);

    FName& GetEmitterName();

    virtual	void SetLODCount(int32 LODCount);

    /** CreateLODLevel
     *	Creates the given LOD level.
     *	Intended for editor-time usage.
     *	Assumes that the given LODLevel will be in the [0..100] range.
     *
     *	@return The index of the created LOD level
     */
    int32 CreateLODLevel(int32 LODLevel, bool bGenerateModuleData = true);

    /** IsLODLevelValid
     *	Returns true if the given LODLevel is one of the array entries.
     *	Intended for editor-time usage.
     *	Assumes that the given LODLevel will be in the [0..(NumLODLevels - 1)] range.
     *
     *	@return false if the requested LODLevel is not valid.
     *			true if the requested LODLevel is valid.
     */
    bool IsLODLevelValid(int32 LODLevel);

    /** GetCurrentLODLevel
    *	Returns the currently set LODLevel. Intended for game-time usage.
    *	Assumes that the given LODLevel will be in the [0..# LOD levels] range.
    *
    *	@return NULL if the requested LODLevel is not valid.
    *			The pointer to the requested UParticleLODLevel if valid.
    */
    UParticleLODLevel* GetCurrentLODLevel(FParticleEmitterInstance* Instance);

    /**
     * This will update the LOD of the particle in the editor.
     *
     * @see GetCurrentLODLevel(FParticleEmitterInstance* Instance)
     */
    //void EditorUpdateCurrentLOD(FParticleEmitterInstance* Instance);

    /** GetLODLevel
     *	Returns the given LODLevel. Intended for game-time usage.
     *	Assumes that the given LODLevel will be in the [0..# LOD levels] range.
     *
     *	@param	LODLevel - the requested LOD level in the range [0..# LOD levels].
     *
     *	@return NULL if the requested LODLevel is not valid.
     *			The pointer to the requested UParticleLODLevel if valid.
     */
    UParticleLODLevel* GetLODLevel(int32 LODLevel);

    /**
     *	Autogenerate the lowest LOD level...
     *
     *	@param	bDuplicateHighest	If true, make the level an exact copy of the highest
     *
     *	@return	bool				true if successful, false if not.
     */
    //virtual bool AutogenerateLowestLODLevel(bool bDuplicateHighest = false);

    /**
     *	CalculateMaxActiveParticleCount
     *	Determine the maximum active particles that could occur with this emitter.
     *	This is to avoid reallocation during the life of the emitter.
     *
     *	@return	true	if the number was determined
     *			false	if the number could not be determined
     */
    //virtual bool CalculateMaxActiveParticleCount();

    /**
     *	Retrieve the parameters associated with this particle system.
     *
     *	@param	ParticleSysParamList	The list of FParticleSysParams used in the system
     *	@param	ParticleParameterList	The list of ParticleParameter distributions used in the system
     */
  /*  void GetParametersUtilized(TArray<FString>& ParticleSysParamList,
        TArray<FString>& ParticleParameterList);*/

    /**
     * Builds data needed for simulation by the emitter from all modules.
     */
    void Build();

    /** Pre-calculate data size/offset and other info from modules in this Emitter */
    void CacheEmitterModuleInfo();

    /**
     *   Calculate spawn rate multiplier based on global effects quality level and emitter's quality scale
     */
    //float GetQualityLevelSpawnRateMult();

    /** Returns true if the is emitter has any enabled LODs, false otherwise. */
    //bool HasAnyEnabledLODs()const;
};


