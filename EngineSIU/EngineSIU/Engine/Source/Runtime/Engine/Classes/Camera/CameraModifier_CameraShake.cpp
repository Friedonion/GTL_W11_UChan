
#include "CameraModifier_CameraShake.h"

#include "CameraShakeBase.h"
#include "UObject/ObjectFactory.h"
#include "World/World.h"

UCameraShakeBase* UCameraModifier_CameraShake::AddCameraShake(UClass* ShakeClass)
{
    if (!ShakeClass || !ShakeClass->IsChildOf(UCameraShakeBase::StaticClass()))
    {
        return nullptr;
    }
    
    UCameraShakeBase* NewInst = ReclaimShakeFromExpiredPool(ShakeClass);
    if (NewInst == nullptr)
    {
        NewInst = Cast<UCameraShakeBase>(FObjectFactory::ConstructObject(ShakeClass, GetWorld()));
    }

    if (NewInst)
    {
        NewInst->StartShake();

        bool bReplacedNull = false;
        for (int32 Idx = 0; Idx < ActiveShakes.Num(); ++Idx)
        {
            if (ActiveShakes[Idx] == nullptr)
            {
                ActiveShakes[Idx] = NewInst;
                bReplacedNull = true;
                break;
            }
        }

        if (!bReplacedNull)
        {
            ActiveShakes.Emplace(NewInst);
        }
    }

    return NewInst;
}

void UCameraModifier_CameraShake::RemoveCameraShake(UCameraShakeBase* ShakeInst, bool bImmediate)
{
    for (int32 i = 0; i < ActiveShakes.Num(); ++i)
    {
        if (ActiveShakes[i] == ShakeInst)
        {
            ShakeInst->StopShake(bImmediate);
            if (bImmediate)
            {
                SaveShakeInExpiredPool(ShakeInst);
                ActiveShakes.RemoveAt(i);
            }
            break;
        }
    }
}

void UCameraModifier_CameraShake::RemoveAllCameraShakesOfClass(UClass* ShakeClass, bool bImmediately)
{
    for (int32 i = ActiveShakes.Num()- 1; i >= 0; --i)
    {
        UCameraShakeBase* ShakeInst = ActiveShakes[i];
        if (ShakeInst != nullptr && (ShakeInst->GetClass()->IsChildOf(ShakeClass)))
        {
            ShakeInst->StopShake(bImmediately);
            if (bImmediately)
            {
                SaveShakeInExpiredPool(ShakeInst);
                ActiveShakes.RemoveAt(i);
            }
        }
    }
}

void UCameraModifier_CameraShake::RemoveAllCameraShakes(bool bImmediately)
{
    // Clean up any active camera shake anims
    for (UCameraShakeBase* Shake : ActiveShakes)
    {
        Shake->StopShake(bImmediately);
    }

    if (bImmediately)
    {
        for (UCameraShakeBase* Shake : ActiveShakes)
        {
            // Shake->TeardownShake();
            SaveShakeInExpiredPool(Shake);
        }

        // Clear ActiveShakes array
        ActiveShakes.Empty();
    }
}

bool UCameraModifier_CameraShake::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
    UCameraModifier::ModifyCamera(DeltaTime, InOutPOV);

    if (Alpha <= 0.f)
    {
        return false;
    }

    if (ActiveShakes.Num() > 0)
    {
        for (UCameraShakeBase* Shake : ActiveShakes)
        {
            if (Shake)
            {
                Shake->UpdateAndApplyCameraShake(DeltaTime, Alpha, InOutPOV);
            }
        }

        // Delete any obsolete shakes
        for (int32 i = ActiveShakes.Num() - 1; i >= 0; i--)
        {
            if (ActiveShakes[i] == nullptr || ActiveShakes[i]->IsFinished())
            {
                UCameraShakeBase* Shake = ActiveShakes[i];
                ActiveShakes.RemoveAt(i);
                SaveShakeInExpiredPool(Shake);
            }
        }
    }

    return true;
}

void UCameraModifier_CameraShake::SaveShakeInExpiredPool(UCameraShakeBase* ShakeInst)
{
    TArray<UCameraShakeBase*>& PooledCameraShakes = ExpiredPooledShakesMap.FindOrAdd(ShakeInst->GetClass());
    if (PooledCameraShakes.Num() < 5)
    {
        PooledCameraShakes.Add(ShakeInst);
    }
}

UCameraShakeBase* UCameraModifier_CameraShake::ReclaimShakeFromExpiredPool(UClass* ShakeClass)
{
    if (TArray<UCameraShakeBase*>* PooledCameraShakes = ExpiredPooledShakesMap.Find(ShakeClass))
    {
        if (!PooledCameraShakes->IsEmpty())
        {
            UCameraShakeBase* OldShake = PooledCameraShakes->Pop();
            OldShake->StartShake();
            return OldShake;
        }
    }
    return nullptr;
}
