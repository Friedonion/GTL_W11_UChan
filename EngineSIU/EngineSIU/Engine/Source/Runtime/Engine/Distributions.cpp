#include "Distributions.h"
#include "Distributions/Distribution.h"
#include "Distributions/DistributionFloat.h"
#include "Distributions/DistributionVector.h"
//#include "Particles/ParticleModule.h"

// Moving UDistributions to PostInitProps to not be default sub-objects:
// Small enough value to be rounded to 0.0 in the editor 
// but significant enough to properly detect uninitialized defaults.
const float UDistribution::DefaultValue = 1.2345E-20f;

uint32 GDistributionType = 1;

// The error threshold used when optimizing lookup table sample counts.
#define LOOKUP_TABLE_ERROR_THRESHOLD (0.05f)

// The maximum number of values to store in a lookup table.
#define LOOKUP_TABLE_MAX_SAMPLES (128)

// UDistribution will bake out (if CanBeBaked returns true)
#define DISTRIBUTIONS_BAKES_OUT 1

// The maximum number of samples must be a power of two.
static_assert((LOOKUP_TABLE_MAX_SAMPLES& (LOOKUP_TABLE_MAX_SAMPLES - 1)) == 0, "Lookup table max samples is not a power of two.");

UDistribution::UDistribution()
{
}

UDistribution::~UDistribution()
{
}

/*-----------------------------------------------------------------------------
    Lookup table related functions.
------------------------------------------------------------------------------*/

//@todo.CONSOLE: Until we have cooking or something in place, these need to be exposed.
/**
 * Builds a lookup table that returns a constant value.
 * @param OutTable - The table to build.
 * @param ValuesPerEntry - The number of values per entry in the table.
 * @param Values - The values to place in the table.
 */
static void BuildConstantLookupTable(FDistributionLookupTable* OutTable, int32 ValuesPerEntry, const float* Values)
{
    if (OutTable == NULL) return;
    if (Values == NULL) return;

    OutTable->Values.Empty(ValuesPerEntry);
    OutTable->Values.AddUninitialized(ValuesPerEntry);
    OutTable->Op = RDO_None;
    OutTable->EntryCount = 1;
    OutTable->EntryStride = ValuesPerEntry;
    OutTable->SubEntryStride = 0;
    OutTable->TimeBias = 0.0f;
    OutTable->TimeScale = 0.0f;
    for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex)
    {
        OutTable->Values[ValueIndex] = Values[ValueIndex];
    }
}

/**
 * Builds a lookup table that returns zero.
 * @param OutTable - The table to build.
 * @param ValuesPerEntry - The number of values per entry in the table.
 */
static void BuildZeroLookupTable(FDistributionLookupTable* OutTable, int32 ValuesPerEntry)
{
    if (OutTable == NULL) return;
    if (ValuesPerEntry < 1 || ValuesPerEntry > 4) return;

    float Zero[4] = { 0 };
    BuildConstantLookupTable(OutTable, ValuesPerEntry, Zero);
}

/**
 * Builds a lookup table from a distribution.
 * @param OutTable - The table to build.
 * @param Distribution - The distribution for which to build a lookup table.
 */
template <typename DistributionType>
void BuildLookupTable(FDistributionLookupTable* OutTable, const DistributionType& Distribution)
{
    
    check(Distribution);

    // Always clear the table.
    OutTable->Empty();

    // Nothing to do if we don't have a distribution.
    if (!Distribution->CanBeBaked())
    {
        BuildZeroLookupTable(OutTable, Distribution->GetValueCount());
        return;
    }

    // Always build a lookup table of maximal size. This can/will be optimized later.
    const int32 EntryCount = LOOKUP_TABLE_MAX_SAMPLES;

    // Determine the domain of the distribution.
    float MinIn, MaxIn;
    Distribution->GetInRange(MinIn, MaxIn);
    const float TimeScale = (MaxIn - MinIn) / (float)(EntryCount - 1);

    // Get the operation to use, and calculate the number of values needed for that operation.
    const uint8 Op = Distribution->GetOperation();
    const int32 ValuesPerEntry = Distribution->GetValueCount();
    const uint32 EntryStride = ((Op == RDO_None) ? 1 : 2) * (uint32)ValuesPerEntry;

    // Get the lock flag to use.
    const uint8 LockFlag = Distribution->GetLockFlag();

    // Allocate a lookup table of the appropriate size.
    OutTable->Op = Op;
    OutTable->EntryCount = EntryCount;
    OutTable->EntryStride = EntryStride;
    OutTable->SubEntryStride = (Op == RDO_None) ? 0 : ValuesPerEntry;
    OutTable->TimeScale = (TimeScale > 0.0f) ? (1.0f / TimeScale) : 0.0f;
    OutTable->TimeBias = MinIn;
    OutTable->Values.Empty(EntryCount * EntryStride);
    OutTable->Values.AddZeroed(EntryCount * EntryStride);
    OutTable->LockFlag = LockFlag;

    // Sample the distribution.
    for (uint32 SampleIndex = 0; SampleIndex < EntryCount; SampleIndex++)
    {
        const float Time = MinIn + SampleIndex * TimeScale;
        float Values[8];
        Distribution->InitializeRawEntry(Time, Values);
        for (uint32 ValueIndex = 0; ValueIndex < EntryStride; ValueIndex++)
        {
            OutTable->Values[SampleIndex * EntryStride + ValueIndex] = Values[ValueIndex];
        }
    }
}

/**
 * Appends one lookup table to another.
 * @param Table - Table which contains the first set of components [1-3].
 * @param OtherTable - Table to append which contains a single component.
 */
static void AppendLookupTable(FDistributionLookupTable* Table, const FDistributionLookupTable& OtherTable)
{
    if (Table == NULL) return;
    if (Table->GetValuesPerEntry() < 1 || Table->GetValuesPerEntry() > 3) return;
    if (OtherTable.GetValuesPerEntry() != 1) return;

    // Copy the input table.
    FDistributionLookupTable TableCopy = *Table;

    // Compute the domain of the composed distribution.
    const float OneOverTimeScale = (TableCopy.TimeScale == 0.0f) ? 0.0f : 1.0f / TableCopy.TimeScale;
    const float OneOverOtherTimeScale = (OtherTable.TimeScale == 0.0f) ? 0.0f : 1.0f / OtherTable.TimeScale;
    const float MinIn = FMath::Min(TableCopy.TimeBias, OtherTable.TimeBias);
    const float MaxIn = FMath::Max(TableCopy.TimeBias + (TableCopy.EntryCount - 1) * OneOverTimeScale, OtherTable.TimeBias + (OtherTable.EntryCount - 1) * OneOverOtherTimeScale);

    const int32 InValuesPerEntry = TableCopy.GetValuesPerEntry();
    const int32 OtherValuesPerEntry = 1;
    const int32 NewValuesPerEntry = InValuesPerEntry + OtherValuesPerEntry;
    const uint8 NewOp = (TableCopy.Op == RDO_None) ? OtherTable.Op : TableCopy.Op;
    const int32 NewEntryCount = LOOKUP_TABLE_MAX_SAMPLES;
    const int32 NewStride = (NewOp == RDO_None) ? NewValuesPerEntry : NewValuesPerEntry * 2;
    const float NewTimeScale = (MaxIn - MinIn) / (float)(NewEntryCount - 1);

    // Now build the new lookup table.
    Table->Op = NewOp;
    Table->EntryCount = NewEntryCount;
    Table->EntryStride = NewStride;
    Table->SubEntryStride = (NewOp == RDO_None) ? 0 : NewValuesPerEntry;
    Table->TimeScale = (NewTimeScale > 0.0f) ? 1.0f / NewTimeScale : 0.0f;
    Table->TimeBias = MinIn;
    Table->Values.Empty(NewEntryCount * NewStride);
    Table->Values.AddZeroed(NewEntryCount * NewStride);
    for (int32 SampleIndex = 0; SampleIndex < NewEntryCount; ++SampleIndex)
    {
        const float* InEntry1;
        const float* InEntry2;
        const float* OtherEntry1;
        const float* OtherEntry2;
        float InLerpAlpha;
        float OtherLerpAlpha;

        const float Time = MinIn + SampleIndex * NewTimeScale;
        TableCopy.GetEntry(Time, InEntry1, InEntry2, InLerpAlpha);
        OtherTable.GetEntry(Time, OtherEntry1, OtherEntry2, OtherLerpAlpha);

        // Store sub-entry 1.
        for (int32 ValueIndex = 0; ValueIndex < InValuesPerEntry; ++ValueIndex)
        {
            Table->Values[SampleIndex * NewStride + ValueIndex] =
                FMath::Lerp(InEntry1[ValueIndex], InEntry2[ValueIndex], InLerpAlpha);
        }
        Table->Values[SampleIndex * NewStride + InValuesPerEntry] =
            FMath::Lerp(OtherEntry1[0], OtherEntry2[0], OtherLerpAlpha);

        // Store sub-entry 2 if needed. 
        if (NewOp != RDO_None)
        {
            InEntry1 += TableCopy.SubEntryStride;
            InEntry2 += TableCopy.SubEntryStride;
            OtherEntry1 += OtherTable.SubEntryStride;
            OtherEntry2 += OtherTable.SubEntryStride;

            for (int32 ValueIndex = 0; ValueIndex < InValuesPerEntry; ++ValueIndex)
            {
                Table->Values[SampleIndex * NewStride + NewValuesPerEntry + ValueIndex] =
                    FMath::Lerp(InEntry1[ValueIndex], InEntry2[ValueIndex], InLerpAlpha);
            }
            Table->Values[SampleIndex * NewStride + NewValuesPerEntry + InValuesPerEntry] =
                FMath::Lerp(OtherEntry1[0], OtherEntry2[0], OtherLerpAlpha);
        }
    }
}

/**
 * Keeps only the first components of each entry in the table.
 * @param Table - Table to slice.
 * @param ChannelsToKeep - The number of channels to keep.
 */
static void SliceLookupTable(FDistributionLookupTable* Table, int32 ChannelsToKeep)
{
    if (Table == NULL) return;
    if (Table->GetValuesPerEntry() < ChannelsToKeep) return;

    // If the table only has the requested number of channels there is nothing to do.
    if (Table->GetValuesPerEntry() == ChannelsToKeep)
    {
        return;
    }

    // Copy the table.
    FDistributionLookupTable OldTable = *Table;

    // The new table will have the same number of entries as the input table.
    const int32 NewEntryCount = OldTable.EntryCount;
    const int32 NewStride = (OldTable.Op == RDO_None) ? (ChannelsToKeep) : (2 * ChannelsToKeep);
    Table->Op = OldTable.Op;
    Table->EntryCount = NewEntryCount;
    Table->EntryStride = NewStride;
    Table->SubEntryStride = (OldTable.Op == RDO_None) ? (0) : (ChannelsToKeep);
    Table->TimeBias = OldTable.TimeBias;
    Table->TimeScale = OldTable.TimeScale;
    Table->Values.Empty(NewEntryCount * NewStride);
    Table->Values.AddZeroed(NewEntryCount * NewStride);

    // Copy values over.
    for (int32 EntryIndex = 0; EntryIndex < NewEntryCount; ++EntryIndex)
    {
        const float* SrcValues = &OldTable.Values[EntryIndex * OldTable.EntryStride];
        float* DestValues = &Table->Values[EntryIndex * Table->EntryStride];
        for (int32 ValueIndex = 0; ValueIndex < ChannelsToKeep; ++ValueIndex)
        {
            DestValues[ValueIndex] = SrcValues[ValueIndex];
        }
        if (OldTable.SubEntryStride > 0)
        {
            SrcValues += OldTable.SubEntryStride;
            DestValues += Table->SubEntryStride;
            for (int32 ValueIndex = 0; ValueIndex < ChannelsToKeep; ++ValueIndex)
            {
                DestValues[ValueIndex] = SrcValues[ValueIndex];
            }
        }
    }
}

/**
 * Scales each value in the lookup table by a constant.
 * @param InTable - Table to be scaled.
 * @param Scale - The amount to scale by.
 */
static void ScaleLookupTableByConstant(FDistributionLookupTable* Table, float Scale)
{
    if (Table == NULL) return;

    for (int32 ValueIndex = 0; ValueIndex < Table->Values.Num(); ++ValueIndex)
    {
        Table->Values[ValueIndex] *= Scale;
    }
}

/**
 * Scales each value in the lookup table by a constant.
 * @param InTable - Table to be scaled.
 * @param Scale - The amount to scale by.
 * @param ValueCount - The number of scale values.
 */
static void ScaleLookupTableByConstants(FDistributionLookupTable* Table, const float* Scale, int32 ValueCount)
{
    
    if (Table == NULL) return;
    if (ValueCount != Table->GetValuesPerEntry()) return;

    const int32 EntryCount = Table->EntryCount;
    const int32 SubEntryCount = (Table->SubEntryStride > 0) ? 2 : 1;
    const int32 Stride = Table->EntryStride;
    const int32 SubEntryStride = Table->SubEntryStride;
    float* Values = Table->Values.GetData();

    for (int32 Index = 0; Index < EntryCount; ++Index)
    {
        float* EntryValues = Values;
        for (int32 SubEntryIndex = 0; SubEntryIndex < SubEntryCount; ++SubEntryIndex)
        {
            for (int32 ValueIndex = 0; ValueIndex < ValueCount; ++ValueIndex)
            {
                EntryValues[ValueIndex] *= Scale[ValueIndex];
            }
            EntryValues += SubEntryStride;
        }
        Values += Stride;
    }
}

/**
 * Adds a constant to each value in the lookup table.
 * @param InTable - Table to be scaled.
 * @param Addend - The amount to add by.
 * @param ValueCount - The number of values per entry.
 */
static void AddConstantToLookupTable(FDistributionLookupTable* Table, const float* Addend, int32 ValueCount)
{
    
    if (Table == NULL) return;
    if (ValueCount != Table->GetValuesPerEntry()) return;

    const int32 EntryCount = Table->EntryCount;
    const int32 SubEntryCount = (Table->SubEntryStride > 0) ? 2 : 1;
    const int32 Stride = Table->EntryStride;
    const int32 SubEntryStride = Table->SubEntryStride;
    float* Values = Table->Values.GetData();

    for (int32 Index = 0; Index < EntryCount; ++Index)
    {
        float* EntryValues = Values;
        for (int32 SubEntryIndex = 0; SubEntryIndex < SubEntryCount; ++SubEntryIndex)
        {
            for (int32 ValueIndex = 0; ValueIndex < ValueCount; ++ValueIndex)
            {
                EntryValues[ValueIndex] += Addend[ValueIndex];
            }
            EntryValues += SubEntryStride;
        }
        Values += Stride;
    }
}

/**
 * Scales one lookup table by another.
 * @param Table - The table to scale.
 * @param OtherTable - The table to scale by. Must have one value per entry OR the same values per entry as Table.
 */
static void ScaleLookupTableByLookupTable(FDistributionLookupTable* Table, const FDistributionLookupTable& OtherTable)
{
    
    if (Table == NULL) return;
    if (!(OtherTable.GetValuesPerEntry() == 1 || OtherTable.GetValuesPerEntry() == Table->GetValuesPerEntry())) return;

    // Copy the original table.
    FDistributionLookupTable InTable = *Table;

    // Compute the domain of the composed distribution.
    const float OneOverTimeScale = (InTable.TimeScale == 0.0f) ? 0.0f : 1.0f / InTable.TimeScale;
    const float OneOverOtherTimeScale = (OtherTable.TimeScale == 0.0f) ? 0.0f : 1.0f / OtherTable.TimeScale;
    const float MinIn = FMath::Min(InTable.TimeBias, OtherTable.TimeBias);
    const float MaxIn = FMath::Max(InTable.TimeBias + (InTable.EntryCount - 1) * OneOverTimeScale, OtherTable.TimeBias + (OtherTable.EntryCount - 1) * OneOverOtherTimeScale);

    const int32 ValuesPerEntry = InTable.GetValuesPerEntry();
    const int32 OtherValuesPerEntry = OtherTable.GetValuesPerEntry();
    const uint8 NewOp = (InTable.Op == RDO_None) ? OtherTable.Op : InTable.Op;
    const int32 NewEntryCount = LOOKUP_TABLE_MAX_SAMPLES;
    const int32 NewStride = (NewOp == RDO_None) ? ValuesPerEntry : ValuesPerEntry * 2;
    const float NewTimeScale = (MaxIn - MinIn) / (float)(NewEntryCount - 1);

    // Now build the new lookup table.
    Table->Op = NewOp;
    Table->EntryCount = NewEntryCount;
    Table->EntryStride = NewStride;
    Table->SubEntryStride = (NewOp == RDO_None) ? 0 : ValuesPerEntry;
    Table->TimeScale = (NewTimeScale > 0.0f) ? 1.0f / NewTimeScale : 0.0f;
    Table->TimeBias = MinIn;
    Table->Values.Empty(NewEntryCount * NewStride);
    Table->Values.AddZeroed(NewEntryCount * NewStride);
    for (int32 SampleIndex = 0; SampleIndex < NewEntryCount; ++SampleIndex)
    {
        const float* InEntry1;
        const float* InEntry2;
        const float* OtherEntry1;
        const float* OtherEntry2;
        float InLerpAlpha;
        float OtherLerpAlpha;

        const float Time = MinIn + SampleIndex * NewTimeScale;
        InTable.GetEntry(Time, InEntry1, InEntry2, InLerpAlpha);
        OtherTable.GetEntry(Time, OtherEntry1, OtherEntry2, OtherLerpAlpha);

        // Scale sub-entry 1.
        for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex)
        {
            Table->Values[SampleIndex * NewStride + ValueIndex] =
                FMath::Lerp(InEntry1[ValueIndex], InEntry2[ValueIndex], InLerpAlpha) *
                FMath::Lerp(OtherEntry1[ValueIndex % OtherValuesPerEntry], OtherEntry2[ValueIndex % OtherValuesPerEntry], OtherLerpAlpha);
        }

        // Scale sub-entry 2 if needed. 
        if (NewOp != RDO_None)
        {
            InEntry1 += InTable.SubEntryStride;
            InEntry2 += InTable.SubEntryStride;
            OtherEntry1 += OtherTable.SubEntryStride;
            OtherEntry2 += OtherTable.SubEntryStride;

            for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex)
            {
                Table->Values[SampleIndex * NewStride + ValuesPerEntry + ValueIndex] =
                    FMath::Lerp(InEntry1[ValueIndex], InEntry2[ValueIndex], InLerpAlpha) *
                    FMath::Lerp(OtherEntry1[ValueIndex % OtherValuesPerEntry], OtherEntry2[ValueIndex % OtherValuesPerEntry], OtherLerpAlpha);
            }
        }
    }
}

/**
 * Adds the values in one lookup table by another.
 * @param Table - The table to which to add values.
 * @param OtherTable - The table from which to add values. Must have one value per entry OR the same values per entry as Table.
 */
static void AddLookupTableToLookupTable(FDistributionLookupTable* Table, const FDistributionLookupTable& OtherTable)
{
    
    if (Table == NULL) return;
    if (!(OtherTable.GetValuesPerEntry() == 1 || OtherTable.GetValuesPerEntry() == Table->GetValuesPerEntry())) return;

    // Copy the original table.
    FDistributionLookupTable InTable = *Table;

    // Compute the domain of the composed distribution.
    const float OneOverTimeScale = (InTable.TimeScale == 0.0f) ? 0.0f : 1.0f / InTable.TimeScale;
    const float OneOverOtherTimeScale = (OtherTable.TimeScale == 0.0f) ? 0.0f : 1.0f / OtherTable.TimeScale;
    const float MinIn = FMath::Min(InTable.TimeBias, OtherTable.TimeBias);
    const float MaxIn = FMath::Max(InTable.TimeBias + (InTable.EntryCount - 1) * OneOverTimeScale, OtherTable.TimeBias + (OtherTable.EntryCount - 1) * OneOverOtherTimeScale);

    const int32 ValuesPerEntry = InTable.GetValuesPerEntry();
    const int32 OtherValuesPerEntry = OtherTable.GetValuesPerEntry();
    const uint8 NewOp = (InTable.Op == RDO_None) ? OtherTable.Op : InTable.Op;
    const int32 NewEntryCount = LOOKUP_TABLE_MAX_SAMPLES;
    const int32 NewStride = (NewOp == RDO_None) ? ValuesPerEntry : ValuesPerEntry * 2;
    const float NewTimeScale = (MaxIn - MinIn) / (float)(NewEntryCount - 1);

    // Now build the new lookup table.
    Table->Op = NewOp;
    Table->EntryCount = NewEntryCount;
    Table->EntryStride = NewStride;
    Table->SubEntryStride = (NewOp == RDO_None) ? 0 : ValuesPerEntry;
    Table->TimeScale = (NewTimeScale > 0.0f) ? 1.0f / NewTimeScale : 0.0f;
    Table->TimeBias = MinIn;
    Table->Values.Empty(NewEntryCount * NewStride);
    Table->Values.AddZeroed(NewEntryCount * NewStride);
    for (int32 SampleIndex = 0; SampleIndex < NewEntryCount; ++SampleIndex)
    {
        const float* InEntry1;
        const float* InEntry2;
        const float* OtherEntry1;
        const float* OtherEntry2;
        float InLerpAlpha;
        float OtherLerpAlpha;

        const float Time = MinIn + SampleIndex * NewTimeScale;
        InTable.GetEntry(Time, InEntry1, InEntry2, InLerpAlpha);
        OtherTable.GetEntry(Time, OtherEntry1, OtherEntry2, OtherLerpAlpha);

        // Scale sub-entry 1.
        for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex)
        {
            Table->Values[SampleIndex * NewStride + ValueIndex] =
                FMath::Lerp(InEntry1[ValueIndex], InEntry2[ValueIndex], InLerpAlpha) +
                FMath::Lerp(OtherEntry1[ValueIndex % OtherValuesPerEntry], OtherEntry2[ValueIndex % OtherValuesPerEntry], OtherLerpAlpha);
        }

        // Scale sub-entry 2 if needed. 
        if (NewOp != RDO_None)
        {
            InEntry1 += InTable.SubEntryStride;
            InEntry2 += InTable.SubEntryStride;
            OtherEntry1 += OtherTable.SubEntryStride;
            OtherEntry2 += OtherTable.SubEntryStride;

            for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex)
            {
                Table->Values[SampleIndex * NewStride + ValuesPerEntry + ValueIndex] =
                    FMath::Lerp(InEntry1[ValueIndex], InEntry2[ValueIndex], InLerpAlpha) +
                    FMath::Lerp(OtherEntry1[ValueIndex % OtherValuesPerEntry], OtherEntry2[ValueIndex % OtherValuesPerEntry], OtherLerpAlpha);
            }
        }
    }
}

/**
 * Computes the L2 norm between the samples in ValueCount dimensional space.
 * @param Values1 - Array of ValueCount values.
 * @param Values2 - Array of ValueCount values.
 * @param ValueCount - The number of values in the two arrays.
 * @returns the L2 norm of the difference of the vectors represented by the two float arrays.
 */
static float ComputeSampleDistance(const float* Values1, const float* Values2, int32 ValueCount)
{
    float Dist = 0.0f;
    for (int32 ValueIndex = 0; ValueIndex < ValueCount; ++ValueIndex)
    {
        const float Diff = Values1[ValueIndex] - Values2[ValueIndex];
        Dist += (Diff * Diff);
    }
    return FMath::Sqrt(Dist);
}

/**
 * Computes the chordal distance between the curves represented by the two tables.
 * @param Table1 - The first table to compare.
 * @param Table2 - The second table to compare.
 * @param MinIn - The time at which to begin comparing.
 * @param MaxIn - The time at which to stop comparing.
 * @param SampleCount - The number of samples to use.
 * @returns the chordal distance representing the error introduced by substituting one table for the other.
 */
static float ComputeLookupTableError(const FDistributionLookupTable& InTable1, const FDistributionLookupTable& InTable2, float MinIn, float MaxIn, int32 SampleCount)
{
    if (InTable1.EntryStride != InTable2.EntryStride) return 0.0f;
    if (InTable1.SubEntryStride != InTable2.SubEntryStride) return 0.0f;
    if (SampleCount <= 0) return 0.0f;

    const FDistributionLookupTable* Table1 = (InTable2.EntryCount > InTable1.EntryCount) ? &InTable2 : &InTable1;
    const FDistributionLookupTable* Table2 = (Table1 == &InTable1) ? &InTable2 : &InTable1;
    const int32 ValuesPerEntry = Table1->GetValuesPerEntry();
    const float TimeStep = (MaxIn - MinIn) / (SampleCount - 1);

    float Values1[4] = { 0 };
    float Values2[4] = { 0 };
    float Error = 0.0f;
    float Time = MinIn;
    for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex, Time += TimeStep)
    {
        const float* Table1Entry1 = NULL;
        const float* Table1Entry2 = NULL;
        float Table1LerpAlpha = 0.0f;
        const float* Table2Entry1 = NULL;
        const float* Table2Entry2 = NULL;
        float Table2LerpAlpha = 0.0f;

        Table1->GetEntry(Time, Table1Entry1, Table1Entry2, Table1LerpAlpha);
        Table2->GetEntry(Time, Table2Entry1, Table2Entry2, Table2LerpAlpha);
        for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex)
        {
            Values1[ValueIndex] = FMath::Lerp(Table1Entry1[ValueIndex], Table1Entry2[ValueIndex], Table1LerpAlpha);
            Values2[ValueIndex] = FMath::Lerp(Table2Entry1[ValueIndex], Table2Entry2[ValueIndex], Table2LerpAlpha);
        }
        Error = FMath::Max<float>(Error, ComputeSampleDistance(Values1, Values2, ValuesPerEntry));

        if (Table1->SubEntryStride > 0)
        {
            Table1Entry1 += Table1->SubEntryStride;
            Table1Entry2 += Table1->SubEntryStride;
            Table2Entry1 += Table2->SubEntryStride;
            Table2Entry2 += Table2->SubEntryStride;
            for (int32 ValueIndex = 0; ValueIndex < ValuesPerEntry; ++ValueIndex)
            {
                Values1[ValueIndex] = FMath::Lerp(Table1Entry1[ValueIndex], Table1Entry2[ValueIndex], Table1LerpAlpha);
                Values2[ValueIndex] = FMath::Lerp(Table2Entry1[ValueIndex], Table2Entry2[ValueIndex], Table2LerpAlpha);
            }
            Error = FMath::Max<float>(Error, ComputeSampleDistance(Values1, Values2, ValuesPerEntry));
        }
    }
    return Error;
}

/**
 * Resamples a lookup table.
 * @param OutTable - The resampled table.
 * @param InTable - The table to be resampled.
 * @param MinIn - The time at which to begin resampling.
 * @param MaxIn - The time at which to stop resampling.
 * @param SampleCount - The number of samples to use.
 */
static void ResampleLookupTable(FDistributionLookupTable* OutTable, const FDistributionLookupTable& InTable, float MinIn, float MaxIn, int32 SampleCount)
{
    
    const int32 Stride = InTable.EntryStride;
    const float OneOverTimeScale = (InTable.TimeScale == 0.0f) ? 0.0f : 1.0f / InTable.TimeScale;
    const float TimeScale = (SampleCount > 1) ? ((MaxIn - MinIn) / (float)(SampleCount - 1)) : 0.0f;

    // Build the resampled table.
    OutTable->Op = InTable.Op;
    OutTable->EntryCount = SampleCount;
    OutTable->EntryStride = InTable.EntryStride;
    OutTable->SubEntryStride = InTable.SubEntryStride;
    OutTable->TimeBias = MinIn;
    OutTable->TimeScale = (TimeScale > 0.0f) ? (1.0f / TimeScale) : 0.0f;
    OutTable->Values.Empty(SampleCount * InTable.EntryStride);
    OutTable->Values.AddZeroed(SampleCount * InTable.EntryStride);

    // Resample entries in the table.
    for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
    {
        const float* Entry1 = NULL;
        const float* Entry2 = NULL;
        float LerpAlpha = 0.0f;
        const float Time = MinIn + TimeScale * SampleIndex;
        InTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
        for (int32 ValueIndex = 0; ValueIndex < Stride; ++ValueIndex)
        {
            OutTable->Values[SampleIndex * Stride + ValueIndex] =
                FMath::Lerp(Entry1[ValueIndex], Entry2[ValueIndex], LerpAlpha);
        }
    }
}

/**
 * Optimizes a lookup table using the minimum number of samples required to represent the distribution.
 * @param Table - The lookup table to optimize.
 * @param ErrorThreshold - Threshold at which the lookup table is considered good enough.
 */
static void OptimizeLookupTable(FDistributionLookupTable* Table, float ErrorThreshold)
{
    
    if (Table == NULL) return;
    if (!((Table->EntryCount & (Table->EntryCount - 1)) == 0));

    // Domain for the table.
    const float OneOverTimeScale = (Table->TimeScale == 0.0f) ? 0.0f : 1.0f / Table->TimeScale;
    const float MinIn = Table->TimeBias;
    const float MaxIn = Table->TimeBias + (Table->EntryCount - 1) * OneOverTimeScale;

    // Duplicate the table.
    FDistributionLookupTable OriginalTable = *Table;

    // Resample the lookup table until error is reduced to an acceptable level.
    const int32 MinSampleCount = 1;
    const int32 MaxSampleCount = LOOKUP_TABLE_MAX_SAMPLES;
    for (int32 SampleCount = MinSampleCount; SampleCount < MaxSampleCount; SampleCount <<= 1)
    {
        ResampleLookupTable(Table, OriginalTable, MinIn, MaxIn, SampleCount);
        if (ComputeLookupTableError(*Table, OriginalTable, MinIn, MaxIn, LOOKUP_TABLE_MAX_SAMPLES) < ErrorThreshold)
        {
            return;
        }
    }

    // The original table is optimal.
    *Table = OriginalTable;
}

void FRawDistribution::GetValue1(float Time, float* Value, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    switch (LookupTable.Op)
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
    switch (LookupTable.Op)
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
    float* Value = InValue;
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    float RandValue = DIST_GET_RANDOM_VALUE(InRandomStream);
    LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    const float* NewEntry1 = Entry1;
    const float* NewEntry2 = Entry2;
    int32 InitialElement = ((Extreme > 0) || ((Extreme == 0) && (RandValue > 0.5f)));
    Value[0] = FMath::Lerp(NewEntry1[InitialElement + 0], NewEntry2[InitialElement + 0], LerpAlpha);
}

void FRawDistribution::GetValue3Extreme(float Time, float* InValue, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    float* Value = InValue;
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    float RandValue = DIST_GET_RANDOM_VALUE(InRandomStream);
    LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    const float* NewEntry1 = Entry1;
    const float* NewEntry2 = Entry2;
    int32 InitialElement = ((Extreme > 0) || ((Extreme == 0) && (RandValue > 0.5f)));
    InitialElement *= 3;
    float T0 = FMath::Lerp(NewEntry1[InitialElement + 0], NewEntry2[InitialElement + 0], LerpAlpha);
    float T1 = FMath::Lerp(NewEntry1[InitialElement + 1], NewEntry2[InitialElement + 1], LerpAlpha);
    float T2 = FMath::Lerp(NewEntry1[InitialElement + 2], NewEntry2[InitialElement + 2], LerpAlpha);
    Value[0] = T0;
    Value[1] = T1;
    Value[2] = T2;
}

void FRawDistribution::GetValue1Random(float Time, float* InValue, struct FRandomStream* InRandomStream) const
{
    float* Value = InValue;
    const float* Entry1;
    const float* Entry2;
    float LerpAlpha = 0.0f;
    float RandValue = DIST_GET_RANDOM_VALUE(InRandomStream);
    LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    const float* NewEntry1 = Entry1;
    const float* NewEntry2 = Entry2;
    float Value1, Value2;
    Value1 = FMath::Lerp(NewEntry1[0], NewEntry2[0], LerpAlpha);
    Value2 = FMath::Lerp(NewEntry1[1 + 0], NewEntry2[1 + 0], LerpAlpha);
    Value[0] = Value1 + (Value2 - Value1) * RandValue;
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
    switch (LookupTable.LockFlag)
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
    }

    LookupTable.GetEntry(Time, Entry1, Entry2, LerpAlpha);
    const float* NewEntry1 = Entry1;
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
    Value[2] = Z0 + (Z1 - Z0) * RandValues[2];
}

void FRawDistribution::GetValue(float Time, float* Value, int32 NumCoords, int32 Extreme, struct FRandomStream* InRandomStream) const
{
    //checkSlow(NumCoords == 3 || NumCoords == 1);
    switch (LookupTable.Op)
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

//#if WITH_EDITOR
//
//void FRawDistributionFloat::Initialize()
//{
//    // Nothing to do if we don't have a distribution.
//    if (Distribution == NULL)
//    {
//        return;
//    }
//
//    // does this FRawDist need updating? (if UDist is dirty or somehow the distribution wasn't dirty, but we have no data)
//    bool bNeedsUpdating = false;
//    if (Distribution->bIsDirty || (LookupTable.IsEmpty() && Distribution->CanBeBaked()))
//    {
//        if (!Distribution->bIsDirty)
//        {
//            UE_LOG(LogDistributions, Log, TEXT("Somehow Distribution %s wasn't dirty, but its FRawDistribution wasn't ever initialized!"), *Distribution->GetFullName());
//        }
//        bNeedsUpdating = true;
//    }
//    // only initialize if we need to
//    if (!bNeedsUpdating)
//    {
//        return;
//    }
//    if (!GIsEditor && !IsInGameThread() && !IsInAsyncLoadingThread())
//    {
//        return;
//    }
//    
//
//    // always empty out the lookup table
//    LookupTable.Empty();
//
//    // distribution is no longer dirty (if it was)
//    // template objects aren't marked as dirty, because any UDists that uses this as an archetype, 
//    // aren't the default values, and has already been saved, needs to know to build the FDist
//    if (!Distribution->IsTemplate())
//    {
//        Distribution->bIsDirty = false;
//    }
//
//    // if the distribution can't be baked out, then we do nothing here
//    if (!Distribution->CanBeBaked())
//    {
//        return;
//    }
//
//    // Build and optimize the lookup table.
//    BuildLookupTable(&LookupTable, Distribution);
//    OptimizeLookupTable(&LookupTable, LOOKUP_TABLE_ERROR_THRESHOLD);
//
//    // fill out our min/max
//    Distribution->GetOutRange(MinValue, MaxValue);
//}
//#endif // WITH_EDITOR

bool FRawDistributionFloat::IsCreated()
{
    return HasLookupTable(/*bInitializeIfNeeded=*/ false) || (Distribution != nullptr);
}

bool FRawDistributionVector::IsCreated()
{
    return HasLookupTable(/*bInitializeIfNeeded=*/ false) || (Distribution != nullptr);
}

float FRawDistributionFloat::GetValue(float F, UObject* Data, struct FRandomStream* InRandomStream)
{
    if (!HasLookupTable())
    {
        if (!Distribution)
        {
            return 0.0f;
        }
        return Distribution->GetValue(F, Data, InRandomStream);
    }

    // if we get here, we better have been initialized!
    if (LookupTable.IsEmpty()) return 0.0f;

    float Value;
    FRawDistribution::GetValue1(F, &Value, 0, InRandomStream);
    return Value;
}

const FRawDistribution* FRawDistributionFloat::GetFastRawDistribution()
{
    if (!IsSimple() || !HasLookupTable())
    {
        return 0;
    }

    // if we get here, we better have been initialized!
    if (LookupTable.IsEmpty()) return nullptr;

    return this;
}

void FRawDistributionFloat::GetOutRange(float& MinOut, float& MaxOut)
{
    if (!HasLookupTable() && Distribution)
    {
        if (!Distribution) return;
        //Distribution->GetOutRange(MinOut, MaxOut);
    }
    else
    {
        MinOut = MinValue;
        MaxOut = MaxValue;
    }
}

void FRawDistributionFloat::InitLookupTable()
{
//#if WITH_EDITOR
//    // make sure it's up to date
//    if (Distribution)
//    {
//        if (GIsEditor || Distribution->bIsDirty)
//        {
//            Distribution->ConditionalPostLoad();
//            Initialize();
//        }
//    }
//#endif
}

//#if WITH_EDITOR
//template <typename RawDistributionType>
//bool HasBakedDistributionDataHelper(const UDistribution* GivenDistribution)
//{
//    if (UObject* Outer = GivenDistribution->GetOuter())
//    {
//        for (TFieldIterator<FProperty> It(Outer->GetClass()); It; ++It)
//        {
//            FProperty* Property = *It;
//
//            if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
//            {
//                UObject* Distribution = FRawDistribution::TryGetDistributionObjectFromRawDistributionProperty(StructProp, reinterpret_cast<uint8*>(Outer));
//                if (Distribution == GivenDistribution)
//                {
//                    if (RawDistributionType* RawDistributionVector = StructProp->ContainerPtrToValuePtr<RawDistributionType>(reinterpret_cast<uint8*>(Outer)))
//                    {
//                        if (RawDistributionVector->HasLookupTable(false))
//                        {
//                            return true;	//We have baked data
//                        }
//                    }
//
//                    return false;
//                }
//            }
//            else if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
//            {
//                if (FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner))
//                {
//                    FScriptArrayHelper ArrayHelper(ArrayProp, Property->ContainerPtrToValuePtr<void>(Outer));
//                    for (int32 Idx = 0; Idx < ArrayHelper.Num(); ++Idx)
//                    {
//                        for (FProperty* ArrayProperty = InnerStructProp->Struct->PropertyLink; ArrayProperty; ArrayProperty = ArrayProperty->PropertyLinkNext)
//                        {
//                            if (FStructProperty* ArrayStructProp = CastField<FStructProperty>(ArrayProperty))
//                            {
//                                UObject* Distribution = FRawDistribution::TryGetDistributionObjectFromRawDistributionProperty(ArrayStructProp, ArrayHelper.GetRawPtr(Idx));
//                                if (Distribution == GivenDistribution)
//                                {
//                                    if (RawDistributionType* RawDistributionVector = ArrayStructProp->ContainerPtrToValuePtr<RawDistributionType>(ArrayHelper.GetRawPtr(Idx)))
//                                    {
//                                        if (RawDistributionVector->HasLookupTable(false))
//                                        {
//                                            return true;	//We have baked data
//                                        }
//                                    }
//
//                                    return false;
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }
//
//    return false;
//}
//#endif

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

float UDistributionFloat::GetValue(float F, UObject* Data, struct FRandomStream* InRandomStream) const
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

//#if WITH_EDITOR
//void FRawDistributionVector::Initialize()
//{
//    // Nothing to do if we don't have a distribution.
//    if (Distribution == NULL)
//    {
//        return;
//    }
//
//    // fill out our min/max
//    Distribution->GetOutRange(MinValue, MaxValue);
//    Distribution->GetRange(MinValueVec, MaxValueVec);
//
//    // does this FRawDist need updating? (if UDist is dirty or somehow the distribution wasn't dirty, but we have no data)
//    bool bNeedsUpdating = false;
//    if (Distribution->bIsDirty || (LookupTable.IsEmpty() && Distribution->CanBeBaked()))
//    {
//        if (!Distribution->bIsDirty)
//        {
//            UE_LOG(LogDistributions, Log, TEXT("Somehow Distribution %s wasn't dirty, but its FRawDistribution wasn't ever initialized!"), *Distribution->GetFullName());
//        }
//        bNeedsUpdating = true;
//    }
//
//    // only initialize if we need to
//    if (!bNeedsUpdating)
//    {
//        return;
//    }
//    
//
//    // always empty out the lookup table
//    LookupTable.Empty();
//
//    // distribution is no longer dirty (if it was)
//    // template objects aren't marked as dirty, because any UDists that uses this as an archetype, 
//    // aren't the default values, and has already been saved, needs to know to build the FDist
//    if (!Distribution->IsTemplate())
//    {
//        Distribution->bIsDirty = false;
//    }
//
//    // if the distribution can't be baked out, then we do nothing here
//    if (!Distribution->CanBeBaked())
//    {
//        return;
//    }
//
//    // Build and optimize the lookup table.
//    BuildLookupTable(&LookupTable, Distribution);
//    const float MinIn = LookupTable.TimeBias;
//    const float MaxIn = MinIn + (LookupTable.EntryCount - 1) * (LookupTable.TimeScale == 0.0f ? 0.0f : (1.0f / LookupTable.TimeScale));
//    OptimizeLookupTable(&LookupTable, LOOKUP_TABLE_ERROR_THRESHOLD);
//
//}
//#endif

FVector FRawDistributionVector::GetValue(float F, UObject* Data, int32 Extreme, struct FRandomStream* InRandomStream)
{
    if (!HasLookupTable())
    {
        if (!Distribution)
        {
            return FVector::ZeroVector;
        }
        return Distribution->GetValue(F, Data, Extreme, InRandomStream);
    }

    // if we get here, we better have been initialized!
    if(LookupTable.IsEmpty()) return FVector::ZeroVector;

    FVector Value;
    FRawDistribution::GetValue3(F, &Value.X, Extreme, InRandomStream);
    return (FVector)Value;
}

const FRawDistribution* FRawDistributionVector::GetFastRawDistribution()
{
    if (!IsSimple() || !HasLookupTable())
    {
        return 0;
    }

    // if we get here, we better have been initialized!
    if (LookupTable.IsEmpty()) return nullptr;

    return this;
}

void FRawDistributionVector::GetOutRange(float& MinOut, float& MaxOut)
{
    if (!HasLookupTable() && Distribution)
    {
        if (!Distribution) return;
        //Distribution->GetOutRange(MinOut, MaxOut);
    }
    else
    {
        MinOut = MinValue;
        MaxOut = MaxValue;
    }
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

void FRawDistributionVector::InitLookupTable()
{
}

//void UDistributionVector::Serialize(FStructuredArchive::FRecord Record)
//{
//#if WITH_EDITOR
//    FArchive& UnderlyingArchive = Record.GetUnderlyingArchive();
//
//    if (UnderlyingArchive.IsCooking() || UnderlyingArchive.IsSaving())
//    {
//        bBakedDataSuccesfully = HasBakedDistributionDataHelper<FRawDistributionVector>(this);
//    }
//#endif
//
//    Super::Serialize(Record);
//}

FVector UDistributionVector::GetValue(float F, UObject* Data, int32 Extreme, struct FRandomStream* InRandomStream) const
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
    BuildZeroLookupTable(&LookupTable, 1);
}

FVectorDistribution::FVectorDistribution()
{
    BuildZeroLookupTable(&LookupTable, 3);
}

FVector4Distribution::FVector4Distribution()
{
    BuildZeroLookupTable(&LookupTable, 4);
}
