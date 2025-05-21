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

// Special module indices...
#define INDEX_TYPEDATAMODULE	(INDEX_NONE - 1)
#define INDEX_REQUIREDMODULE	(INDEX_NONE - 2)
#define INDEX_SPAWNMODULE		(INDEX_NONE - 3)

/*-----------------------------------------------------------------------------
    FBaseParticle
-----------------------------------------------------------------------------*/

struct FBaseParticle
{
    // 24 bytes
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

    // 8 bytes
    float			RelativeTime;			// Relative time, range is 0 (==spawn) to 1 (==death)
    float			OneOverMaxLifetime;		// Reciprocal of lifetime
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

FORCEINLINE FVector GetParticleBaseSize(const FBaseParticle& Particle, bool bKeepFlipScale = false)
{
    return bKeepFlipScale ? Particle.BaseSize : FVector(FMath::Abs(Particle.BaseSize.X), FMath::Abs(Particle.BaseSize.Y), FMath::Abs(Particle.BaseSize.Z));
};
