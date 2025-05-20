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

void FRawDistribution::GetValue1(float Time, float* Value, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    switch (Op)
    {
    case RDO_None:
        GetValue1None(Time, Value);
        break;
    case RDO_Extreme:
        GetValue1Extreme(Time, Value, Extreme, InRandomStream);
        break;
    case RDO_Random:
        GetValue1Random(Time, Value, InRandomStream);
        break;
    default: // compiler complains
        //checkSlow(0);
        *Value = 0.0f;
        break;
    }
}

void FRawDistribution::GetValue3(float Time, float* Value, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    switch (Op)
    {
    case RDO_None:
        GetValue3None(Time, Value);
        break;
    case RDO_Extreme:
        GetValue3Extreme(Time, Value, Extreme, InRandomStream);
        break;
    case RDO_Random:
        GetValue3Random(Time, Value, InRandomStream);
        break;
    }
}

void FRawDistribution::GetValue1Extreme(float Time, float* InValue, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    //float* Value = InValue;
    //const float* Entry1;
    //const float* Entry2;
    //float LerpAlpha = 0.0f;
    //float RandValue = DIST_GET_RANDOM_VALUE(InRandomStream);
    ////LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    //const float* NewEntry1 = Entry1;
    //const float* NewEntry2 = Entry2;
    //int32 InitialElement = ((Extreme > 0) || ((Extreme == 0) && (RandValue > 0.5f)));
    //Value[0] = FMath::Lerp(NewEntry1[InitialElement + 0], NewEntry2[InitialElement + 0], LerpAlpha);
}

void FRawDistribution::GetValue3Extreme(float Time, float* InValue, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    //float* Value = InValue;
    //const float* Entry1;
    //const float* Entry2;
    //float LerpAlpha = 0.0f;
    //float RandValue = DIST_GET_RANDOM_VALUE(InRandomStream);
    ////LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    //const float* NewEntry1 = Entry1;
    //const float* NewEntry2 = Entry2;
    //int32 InitialElement = ((Extreme > 0) || ((Extreme == 0) && (RandValue > 0.5f)));
    //InitialElement *= 3;
    //float T0 = FMath::Lerp(NewEntry1[InitialElement + 0], NewEntry2[InitialElement + 0], LerpAlpha);
    //float T1 = FMath::Lerp(NewEntry1[InitialElement + 1], NewEntry2[InitialElement + 1], LerpAlpha);
    //float T2 = FMath::Lerp(NewEntry1[InitialElement + 2], NewEntry2[InitialElement + 2], LerpAlpha);
    //Value[0] = T0;
    //Value[1] = T1;
    //Value[2] = T2;
}

void FRawDistribution::GetValue1Random(float Time, float* InValue, struct FRandomStream* InRandomStream) const
{
    float* Value = InValue;
    float RandValue = DIST_GET_RANDOM_VALUE(InRandomStream);
    Value[0] = RandValue;
    //const float* Entry1;
    //const float* Entry2;
    //float LerpAlpha = 0.0f;
    //float RandValue = DIST_GET_RANDOM_VALUE(InRandomStream);
    ////LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    //const float* NewEntry1 = Entry1;
    //const float* NewEntry2 = Entry2;
    //float Value1, Value2;
    //Value1 = FMath::Lerp(NewEntry1[0], NewEntry2[0], LerpAlpha);
    //Value2 = FMath::Lerp(NewEntry1[1 + 0], NewEntry2[1 + 0], LerpAlpha);
    //Value[0] = Value1 + (Value2 - Value1) * RandValue;
}

void FRawDistribution::GetValue3Random(float Time, float* InValue, struct FRandomStream* InRandomStream) const
{
    float* Value = InValue;
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    FVector RandValues;

    RandValues[0] = DIST_GET_RANDOM_VALUE(InRandomStream);
    RandValues[1] = DIST_GET_RANDOM_VALUE(InRandomStream);
    RandValues[2] = DIST_GET_RANDOM_VALUE(InRandomStream);
    
    Value[0] = RandValues[0];
    Value[1] = RandValues[1];
    Value[2] = RandValues[2];
    /*switch (LookupTable.LockFlag)
    {
    case EDVLF_XY:
        RandValues.Y = RandValues.X;
        break;
    case EDVLF_XZ:
        RandValues.Z = RandValues.X;
        break;
    case EDVLF_YZ:
        RandValues.Z = RandValues.Y;
        break;
    case EDVLF_XYZ:
        RandValues.Y = RandValues.X;
        RandValues.Z = RandValues.X;
        break;
    }*/

    //LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
   /* const float* NewEntry1 = Entry1;
    const float* NewEntry2 = Entry2;
    float X0, X1;
    float Y0, Y1;
    float Z0, Z1;
    X0 = FMath::Lerp(NewEntry1[0], NewEntry2[0], LerpAlpha);
    Y0 = FMath::Lerp(NewEntry1[1], NewEntry2[1], LerpAlpha);
    Z0 = FMath::Lerp(NewEntry1[2], NewEntry2[2], LerpAlpha);
    X1 = FMath::Lerp(NewEntry1[3 + 0], NewEntry2[3 + 0], LerpAlpha);
    Y1 = FMath::Lerp(NewEntry1[3 + 1], NewEntry2[3 + 1], LerpAlpha);
    Z1 = FMath::Lerp(NewEntry1[3 + 2], NewEntry2[3 + 2], LerpAlpha);
    Value[0] = X0 + (X1 - X0) * RandValues[0];
    Value[1] = Y0 + (Y1 - Y0) * RandValues[1];
    Value[2] = Z0 + (Z1 - Z0) * RandValues[2];*/
}

void FRawDistribution::GetValue(float Time, float* Value, int32 NumCoords, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    //checkSlow(NumCoords == 3 || NumCoords == 1);
    switch (Op)
    {
    case RDO_None:
        if (NumCoords == 1)
        {
            GetValue1None(Time, Value);
        }
        else
        {
            GetValue3None(Time, Value);
        }
        break;
    case RDO_Extreme:
        if (NumCoords == 1)
        {
            GetValue1Extreme(Time, Value, Extreme, InRandomStream);
        }
        else
        {
            GetValue3Extreme(Time, Value, Extreme, InRandomStream);
        }
        break;
    case RDO_Random:
        if (NumCoords == 1)
        {
            GetValue1Random(Time, Value, InRandomStream);
        }
        else
        {
            GetValue3Random(Time, Value, InRandomStream);
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
    FRawDistribution::GetValue1(F, &Value, 0, InRandomStream);
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

//@todo.CONSOLE
uint32 UDistributionFloat::InitializeRawEntry(float Time, float* Values) const
{
    Values[0] = GetValue(Time);
    return 1;
}

FVector FRawDistributionVector::GetValue(float F, int32 Extreme, struct FRandomStream* InRandomStream)
{
    FVector Value;
    FRawDistribution::GetValue3(F, &Value.X, Extreme, InRandomStream);
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

//@todo.CONSOLE
uint32 UDistributionVector::InitializeRawEntry(float Time, float* Values) const
{
    FVector Value = GetValue(Time);
    Values[0] = Value.X;
    Values[1] = Value.Y;
    Values[2] = Value.Z;
    return 3;
}

FFloatDistribution::FFloatDistribution()
{
    Op = RDO_None;
}

FVectorDistribution::FVectorDistribution()
{
    Op = RDO_None;
}

FVector4Distribution::FVector4Distribution()
{
    Op = RDO_None;
}
