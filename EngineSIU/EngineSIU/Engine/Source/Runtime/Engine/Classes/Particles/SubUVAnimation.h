#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class USubUVAnimation : public UObject
{
	DECLARE_CLASS(USubUVAnimation, UObject)

    USubUVAnimation();
    ~USubUVAnimation() = default;

	/** 
	 * Texture to generate bounding geometry from.
	 */
	//UPROPERTY(EditAnywhere, Category=SubUV)
	//TObjectPtr<UTexture2D> SubUVTexture;

	/** The number of sub-images horizontally in the texture							*/
	//UPROPERTY(EditAnywhere, Category=SubUV)
	int32 SubImages_Horizontal;

	/** The number of sub-images vertically in the texture								*/
	//UPROPERTY(EditAnywhere, Category=SubUV)
	int32 SubImages_Vertical;

	/** 
	 * More bounding vertices results in reduced overdraw, but adds more triangle overhead.
	 * The eight vertex mode is best used when the SubUV texture has a lot of space to cut out that is not captured by the four vertex version,
	 * and when the particles using the texture will be few and large.
	 */
	//UPROPERTY(EditAnywhere, Category=SubUV)
	//TEnumAsByte<enum ESubUVBoundingVertexCount> BoundingMode;

	//UPROPERTY(EditAnywhere, Category=SubUV)
	//TEnumAsByte<enum EOpacitySourceMode> OpacitySourceMode;

	/** 
	 * Alpha channel values larger than the threshold are considered occupied and will be contained in the bounding geometry.
	 * Raising this threshold slightly can reduce overdraw in particles using this animation asset.
	 */
	//UPROPERTY(EditAnywhere, Category=SubUV, meta=(UIMin = "0", UIMax = "1"))
	float AlphaThreshold;

private:

	// /** Derived data for this asset, generated off of SubUVTexture. */
	// FSubUVDerivedData DerivedData;
	//
	// /** Tracks progress of BoundingGeometryBuffer release during destruction. */
	// FRenderCommandFence ReleaseFence;
	//
	// /** Used on platforms that support instancing, the bounding geometry is fetched from a vertex shader instead of on the CPU. */
	// FSubUVBoundingGeometryBuffer* BoundingGeometryBuffer;

public:

	// inline int32 GetNumBoundingVertices() const 
	// { 
	// 	if (BoundingMode == BVC_FourVertices)
	// 	{
	// 		return 4;
	// 	}
	//
	// 	return 8;
	// }

	// inline int32 GetNumBoundingTriangles() const 
	// { 
	// 	if (BoundingMode == BVC_FourVertices)
	// 	{
	// 		return 2;
	// 	}
	//
	// 	return 6;
	// }

	inline int32 GetNumFrames() const
	{
		return SubImages_Vertical * SubImages_Horizontal;
	}

	// inline bool IsBoundingGeometryValid() const
	// {
	// 	return DerivedData.BoundingGeometry.Num() != 0;
	// }

	// inline const FVector2D* GetFrameData(int32 FrameIndex) const
	// {
	// 	return &DerivedData.BoundingGeometry[FrameIndex * GetNumBoundingVertices()];
	// }

	// inline FRHIShaderResourceView* GetBoundingGeometrySRV() const
	// {
	// 	return BoundingGeometryBuffer->ShaderResourceView;
	// }

	//~ Begin UObject Interface.
//     virtual void PostInitProperties() override;
// 	virtual void Serialize(FStructuredArchive::FRecord Record) override;
// 	virtual void PostLoad() override;
// #if WITH_EDITOR
// 	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
// 	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
// #endif // WITH_EDITOR
// 	virtual void BeginDestroy() override;
// 	virtual bool IsReadyForFinishDestroy() override;
// 	virtual void FinishDestroy() override;
	//~ End UObject Interface.

private:
	//void CacheDerivedData();
};
