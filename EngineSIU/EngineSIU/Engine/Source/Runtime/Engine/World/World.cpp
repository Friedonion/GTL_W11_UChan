#include "World.h"

#include "CollisionManager.h"
#include "Actors/Cube.h"
#include "Actors/Player.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Classes/Components/StaticMeshComponent.h"
#include "Components/SkySphereComponent.h"
#include "Engine/FObjLoader.h"
#include "Actors/HeightFogActor.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"
#include "UnrealEd/SceneManager.h"
#include "Actors/PointLightActor.h"
#include "Actors/SpotLightActor.h"
#include "GameFramework/GameMode.h"
#include "Classes/Components/TextComponent.h"
#include "Contents/Actors/Fish.h"

class UEditorEngine;

UWorld* UWorld::CreateWorld(UObject* InOuter, const EWorldType InWorldType, const FString& InWorldName)
{
    UWorld* NewWorld = FObjectFactory::ConstructObject<UWorld>(InOuter);
    NewWorld->WorldName = InWorldName;
    NewWorld->WorldType = InWorldType;
    NewWorld->InitializeNewWorld();
    
    return NewWorld;
}

void UWorld::InitializeNewWorld()
{
    ActiveLevel = FObjectFactory::ConstructObject<ULevel>(this);
    ActiveLevel->InitLevel(this);
    //InitializeLightScene(); // 테스트용 LightScene 비활성화

    CollisionManager = new FCollisionManager();
}

void UWorld::InitializeLightScene()
{
    const int TotalLights = 1000;        // 최대 개수
    const int HalfCountPerAxis = 0;    // -4 ~ +4. 9*9*9 개수만큼 생성
    const float Spacing = 20.0f;        // 오브젝트간의 간격
    const float JitterAmount = 100.0f;    // 랜덤 흔들림 정도. 

    //고정 시드 랜덤 오프셋. 매 실행마다 같은 결과.
    auto HashOffset = [JitterAmount](int x, int y, int z) -> FVector
        {
            // 단순한 LCG 기반 해시: 고정된 결과를 줌
            auto LCG = [](int seed) -> float {
                seed = (1103515245 * seed + 12345) & 0x7fffffff;
                return (seed % 1000) / 1000.0f; // 0.0 ~ 0.999
                };

            int seedBase = x * 73856093 ^ y * 19349663 ^ z * 83492791;
            float dx = (LCG(seedBase + 1) - 0.5f) * 2.0f * JitterAmount;
            float dy = (LCG(seedBase + 2) - 0.5f) * 2.0f * JitterAmount;
            float dz = (LCG(seedBase + 3) - 0.5f) * 2.0f * JitterAmount;

            return FVector(dx, dy, dz);
        };

    //그리드 기반 생성. 랜덤 오프셋 추가함.
    int LightCount = 0;
    for (int x = -HalfCountPerAxis; x <= HalfCountPerAxis; ++x)
    {
        for (int y = -HalfCountPerAxis; y <= HalfCountPerAxis; ++y)
        {
            for (int z = -HalfCountPerAxis; z <= HalfCountPerAxis; ++z)
            {
                if (x == 0 && y == 0 && z == 0)
                    continue;

                FVector basePos = FVector(x * Spacing, y * Spacing, z * Spacing);
                FVector jitter = HashOffset(x, y, z);        //랜덤 오프셋 추가.
                FVector finalPos = basePos+jitter;

                /*if (LightCount % 2 == 0)
                {
                    APointLight* PointLightActor = SpawnActor<APointLight>();
                    PointLightActor->SetActorLabel(FString::Printf(TEXT("OBJ_PointLight_%d"), LightCount));
                    PointLightActor->SetActorLocation(finalPos);
                }
                else*/
                {
                    //continue;
                    ASpotLight* SpotLightActor = SpawnActor<ASpotLight>();
                    SpotLightActor->SetActorLabel(FString::Printf(TEXT("OBJ_SpotLight_%d"), LightCount));
                    SpotLightActor->SetActorLocation(finalPos);
                }

                ++LightCount;
                UE_LOG(ELogLevel::Display,"LightCount %d", LightCount);
            }
        }
    }
}

UObject* UWorld::Duplicate(UObject* InOuter)
{
    // TODO: UWorld의 Duplicate는 역할 분리후 만드는것이 좋을듯
    UWorld* NewWorld = Cast<UWorld>(Super::Duplicate(InOuter));
    NewWorld->ActiveLevel = Cast<ULevel>(ActiveLevel->Duplicate(NewWorld));
    NewWorld->ActiveLevel->InitLevel(NewWorld);
    
    NewWorld->CollisionManager = new FCollisionManager();
    
    return NewWorld;
}

void UWorld::Tick(float DeltaTime)
{
    TimeSeconds += DeltaTime;
    
    // SpawnActor()에 의해 Actor가 생성된 경우, 여기서 BeginPlay 호출
    if (WorldType != EWorldType::Editor)
    {
        for (AActor* Actor : PendingBeginPlayActors)
        {
            Actor->BeginPlay();
        }
        PendingBeginPlayActors.Empty();
    }
}

void UWorld::BeginPlay()
{
    if (!GameMode && this->WorldType == EWorldType::PIE)
    {
        GameMode = this->SpawnActor<AGameMode>();
        GameMode->SetActorLabel(TEXT("OBJ_GAMEMODE"));
        GameMode->InitializeComponent();

        for (const auto Iter : TObjectRange<UTextComponent>())
        {
            if (Iter->GetWorld() == GEngine->ActiveWorld)
            {
                MainTextComponent = Iter;
            }
        }

        // GameMode Delegate addition [Need to delete later]
        GameMode->OnGameInit.AddLambda([this]() {
            if (MainTextComponent) {
                FVector Target = MainTextComponent->GetComponentLocation();
                Target.X -= 20.0f;
            
                MainTextComponent->SetText(L"Press Space to start");
                //GetMainCamera()->SetFInterpToSpeed(3.0f);
                //GetMainCamera()->SetFollowCustomTarget(Target);
                GetPlayerController()->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 4.0f, FLinearColor(.0f, .0f, .0f, 1.0f));
            }
        });

        GameMode->OnGameStart.AddLambda([this]() {
            //GetMainCamera()->SetFInterpToSpeed(0.8f);
            //GetMainCamera()->ResetFollowToPlayer();
            });

        GameMode->OnGameEnd.AddLambda([this](bool bIsWin) {
            if (MainTextComponent) {
//                FVector Target = MainTextComponent->GetWorldLocation();
//                FVector TextLoc = Target;
//                Target.X -= 20.0f;
//                Target.Z -= GetMainCamera()->CameraHeight;

//                GetMainCamera()->SetFollowCustomTarget(Target);
//                GetMainCamera()->SetLookTarget(TextLoc);
                
                if (bIsWin)
                {
                    AFish* Fish = Cast<AFish>(GEngine->ActiveWorld->GetMainPlayer());
                    
                    FString Message = FString::Printf(TEXT("Earned Coin %d"), Fish->GetScore());
                    MainTextComponent->SetText(Message.ToWideString());
                    //MainCamera->CameraZOffset = 0.0f;
                } else
                {
                    MainTextComponent->SetText(L"Fish roasted");
                    
                    AFish* Fish = Cast<AFish>(GEngine->ActiveWorld->GetMainPlayer());
                    Fish->SetActorLocation(FVector(-110.0f, 0.0f, -4.0f));
                    Fish->SetActorRotation(FRotator(70.0f, 0.0f, 0.0f));
                }
            }
            });

        GameMode->InitGame();
    }
    for (AActor* Actor : ActiveLevel->Actors)
    {
        if (Actor->GetWorld() == this)
        {
            Actor->BeginPlay();
        }
    }
}

void UWorld::Release()
{
    if (ActiveLevel)
    {
        ActiveLevel->Release();
        GUObjectArray.MarkRemoveObject(ActiveLevel);
        ActiveLevel = nullptr;
    }

    if (CollisionManager)
    {
        delete CollisionManager;
        CollisionManager = nullptr;
    }
    
    GUObjectArray.ProcessPendingDestroyObjects();
}

AActor* UWorld::SpawnActor(UClass* InClass, FName InActorName)
{
    if (!InClass)
    {
        UE_LOG(ELogLevel::Error, TEXT("SpawnActor failed: ActorClass is null."));
        return nullptr;
    }

    
    // TODO: SpawnParams에서 이름 가져오거나, 필요시 여기서 자동 생성
    // if (SpawnParams.Name != NAME_None) ActorName = SpawnParams.Name;
    
    if (InClass->IsChildOf<AActor>())
    {
        AActor* NewActor = Cast<AActor>(FObjectFactory::ConstructObject(InClass, this, InActorName));
        // TODO: 일단 AddComponent에서 Component마다 초기화
        // 추후에 RegisterComponent() 만들어지면 주석 해제
        // Actor->InitializeComponents();
        ActiveLevel->Actors.Add(NewActor);
        PendingBeginPlayActors.Add(NewActor);

        NewActor->PostSpawnInitialize();

        if (NewActor->GetRootComponent() == nullptr)
        {
            NewActor->SetRootComponent(NewActor->AddComponent<USceneComponent>());
        }
        
        return NewActor;
    }
    
    UE_LOG(ELogLevel::Error, TEXT("SpawnActor failed: Class '%s' is not derived from AActor."), *InClass->GetName());
    return nullptr;
}

bool UWorld::DestroyActor(AActor* ThisActor)
{
    if (ThisActor->GetWorld() == nullptr)
    {
        return false;
    }

    if (ThisActor->IsActorBeingDestroyed())
    {
        return true;
    }
    
    // UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    //
    // Engine->DeselectActor(ThisActor);

    // 액터의 Destroyed 호출
    ThisActor->Destroyed();

    if (ThisActor->GetOwner())
    {
        ThisActor->SetOwner(nullptr);
    }

    TSet<UActorComponent*> Components = ThisActor->GetComponents();
    for (UActorComponent* Component : Components)
    {
        Component->DestroyComponent();
    }

    // World에서 제거
    ActiveLevel->Actors.Remove(ThisActor);

    // 제거 대기열에 추가
    GUObjectArray.MarkRemoveObject(ThisActor);
    return true;
}

UWorld* UWorld::GetWorld() const
{
    return const_cast<UWorld*>(this);
}

APlayer* UWorld::GetMainPlayer() const
{
    if (MainPlayer)
    {
        return MainPlayer;
    }
    
    //메인플레이어 설정안하면 있는거중 한개
    for (const auto Iter: TObjectRange<APlayer>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            return Iter;
        }
    }
    
    return nullptr;
}

APlayerController* UWorld::GetPlayerController() const
{
    if (PlayerController)
    {
        return PlayerController;
    }

    //메인플레이어컨트롤러 설정안하면 있는거중 한개
    for (const auto Iter: TObjectRange<APlayerController>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            return Iter;
        }
    }

    return nullptr;
}

void UWorld::CheckOverlap(const UPrimitiveComponent* Component, TArray<FOverlapResult>& OutOverlaps) const
{
    if (CollisionManager)
    {
        CollisionManager->CheckOverlap(this, Component, OutOverlaps);
    }
}

