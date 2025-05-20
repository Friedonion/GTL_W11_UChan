#pragma once

#include "UObject/ObjectMacros.h"
#include "Distributions.h"
#include "Distributions/Distribution.h"

struct FPropertyChangedEvent;

//UENUM()
enum EDistributionVectorLockFlags : int
{
    EDVLF_None,
    EDVLF_XY,
    EDVLF_XZ,
    EDVLF_YZ,
    EDVLF_XYZ,
    EDVLF_MAX,
};

//UENUM()
enum EDistributionVectorMirrorFlags : int
{
    EDVMF_Same,
    EDVMF_Different,
    EDVMF_Mirror,
    EDVMF_MAX,
};

//UCLASS(abstract, customconstructor, MinimalAPI)
class UDistributionVector : public UDistribution
{
    DECLARE_CLASS(UDistributionVector, UDistribution)
public:
    FVector Constant;

    /** Set internally when the distribution is updated so that that FRawDistribution can know to update itself*/
    //UPROPERTY()
    uint8 bIsDirty : 1;
protected:
    //UPROPERTY()
    uint8 bBakedDataSuccesfully : 1;	//It's possible that even though we want to bake we are not able to because of content or code.
public:

    /** Script-accessible way to query a FVector distribution */
    virtual FVector GetVectorValue(float F = 0);


    UDistributionVector()
        : Constant(FVector::ZeroVector)
        , bIsDirty(true) // make sure the FRawDistribution is initialized
    {
    }

    //@todo.CONSOLE: Currently, consoles need this? At least until we have some sort of cooking/packaging step!
    /**
     * Return the operation used at runtime to calculate the final value
     */
    virtual ERawDistributionOperation GetOperation() const { return RDO_None; }

    /**
     * Returns the lock axes flag used at runtime to swizzle random stream values.
     */
    virtual uint8 GetLockFlag() const { return 0; }

    /**
     * Fill out an array of vectors and return the number of elements in the entry
     *
     * @param Time The time to evaluate the distribution
     * @param Values An array of values to be filled out, guaranteed to be big enough for 2 vectors
     * @return The number of elements (values) set in the array
     */
    virtual uint32 InitializeRawEntry(float Time, float* Values) const;

    virtual FVector	GetValue(float F = 0.f, int32 LastExtreme = 0, struct FRandomStream* InRandomStream = NULL) const;

    /**
     * Returns the number of values in the distribution. 3 for vector.
     */
    int32 GetValueCount() const
    {
        return 3;
    }

    /** Begin UObject interface */
    /*virtual bool NeedsLoadForClient() const override;
    virtual bool NeedsLoadForServer() const override;
    virtual bool NeedsLoadForEditorGame() const override;
    virtual void Serialize(FStructuredArchive::FRecord Record) override;*/
    /** End UObject interface */
};


//USTRUCT()
struct FRawDistributionVector : public FRawDistribution
{
private:
    //UPROPERTY()
    float MinValue;

    //UPROPERTY()
    float MaxValue;

    //UPROPERTY()
    FVector MinValueVec;

    //UPROPERTY()
    FVector MaxValueVec;

public:
    //UPROPERTY(EditAnywhere, export, noclear, Category = RawDistributionVector)
    UDistributionVector* Distribution;

    /** Whether the distribution data has been cooked or the object itself is available */
    bool IsCreated();

    FRawDistributionVector()
        : MinValue(0)
        , MaxValue(0)
        , MinValueVec(FVector::ZeroVector)
        , MaxValueVec(FVector::ZeroVector)
        , Distribution(NULL)
    {
    }

    /**
    * Gets a pointer to the raw distribution if you can just call FRawDistribution::GetValue3 on it, otherwise NULL
    */
    const FRawDistribution* GetFastRawDistribution();

    /**
    * Get the value at the specified F
    */
    FVector GetValue(float F = 0.0f, int32 LastExtreme = 0, struct FRandomStream* InRandomStream = NULL);

    /**
    * Get the min and max values
    */
    void GetOutRange(float& MinOut, float& MaxOut);

    /**
    * Get the min and max values
    */
    void GetRange(FVector& MinOut, FVector& MaxOut);
};
