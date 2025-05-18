#pragma once

#include "Core/HAL/PlatformType.h"
#include <ctime>

#include "UObject/NameTypes.h"

struct FRandomStream
{
public:
    FRandomStream()
        : InitialSeed(0)
        , Seed(0)
    {
    }
    
    FRandomStream(int32 InSeed)
    {
        Initialize(InSeed);
    }

    FRandomStream(const char* InName)
    {
        Initialize(SimpleHash(InName));
    }

    void Initialize(int32 InSeed)
    {
        InitialSeed = InSeed;
        Seed = (uint32)InSeed;
    }

    void Reset() const
    {
        Seed = (uint32)InitialSeed;
    }

    void GenerateNewSeed()
    {
        Initialize((int32)std::time(nullptr));
    }

    float GetFraction() const
    {
        MutateSeed();
        return (float)(Seed & 0x7FFFFFFF) / (float)0x7FFFFFFF;
    }

    uint32 GetUnsignedInt() const
    {
        MutateSeed();
        return Seed;
    }

    int32 GetCurrentSeed() const
    {
        return (int32)Seed;
    }

    float FRand() const
    {
        return GetFraction();
    }

    int32 RandHelper(int32 A) const
    {
        return ((A > 0) ? (int32)(GetFraction() * A) : 0);
    }

    int32 RandRange(int32 Min, int32 Max) const
    {
        const int32 Range = (Max - Min) + 1;
        return Min + RandHelper(Range);
    }

    float FRandRange(float InMin, float InMax) const
    {
        return InMin + (InMax - InMin) * FRand();
    }

protected:
    void MutateSeed() const
    {
        Seed = (Seed * 196314165u) + 907633515u;
    }

    int32 SimpleHash(const char* Str) const
    {
        int32 Hash = 0;
        if (!Str) return 0;

        while (*Str)
        {
            Hash = (Hash << 5) - Hash + *Str++;
        }
        return Hash;
    }

private:
    int32 InitialSeed;
    mutable uint32 Seed;
};
