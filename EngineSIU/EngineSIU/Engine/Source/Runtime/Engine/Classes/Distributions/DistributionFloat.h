// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once

#include "UObject/ObjectMacros.h"
#include "Distributions.h"
#include "Distributions/Distribution.h"

struct FPropertyChangedEvent;

//UCLASS(abstract, customconstructor, MinimalAPI)
class UDistributionFloat : public UDistribution
{
    DECLARE_CLASS(UDistributionFloat, UDistribution)
public:
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, float, Constant)

    /** Set internally when the distribution is updated so that that FRawDistribution can know to update itself*/
    uint8 bIsDirty : 1;

protected:
    //UPROPERTY()
    uint8 bBakedDataSuccesfully : 1;	//It's possible that even though we want to bake we are not able to because of content or code.

public:
    /** Script-accessible way to query a float distribution */
    virtual float GetFloatValue(float F = 0);

    UDistributionFloat()
        : Constant(0)
        , bIsDirty(true) // make sure the FRawDistribution is initialized
    {
    }

    //@todo.CONSOLE: Currently, consoles need this? At least until we have some sort of cooking/packaging step!
    /**
     * Return the operation used at runtime to calculate the final value
     */
    virtual ERawDistributionOperation GetOperation() const { return ERawDistributionOperation::RDO_None; }

    /**
     *  Returns the lock axes flag used at runtime to swizzle random stream values. Not used for distributions derived from UDistributionFloat.
     */
    virtual uint8 GetLockFlag() const { return 0; }

    /**
     * Fill out an array of floats and return the number of elements in the entry
     *
     * @param Time The time to evaluate the distribution
     * @param Values An array of values to be filled out, guaranteed to be big enough for 4 values
     * @return The number of elements (values) set in the array
     */
    virtual uint32 InitializeRawEntry(float Time, float* Values) const;

    /** @todo document */
    virtual float GetValue(float F = 0.f, struct FRandomStream* InRandomStream = NULL) const;

    /**
     * Returns the number of values in the distribution. 1 for float.
     */
    int32 GetValueCount() const
    {
        return 1;
    }

    /** Begin UObject interface */
    /*virtual bool NeedsLoadForClient() const override;
    virtual bool NeedsLoadForServer() const override;
    virtual bool NeedsLoadForEditorGame() const override;
    virtual void Serialize(FStructuredArchive::FRecord Record) override;*/
    /** End UObject interface */
};


//USTRUCT()
struct FRawDistributionFloat : public FRawDistribution
{
    DECLARE_STRUCT_WITH_SUPER(FRawDistributionFloat, FRawDistribution)
    //GENERATED_USTRUCT_BODY()
public:
    //UPROPERTY()
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, float, MinValue)

    //UPROPERTY()
    UPROPERTY_WITH_FLAGS
    (EditAnywhere, float, MaxValue)

public:
    //UPROPERTY(EditAnywhere, export, noclear, Category = RawDistributionFloat)
    UPROPERTY_WITH_FLAGS
    (VisibleAnywhere, UDistributionFloat*, Distribution)

    FRawDistributionFloat()
        : MinValue(0)
        , MaxValue(0)
        , Distribution(nullptr)
    {
    }

    /** Whether the distribution data has been cooked or the object itself is available */
    bool IsCreated();

    /**
        * Gets a pointer to the raw distribution if you can just call FRawDistribution::GetValue1 on it, otherwise NULL
        */
    const FRawDistribution* GetFastRawDistribution();

    /**
        * Get the value at the specified F
        */
    float GetValue(float F = 0.0f, struct FRandomStream* InRandomStream = NULL);

    /**
        * Get the min and max values
        */
    void GetOutRange(float& MinOut, float& MaxOut);
};
