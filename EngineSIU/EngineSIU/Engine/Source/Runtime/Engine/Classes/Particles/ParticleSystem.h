// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
//#include "UObject/ScriptMacros.h"
//#include "RHIDefinitions.h"

class UInterpCurveEdSetup;
class UParticleSystemComponent;
class UParticleEmitter;

/**
 *	ParticleSystemUpdateMode
 *	Enumeration indicating the method by which the system should be updated
 */
//UENUM()
enum EParticleSystemUpdateMode : int
{
    /** RealTime	- update via the delta time passed in				*/
    EPSUM_RealTime,
    /** FixedTime	- update via a fixed time step						*/
    EPSUM_FixedTime
};

/**
 *	ParticleSystemLODMethod
 */
//UENUM()
enum ParticleSystemLODMethod : int
{
    // Automatically set the LOD level, checking every LODDistanceCheckTime seconds.
    PARTICLESYSTEMLODMETHOD_Automatic,

    // LOD level is directly set by the game code.
    PARTICLESYSTEMLODMETHOD_DirectSet,

    // LOD level is determined at Activation time, then left alone unless directly set by game code.
    PARTICLESYSTEMLODMETHOD_ActivateAutomatic
};

/** Structure containing per-LOD settings that pertain to the entire UParticleSystem. */
//USTRUCT()
struct FParticleSystemLOD
{
    //GENERATED_USTRUCT_BODY()

    FParticleSystemLOD()
    {
    }

    static FParticleSystemLOD CreateParticleSystemLOD()
    {
        FParticleSystemLOD NewLOD;
        return NewLOD;
    }
};

/**
 *	Temporary array for tracking 'solo' emitter mode.
 *	Entry will be true if emitter was enabled
 */
//USTRUCT()
struct FLODSoloTrack
{
    //GENERATED_USTRUCT_BODY()

    //UPROPERTY(transient)
    TArray<uint8> SoloEnableSetting;

};

//USTRUCT()
struct FNamedEmitterMaterial
{
    //GENERATED_USTRUCT_BODY()

    FNamedEmitterMaterial()
        : Name(NAME_None)
        , Material(nullptr)
    {
    }

    //UPROPERTY(EditAnywhere, Category = NamedMaterial)
    FName Name;

    //UPROPERTY(EditAnywhere, Category = NamedMaterial)
    UMaterial* Material;
};

/**
 * A ParticleSystem is a complete particle effect that contains any number of ParticleEmitters. By allowing multiple emitters
 * in a system, the designer can create elaborate particle effects that are held in a single system. Once created using
 * Cascade, a ParticleSystem can then be inserted into a level or created in script.
 */
//UCLASS(hidecategories = Object, MinimalAPI, BlueprintType)
class UParticleSystem : public UObject
{
    DECLARE_CLASS(UParticleSystem, UObject)
public:
    UParticleSystem() = default;
    ~UParticleSystem() = default;

    /** Max number of components of this system to keep resident in the world component pool. */
    //UPROPERTY(EditAnywhere, Category = Performance, AdvancedDisplay)
    uint32 MaxPoolSize;
    //TODO: Allow pool size overriding per world and possibly implement some preallocation too.

    /**
    * How many instances we should use to initially prime the pool.
    * This can amortize runtime activation cost by moving it to load time.
    * Use with care as this could cause large hitches for systems loaded/unloaded during play rather than at level load.
    */
    //UPROPERTY(EditAnywhere, Category = Performance, AdvancedDisplay)
    uint32 PoolPrimeSize = 0;

    /** UpdateTime_FPS	- the frame per second to update at in FixedTime mode		*/
    //UPROPERTY(EditAnywhere, Category = ParticleSystem)
    float UpdateTime_FPS;

    /** UpdateTime_Delta	- internal												*/
    // UPROPERTY()
    float UpdateTime_Delta;

    /**
     * WarmupTime - the time to warm-up the particle system when first rendered
     * Warning: WarmupTime is implemented by simulating the particle system for the time requested upon activation.
     * This is extremely prone to cause hitches, especially with large particle counts - use with caution.
     */
    //UPROPERTY(EditAnywhere, Category = ParticleSystem, meta = (DisplayName = "Warmup Time - beware hitches!"))
    float WarmupTime;

    /**	WarmupTickRate - the time step for each tick during warm up.
        Increasing this improves performance. Decreasing, improves accuracy.
        Set to 0 to use the default tick time.										*/
    //UPROPERTY(EditAnywhere, Category = ParticleSystem)
    float WarmupTickRate;

    /** Emitters	- internal - the array of emitters in the system				*/
    //UPROPERTY(instanced)
    TArray<UParticleEmitter*> Emitters;

    /** The component used to preview the particle system in Cascade				*/
    //UPROPERTY(transient)
    UParticleSystemComponent* PreviewComponent;

    /** Used for curve editor to remember curve-editing setup.						*/
    //UPROPERTY(export)
    //UInterpCurveEdSetup* CurveEdSetup;

    //
    //	LOD
    //
    /**
     *	How often (in seconds) the system should perform the LOD distance check.
     */
    //UPROPERTY(EditAnywhere, Category = LOD, AssetRegistrySearchable)
    float LODDistanceCheckTime;

    /** World space radius that UVs generated with the ParticleMacroUV material node will tile based on. */
    //UPROPERTY(EditAnywhere, Category = MacroUV)
    float MacroUVRadius;

    /**
     *	The array of distances for each LOD level in the system.
     *	Used when LODMethod is set to PARTICLESYSTEMLODMETHOD_Automatic.
     *
     *	Example: System with 3 LOD levels
     *		LODDistances(0) = 0.0
     *		LODDistances(1) = 2500.0
     *		LODDistances(2) = 5000.0
     *
     *		In this case, when the system is [   0.0 ..   2499.9] from the camera, LOD level 0 will be used.
     *										 [2500.0 ..   4999.9] from the camera, LOD level 1 will be used.
     *										 [5000.0 .. INFINITY] from the camera, LOD level 2 will be used.
     *
     */
    //UPROPERTY(EditAnywhere, editfixedsize, Category = LOD)
    TArray<float> LODDistances;

    //UPROPERTY(EditAnywhere, Category = LOD)
    TArray<FParticleSystemLOD> LODSettings;

    /**	Fixed relative bounding box for particle system.							*/
    //UPROPERTY(EditAnywhere, Category = Bounds)
    // FBox -> FBoundingBox
    FBoundingBox FixedRelativeBoundingBox;

    /**
     * Number of seconds of emitter not being rendered that need to pass before it
     * no longer gets ticked/ becomes inactive.
     */
    //UPROPERTY(EditAnywhere, Category = ParticleSystem)
    float SecondsBeforeInactive;

    /** How long this Particle system should delay when ActivateSystem is called on it. */
    //UPROPERTY(EditAnywhere, Category = Delay, AssetRegistrySearchable)
    float Delay;

    /** The low end of the emitter delay if using a range. */
    //UPROPERTY(EditAnywhere, Category = Delay)
    float DelayLow;

    /** If true, the system's Z axis will be oriented toward the camera				*/
    //UPROPERTY(EditAnywhere, Category = ParticleSystem)
    uint8 bOrientZAxisTowardCamera : 1;

    /** Whether to use the fixed relative bounding box or calculate it every frame. */
    //UPROPERTY(EditAnywhere, Category = Bounds)
    uint8 bUseFixedRelativeBoundingBox : 1;

    /** EDITOR ONLY: Indicates that Cascade would like to have the PeakActiveParticles count reset */
    // UPROPERTY()
    uint8 bShouldResetPeakCounts : 1;

    /** Set during load time to indicate that physics is used... */
    //UPROPERTY(transient)
    uint8 bHasPhysics : 1;

    /** Inidicates the old 'real-time' thumbnail rendering should be used	*/
    //UPROPERTY(EditAnywhere, Category = Thumbnail)
    uint8 bUseRealtimeThumbnail : 1;

    /** Internal: Indicates the PSys thumbnail image is out of date			*/
    // UPROPERTY()
    uint8 ThumbnailImageOutOfDate : 1;

public:
    /**
     *	If true, select the emitter delay from the range
     *		[DelayLow..Delay]
     */
    //UPROPERTY(EditAnywhere, Category = Delay)
    uint8 bUseDelayRange : 1;

    //UPROPERTY(EditAnywhere, Category = Performance, meta = (ToolTip = "Whether or not to allow instances of this system to have their ticks managed."), AdvancedDisplay)
    uint8 bAllowManagedTicking : 1;

    //UPROPERTY(EditAnywhere, Category = Performance, meta = (ToolTip = "Auto-deactivate system if all emitters are determined to not spawn particles again, regardless of lifetime."))
    uint8 bAutoDeactivate : 1;

    /**
     *	Internal value that tracks the regenerate LOD levels preference.
     *	If true, when autoregenerating LOD levels in code, the low level will
     *	be a duplicate of the high.
     */
    // UPROPERTY()
    uint8 bRegenerateLODDuplicate : 1;

    //UPROPERTY(EditAnywhere, Category = ParticleSystem, AssetRegistrySearchable)
    EParticleSystemUpdateMode SystemUpdateMode;

    /**
     *	The method of LOD level determination to utilize for this particle system
     *	  PARTICLESYSTEMLODMETHOD_Automatic - Automatically set the LOD level, checking every LODDistanceCheckTime seconds.
     *    PARTICLESYSTEMLODMETHOD_DirectSet - LOD level is directly set by the game code.
     *    PARTICLESYSTEMLODMETHOD_ActivateAutomatic - LOD level is determined at Activation time, then left alone unless directly set by game code.
     */
    //UPROPERTY(EditAnywhere, Category = LOD)
    ParticleSystemLODMethod LODMethod;

private:
    /** Does any emitter loop forever? */
    uint8 bAnyEmitterLoopsForever : 1;

public:
    //UPROPERTY(EditAnywhere, Category = Performance, meta = (ToolTip = "Minimum duration between ticks; 33=tick at max. 30FPS, 16=60FPS, 8=120FPS"))
    uint32 MinTimeBetweenTicks;

    /** Local space position that UVs generated with the ParticleMacroUV material node will be centered on. */
    //UPROPERTY(EditAnywhere, Category = MacroUV)
    FVector MacroUVPosition;

    //UPROPERTY(transient)
    TArray<struct FLODSoloTrack> SoloTracking;

    /**
    *	Array of named material slots for use by emitters of this system.
    *	Emitters can use these instead of their own materials by providing the name to the NamedMaterialOverrides property of their required module.
    *	These materials can be overridden using CreateNamedDynamicMaterialInstance() on a ParticleSystemComponent.
    */
    //UPROPERTY(EditAnywhere, Category = Materials)
    TArray<FNamedEmitterMaterial> NamedMaterialSlots;

    //bool UsesCPUCollision() const;
    //virtual void Serialize(FArchive& Ar) override;

    //~ End UObject Interface.
    bool CanBePooled()const;

    // @todo document
    //void UpdateColorModuleClampAlpha(class UParticleModuleColorBase* ColorModule);

    /**
     *	CalculateMaxActiveParticleCounts
     *	Determine the maximum active particles that could occur with each emitter.
     *	This is to avoid reallocation during the life of the emitter.
     *
     *	@return	true	if the numbers were determined for each emitter
     *			false	if not be determined
     */
    virtual bool		CalculateMaxActiveParticleCounts();

    /**
     *	Retrieve the parameters associated with this particle system.
     *
     *	@param	ParticleSysParamList	The list of FParticleSysParams used in the system
     *	@param	ParticleParameterList	The list of ParticleParameter distributions used in the system
     */
    //void GetParametersUtilized(TArray<TArray<FString> >& ParticleSysParamList,
    //    TArray<TArray<FString> >& ParticleParameterList);

    // Soloing관련 (에디터 상에서 단일한 Emitter만 확인하는 기능)
    /**
     *	Setup the soloing information... Obliterates all current soloing.
     */
    //void SetupSoloing();

    //bool ToggleSoloing(class UParticleEmitter* InEmitter);

    //bool TurnOffSoloing();

    /**
     *	Editor helper function for setting the LOD validity flags used in Cascade.
     */
    //void SetupLODValidity();

    /**
     * Set the time to delay spawning the particle system
     */
    //void SetDelay(float InDelay);

    /** Return the currently set LOD method											*/
    //virtual enum ParticleSystemLODMethod GetCurrentLODMethod();

    /**
     *	Return the number of LOD levels for this particle system
     *
     *	@return	The number of LOD levels in the particle system
     */
    //virtual int32 GetLODLevelCount();

    /**
     *	Return the distance for the given LOD level
     *
     *	@param	LODLevelIndex	The LOD level that the distance is being retrieved for
     *
     *	@return	-1.0f			If the index is invalid
     *			Distance		The distance set for the LOD level
     */
    //virtual float GetLODDistance(int32 LODLevelIndex);

    /**
     *	Set the LOD method
     *
     *	@param	InMethod		The desired method
     */
    //virtual void SetCurrentLODMethod(ParticleSystemLODMethod InMethod);

    /**
     *	Set the distance for the given LOD index
     *
     *	@param	LODLevelIndex	The LOD level to set the distance of
     *	@param	InDistance		The distance to set
     *
     *	@return	true			If successful
     *			false			Invalid LODLevelIndex
     */
    //virtual bool SetLODDistance(int32 LODLevelIndex, float InDistance);

    /**
     * Builds all emitters in the particle system.
     */
    //void BuildEmitters();

    /**
    Returns true if this system contains an emitter of the pasesd type.
    @ param TypeData - The emitter type to check for. Must be a child class of UParticleModuleTypeDataBase
    */
    //UFUNCTION(BlueprintCallable, Category = "Particle System")
    //bool ContainsEmitterType(UClass* TypeData);

    /** Returns true if the particle system is looping (contains one or more looping emitters) */
    bool IsLooping() const { return bAnyEmitterLoopsForever; }
    bool IsImmortal() const { return bIsImmortal; }
    bool WillBecomeZombie() const { return bWillBecomeZombie; }

    FORCEINLINE bool AllowManagedTicking()const { return bAllowManagedTicking; }
private:
    /** Does any emitter never die due to infinite looping AND indefinite duration? */
    uint8 bIsImmortal : 1;
    /** Does any emitter ever become a zombie (is immortal AND stops spawning at some point, i.e. is burst only)? */
    uint8 bWillBecomeZombie : 1;
};
