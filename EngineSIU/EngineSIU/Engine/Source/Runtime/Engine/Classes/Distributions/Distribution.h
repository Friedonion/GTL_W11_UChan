#pragma once

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Distributions.h"

extern uint32 GDistributionType;

//UENUM()
enum DistributionParamMode : int
{
    DPM_Normal,
    DPM_Abs,
    DPM_Direct,
    DPM_MAX,
};

//UCLASS(DefaultToInstanced, collapsecategories, hidecategories = Object, editinlinenew, abstract, MinimalAPI)
class UDistribution : public UObject
{
    DECLARE_CLASS(UDistribution, UObject)
public:
    UDistribution();
    ~UDistribution();

    /** Default value for initializing and checking correct values on UDistributions. */
    static const float DefaultValue;

    //~ Begin UObject Interface. 
    //virtual bool IsPostLoadThreadSafe() const override;
    //~ End UObject Interface
};

