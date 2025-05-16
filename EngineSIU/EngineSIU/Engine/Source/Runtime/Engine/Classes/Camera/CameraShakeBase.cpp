
#include "CameraShakeBase.h"

#include "Math/Quat.h"
#include "UObject/UObjectArray.h"

FCameraShakeState::FCameraShakeState()
    : ElapsedTime(0.f)
    , CurrentBlendInTime(0.f)
    , CurrentBlendOutTime(0.f)
    , bIsBlendingIn(false)
    , bIsBlendingOut(false)
    , bIsPlaying(false)
    , bHasBlendIn(false)
    , bHasBlendOut(false)
{
}

void FCameraShakeState::Start(const UCameraShakePattern* InShakePattern)
{
    const FCameraShakeState PrevState(*this);
    
    FCameraShakeInfo Info;
    InShakePattern->GetShakePatternInfo(Info);

    ShakeInfo = Info;
    bHasBlendIn = Info.BlendIn > 0.f;
    bHasBlendOut = Info.BlendOut > 0.f;

    InitializePlaying();

    if (PrevState.bIsPlaying)
    {
        // Single instance shake is being restarted... let's see if we need to
        // reverse a blend-out into a blend-in.
        if (bHasBlendIn && PrevState.bHasBlendOut && PrevState.bIsBlendingOut)
        {
            // We had started blending out... let's start at an equivalent weight into the blend in.
            CurrentBlendInTime = ShakeInfo.BlendIn * (1.f - PrevState.CurrentBlendOutTime / PrevState.ShakeInfo.BlendOut);
        }
        else if (bHasBlendIn && !PrevState.bHasBlendOut)
        {
            // We have no blend out, so we were still at 100%. But we don't want to suddenly drop 
            // to 0% at the beginning of the blend in, so we skip the blend-in.
            bIsBlendingIn = false;
        }
    }
}

float FCameraShakeState::Update(float DeltaTime)
{
    // If we have duration information for our shake, we can do all the time-keeping stuff ourselves.
    // This includes figuring out if the shake is finished, and what kind of blend in/out weight
    // we should apply.
    float BlendingWeight = 1.f;

    // Advance shake and blending times.
    ElapsedTime += DeltaTime;
    if (bIsBlendingIn)
    {
        CurrentBlendInTime += DeltaTime;
    }
    if (bIsBlendingOut)
    {
        CurrentBlendOutTime += DeltaTime;
    }

    // Advance progress into the shake.
    if (HasDuration())
    {
        const float ShakeDuration = ShakeInfo.Duration;
        if (ElapsedTime < 0.f || ElapsedTime >= ShakeDuration)
        {
            // The shake has ended, or hasn't started yet (which can happen if we update backwards)
            bIsPlaying = false;
            return 0.f;
        }

        const float DurationRemaining = (ShakeDuration - ElapsedTime);
        if (bHasBlendOut && !bIsBlendingOut && DurationRemaining < ShakeInfo.BlendOut)
        {
            // We started blending out.
            bIsBlendingOut = true;
            CurrentBlendOutTime = (ShakeInfo.BlendOut - DurationRemaining);
        }
    }

    // Compute blend-in and blend-out weight.
    if (bHasBlendIn && bIsBlendingIn)
    {
        if (CurrentBlendInTime < ShakeInfo.BlendIn)
        {
            BlendingWeight *= (CurrentBlendInTime / ShakeInfo.BlendIn);
        }
        else
        {
            // Finished blending in!
            bIsBlendingIn = false;
            CurrentBlendInTime = ShakeInfo.BlendIn;
        }
    }
    if (bHasBlendOut && bIsBlendingOut)
    {
        if (CurrentBlendOutTime < ShakeInfo.BlendOut)
        {
            BlendingWeight *= (1.f - CurrentBlendOutTime / ShakeInfo.BlendOut);
        }
        else
        {
            // Finished blending out!
            bIsBlendingOut = false;
            CurrentBlendOutTime = ShakeInfo.BlendOut;
            // We also end the shake itself. In most cases we would have hit the similar case
            // above already, but if we have an infinite shake we have no duration to reach the end
            // of so we only finish here.
            bIsPlaying = false;
            return 0.f;
        }
    }
    return BlendingWeight;
}

float FCameraShakeState::Scrub(float AbsoluteTime)
{
    // Reset the state to active, at the beginning, and update from there.
    InitializePlaying();
    return Update(AbsoluteTime);
}

void FCameraShakeState::Stop(bool bImmediately)
{
    // For stopping immediately, we don't do anything besides render the shake inactive.
    if (bImmediately || !bHasBlendOut)
    {
        bIsPlaying = false;
    }
    // For stopping with a "graceful" blend-out:
    // - If we are already blending out, let's keep doing that and not change anything.
    // - If we are not, let's start blending out.
    else if (bHasBlendOut && !bIsBlendingOut)
    {
        bIsBlendingOut = true;
        CurrentBlendOutTime = 0.f;
    }
}

void FCameraShakeState::InitializePlaying()
{
    ElapsedTime = 0.f;
    CurrentBlendInTime = 0.f;
    CurrentBlendOutTime = 0.f;
    bIsBlendingIn = bHasBlendIn;
    bIsBlendingOut = false;
    bIsPlaying = true;
}

UCameraShakeBase::UCameraShakeBase()
    : ShakeScale(1.0f)
    , RootShakePattern(nullptr)
    , CameraManager(nullptr)
    , bIsActive(true)
{
}

void UCameraShakeBase::UpdateAndApplyCameraShake(float DeltaTime, float Alpha, FMinimalViewInfo& InOutPOV)
{
    if (RootShakePattern)
    {
        FCameraShakePatternUpdateParams Params(InOutPOV);
        Params.DeltaTime = DeltaTime;
        Params.ShakeScale = ShakeScale;
        Params.DynamicScale = Alpha;

        FCameraShakePatternUpdateResult Result;

        // 쉐이크 업데이트
        RootShakePattern->UpdateShakePattern(Params, Result);

        if (!RootShakePattern->IsFinished())
        {
            // 쉐이크 적용
            ApplyResult(Params.GetTotalScale(), Result, InOutPOV);
        }
    }
}

void UCameraShakeBase::StartShake()
{
    bIsActive = true;

    if (RootShakePattern)
    {
        RootShakePattern->StartShakePattern();
    }
}

void UCameraShakeBase::StopShake(bool bImmediately)
{
    bIsActive = false;
    
    if (RootShakePattern)
    {
        RootShakePattern->StopShakePattern(bImmediately);
    }
}

void UCameraShakeBase::ApplyResult(float Scale, const FCameraShakePatternUpdateResult& InResult, FMinimalViewInfo& InOutPOV)
{
    // InResult를 기반으로 InOutPOV 업데이트
    InOutPOV.Location += InResult.Location * Scale;
    InOutPOV.FOV += InResult.FOV * Scale;

    FRotator ScaledDeltaRotator = InResult.Rotation;
    /*
    ScaledDeltaRotator.Pitch *= Scale;
    ScaledDeltaRotator.Yaw *= Scale;
    ScaledDeltaRotator.Roll *= Scale;
    */
    
    FQuat CurrentRotation = InOutPOV.Rotation.Quaternion();
    FQuat DeltaRotation = ScaledDeltaRotator.Quaternion();

    FQuat FinalRotationQuat = DeltaRotation * CurrentRotation; // TODO: 앞 뒤 바꿔야 하는지 생각하기.
    FinalRotationQuat.Normalize();
    
    InOutPOV.Rotation = FinalRotationQuat.Rotator();
}

bool UCameraShakeBase::IsFinished() const
{
    if (RootShakePattern)
    {
        return RootShakePattern->IsFinished();
    }
    return true;
}

void UCameraShakePattern::GetShakePatternInfo(FCameraShakeInfo& Info) const
{
    GetShakePatternInfoImpl(Info);
}

void UCameraShakePattern::StartShakePattern()
{
    StartShakePatternImpl();
}

void UCameraShakePattern::UpdateShakePattern(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult)
{
    UpdateShakePatternImpl(Params, OutResult);
}

bool UCameraShakePattern::IsFinished() const
{
    return IsFinishedImpl();
}

void UCameraShakePattern::StopShakePattern(bool bImmediately)
{
    StopShakePatternImpl(bImmediately);
}
