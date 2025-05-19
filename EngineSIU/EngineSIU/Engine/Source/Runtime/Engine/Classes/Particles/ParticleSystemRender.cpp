#include "ParticleHelper.h"
#include "Components/Material/Material.h"

// void FDynamicSpriteEmitterDataBase::SortSpriteParticles(int32 SortMode, bool bLocalSpace, 
// 	int32 ParticleCount, const uint8* ParticleData, int32 ParticleStride, const uint16* ParticleIndices,
// 	const FSceneView* View, const FMatrix& LocalToWorld, FParticleOrder* ParticleOrder) const
// {
// 	SCOPE_CYCLE_COUNTER(STAT_SortingTime);
//
// 	if (SortMode == PSORTMODE_ViewProjDepth)
// 	{
// 		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
// 		{
// 			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
// 			float InZ;
// 			if (bLocalSpace)
// 			{
// 				InZ = View->ViewMatrices.GetViewProjectionMatrix().TransformPosition(LocalToWorld.TransformPosition(Particle.Location)).W;
// 			}
// 			else
// 			{
// 				InZ = View->ViewMatrices.GetViewProjectionMatrix().TransformPosition(Particle.Location).W;
// 			}
// 			ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
//
// 			ParticleOrder[ParticleIndex].Z = InZ;
// 		}
// 		Algo::SortBy(MakeArrayView(ParticleOrder, ParticleCount), &FParticleOrder::Z, TGreater<>());
// 	}
// 	else if (SortMode == PSORTMODE_DistanceToView)
// 	{
// 		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
// 		{
// 			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
// 			float InZ;
// 			FVector Position;
// 			if (bLocalSpace)
// 			{
// 				Position = LocalToWorld.TransformPosition(Particle.Location);
// 			}
// 			else
// 			{
// 				Position = Particle.Location;
// 			}
// 			InZ = (View->ViewMatrices.GetViewOrigin() - Position).SizeSquared();
// 			ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
// 			ParticleOrder[ParticleIndex].Z = InZ;
// 		}
// 		Algo::SortBy(MakeArrayView(ParticleOrder, ParticleCount), &FParticleOrder::Z, TGreater<>());
// 	}
// 	else if (SortMode == PSORTMODE_Age_OldestFirst)
// 	{
// 		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
// 		{
// 			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
// 			ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
// 			ParticleOrder[ParticleIndex].C = Particle.Flags & STATE_CounterMask;
// 		}
// 		Algo::SortBy(MakeArrayView(ParticleOrder, ParticleCount), &FParticleOrder::C, TGreater<>());
// 	}
// 	else if (SortMode == PSORTMODE_Age_NewestFirst)
// 	{
// 		for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
// 		{
// 			DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
// 			ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
// 			ParticleOrder[ParticleIndex].C = (~Particle.Flags) & STATE_CounterMask;
// 		}
// 		Algo::SortBy(MakeArrayView(ParticleOrder, ParticleCount), &FParticleOrder::C, TGreater<>());
// 	}
// }

///////////////////////////////////////////////////////////////////////////////
//	ParticleMeshEmitterInstance
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//	FDynamicSpriteEmitterData
///////////////////////////////////////////////////////////////////////////////

/** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
void FDynamicSpriteEmitterData::Init( bool bInSelected )
{
    bSelected = bInSelected;

    bUsesDynamicParameter = GetSourceData()->DynamicParameterDataOffset > 0;

    UMaterial const* Material = const_cast<UMaterial const*>(Source.Material);
    MaterialResource = Material;
    
    // We won't need this on the render thread
    Source.Material = NULL;
}
