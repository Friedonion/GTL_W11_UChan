#pragma once
#include "SkinnedMeshComponent.h"
#include "Actors/Player.h"
#include "Engine/AssetManager.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "Template/SubclassOf.h"
#include "Animation/AnimNodeBase.h"

class UAnimSequence;
class USkeletalMesh;
class FAnimNotifyEvent;
class UAnimSequenceBase;
class UAnimInstance;
class UAnimSingleNodeInstance;

enum class EAnimationMode : uint8
{
    AnimationBlueprint,
    AnimationSingleNode,
};

class USkeletalMeshComponent : public USkinnedMeshComponent
{
    DECLARE_CLASS(USkeletalMeshComponent, USkinnedMeshComponent)

public:
    USkeletalMeshComponent();
    virtual ~USkeletalMeshComponent() override = default;

    virtual void InitializeComponent() override;

    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void TickComponent(float DeltaTime) override;

    virtual void TickPose(float DeltaTime) override;

    void TickAnimation(float DeltaTime);

    void TickAnimInstances(float DeltaTime);

    bool ShouldTickAnimation() const;

    bool InitializeAnimScriptInstance();

    void ClearAnimScriptInstance();

    USkeletalMesh* GetSkeletalMeshAsset() const { return SkeletalMeshAsset; }

    void SetSkeletalMeshAsset(USkeletalMesh* InSkeletalMeshAsset);

    FTransform GetSocketTransform(FName SocketName) const;

    TArray<FTransform> RefBonePoseTransforms; // 원본 BindPose에서 복사해온 에디팅을 위한 Transform

    void GetCurrentGlobalBoneMatrices(TArray<FMatrix>& OutBoneMatrices) const;

    void DEBUG_SetAnimationEnabled(bool bEnable);

    void PlayAnimation(UAnimationAsset* NewAnimToPlay, bool bLooping);

    void SetAnimation(UAnimationAsset* NewAnimToPlay);

    UAnimationAsset* GetAnimation() const;

    void Play(bool bLooping);

    void Stop();

    void SetPlaying(bool bPlaying);
    
    bool IsPlaying() const;

    void SetReverse(bool bIsReverse);
    
    bool IsReverse() const;

    void SetPlayRate(float Rate);

    float GetPlayRate() const;

    void SetLooping(bool bIsLooping);

    bool IsLooping() const;

    int GetCurrentKey() const;

    void SetCurrentKey(int InKey);

    void SetElapsedTime(float InElapsedTime);

    float GetElapsedTime() const;

    int32 GetLoopStartFrame() const;

    void SetLoopStartFrame(int32 InLoopStartFrame);

    int32 GetLoopEndFrame() const;

    void SetLoopEndFrame(int32 InLoopEndFrame);
    
    bool bIsAnimationEnabled() const { return bPlayAnimation; }
    
    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;

    const FSkeletalMeshRenderData* GetCPURenderData() const;

    static void SetCPUSkinning(bool Flag);

    static bool GetCPUSkinning();

    UAnimInstance* GetAnimInstance() const { return AnimScriptInstance; }

    void SetAnimationMode(EAnimationMode InAnimationMode);

    EAnimationMode GetAnimationMode() const { return AnimationMode; }

    virtual void InitAnim();
    
protected:
    bool NeedToSpawnAnimScriptInstance() const;

    EAnimationMode AnimationMode;
    
private:
    FPoseContext BonePoseContext;
    
    USkeletalMesh* SkeletalMeshAsset;

    bool bPlayAnimation;

    std::unique_ptr<FSkeletalMeshRenderData> CPURenderData;

    static bool bIsCPUSkinning;

    void CPUSkinning(bool bForceUpdate = false);

public:
    TSubclassOf<UAnimInstance> AnimClass;
    
    UAnimInstance* AnimScriptInstance;

    UAnimSingleNodeInstance* GetSingleNodeInstance() const;

    void SetAnimClass(UClass* NewClass);
    
    UClass* GetAnimClass();
    
    void SetAnimInstanceClass(class UClass* NewClass);
};
