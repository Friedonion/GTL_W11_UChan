#pragma once

#include "Core/HAL/PlatformType.h"
#include "Core/Math/MathUtility.h"
#include "Core/Container/Array.h"
#include "Core/Math/Vector.h"
#include "Math/RandomStream.h"
#include "UObject/ObjectMacros.h"

/**
 * Operation to perform when looking up a value.
 */
enum class ERawDistributionOperation
{
    RDO_Uninitialized,
    RDO_None,
    RDO_Random,
    //RDO_Extreme,
};

// Helper macro for retrieving random value
#define DIST_GET_RANDOM_VALUE(RandStream) ((RandStream == NULL) ? FMath::SRand() : RandStream->GetFraction())

/**
 * Raw distribution used to quickly sample distributions at runtime.
 */
struct FRawDistribution
{
    DECLARE_STRUCT(FRawDistribution)
    /** Default constructor. */
    FRawDistribution()
    {
        Op = ERawDistributionOperation::RDO_None;
    }

    /**
     * Serialization.
     * @param Ar - The archive with which to serialize.
     * @returns true if serialization was successful.
     */
    //bool Serialize(FArchive& Ar);

    /**
     * Calcuate the float or vector value at the given time
     * @param Time The time to evaluate
     * @param Value An array of (1 or 3) FLOATs to receive the values
     * @param NumCoords The number of floats in the Value array
     * @param Extreme For distributions that use one of the extremes, this is which extreme to use
     */
    void GetValue(float Time, float* Value, int32 NumCoords, int32 Extreme, struct FRandomStream* InRandomStream) const;

    // prebaked versions of these
    void GetValue1(float Time, float* Value, int32 Extreme, struct FRandomStream* InRandomStream) const;
    void GetValue3(float Time, float* Value, int32 Extreme, struct FRandomStream* InRandomStream) const;
    inline void GetValue1None(float Time, float* InValue) const
    {
        //float* Value = InValue;
        //const float* Entry1;
        //const float* Entry2;
        //float LerpAlpha = 0.0f;
        ////LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
        //const float* NewEntry1 = Entry1;
        //const float* NewEntry2 = Entry2;
        //Value[0] = FMath::Lerp(NewEntry1[0], NewEntry2[0], LerpAlpha);
    }
    inline void GetValue3None(float Time, float* InValue) const
    {
        /*float* Value = InValue;
        const float* Entry1;
        const float* Entry2;
        float LerpAlpha = 0.0f;
        LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
        const float* NewEntry1 = Entry1;
        const float* NewEntry2 = Entry2;
        float T0 = FMath::Lerp(NewEntry1[0], NewEntry2[0], LerpAlpha);
        float T1 = FMath::Lerp(NewEntry1[1], NewEntry2[1], LerpAlpha);
        float T2 = FMath::Lerp(NewEntry1[2], NewEntry2[2], LerpAlpha);
        Value[0] = T0;
        Value[1] = T1;
        Value[2] = T2;*/
    }
    //void GetValue1Extreme(float Time, float* Value, int32 Extreme, struct FRandomStream* InRandomStream) const;
    //void GetValue3Extreme(float Time, float* Value, int32 Extreme, struct FRandomStream* InRandomStream) const;

    FORCEINLINE bool IsSimple()
    {
        return Op == ERawDistributionOperation::RDO_None;
    }

public:
    UPROPERTY
    (EditAnywhere, ERawDistributionOperation,  Op, = ERawDistributionOperation::RDO_None)
};
