#pragma once
#include "ActorComponent.h"
#include "Math/Rotator.h"
#include "UObject/ObjectMacros.h"
#include "Math/Transform.h"

struct FHitResult;
struct FOverlapInfo;

class USceneComponent : public UActorComponent
{
    DECLARE_CLASS(USceneComponent, UActorComponent)

public:
    USceneComponent();

    virtual UObject* Duplicate(UObject* InOuter) override;
    
    void GetProperties(TMap<FString, FString>& OutProperties) const override;
    void SetProperties(const TMap<FString, FString>& InProperties) override;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;
    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const;
    virtual void DestroyComponent(bool bPromoteChildren = false) override;

    FVector GetForwardVector() const;
    FVector GetRightVector() const;
    FVector GetUpVector() const;
    
    void AddLocation(const FVector& InAddValue);
    void AddRotation(const FRotator& InAddValue);
    void AddRotation(const FQuat& InAddValue);
    void AddScale(const FVector& InAddValue);

    USceneComponent* GetAttachParent() const { return AttachParent; }
    const TArray<USceneComponent*>& GetAttachChildren() const { return AttachChildren; }

    void AttachToComponent(USceneComponent* InParent);
    void SetupAttachment(USceneComponent* InParent);
    void DetachFromComponent(USceneComponent* Target);
    
public:
    void SetRelativeLocation(const FVector& InLocation) { RelativeLocation = InLocation; }
    void SetRelativeRotation(const FRotator& InRotation);
    void SetRelativeRotation(const FQuat& InQuat);
    void SetRelativeScale3D(const FVector& InScale) { RelativeScale3D = InScale; }
    void SetRelativeTransform(const FTransform& InTransform);
    
    FVector GetRelativeLocation() const { return RelativeLocation; }
    FRotator GetRelativeRotation() const { return RelativeRotation; }
    FVector GetRelativeScale3D() const { return RelativeScale3D; }
    FTransform GetRelativeTransform() const;

    void SetWorldLocation(const FVector& InLocation);
    void SetWorldRotation(const FRotator& InRotation);
    void SetWorldRotation(const FQuat& InQuat);
    
    void SetWorldScale3D(const FVector& InScale);
    void SetWorldTransform(const FTransform& InTransform);
    
    FVector GetComponentLocation() const;
    FRotator GetComponentRotation() const;
    FVector GetComponentScale3D() const;
    FTransform GetComponentTransform() const;

    FMatrix GetScaleMatrix() const;
    FMatrix GetRotationMatrix() const;
    FMatrix GetTranslationMatrix() const;

    FMatrix GetWorldMatrix() const;

    void UpdateOverlaps(const TArray<FOverlapInfo>* PendingOverlaps = nullptr, bool bDoNotifies = true, const TArray<const FOverlapInfo>* OverlapsAtEndLocation = nullptr);

    bool MoveComponent(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit = nullptr);
    bool MoveComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* OutHit = nullptr);

protected:
    /** 부모 컴포넌트로부터 상대적인 위치 */
    UPROPERTY_WITH_FLAGS
    (EPropertyFlags::EditAnywhere, FVector, RelativeLocation)

    /** 부모 컴포넌트로부터 상대적인 회전 */
    UPROPERTY_WITH_FLAGS
    (EPropertyFlags::EditAnywhere, FRotator, RelativeRotation)

    /** 부모 컴포넌트로부터 상대적인 크기 */
    UPROPERTY_WITH_FLAGS
    (EPropertyFlags::EditAnywhere, FVector, RelativeScale3D)

    UPROPERTY
    (USceneComponent*, AttachParent, = nullptr)

    UPROPERTY
    (TArray<USceneComponent*>, AttachChildren)

    virtual void UpdateOverlapsImpl(const TArray<FOverlapInfo>* PendingOverlaps = nullptr, bool bDoNotifies = true, const TArray<const FOverlapInfo>* OverlapsAtEndLocation = nullptr);

    virtual bool MoveComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit = nullptr);

public:
    bool IsUsingAbsoluteRotation() const;
    void SetUsingAbsoluteRotation(const bool bInAbsoluteRotation);
protected:
    uint8 bAbsoluteRotation : 1;
    
private:
    // TODO: 캐싱해서 사용하기
    bool bComponentToWorldUpdated = true;

    FTransform ComponentToWorld;
};
