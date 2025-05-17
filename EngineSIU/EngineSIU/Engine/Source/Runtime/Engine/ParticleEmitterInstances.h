#pragma once

#include "Container/Array.h"
#include "Core/Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/Transform.h"

struct FDynamicEmitterReplayDataBase;
struct FBaseParticle;
struct FParticleRandomSeedInstancePayload;
class UParticleModule;
class UWorld;
class UMaterial;
enum EParticleAxisLock : int;
class UParticleSystemComponent;
class UParticleEmitter;
class UParticleLODLevel;

struct FLODBurstFired
{
    TArray<bool> Fired;
};

// Hacky base class to avoid 8 bytes of padding after the vtable
struct FParticleEmitterInstanceFixLayout
{
    virtual ~FParticleEmitterInstanceFixLayout() = default;
};

/*-----------------------------------------------------------------------------
    FParticleEmitterInstance
-----------------------------------------------------------------------------*/
struct FParticleEmitterInstance : FParticleEmitterInstanceFixLayout
{
public:
    /** The maximum DeltaTime allowed for updating PeakActiveParticle tracking.
     *	Any delta time > this value will not impact active particle tracking.
     */
    static const float PeakActiveParticleUpdateDelta;

    /** The template this instance is based on.							*/
    UParticleEmitter* SpriteTemplate;
    /** The component who owns it.										*/
    UParticleSystemComponent* Component;
    /** The currently set LOD level.									*/
    UParticleLODLevel* CurrentLODLevel;
    /** The index of the currently set LOD level.						*/
    int32 CurrentLODLevelIndex;
    /** The offset to the TypeData payload in the particle data.		*/
    int32 TypeDataOffset;
    /** The offset to the TypeData instance payload.					*/
    int32 TypeDataInstanceOffset;
    /** The offset to the SubUV payload in the particle data.			*/
    int32 SubUVDataOffset;
    /** The offset to the dynamic parameter payload in the particle data*/
    int32 DynamicParameterDataOffset;
    /** Offset to the light module data payload.						*/
    int32 LightDataOffset;
    float LightVolumetricScatteringIntensity;
    /** The offset to the Orbit module payload in the particle data.	*/
    int32 OrbitModuleOffset;
    /** The offset to the Camera payload in the particle data.			*/
    int32 CameraPayloadOffset;
    /** The offset to the particle data.								*/
    int32 PayloadOffset;
    /** The location of the emitter instance							*/
    FVector Location;
    /** Transform from emitter local space to simulation space.			*/
    FMatrix EmitterToSimulation;
    /** Transform from simulation space to world space.					*/
    FMatrix SimulationToWorld;
    /** Component can disable Tick and Rendering of this emitter. */
    uint8 bEnabled : 1;
    /** If true, kill this emitter instance when it is deactivated.		*/
    uint8 bKillOnDeactivate : 1;
    /** if true, kill this emitter instance when it has completed.		*/
    uint8 bKillOnCompleted : 1;
    /** Whether this emitter requires sorting as specified by artist.	*/
    uint8 bRequiresSorting : 1;
    /** If true, halt spawning for this instance.						*/
    uint8 bHaltSpawning : 1;
    /** If true, this emitter has been disabled by game code and some systems to re-enable are not allowed. */
    uint8 bHaltSpawningExternal : 1;
    /** If true, the emitter has modules that require loop notification.*/
    uint8 bRequiresLoopNotification : 1;
    /** If true, the emitter ignores the component's scale. (Mesh emitters only). */
    uint8 bIgnoreComponentScale : 1;
    /** Hack: Make sure this is a Beam type to avoid casting from/to wrong types. */
    uint8 bIsBeam : 1;
    /** Whether axis lock is enabled, cached here to avoid finding it from the module each frame */
    uint8 bAxisLockEnabled : 1;
    /** When true and spawning is supressed, the bursts will be faked so that when spawning is enabled again, the bursts don't fire late. */
    uint8 bFakeBurstsWhenSpawningSupressed : 1;
    /** true if the emitter has no active particles and will no longer spawn any in the future */
    uint8 bEmitterIsDone : 1;
    /** Axis lock flags, cached here to avoid finding it from the module each frame */
    EParticleAxisLock LockAxisFlags;
    /** The sort mode to use for this emitter as specified by artist.	*/
    int32 SortMode;
    /** Pointer to the particle data array.								*/
    uint8* ParticleData;
    /** Pointer to the particle index array.							*/
    uint16* ParticleIndices;
    /** Pointer to the instance data array.								*/
    uint8* InstanceData;
    /** The size of the Instance data array.							*/
    int32 InstancePayloadSize;
    /** The total size of a particle (in bytes).						*/
    int32 ParticleSize;
    /** The stride between particles in the ParticleData array.			*/
    int32 ParticleStride;
    /** The number of particles currently active in the emitter.		*/
    int32 ActiveParticles;
    /** Monotonically increasing counter. */
    uint32 ParticleCounter;
    /** The maximum number of active particles that can be held in
     *	the particle data array.
     */
    int32 MaxActiveParticles;
    /** The fraction of time left over from spawning.					*/
    float SpawnFraction;
    /** The number of seconds that have passed since the instance was
     *	created.
     */
    float SecondsSinceCreation;
    /** */
    float EmitterTime;
    /** The amount of time simulated in the previous time step. */
    float LastDeltaTime;
    /** how long did the last tick take? */
    /** The previous location of the instance.							*/
    FVector OldLocation;
    /** The bounding box for the particles.								*/
    //FBox ParticleBoundingBox;
    /** The BurstFire information.										*/
    TArray<struct FLODBurstFired> BurstFired;
    /** The number of loops completed by the instance.					*/
    int32 LoopCount;
    /** Flag indicating if the render data is dirty.					*/
    int32 IsRenderDataDirty;
    /** The current duration fo the emitter instance.					*/
    float EmitterDuration;
    /** The emitter duration at each LOD level for the instance.		*/
    TArray<float> EmitterDurations;
    /** The emitter's delay for the current loop		*/
    float CurrentDelay;

    /** The number of triangles to render								*/
    int32	TrianglesToRender;
    int32 MaxVertexIndex;

    /** The material to render this instance with.						*/
    UMaterial* CurrentMaterial;

    /** Position offset for each particle. Will be reset to zero at the end of the tick	*/
    FVector PositionOffsetThisTick;

    /** The PivotOffset applied to the vertex positions 			*/
    FVector2D PivotOffset;

    TArray<class UPointLightComponent*> HighQualityLights;

    /** Constructor	*/
    FParticleEmitterInstance();

    /** Destructor	*/
    virtual ~FParticleEmitterInstance();

    virtual void InitParameters(UParticleEmitter* InTemplate, UParticleSystemComponent* InComponent);
    virtual void Init();

    /** @return The world that the component that owns this instance is in */
    UWorld* GetWorld() const;

    /**
     * Ensures enough memory is allocated for the requested number of particles.
     *
     * @param NewMaxActiveParticles		The number of particles for which memory must be allocated.
     * @param bSetMaxActiveCount		If true, update the peak active particles for this LOD.
     * @returns bool					true if memory is allocated for at least NewMaxActiveParticles.
     */
    virtual bool Resize(int32 NewMaxActiveParticles, bool bSetMaxActiveCount = true);

    virtual void Tick(float DeltaTime, bool bSuppressSpawning);
    void CheckEmitterFinished();

    /** Advances the bursts as though they were fired with out actually firing them. */
    void FakeBursts();

    /**
     *	Tick sub-function that handles EmitterTime setup, looping, etc.
     *
     *	@param	DeltaTime			The current time slice
     *	@param	CurrentLODLevel		The current LOD level for the instance
     *
     *	@return	float				The EmitterDelay
     */
    virtual float Tick_EmitterTimeSetup(float DeltaTime, UParticleLODLevel* CurrentLODLevel);
    /**
     *	Tick sub-function that handles spawning of particles
     *
     *	@param	DeltaTime			The current time slice
     *	@param	CurrentLODLevel		The current LOD level for the instance
     *	@param	bSuppressSpawning	true if spawning has been suppressed on the owning particle system component
     *	@param	bFirstTime			true if this is the first time the instance has been ticked
     *
     *	@return	float				The SpawnFraction remaining
     */
    virtual float Tick_SpawnParticles(float DeltaTime, UParticleLODLevel* CurrentLODLevel, bool bSuppressSpawning, bool bFirstTime);
    /**
     *	Tick sub-function that handles module updates
     *
     *	@param	DeltaTime			The current time slice
     *	@param	CurrentLODLevel		The current LOD level for the instance
     */
    virtual void Tick_ModuleUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel);
    /**
     *	Tick sub-function that handles module post updates
     *
     *	@param	DeltaTime			The current time slice
     *	@param	CurrentLODLevel		The current LOD level for the instance
     */
    virtual void Tick_ModulePostUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel);
    /**
     *	Tick sub-function that handles module FINAL updates
     *
     *	@param	DeltaTime			The current time slice
     *	@param	CurrentLODLevel		The current LOD level for the instance
     */
    virtual void Tick_ModuleFinalUpdate(float DeltaTime, UParticleLODLevel* CurrentLODLevel);

    /**
     *	Set the LOD to the given index
     *
     *	@param	InLODIndex			The index of the LOD to set as current
     *	@param	bInFullyProcess		If true, process burst lists, etc.
     */
    virtual void SetCurrentLODIndex(int32 InLODIndex, bool bInFullyProcess);

    virtual void Rewind();
    //virtual FBox GetBoundingBox();
    virtual void UpdateBoundingBox(float DeltaTime);
    virtual void ForceUpdateBoundingBox();
    virtual uint32 RequiredBytes();
    /** Get offset for particle payload data for a particular module */
    uint32 GetModuleDataOffset(UParticleModule* Module);
    /** Get pointer to emitter instance payload data for a particular module */
    uint8* GetModuleInstanceData(UParticleModule* Module);
    /** Get pointer to emitter instance random seed payload data for a particular module */
    FParticleRandomSeedInstancePayload* GetModuleRandomSeedInstanceData(UParticleModule* Module);
    virtual uint8* GetTypeDataModuleInstanceData();
    virtual uint32 CalculateParticleStride(uint32 ParticleSize);
    virtual void ResetBurstList();
    virtual float GetCurrentBurstRateOffset(float& DeltaTime, int32& Burst);
    virtual void ResetParticleParameters(float DeltaTime);
    /*void CalculateOrbitOffset(FOrbitChainModuleInstancePayload& Payload,
        FVector& AccumOffset, FVector& AccumRotation, FVector& AccumRotationRate,
        float DeltaTime, FVector& Result, FMatrix& RotationMat);*/
    //virtual void UpdateOrbitData(float DeltaTime);
    virtual void ParticlePrefetch();

    /**
     *	Spawn particles for this emitter instance
     *	@param	DeltaTime		The time slice to spawn over
     *	@return	float			The leftover fraction of spawning
     */
    virtual float Spawn(float DeltaTime);

    /**
     * Spawn the indicated number of particles.
     *
     * @param Count The number of particles to spawn.
     * @param StartTime			The local emitter time at which to begin spawning particles.
     * @param Increment			The time delta between spawned particles.
     * @param InitialLocation	The initial location of spawned particles.
     * @param InitialVelocity	The initial velocity of spawned particles.
     * @param EventPayload		Event generator payload if events should be triggered.
     */
    void SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity, struct FParticleEventInstancePayload* EventPayload);

    /**
     *	Spawn/burst the given particles...
     *
     *	@param	DeltaTime		The time slice to spawn over.
     *	@param	InSpawnCount	The number of particles to forcibly spawn.
     *	@param	InBurstCount	The number of particles to forcibly burst.
     *	@param	InLocation		The location to spawn at.
     *	@param	InVelocity		OPTIONAL velocity to have the particle inherit.
     *
     */
    virtual void ForceSpawn(float DeltaTime, int32 InSpawnCount, int32 InBurstCount, FVector& InLocation, FVector& InVelocity);
    void CheckSpawnCount(int32 InNewCount, int32 InMaxCount);

    /**
     * Handle any pre-spawning actions required for particles
     *
     * @param Particle			The particle being spawned.
     * @param InitialLocation	The initial location of the particle.
     * @param InitialVelocity	The initial velocity of the particle.
     */
    virtual void PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity);

    /**
     * Handle any post-spawning actions required by the instance
     *
     * @param	Particle					The particle that was spawned
     * @param	InterpolationPercentage		The percentage of the time slice it was spawned at
     * @param	SpawnTIme					The time it was spawned at
     */
    virtual void PostSpawn(FBaseParticle* Particle, float InterpolationPercentage, float SpawnTime);

    virtual bool HasCompleted();
    virtual void KillParticles();
    /**
     *	Kill the particle at the given instance
     *
     *	@param	Index		The index of the particle to kill.
     */
    virtual void KillParticle(int32 Index);

    /**
     *	Force kill all particles in the emitter.
     *
     *	@param	bFireEvents		If true, fire events for the particles being killed.
     */
    virtual void KillParticlesForced(bool bFireEvents = false);

    /** Set the HaltSpawning flag */
    virtual void SetHaltSpawning(bool bInHaltSpawning)
    {
        bHaltSpawning = bInHaltSpawning;
    }

    /** Set the bHaltSpawningExternal flag */
    virtual void SetHaltSpawningExternal(bool bInHaltSpawning)
    {
        bHaltSpawningExternal = bInHaltSpawning;
    }

    FORCEINLINE void SetFakeBurstWhenSpawningSupressed(bool bInFakeBurstsWhenSpawningSupressed)
    {
        bFakeBurstsWhenSpawningSupressed = bInFakeBurstsWhenSpawningSupressed;
    }

    /** Get the offset of the orbit payload. */
    int32 GetOrbitPayloadOffset();

    /** Get the position of the particle taking orbit in to account. */
    FVector GetParticleLocationWithOrbitOffset(FBaseParticle* Particle);

    virtual FBaseParticle* GetParticle(int32 Index);
    /**
     *	Get the physical index of the particle at the given index
     *	(ie, the contents of ParticleIndices[InIndex])
     *
     *	@param	InIndex			The index of the particle of interest
     *
     *	@return	int32				The direct index of the particle
     */
    FORCEINLINE int32 GetParticleDirectIndex(int32 InIndex)
    {
        if (InIndex < MaxActiveParticles)
        {
            return ParticleIndices[InIndex];
        }
        return -1;
    }
    /**
     *	Get the particle at the given direct index
     *
     *	@param	InDirectIndex		The direct index of the particle of interest
     *
     *	@return	FBaseParticle*		The particle, if valid index was given; NULL otherwise
     */
    virtual FBaseParticle* GetParticleDirect(int32 InDirectIndex);

    /**
     *	Calculates the emitter duration for the instance.
     */
    void SetupEmitterDuration();

    /**
     * Returns whether the system has any active particles.
     *
     * @return true if there are active particles, false otherwise.
     */
    bool HasActiveParticles()
    {
        return ActiveParticles > 0;
    }

    /**
     *	Checks some common values for GetDynamicData validity
     *
     *	@return	bool		true if GetDynamicData should continue, false if it should return NULL
     */
    virtual bool IsDynamicDataRequired(UParticleLODLevel* CurrentLODLevel);

    /**
     *	Retrieves the dynamic data for the emitter
     */
   /* virtual FDynamicEmitterDataBase* GetDynamicData(bool bSelected, ERHIFeatureLevel::Type InFeatureLevel)
    {
        return NULL;
    }*/

    /**
     *	Retrieves replay data for the emitter
     *
     *	@return	The replay data, or NULL on failure
     */
    virtual FDynamicEmitterReplayDataBase* GetReplayData()
    {
        return NULL;
    }

    /**
     *	Retrieve the allocated size of this instance.
     *
     *	@param	OutNum			The size of this instance
     *	@param	OutMax			The maximum size of this instance
     */
    virtual void GetAllocatedSize(int32& OutNum, int32& OutMax)
    {
        OutNum = 0;
        OutMax = 0;
    }

    /** Returns resource size, similar to UObject function */
   /* virtual void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize)
    {
    }*/

    /**
     *	Process received events.
     */
    virtual void ProcessParticleEvents(float DeltaTime, bool bSuppressSpawning);

    /**
     *	Called when the particle system is deactivating...
     */
    virtual void OnDeactivateSystem()
    {
    }

    /**
     * Returns the offset to the mesh rotation payload, if any.
     */
    virtual int32 GetMeshRotationOffset() const
    {
        return 0;
    }

    /**
     * Returns true if mesh rotation is active.
     */
    virtual bool IsMeshRotationActive() const
    {
        return false;
    }

    /**
     * Sets the materials with which mesh particles should be rendered.
     * @param InMaterials - The materials.
     */
    virtual void SetMeshMaterials(const TArray<UMaterial*>& InMaterials)
    {
    }

    /**
     * Gathers material relevance flags for this emitter instance.
     * @param OutMaterialRelevance - Pointer to where material relevance flags will be stored.
     * @param LODLevel - The LOD level for which to compute material relevance flags.
     */
    //virtual void GatherMaterialRelevance(FMaterialRelevance* OutMaterialRelevance, const UParticleLODLevel* LODLevel, ERHIFeatureLevel::Type InFeatureLevel) const;

    /**
     * When an emitter is killed, this will check other emitters and clean up anything pointing to this one
     */
    virtual void OnEmitterInstanceKilled(FParticleEmitterInstance* Instance)
    {

    }

    // Beam interface
    /*virtual void SetBeamEndPoint(FVector NewEndPoint) {};
    virtual void SetBeamSourcePoint(FVector NewSourcePoint, int32 SourceIndex) {};
    virtual void SetBeamSourceTangent(FVector NewTangentPoint, int32 SourceIndex) {};
    virtual void SetBeamSourceStrength(float NewSourceStrength, int32 SourceIndex) {};
    virtual void SetBeamTargetPoint(FVector NewTargetPoint, int32 TargetIndex) {};
    virtual void SetBeamTargetTangent(FVector NewTangentPoint, int32 TargetIndex) {};
    virtual void SetBeamTargetStrength(float NewTargetStrength, int32 TargetIndex) {};*/

    //Beam get interface
    /*virtual bool GetBeamEndPoint(FVector& OutEndPoint) const { return false; }
    virtual bool GetBeamSourcePoint(int32 SourceIndex, FVector& OutSourcePoint) const { return false; }
    virtual bool GetBeamSourceTangent(int32 SourceIndex, FVector& OutTangentPoint) const { return false; }
    virtual bool GetBeamSourceStrength(int32 SourceIndex, float& OutSourceStrength) const { return false; }
    virtual bool GetBeamTargetPoint(int32 TargetIndex, FVector& OutTargetPoint) const { return false; }
    virtual bool GetBeamTargetTangent(int32 TargetIndex, FVector& OutTangentPoint) const { return false; }
    virtual bool GetBeamTargetStrength(int32 TargetIndex, float& OutTargetStrength) const { return false; }*/

    // Called on world origin changes
    virtual void ApplyWorldOffset(FVector InOffset, bool bWorldShift);

    /**
    Ticks the emitter's material overrides.
    @return True if there were material overrides. Otherwise revert to default behaviour.
    */
    virtual void Tick_MaterialOverrides(int32 EmitterIndex);

    /**
    * True if this emitter emits in local space
    */
    bool UseLocalSpace();

    /**
    * returns the screen alignment and scale of the component.
    */
    void GetScreenAlignmentAndScale(int32& OutScreenAlign, FVector& OutScale);

protected:

    /**
     * Captures dynamic replay data for this particle system.
     *
     * @param	OutData		[Out] Data will be copied here
     *
     * @return Returns true if successful
     */
    virtual bool FillReplayData(FDynamicEmitterReplayDataBase& OutData);

    /**
     * Updates all internal transforms.
     */
    void UpdateTransforms();

    /**
    * Retrieves the current LOD level and asserts that it is valid.
    */
    class UParticleLODLevel* GetCurrentLODLevelChecked();

    /**
     * Get the current material to render with.
     */
    UMaterial* GetCurrentMaterial();


    /**
     * Fixup particle indices to only have valid entries.
     */
    void FixupParticleIndices();

};

struct FParticleEmitterBuildInfo
{
    /** The required module. */
    class UParticleModuleRequired* RequiredModule;
    /** The spawn module. */
    class UParticleModuleSpawn* SpawnModule;
    /** The spawn-per-unit module. */
    class UParticleModuleSpawnPerUnit* SpawnPerUnitModule;
    /** List of spawn modules that need to be invoked at runtime. */
    TArray<class UParticleModule*> SpawnModules;

    ///** The accumulated orbit offset. */
    //FComposableVectorDistribution OrbitOffset;
    ///** The accumulated orbit initial rotation. */
    //FComposableVectorDistribution OrbitInitialRotation;
    ///** The accumulated orbit rotation rate. */
    //FComposableVectorDistribution OrbitRotationRate;

    ///** The color scale of a particle over time. */
    //FComposableVectorDistribution ColorScale;
    ///** The alpha scale of a particle over time. */
    //FComposableFloatDistribution AlphaScale;

    ///** An additional color scale for allowing parameters to be used for ColorOverLife modules. */
    //FRawDistributionVector DynamicColor;
    ///** An additional alpha scale for allowing parameters to be used for ColorOverLife modules. */
    //FRawDistributionFloat DynamicAlpha;

    ///** An additional color scale for allowing parameters to be used for ColorScaleOverLife modules. */
    //FRawDistributionVector DynamicColorScale;
    ///** An additional alpha scale for allowing parameters to be used for ColorScaleOverLife modules. */
    //FRawDistributionFloat DynamicAlphaScale;

    ///** How to scale a particle's size over time. */
    //FComposableVectorDistribution SizeScale;
    /** The maximum size of a particle. */
    FVector2D MaxSize;
    /** How much to scale a particle's size based on its speed. */
    FVector2D SizeScaleBySpeed;
    /** The maximum amount by which to scale a particle based on its speed. */
    FVector2D MaxSizeScaleBySpeed;

    /** The sub-image index over the particle's life time. */
    //FComposableFloatDistribution SubImageIndex;

    ///** Drag coefficient. */
    //FComposableFloatDistribution DragCoefficient;
    ///** Drag scale over life. */
    //FComposableFloatDistribution DragScale;

    /** Enable collision? */
    bool bEnableCollision;
    /** How particles respond to collision. */
    uint8 CollisionResponse;
    uint8 CollisionMode;
    /** Radius scale applied to friction. */
    float CollisionRadiusScale;
    /** Bias applied to the collision radius. */
    float CollisionRadiusBias;
    /** Factor reflection spreading cone when colliding. */
    float CollisionRandomSpread;
    /** Random distribution across the reflection spreading cone when colliding. */
    float CollisionRandomDistribution;
    /** Friction. */
    float Friction;
    /** Collision damping factor. */
    //FComposableFloatDistribution Resilience;
    /** Collision damping factor scale over life. */
    //FComposableFloatDistribution ResilienceScaleOverLife;

    /** Location of a point source attractor. */
    FVector PointAttractorPosition;
    /** Radius of the point source attractor. */
    float PointAttractorRadius;
    /** Strength of the point attractor. */
    //FComposableFloatDistribution PointAttractorStrength;

    /** The per-particle vector field scale. */
    //FComposableFloatDistribution VectorFieldScale;
    /** The per-particle vector field scale-over-life. */
    //FComposableFloatDistribution VectorFieldScaleOverLife;
    /** Global vector field scale. */
    float GlobalVectorFieldScale;
    /** Global vector field tightness. */
    float GlobalVectorFieldTightness;

    /** Local vector field. */
    class UVectorField* LocalVectorField;
    /** Local vector field transform. */
    FTransform LocalVectorFieldTransform;
    /** Local vector field intensity. */
    float LocalVectorFieldIntensity;
    /** Tightness tweak for local vector fields. */
    float LocalVectorFieldTightness;
    /** Minimum initial rotation applied to local vector fields. */
    FVector LocalVectorFieldMinInitialRotation;
    /** Maximum initial rotation applied to local vector fields. */
    FVector LocalVectorFieldMaxInitialRotation;
    /** Local vector field rotation rate. */
    FVector LocalVectorFieldRotationRate;

    /** Constant acceleration to apply to particles. */
    FVector ConstantAcceleration;

    /** The maximum lifetime of any particle that will spawn. */
    float MaxLifetime;
    /** The maximum rotation rate of particles. */
    float MaxRotationRate;
    /** The estimated maximum number of particles for this emitter. */
    int32 EstimatedMaxActiveParticleCount;

    int32 ScreenAlignment;

    /** An offset in UV space for the positioning of a sprites vertices. */
    FVector2D PivotOffset;

    /** If true, local vector fields ignore the component transform. */
    uint32 bLocalVectorFieldIgnoreComponentTransform : 1;
    /** Tile vector field in x axis? */
    uint32 bLocalVectorFieldTileX : 1;
    /** Tile vector field in y axis? */
    uint32 bLocalVectorFieldTileY : 1;
    /** Tile vector field in z axis? */
    uint32 bLocalVectorFieldTileZ : 1;
    /** Use fix delta time in the simulation? */
    uint32 bLocalVectorFieldUseFixDT : 1;

    uint32 bUseVelocityForMotionBlur : 1;

    /** Particle alignment overrides */
    uint32 bRemoveHMDRoll : 1;
    float MinFacingCameraBlendDistance;
    float MaxFacingCameraBlendDistance;

    /** Default constructor. */
    FParticleEmitterBuildInfo();
};
