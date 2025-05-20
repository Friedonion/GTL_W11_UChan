#include "Distributions.h"
#include "Distributions/Distribution.h"
#include "Distributions/DistributionFloat.h"
#include "Distributions/DistributionVector.h"

// Small enough value to be rounded to 0.0 in the editor 
// but significant enough to properly detect uninitialized defaults.
const float UDistribution::DefaultValue = 1.2345E-20f;

UDistribution::UDistribution()
{
}

UDistribution::~UDistribution()
{
}

void FRawDistribution::GetValue(float Time, float* Value, int32 NumCoords, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    //checkSlow(NumCoords == 3 || NumCoords == 1);
    switch (Op)
    {
    case ERawDistributionOperation::RDO_None:
        if (NumCoords == 1)
        {
            //GetValue1None(Time, Value);
        }
        else
        {
            //GetValue3None(Time, Value);
        }
        break;
    case ERawDistributionOperation::RDO_Random:
        if (NumCoords == 1)
        {
            //GetValue1Random(Time, Value, InRandomStream);
        }
        else
        {
            //GetValue3Random(Time, Value, InRandomStream);
        }
        break;
    }
}

bool FRawDistributionFloat::IsCreated()
{
    return (Distribution != nullptr);
}

bool FRawDistributionVector::IsCreated()
{
    return (Distribution != nullptr);
}

float FRawDistributionFloat::GetValue(float F, struct FRandomStream* InRandomStream)
{
    float Value;

    switch (Op)
    {
    case ERawDistributionOperation::RDO_None:
        Value = Distribution->Constant;
        break;
    case ERawDistributionOperation::RDO_Random:
    {
        float RandomFractions;
        RandomFractions = DIST_GET_RANDOM_VALUE(InRandomStream);

        float RandomOffset;
        RandomOffset = FMath::Lerp(MinValue, MaxValue, RandomFractions);

        Value = Distribution->Constant + RandomOffset;
    }
    break;
    }

    return Value;
}

const FRawDistribution* FRawDistributionFloat::GetFastRawDistribution()
{
    return this;
}

void FRawDistributionFloat::GetOutRange(float& MinOut, float& MaxOut)
{
    MinOut = MinValue;
    MaxOut = MaxValue;
}

//void UDistributionFloat::Serialize(FStructuredArchive::FRecord Record)
//{
//#if WITH_EDITOR
//    FArchive& UnderlyingArchive = Record.GetUnderlyingArchive();
//
//    if (UnderlyingArchive.IsCooking() || UnderlyingArchive.IsSaving())
//    {
//        bBakedDataSuccesfully = HasBakedDistributionDataHelper<FRawDistributionFloat>(this);
//    }
//#endif
//
//    Super::Serialize(Record);
//}

float UDistributionFloat::GetValue(float F, struct FRandomStream* InRandomStream) const
{
    return 0.0;
}

float UDistributionFloat::GetFloatValue(float F)
{
    return GetValue(F);
}

uint32 UDistributionFloat::InitializeRawEntry(float Time, float* Values) const
{
    Values[0] = GetValue(Time);
    return 1;
}

FVector FRawDistributionVector::GetValue(float F, int32 Extreme, struct FRandomStream* InRandomStream)
{
    FVector Value;

    switch (Op)
    {
    case ERawDistributionOperation::RDO_None:
            Value = Distribution->Constant;
        break;
    case ERawDistributionOperation::RDO_Random:
        {
            FVector RandomFractions;
            RandomFractions.X = DIST_GET_RANDOM_VALUE(InRandomStream);
            RandomFractions.Y = DIST_GET_RANDOM_VALUE(InRandomStream);
            RandomFractions.Z = DIST_GET_RANDOM_VALUE(InRandomStream);
             
            FVector RandomOffset;
            RandomOffset.X = FMath::Lerp(MinValueVec.X, MaxValueVec.X, RandomFractions.X);
            RandomOffset.Y = FMath::Lerp(MinValueVec.Y, MaxValueVec.Y, RandomFractions.Y);
            RandomOffset.Z = FMath::Lerp(MinValueVec.Z, MaxValueVec.Z, RandomFractions.Z);

            Value = Distribution->Constant + RandomOffset;
        }
        break;
    }
    
    return (FVector)Value;
}

const FRawDistribution* FRawDistributionVector::GetFastRawDistribution()
{
    return this;
}

void FRawDistributionVector::GetOutRange(float& MinOut, float& MaxOut)
{
    MinOut = MinValue;
    MaxOut = MaxValue;
}

void FRawDistributionVector::GetRange(FVector& MinOut, FVector& MaxOut)
{
    if (Distribution)
    {
        if (!Distribution) return;
        //Distribution->GetRange(MinOut, MaxOut);
    }
    else
    {
        MinOut = MinValueVec;
        MaxOut = MaxValueVec;
    }
}

FVector UDistributionVector::GetValue(float F, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    return FVector::ZeroVector;
}

FVector UDistributionVector::GetVectorValue(float F)
{
    return GetValue(F);
}

uint32 UDistributionVector::InitializeRawEntry(float Time, float* Values) const
{
    FVector Value = GetValue(Time);
    Values[0] = Value.X;
    Values[1] = Value.Y;
    Values[2] = Value.Z;
    return 3;
}
