#pragma once
#include "CameraTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class APlayerCameraManager;
class UCameraShakePattern;

struct FCameraShakePatternUpdateParams
{
    FCameraShakePatternUpdateParams()
    {}

    FCameraShakePatternUpdateParams(FMinimalViewInfo InPOV)
        : POV(std::move(InPOV))
    {}

    /** The time elapsed since last update */
    float DeltaTime = 0.f;

    /** The base scale for this shake */
    float ShakeScale = 1.f;
    /** The dynamic scale being passed down from the camera manger for the next update */
    float DynamicScale = 1.f;

    /** The current view that this camera shake should modify */
    FMinimalViewInfo POV;

    /** The total scale to apply to the camera shake during the current update. Equals ShakeScale * DynamicScale */
    float GetTotalScale() const
    {
        return FMath::Max(ShakeScale * DynamicScale, 0.f);
    }
};

enum class ECameraShakeDurationType : uint8
{
	/** Camera shake has a fixed duration */
	Fixed,
	/** Camera shake is playing indefinitely, until explicitly stopped */
	Infinite,
	/** Camera shake has custom/dynamic duration */
	Custom
};

struct FCameraShakePatternUpdateResult
{
    FCameraShakePatternUpdateResult()
        : Location(FVector::ZeroVector)
        , Rotation(FRotator::ZeroRotator)
        , FOV(0.f)
    {}

    /** Location offset for the view, or new absolute location if ApplyAsAbsolute flag is set */
    FVector Location;
    /** Rotation offset for the view, or new absolute rotation if ApplyAsAbsolute flag is set */
    FRotator Rotation;
    /** Field-of-view offset for the view, or new absolute field-of-view if ApplyAsAbsolute flag is set */
    float FOV;
};

struct FCameraShakeInfo
{
    /** The duration of the camera shake */
    ECameraShakeDurationType DurationType;

    float Duration = 0.f;

    /** How much blending-in the camera shake should have */
    float BlendIn = 0.f;

    /** How much blending-out the camera shake should have */
    float BlendOut = 0.f;
};

struct FCameraShakeState
{
	/**
	 * Create a new camera shake state
	 */
	FCameraShakeState();

	/**
	 * Initialize the state with a shake's info and start playing.
	 */
	void Start(const UCameraShakePattern* InShakePattern);

	/**
	 * Updates the state with a delta time.
	 *
	 * If the state isn't managed (i.e. it doesn't have any fixed duration information), this doesn't
	 * do anything and returns 1.
	 *
	 * If the state is managed, this updates the internal state of this class and auto-computes when
	 * the shake has ended, along with a blending weight if there is any blending in/out.
	 *
	 * @param DeltaTime The elapsed time since last update
	 * @return The evaluated blending weight (if any) for the new time
	 */
	float Update(float DeltaTime);

	/**
	 * Scrub the state to the given absolute time.
	 *
	 * If the state isn't managed (i.e. it doesn't have any fixed duration information), this doesn't
	 * do anything and returns 1.
	 *
	 * If the state is managed, this updates the internal state of this class and auto-computes when
	 * the shake has ended, along with a blending weight if there is any blending in/out.
	 *
	 * @param AbsoluteTime The time to scrub to
	 * @return The evaluated blending weight (if any) for the scrub time
	 */
	float Scrub(float AbsoluteTime);

	/**
	 * Marks the shake has having been stopped.
	 *
	 * This renders the shake inactive (if we need to stop immediately), or starts the shake's blend-out,
	 * if any (if we don't stop immediately). If no duration or blending information is available (i.e. if
	 * the shake duration is "Custom"), stopping non-immediately does nothing: the sub-class is expected
	 * to handle it.
	 */
	void Stop(bool bImmediately);

	/** Returns whether the shake is playing */
	bool IsPlaying() const { return bIsPlaying; }

	/** Returns whether the shake is blending in */
	bool IsBlendingIn() const { return bIsBlendingIn; }

	/** Returns whether the shake is blending out */
	bool IsBlendingOut() const { return bIsBlendingOut; }

	/** Returns the elapsed time of the shake's current run */
	float GetElapsedTime() const { return ElapsedTime; }

	/** Returns the current time into the blend in (only valid if IsBlendingIn() returns true) */
	float GetCurrentBlendInTime() const { return CurrentBlendInTime; }

	/** Returns the current time into the blend in (only valid if IsBlendingOut() returns true) */
	float GetCurrentBlendOutTime() const { return CurrentBlendOutTime; }

	/** Returns the current shake info */
	const FCameraShakeInfo& GetShakeInfo() const { return ShakeInfo; }

	/** Helper method to get GetShakeInfo().Duration.IsFixed() */
	bool HasDuration() const { return ShakeInfo.DurationType != ECameraShakeDurationType::Infinite; }

	/** Helper method to get GetShakeInfo().Duration.Get() */
	float GetDuration() const { return ShakeInfo.Duration; }

	/** Helper method to get GetShakeInfo().Duration.IsInifnite() */
	bool IsInfinite() const { return ShakeInfo.DurationType == ECameraShakeDurationType::Infinite; }

private:
	void InitializePlaying();

private:

	// Information about the shake/shake pattern we're managing
	FCameraShakeInfo ShakeInfo;

	// Running state
	float ElapsedTime;

	float CurrentBlendInTime;
	float CurrentBlendOutTime;

	bool bIsBlendingIn : 1;
	bool bIsBlendingOut : 1;

	bool bIsPlaying : 1;

	// Cached values for blending information
	bool bHasBlendIn : 1;
	bool bHasBlendOut : 1;
};

class UCameraShakeBase : public UObject
{
    DECLARE_CLASS(UCameraShakeBase, UObject)

public:
    UCameraShakeBase();
    virtual ~UCameraShakeBase() override = default;

    void UpdateAndApplyCameraShake(float DeltaTime, float Alpha, FMinimalViewInfo& InOutPOV);

    void StartShake();
    
    void StopShake(bool bImmediately = true);

    float ShakeScale;

    UCameraShakePattern* GetRootShakePattern() const { return RootShakePattern; }

    void SetRootShakePattern(UCameraShakePattern* InPattern) { RootShakePattern = InPattern; }

    void ApplyResult(float Scale, const FCameraShakePatternUpdateResult& InResult, FMinimalViewInfo& InOutPOV);

    bool IsFinished() const;
    
private:
    UCameraShakePattern* RootShakePattern;

    APlayerCameraManager* CameraManager;

    bool bIsActive;
};

class UCameraShakePattern : public UObject
{
    DECLARE_CLASS(UCameraShakePattern, UObject)

public:
    UCameraShakePattern() = default;
    virtual ~UCameraShakePattern() override = default;

    void GetShakePatternInfo(FCameraShakeInfo& Info) const;
    /** Called when the shake pattern starts */
	void StartShakePattern();
	/** Updates the shake pattern, which should add its generated offset to the given result */
	void UpdateShakePattern(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult);
	/** Returns whether this shake pattern is finished */
	bool IsFinished() const;
	/** Called when the shake pattern is manually stopped */
	void StopShakePattern(bool bImmediately = true);

private:
	// UCameraShakePattern interface
    virtual void GetShakePatternInfoImpl(FCameraShakeInfo& OutInfo) const {}
	virtual void StartShakePatternImpl() {}
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& OutResult) {}
	virtual bool IsFinishedImpl() const { return true; }
	virtual void StopShakePatternImpl(bool bImmediately) {}
	virtual void TeardownShakePatternImpl()  {}
};
