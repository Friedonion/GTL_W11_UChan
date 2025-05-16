#include "ProjectileMovementComponent.h"
#include "GameFramework/Actor.h"

UProjectileMovementComponent::UProjectileMovementComponent()
{
    InitialSpeed = 0;
    MaxSpeed = 0;
    Gravity = 0.f;
    Velocity = FVector(0.f, 0.f, 0.f);
    ProjectileLifetime = 10.0f; // 기본 생명주기 설정
    AccumulatedTime = 0;
}

UObject* UProjectileMovementComponent::Duplicate(UObject* InOuter)
{

    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewComponent->ProjectileLifetime = ProjectileLifetime;
    NewComponent->AccumulatedTime = AccumulatedTime;
    NewComponent->InitialSpeed = InitialSpeed;
    NewComponent->MaxSpeed = MaxSpeed;
    NewComponent->Gravity = Gravity;
    NewComponent->Velocity = Velocity;

    return NewComponent;
    
}

void UProjectileMovementComponent::BeginPlay()
{
    FVector Forward = GetOwner()->GetActorForwardVector();
    Velocity = Forward * InitialSpeed;
}

void UProjectileMovementComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    Velocity.Z += Gravity * DeltaTime;

    if (Velocity.Length() > MaxSpeed)
    {
        Velocity = Velocity.GetSafeNormal() * MaxSpeed;
    }
    if (GetOwner())
    {
        FVector NewLocation = GetOwner()->GetRootComponent()->GetRelativeLocation() + Velocity * DeltaTime;
        GetOwner()->GetRootComponent()->SetRelativeLocation(NewLocation);
    }

    //ToDo : PIE모드 진입 후에도 PickedActor를 유지했을 때 예외발생할 수 있음.
    AccumulatedTime += DeltaTime;
    if (AccumulatedTime >= ProjectileLifetime)
    {
        if (GetOwner())
        {
            GetOwner()->Destroy();
        }
    }
}

void UProjectileMovementComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);
    OutProperties.Add(TEXT("ProjectileLifetime"), FString::Printf(TEXT("%f"), ProjectileLifetime));
    OutProperties.Add(TEXT("AccumulatedTime"), FString::Printf(TEXT("%f"), AccumulatedTime));
    OutProperties.Add(TEXT("InitialSpeed"), FString::Printf(TEXT("%f"), InitialSpeed));
    OutProperties.Add(TEXT("MaxSpeed"), FString::Printf(TEXT("%f"), MaxSpeed));
    OutProperties.Add(TEXT("Gravity"), FString::Printf(TEXT("%f"), Gravity));
    OutProperties.Add(TEXT("Velocity"), Velocity.ToString());
    
    
}

void UProjectileMovementComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);
    const FString* TempStr = nullptr;
    TempStr = InProperties.Find(TEXT("ProjectileLifetime"));
    if (TempStr)
    {
        ProjectileLifetime = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("AccumulatedTime"));
    if (TempStr)
    {
        AccumulatedTime = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("InitialSpeed"));
    if (TempStr)
    {
        InitialSpeed = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("MaxSpeed"));
    if (TempStr)
    {
        MaxSpeed = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Gravity"));
    if (TempStr)
    {
        Gravity = FString::ToFloat(*TempStr);
    }
    TempStr = InProperties.Find(TEXT("Velocity"));
    if (TempStr)
    {
        Velocity.InitFromString(*TempStr);
    }
    
}
