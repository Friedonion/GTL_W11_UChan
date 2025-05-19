#pragma once

#include "Core/Math/Vector.h"
#include "Core/Math/Color.h"
#include "Core/Math/RandomStream.h"

/*-----------------------------------------------------------------------------
    Helper macros.
-----------------------------------------------------------------------------*/
//	Macro fun.
#define _PARTICLES_USE_PREFETCH_
#if defined(_PARTICLES_USE_PREFETCH_)
#define	PARTICLE_PREFETCH(Index)					FPlatformMisc::Prefetch( ParticleData, ParticleStride * ParticleIndices[Index] )
#define PARTICLE_INSTANCE_PREFETCH(Instance, Index)	FPlatformMisc::Prefetch( Instance->ParticleData, Instance->ParticleStride * Instance->ParticleIndices[Index] )
#define	PARTICLE_OWNER_PREFETCH(Index)				FPlatformMisc::Prefetch( Owner->ParticleData, Owner->ParticleStride * Owner->ParticleIndices[Index] )
#else	//#if defined(_PARTICLES_USE_PREFETCH_)
#define	PARTICLE_PREFETCH(Index)					
#define	PARTICLE_INSTANCE_PREFETCH(Instance, Index)	
#define	PARTICLE_OWNER_PREFETCH(Index)				
#endif	//#if defined(_PARTICLES_USE_PREFETCH_)

#define DECLARE_PARTICLE(Name,Address)		\
	FBaseParticle& Name = *((FBaseParticle*) (Address));

#define DECLARE_PARTICLE_CONST(Name,Address)		\
	const FBaseParticle& Name = *((const FBaseParticle*) (Address));

#define DECLARE_PARTICLE_PTR(Name,Address)		\
	FBaseParticle* Name = (FBaseParticle*) (Address);

#define BEGIN_UPDATE_LOOP																								\
	{																													\
		if((Owner != NULL) && (Owner->Component != NULL));															\
		int32&			ActiveParticles = Owner->ActiveParticles;														\
		uint32			CurrentOffset	= Offset;																		\
		const uint8*		ParticleData	= Owner->ParticleData;															\
		const uint32		ParticleStride	= Owner->ParticleStride;														\
		uint16*			ParticleIndices	= Owner->ParticleIndices;														\
		for(int32 i=ActiveParticles-1; i>=0; i--)																			\
		{																												\
			const int32	CurrentIndex	= ParticleIndices[i];															\
			const uint8* ParticleBase	= ParticleData + CurrentIndex * ParticleStride;									\
			FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);												\
			if ((Particle.Flags & STATE_Particle_Freeze) == 0)															\
			{																											\

#define END_UPDATE_LOOP																									\
			}																											\
			CurrentOffset				= Offset;																		\
		}																												\
	}

#define CONTINUE_UPDATE_LOOP																							\
		CurrentOffset = Offset;																							\
		continue;

#define SPAWN_INIT																										\
	if((Owner != NULL) && (Owner->Component != NULL));																\
	const int32		ActiveParticles	= Owner->ActiveParticles;															\
	const uint32		ParticleStride	= Owner->ParticleStride;															\
	uint32			CurrentOffset	= Offset;																			\
	FBaseParticle&	Particle		= *(ParticleBase);

#define PARTICLE_ELEMENT(Type,Name)																						\
	Type& Name = *((Type*)((uint8*)ParticleBase + CurrentOffset));																\
	CurrentOffset += sizeof(Type);

#define KILL_CURRENT_PARTICLE																							\
	{																													\
		ParticleIndices[i]					= ParticleIndices[ActiveParticles-1];										\
		ParticleIndices[ActiveParticles-1]	= CurrentIndex;																\
		ActiveParticles--;																								\
	}


class UMaterial;
/*-----------------------------------------------------------------------------
    Particle State Flags
-----------------------------------------------------------------------------*/
enum EParticleStates
{
    /** Ignore updates to the particle						*/
    STATE_Particle_JustSpawned = 0x02000000,
    /** Ignore updates to the particle						*/
    STATE_Particle_Freeze = 0x04000000,
    /** Ignore collision updates to the particle			*/
    STATE_Particle_IgnoreCollisions = 0x08000000,
    /**	Stop translations of the particle					*/
    STATE_Particle_FreezeTranslation = 0x10000000,
    /**	Stop rotations of the particle						*/
    STATE_Particle_FreezeRotation = 0x20000000,
    /** Combination for a single check of 'ignore' flags	*/
    STATE_Particle_CollisionIgnoreCheck = STATE_Particle_Freeze | STATE_Particle_IgnoreCollisions | STATE_Particle_FreezeTranslation | STATE_Particle_FreezeRotation,
    /** Delay collision updates to the particle				*/
    STATE_Particle_DelayCollisions = 0x40000000,
    /** Flag indicating the particle has had at least one collision	*/
    STATE_Particle_CollisionHasOccurred = 0x80000000,
    /** State mask. */
    STATE_Mask = 0xFE000000,
    /** Counter mask. */
    STATE_CounterMask = (~STATE_Mask)
};

/*-----------------------------------------------------------------------------
    Particle Dynamic Data
-----------------------------------------------------------------------------*/

/**
 * Dynamic particle emitter types
 *
 * NOTE: These are serialized out for particle replay data, so be sure to update all appropriate
 *    when changing anything here.
 */
enum EDynamicEmitterType
{
    DET_Unknown = 0,
    DET_Sprite,
    DET_Mesh,
    DET_Beam2,
    DET_Ribbon,
    DET_AnimTrail,
    DET_Custom
};

/*-----------------------------------------------------------------------------
    Helper functions.
-----------------------------------------------------------------------------*/

inline void Particle_SetColorFromVector(const FVector& InColorVec, const float InAlpha, FLinearColor& OutColor)
{
    OutColor.R = InColorVec.X;
    OutColor.G = InColorVec.Y;
    OutColor.B = InColorVec.Z;
    OutColor.A = InAlpha;
}

/*-----------------------------------------------------------------------------
    FBaseParticle
-----------------------------------------------------------------------------*/

struct FBaseParticle
{
    // 48 bytes
    FVector		OldLocation;			// Last frame's location, used for collision
    FVector		Location;				// Current location

    // 16 bytes
    FVector		BaseVelocity;			// Velocity = BaseVelocity at the start of each frame.
    float			Rotation;				// Rotation of particle (in Radians)

    // 16 bytes
    FVector		Velocity;				// Current velocity, gets reset to BaseVelocity each frame to allow 
    float			BaseRotationRate;		// Initial angular velocity of particle (in Radians per second)

    // 16 bytes
    FVector		BaseSize;				// Size = BaseSize at the start of each frame
    float			RotationRate;			// Current rotation rate, gets reset to BaseRotationRate each frame

    // 16 bytes
    FVector		Size;					// Current size, gets reset to BaseSize each frame
    int32			Flags;					// Flags indicating various particle states

    // 16 bytes
    FLinearColor	Color;					// Current color of particle.

    // 16 bytes
    FLinearColor	BaseColor;				// Base color of the particle

    // 16 bytes
    float			RelativeTime;			// Relative time, range is 0 (==spawn) to 1 (==death)
    float			OneOverMaxLifetime;		// Reciprocal of lifetime
    float			Placeholder0;
    float			Placeholder1;
};

struct FParticleRandomSeedInstancePayload
{
    FRandomStream RandomStream;
};

struct FParticleDataContainer
{
    int32 MemBlockSize;
    int32 ParticleDataNumBytes;
    int32 ParticleIndicesNumShorts;
    uint8* ParticleData; // this is also the memory block we allocated
    uint16* ParticleIndices; // not allocated, this is at the end of the memory block

    FParticleDataContainer()
        : MemBlockSize(0)
        , ParticleDataNumBytes(0)
        , ParticleIndicesNumShorts(0)
        , ParticleData(nullptr)
        , ParticleIndices(nullptr)
    {
    }
    ~FParticleDataContainer()
    {
        Free();
    }
    void Alloc(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts);
    void Free();
};

struct FDynamicEmitterReplayDataBase
{
    /**	The type of emitter. */
    EDynamicEmitterType	eEmitterType;

    /**	The number of particles currently active in this emitter. */
    int32 ActiveParticleCount;

    int32 ParticleStride;
    FParticleDataContainer DataContainer;

    FVector Scale;

    /** Whether this emitter requires sorting as specified by artist.	*/
    int32 SortMode;

    /** MacroUV (override) data **/
    //FMacroUVOverride MacroUVOverride;

    /** Constructor */
    FDynamicEmitterReplayDataBase()
        : eEmitterType(DET_Unknown),
        ActiveParticleCount(0),
        ParticleStride(0),
        Scale(FVector(1.0f)),
        SortMode(0)	// Default to PSORTMODE_None		  
    {
    }

    virtual ~FDynamicEmitterReplayDataBase()
    {
    }

    /** Serialization */
    virtual void Serialize(FArchive& Ar)
    {
        /*int32 EmitterTypeAsInt = eEmitterType;
        Ar << EmitterTypeAsInt;
        eEmitterType = static_cast<EDynamicEmitterType>(EmitterTypeAsInt);

        Ar << ActiveParticleCount;
        Ar << ParticleStride;

        TArray<uint8> ParticleData;
        TArray<uint16> ParticleIndices;

        if (!Ar.IsLoading() && !Ar.IsObjectReferenceCollector())
        {
            if (DataContainer.ParticleDataNumBytes)
            {
                ParticleData.AddUninitialized(DataContainer.ParticleDataNumBytes);
                FMemory::Memcpy(ParticleData.GetData(), DataContainer.ParticleData, DataContainer.ParticleDataNumBytes);
            }
            if (DataContainer.ParticleIndicesNumShorts)
            {
                ParticleIndices.AddUninitialized(DataContainer.ParticleIndicesNumShorts);
                FMemory::Memcpy(ParticleIndices.GetData(), DataContainer.ParticleIndices, DataContainer.ParticleIndicesNumShorts * sizeof(uint16));
            }
        }

        Ar << ParticleData;
        Ar << ParticleIndices;

        if (Ar.IsLoading())
        {
            DataContainer.Free();
            if (ParticleData.Num())
            {
                DataContainer.Alloc(ParticleData.Num(), ParticleIndices.Num());
                FMemory::Memcpy(DataContainer.ParticleData, ParticleData.GetData(), DataContainer.ParticleDataNumBytes);
                if (DataContainer.ParticleIndicesNumShorts)
                {
                    FMemory::Memcpy(DataContainer.ParticleIndices, ParticleIndices.GetData(), DataContainer.ParticleIndicesNumShorts * sizeof(uint16));
                }
            }
            else
            {
                check(!ParticleIndices.Num());
            }
        }

        Ar << Scale;
        Ar << SortMode;
        Ar << MacroUVOverride;*/
    }
};

// 일단 여기 선언
enum EParticleAxisLock : int
{
    /** No locking to an axis...							*/
    EPAL_NONE, 
    /** Lock the sprite facing towards the positive X-axis	*/
    EPAL_X, 
    /** Lock the sprite facing towards the positive Y-axis	*/
    EPAL_Y, 
    /** Lock the sprite facing towards the positive Z-axis	*/
    EPAL_Z, 
    /** Lock the sprite facing towards the negative X-axis	*/
    EPAL_NEGATIVE_X, 
    /** Lock the sprite facing towards the negative Y-axis	*/
    EPAL_NEGATIVE_Y, 
    /** Lock the sprite facing towards the negative Z-axis	*/
    EPAL_NEGATIVE_Z, 
    /** Lock the sprite rotation on the X-axis				*/
    EPAL_ROTATE_X,
    /** Lock the sprite rotation on the Y-axis				*/
    EPAL_ROTATE_Y,
    /** Lock the sprite rotation on the Z-axis				*/
    EPAL_ROTATE_Z,
    EPAL_MAX,
};

struct FParticleOrder
{
    int32 ParticleIndex;

    union
    {
        float Z;
        uint32 C;
    };

    FParticleOrder(int32 InParticleIndex, float InZ) :
        ParticleIndex(InParticleIndex),
        Z(InZ)
    {
    }

    FParticleOrder(int32 InParticleIndex, uint32 InC) :
        ParticleIndex(InParticleIndex),
        C(InC)
    {
    }
};

struct FDynamicEmitterDataBase
{
    FDynamicEmitterDataBase(const class UParticleModuleRequired* RequiredModule);

    virtual ~FDynamicEmitterDataBase()
    {
    }

    /** Custom new/delete with recycling */
    //void* operator new(size_t Size);
    //void operator delete(void* RawMemory, size_t Size);

    // Proxy관련 내용 전부 제거 하도록 함, 원 함수 선언만 표시 목적을 위해 남겨 둠
    // DirectX로 직접 데이터를 전달하여 그리도록 하는 새로운 메서드를 만드는 것이 현재 단계에서 나을 듯
    //virtual void UpdateRenderThreadResourcesEmitter(const FParticleSystemSceneProxy* InOwnerProxy) {}
    //virtual void ReleaseRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy) {}
    //virtual void GetDynamicMeshElementsEmitter(const FParticleSystemSceneProxy* Proxy, const FSceneView* View, const FSceneViewFamily& ViewFamily, int32 ViewIndex, FMeshElementCollector& Collector) const {}
    //virtual const FMaterialRenderProxy* GetMaterialRenderProxy() = 0;
    //virtual void GatherSimpleLights(const FParticleSystemSceneProxy* Proxy, const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const {}

    /** Returns the source data for this particle system */
    virtual const FDynamicEmitterReplayDataBase& GetSource() const = 0;

    /** Returns the current macro uv override. Specialized by FGPUSpriteDynamicEmitterData  */
    //virtual const FMacroUVOverride& GetMacroUVOverride() const { return GetSource().MacroUVOverride; }

    /** Stat id of this object, 0 if nobody asked for it yet */
    //mutable TStatId StatID;
    /** true if this emitter is currently selected */
    uint32	bSelected : 1;
    /** true if this emitter has valid rendering data */
    uint32	bValid : 1;

    int32  EmitterIndex;
};

/** Source data base class for Sprite emitters */
struct FDynamicSpriteEmitterReplayDataBase
    : public FDynamicEmitterReplayDataBase
{
    UMaterial* Material;
    struct FParticleRequiredModule *RequiredModule;
    FVector							NormalsSphereCenter;
    FVector							NormalsCylinderDirection;
    float							InvDeltaSeconds;
    FVector						LWCTile;
    int32							MaxDrawCount;
    int32							OrbitModuleOffset;
    int32							DynamicParameterDataOffset;
    int32							LightDataOffset;
    float							LightVolumetricScatteringIntensity;
    int32							CameraPayloadOffset;
    int32							SubUVDataOffset;
    int32							SubImages_Horizontal;
    int32							SubImages_Vertical;
    bool						bUseLocalSpace;
    bool						bLockAxis;
    uint8						ScreenAlignment;
    uint8						LockAxisFlag;
    uint8						EmitterRenderMode;
    uint8						EmitterNormalsMode;
    FVector2D					PivotOffset;
    bool						bUseVelocityForMotionBlur;
    bool						bRemoveHMDRoll;
    float						MinFacingCameraBlendDistance;
    float						MaxFacingCameraBlendDistance;

    /** Constructor */
    FDynamicSpriteEmitterReplayDataBase();
    ~FDynamicSpriteEmitterReplayDataBase();

    /** Serialization */
    virtual void Serialize(FArchive& Ar);
};

/** Base class for Sprite emitters and other emitter types that share similar features. */
struct FDynamicSpriteEmitterDataBase : public FDynamicEmitterDataBase
{
    FDynamicSpriteEmitterDataBase(const UParticleModuleRequired* RequiredModule) :
        FDynamicEmitterDataBase(RequiredModule),
        bUsesDynamicParameter(false)
    {
        MaterialResource = nullptr;
    }

    virtual ~FDynamicSpriteEmitterDataBase()
    {
    }

    const UMaterial* GetMaterialRenderProxy()
    {
        return MaterialResource;
    }

    /**
     *	Sort the given sprite particles
     *
     *	@param	SorceMode			The sort mode to utilize (EParticleSortMode)
     *	@param	bLocalSpace			true if the emitter is using local space
     *	@param	ParticleCount		The number of particles
     *	@param	ParticleData		The actual particle data
     *	@param	ParticleStride		The stride between entries in the ParticleData array
     *	@param	ParticleIndices		Indirect index list into ParticleData
     *	@param	View				The scene view being rendered
     *	@param	LocalToWorld		The local to world transform of the component rendering the emitter
     *	@param	ParticleOrder		The array to fill in with ordered indices
     */
    // SceneView 정의 필요
    /*void SortSpriteParticles(int32 SortMode, bool bLocalSpace,
        int32 ParticleCount, const uint8* ParticleData, int32 ParticleStride, const uint16* ParticleIndices,
        const FSceneView* View, const FMatrix& LocalToWorld, FParticleOrder* ParticleOrder) const;*/
    // 반투명 파티클들 렌더 순서 정렬
    void SortSpriteParticles(int32 SortMode, bool bLocalSpace,
        int32 ParticleCount, const uint8* ParticleData, int32 ParticleStride, const uint16* ParticleIndices,
        const FMatrix& LocalToWorld, FParticleOrder* ParticleOrder) const;

    /**
     *	Get the vertex stride for the dynamic rendering data
     */
    virtual int32 GetDynamicVertexStride() const
    {
        //checkf(0, TEXT("GetDynamicVertexStride MUST be overridden"));
        return 0;
    }

    /**
     *	Get the vertex stride for the dynamic parameter rendering data
     */
    virtual int32 GetDynamicParameterVertexStride() const
    {
        //checkf(0, TEXT("GetDynamicParameterVertexStride MUST be overridden"));
        return 0;
    }

    /**
     *	Get the source replay data for this emitter
     */
    virtual const FDynamicSpriteEmitterReplayDataBase* GetSourceData() const
    {
        //checkf(0, TEXT("GetSourceData MUST be overridden"));
        return NULL;
    }

    /**
     *	Gets the information required for allocating this emitters indices from the global index array.
     */
    virtual void GetIndexAllocInfo(int32& OutNumIndices, int32& OutStride) const
    {
        //checkf(0, TEXT("GetIndexAllocInfo is not valid for this class."));
    }

    /**
     *	Debug rendering
     *
     *	@param	Proxy		The primitive scene proxy for the emitter.
     *	@param	PDI			The primitive draw interface to render with
     *	@param	View		The scene view being rendered
     *	@param	bCrosses	If true, render Crosses at particle position; false, render points
     */
    //virtual void RenderDebug(const FParticleSystemSceneProxy* Proxy, FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bCrosses) const;

    //virtual void DoBufferFill(FAsyncBufferFillData& Me) const
    //{
    //    // Must be overridden if called
    //    check(0);
    //}

    /**
     *	Set up an buffer for async filling
     *
     *	@param	Proxy					The primitive scene proxy for the emitter.
     *	@param	InView					View for this buffer
     *	@param	InVertexCount			Count of verts for this buffer
     *	@param	InVertexSize			Stride of these verts, only used for verification
     *	@param	InDynamicParameterVertexStride	Stride of the dynamic parameter
     */
   /* void BuildViewFillData(
        const FParticleSystemSceneProxy* Proxy,
        const FSceneView* InView,
        int32 InVertexCount,
        int32 InVertexSize,
        int32 InDynamicParameterVertexSize,
        FGlobalDynamicIndexBuffer& DynamicIndexBuffer,
        FGlobalDynamicVertexBuffer& DynamicVertexBuffer,
        FGlobalDynamicVertexBufferAllocation& DynamicVertexAllocation,
        FGlobalDynamicIndexBufferAllocation& DynamicIndexAllocation,
        FGlobalDynamicVertexBufferAllocation* DynamicParameterAllocation,
        FAsyncBufferFillData& Data) const;*/

    /** The material render proxy for this emitter */
    const UMaterial* MaterialResource;
    /** true if the particle emitter utilizes the DynamicParameter module */
    uint32 bUsesDynamicParameter : 1;
};

/** Source data for Sprite emitters */
struct FDynamicSpriteEmitterReplayData
    : public FDynamicSpriteEmitterReplayDataBase
{
    /** Constructor */
    FDynamicSpriteEmitterReplayData()
    {
    }


    /** Serialization */
    virtual void Serialize(FArchive& Ar)
    {
        // Call parent implementation
        FDynamicSpriteEmitterReplayDataBase::Serialize(Ar);
    }

};

struct FParticleSpriteVertex
{
    /** The position of the particle. */
    FVector Position;
    /** The relative time of the particle. */
    float RelativeTime;
    /** The previous position of the particle. */
    FVector	OldPosition;
    /** Value that remains constant over the lifetime of a particle. */
    float ParticleId;
    /** The size of the particle. */
    FVector2D Size;
    /** The rotation of the particle. */
    float Rotation;
    /** The sub-image index for the particle. */
    float SubImageIndex;
    /** The color of the particle. */
    FLinearColor Color;
};

struct FParticleVertexDynamicParameter
{
    /** The dynamic parameter of the particle			*/
    float			DynamicValue[4];
};

/** Dynamic emitter data for sprite emitters */
struct FDynamicSpriteEmitterData : public FDynamicSpriteEmitterDataBase
{
    FDynamicSpriteEmitterData(const UParticleModuleRequired* RequiredModule) :
        FDynamicSpriteEmitterDataBase(RequiredModule)
    {
    }

    ~FDynamicSpriteEmitterData()
    {
    }

    /** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
    void Init(bool bInSelected);

    /**
     *	Get the vertex stride for the dynamic rendering data
     */
    virtual int32 GetDynamicVertexStride() const override
    {
        return sizeof(FParticleSpriteVertex);
    }

    /**
     *	Get the vertex stride for the dynamic parameter rendering data
     */
    virtual int32 GetDynamicParameterVertexStride() const override
    {
        return sizeof(FParticleVertexDynamicParameter);
    }

    /**
     *	Get the source replay data for this emitter
     */
    virtual const FDynamicSpriteEmitterReplayDataBase* GetSourceData() const override
    {
        return &Source;
    }

    /**
     *	Retrieve the vertex and (optional) index required to render this emitter.
     *	Render-thread only
     *
     *	@param	VertexData			The memory to fill the vertex data into
     *	@param	FillIndexData		The index data to fill in
     *	@param	ParticleOrder		The (optional) particle ordering to use
     *	@param	InCameraPosition	The position of the camera in world space.
     *	@param	InLocalToWorld		Transform from local to world space.
     *	@param	InstanceFactor		The factor to duplicate instances by.
     *
     *	@return	bool			true if successful, false if failed
     */
    bool GetVertexAndIndexData(void* VertexData, void* DynamicParameterVertexData, void* FillIndexData, FParticleOrder* ParticleOrder, const FVector& InCameraPosition, const FMatrix& InLocalToWorld, uint32 InstanceFactor) const;

    /** Gathers simple lights for this emitter. */
    //virtual void GatherSimpleLights(const FParticleSystemSceneProxy* Proxy, const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const override;

    //virtual void GetDynamicMeshElementsEmitter(const FParticleSystemSceneProxy* Proxy, const FSceneView* View, const FSceneViewFamily& ViewFamily, int32 ViewIndex, FMeshElementCollector& Collector) const override;

    /**
     *	Create the render thread resources for this emitter data
     *
     *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
     *
     *	@return	bool			true if successful, false if failed
     */
    //virtual void UpdateRenderThreadResourcesEmitter(const FParticleSystemSceneProxy* InOwnerProxy) override;

    /** Returns the source data for this particle system */
    virtual const FDynamicEmitterReplayDataBase& GetSource() const override
    {
        return Source;
    }

    /** The frame source data for this particle system.  This is everything needed to represent this
        this particle system frame.  It does not include any transient rendering thread data.  Also, for
        non-simulating 'replay' particle systems, this data may have come straight from disk! */
    FDynamicSpriteEmitterReplayData Source;

    /** Uniform parameters. Most fields are filled in when updates are sent to the rendering thread, some are per-view! */
    //FParticleSpriteUniformParameters UniformParameters;
};

/** Source data for Mesh emitters */
struct FDynamicMeshEmitterReplayData
    : public FDynamicSpriteEmitterReplayDataBase
{
    int32	SubUVInterpMethod;
    int32	SubUVDataOffset;
    int32	SubImages_Horizontal;
    int32	SubImages_Vertical;
    bool	bScaleUV;
    int32	MeshRotationOffset;
    int32	MeshMotionBlurOffset;
    uint8	MeshAlignment;
    bool	bMeshRotationActive;
    FVector	LockedAxis;

    /** Constructor */
    FDynamicMeshEmitterReplayData() :
        SubUVInterpMethod(0),
        SubUVDataOffset(0),
        SubImages_Horizontal(0),
        SubImages_Vertical(0),
        bScaleUV(false),
        MeshRotationOffset(0),
        MeshMotionBlurOffset(0),
        MeshAlignment(0),
        bMeshRotationActive(false),
        LockedAxis(1.0f, 0.0f, 0.0f)
    {
    }


    /** Serialization */
    virtual void Serialize(FArchive& Ar)
    {
        // Call parent implementation
        FDynamicSpriteEmitterReplayDataBase::Serialize(Ar);

        Ar << SubUVInterpMethod;
        Ar << SubUVDataOffset;
        Ar << SubImages_Horizontal;
        Ar << SubImages_Vertical;
        Ar << bScaleUV;
        Ar << MeshRotationOffset;
        Ar << MeshMotionBlurOffset;
        Ar << MeshAlignment;
        Ar << bMeshRotationActive;
        Ar << LockedAxis;
    }

};

// Mesh Emitter 일단 나중에 생각
/** Dynamic emitter data for Mesh emitters */
//struct FDynamicMeshEmitterData : public FDynamicSpriteEmitterDataBase
//{
//    FDynamicMeshEmitterData(const UParticleModuleRequired* RequiredModule);
//
//    virtual ~FDynamicMeshEmitterData();
//
//    uint32 GetMeshLODIndexFromProxy(const FParticleSystemSceneProxy* InOwnerProxy) const;
//    /** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
//    void Init(bool bInSelected,
//        const FParticleMeshEmitterInstance* InEmitterInstance,
//        UStaticMesh* InStaticMesh,
//        bool InUseStaticMeshLODs,
//        float InLODSizeScale,
//        ERHIFeatureLevel::Type InFeatureLevel);
//
//    /**
//     *	Create the render thread resources for this emitter data
//     *
//     *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
//     *
//     *	@return	bool			true if successful, false if failed
//     */
//    virtual void UpdateRenderThreadResourcesEmitter(const FParticleSystemSceneProxy* InOwnerProxy) override;
//
//    /**
//     *	Release the render thread resources for this emitter data
//     *
//     *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
//     *
//     *	@return	bool			true if successful, false if failed
//     */
//    virtual void ReleaseRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy) override;
//
//    virtual void GetDynamicMeshElementsEmitter(const FParticleSystemSceneProxy* Proxy, const FSceneView* View, const FSceneViewFamily& ViewFamily, int32 ViewIndex, FMeshElementCollector& Collector) const override;
//
//    /**
//     *	Retrieve the instance data required to render this emitter.
//     *	Render-thread only
//     *
//     *	@param	InstanceData            The memory to fill the vertex data into
//     *	@param	DynamicParameterData    The memory to fill the vertex dynamic parameter data into
//     *	@param	PrevTransformBuffer     The memory to fill the vertex prev transform data into. May be null
//     *	@param	Proxy                   The scene proxy for the particle system that owns this emitter
//     *	@param	View                    The scene view being rendered
//     *	@param	InstanceFactor			The factor to duplicate instances by
//     */
//    void GetInstanceData(void* InstanceData, void* DynamicParameterData, void* PrevTransformBuffer, const FParticleSystemSceneProxy* Proxy, const FSceneView* View, uint32 InstanceFactor) const;
//
//    /**
//     *	Helper function for retrieving the particle transform.
//     *
//     *	@param	InParticle					The particle being processed
//     *  @param	Proxy					    The scene proxy for the particle system that owns this emitter
//     *	@param	View						The scene view being rendered
//     *	@param	OutTransform				The InstanceToWorld transform matrix for the particle
//     */
//    void GetParticleTransform(const FBaseParticle& InParticle, const FParticleSystemSceneProxy* Proxy, const FSceneView* View, FMatrix& OutTransformMat) const;
//
//    void GetParticlePrevTransform(const FBaseParticle& InParticle, const FParticleSystemSceneProxy* Proxy, const FSceneView* View, FMatrix& OutTransformMat) const;
//
//    void CalculateParticleTransform(
//        const FMatrix& ProxyLocalToWorld,
//        const FVector& ParticleLocation,
//        float    ParticleRotation,
//        const FVector& ParticleVelocity,
//        const FVector& ParticleSize,
//        const FVector& ParticlePayloadInitialOrientation,
//        const FVector& ParticlePayloadRotation,
//        const FVector& ParticlePayloadCameraOffset,
//        const FVector& ParticlePayloadOrbitOffset,
//        const FVector& ViewOrigin,
//        const FVector& ViewDirection,
//        FMatrix& OutTransformMat
//    ) const;
//
//    /** Gathers simple lights for this emitter. */
//    virtual void GatherSimpleLights(const FParticleSystemSceneProxy* Proxy, const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const override;
//
//    /**
//     *	Get the vertex stride for the dynamic rendering data
//     */
//    virtual int32 GetDynamicVertexStride(ERHIFeatureLevel::Type /*InFeatureLevel*/) const override
//    {
//        return sizeof(FMeshParticleInstanceVertex);
//    }
//
//    virtual int32 GetDynamicParameterVertexStride() const override
//    {
//        return sizeof(FMeshParticleInstanceVertexDynamicParameter);
//    }
//
//    /**
//     *	Get the source replay data for this emitter
//     */
//    virtual const FDynamicSpriteEmitterReplayDataBase* GetSourceData() const override
//    {
//        return &Source;
//    }
//
//    /**
//     *	 Initialize this emitter's vertex factory with the vertex buffers from the mesh's rendering data.
//     */
//    void SetupVertexFactory(FRHICommandListBase& RHICmdList, FMeshParticleVertexFactory* InVertexFactory, const FStaticMeshLODResources& LODResources, uint32 LODIdx) const;
//
//    /** Returns the source data for this particle system */
//    virtual const FDynamicEmitterReplayDataBase& GetSource() const override
//    {
//        return Source;
//    }
//
//    /** The frame source data for this particle system.  This is everything needed to represent this
//        this particle system frame.  It does not include any transient rendering thread data.  Also, for
//        non-simulating 'replay' particle systems, this data may have come straight from disk! */
//    FDynamicMeshEmitterReplayData Source;
//
//    int32					LastFramePreRendered;
//
//    UStaticMesh* StaticMesh;
//    TArray<FMaterialRenderProxy*, TInlineAllocator<2>> MeshMaterials;
//
//    /** offset to FMeshTypeDataPayload */
//    uint32 MeshTypeDataOffset;
//
//    // 'orientation' items...
//    // These don't need to go into the replay data, as they are constant over the life of the emitter
//    /** If true, apply the 'pre-rotation' values to the mesh. */
//    uint32 bApplyPreRotation : 1;
//    /** If true, then use the locked axis setting supplied. Trumps locked axis module and/or TypeSpecific mesh settings. */
//    uint32 bUseMeshLockedAxis : 1;
//    /** If true, then use the camera facing options supplied. Trumps all other settings. */
//    uint32 bUseCameraFacing : 1;
//    /**
//     *	If true, apply 'sprite' particle rotation about the orientation axis (direction mesh is pointing).
//     *	If false, apply 'sprite' particle rotation about the camera facing axis.
//     */
//    uint32 bApplyParticleRotationAsSpin : 1;
//    /**
//    *	If true, all camera facing options will point the mesh against the camera's view direction rather than pointing at the cameras location.
//    *	If false, the camera facing will point to the cameras position as normal.
//    */
//    uint32 bFaceCameraDirectionRatherThanPosition : 1;
//    /** The EMeshCameraFacingOption setting to use if bUseCameraFacing is true. */
//    uint8 CameraFacingOption;
//
//    bool bUseStaticMeshLODs;
//    float LODSizeScale;
//    mutable int32 LastCalculatedMeshLOD;
//    const FParticleMeshEmitterInstance* EmitterInstance;
//};

FORCEINLINE FVector GetParticleBaseSize(const FBaseParticle& Particle, bool bKeepFlipScale = false)
{
    return bKeepFlipScale ? Particle.BaseSize : FVector(FMath::Abs(Particle.BaseSize.X), FMath::Abs(Particle.BaseSize.Y), FMath::Abs(Particle.BaseSize.Z));
};
