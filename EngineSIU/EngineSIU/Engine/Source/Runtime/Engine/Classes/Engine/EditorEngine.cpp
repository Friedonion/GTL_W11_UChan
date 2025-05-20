#include "EditorEngine.h"

#include "FbxLoader.h"
#include "FObjLoader.h"
#include "World/World.h"
#include "Level.h"
#include "Animation/SkeletalMeshActor.h"
#include "GameFramework/Actor.h"
#include "Classes/Engine/AssetManager.h"
#include "Contents/Actors/SkeletalMeshActorTest.h"
#include "UObject/UObjectIterator.h"
#include "Animation/SkeletalMeshActor.h"
#include "Actors/DirectionalLightActor.h"
#include "Actors/ParticleSystemActor.h"
#include "Components/ParticleSystemComponent.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Editor/UnrealEd/EditorViewportClient.h"
#include "UnrealEd/UnrealEd.h"
#include "PropertyEditor/ParticleSystemEmittersPanel.h"

extern FEngineLoop GEngineLoop;

namespace PrivateEditorSelection
{
    static AActor* GActorSelected = nullptr;
    static AActor* GActorHovered = nullptr;

    static USceneComponent* GComponentSelected = nullptr;
    static USceneComponent* GComponentHovered = nullptr;
}

void UEditorEngine::Init()
{
    Super::Init();

    // Initialize the engine
    GEngine = this;

    FWorldContext& EditorWorldContext = CreateNewWorldContext(EWorldType::Editor);

    EditorWorld = UWorld::CreateWorld(this, EWorldType::Editor, FString("EditorWorld"));

    EditorWorldContext.SetCurrentWorld(EditorWorld);
    ActiveWorld = EditorWorld;

    EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);

    if (AssetManager == nullptr)
    {
        AssetManager = FObjectFactory::ConstructObject<UAssetManager>(this);
        assert(AssetManager);
        AssetManager->InitAssetManager();
    }
    // TODO: 필요할 때 활성화 하기
    // LoadLevel("Saved/AutoSaves.scene");
}

void UEditorEngine::Release()
{
    SaveLevel("Saved/AutoSaves.scene");
    
    for (FWorldContext* WorldContext : WorldList)
    {
        WorldContext->World()->Release();
    }
    WorldList.Empty();

    LevelEditor->Release();
    LevelEditor = nullptr;

    delete UnrealEditor;
    delete LevelEditor;
}

void UEditorEngine::Tick(float DeltaTime)
{
    for (FWorldContext* WorldContext : WorldList)
    {
        if (WorldContext->WorldType == EWorldType::Editor)
        {
            if (UWorld* World = WorldContext->World())
            {
                // TODO: World에서 EditorPlayer 제거 후 Tick 호출 제거 필요.
                World->Tick(DeltaTime);
                EditorPlayer->Tick(DeltaTime);
                ULevel* Level = World->GetActiveLevel();
                TArray CachedActors = Level->Actors;
                if (Level)
                {
                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor && Actor->IsActorTickInEditor())
                        {
                            Actor->Tick(DeltaTime);
                        }
                    }
                }
            }
        }
        else if (WorldContext->WorldType == EWorldType::PIE)
        {
            if (UWorld* World = WorldContext->World())
            {
                World->Tick(DeltaTime);
                ULevel* Level = World->GetActiveLevel();
                TArray CachedActors = Level->Actors;
                if (Level)
                {
                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor)
                        {
                            Actor->Tick(DeltaTime);
                        }
                    }
                }
            }
        }
        else if (WorldContext->WorldType == EWorldType::EditorPreview)
        {
            if (UWorld* World = WorldContext->World())
            {
                World->Tick(DeltaTime);
                EditorPlayer->Tick(DeltaTime);
                ULevel* Level = World->GetActiveLevel();
                TArray CachedActors = Level->Actors;
                if (Level)
                {
                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor && Actor->IsActorTickInEditor())
                        {
                            Actor->Tick(DeltaTime);
                        }
                    }
                }
            }
        }
    }
}

void UEditorEngine::StartPIE()
{
    if (PIEWorld)
    {
        UE_LOG(ELogLevel::Warning, TEXT("PIEWorld already exists!"));
        return;
    }
    this->ClearActorSelection(); // Editor World 기준 Select Actor 해제
    this->ClearComponentSelection();
    
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    Handler->OnPIEModeStart();

    FWorldContext& PIEWorldContext = CreateNewWorldContext(EWorldType::PIE);

    PIEWorld = Cast<UWorld>(EditorWorld->Duplicate(this));
    PIEWorld->WorldType = EWorldType::PIE;

    PIEWorldContext.SetCurrentWorld(PIEWorld);
    ActiveWorld = PIEWorld;
    
    BindEssentialObjects();
    
    PIEWorld->BeginPlay();
    // 여기서 Actor들의 BeginPlay를 해줄지 안에서 해줄 지 고민.
    // WorldList.Add(GetWorldContextFromWorld(PIEWorld));
}

void UEditorEngine::StartSkeletalMeshViewer(FName SkeletalMeshName, UAnimationAsset* AnimAsset)
{
    if (SkeletalMeshName == "")
    {
        return;
    }
    if (SkeletalMeshViewerWorld)
    {
        UE_LOG(ELogLevel::Warning, TEXT("SkeletalMeshViewerWorld already exists!"));
        return;
    }
    
    FWorldContext& WorldContext = CreateNewWorldContext(EWorldType::EditorPreview, EPreviewTypeBitFlag::AnimSequence);
    
    SkeletalMeshViewerWorld = USkeletalViewerWorld::CreateWorld(this, EWorldType::EditorPreview, FString("SkeletalMeshViewerWorld"));

    WorldContext.SetCurrentWorld(SkeletalMeshViewerWorld);
    ActiveWorld = SkeletalMeshViewerWorld;
    SkeletalMeshViewerWorld->WorldType = EWorldType::EditorPreview;

    // 스켈레탈 액터 스폰
    ASkeletalMeshActor* SkeletalActor = SkeletalMeshViewerWorld->SpawnActor<ASkeletalMeshActor>();
    SkeletalActor->SetActorTickInEditor(true);
    
    USkeletalMeshComponent* MeshComp = SkeletalActor->AddComponent<USkeletalMeshComponent>();
    SkeletalActor->SetRootComponent(MeshComp);
    SkeletalActor->SetActorLabel(TEXT("OBJ_SKELETALMESH"));
    MeshComp->SetSkeletalMeshAsset(UAssetManager::Get().GetSkeletalMesh(SkeletalMeshName.ToString()));
    SkeletalMeshViewerWorld->SetSkeletalMeshComponent(MeshComp);

    MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    MeshComp->PlayAnimation(AnimAsset, true);
    MeshComp->DEBUG_SetAnimationEnabled(true);
    MeshComp->SetPlaying(true);
    
    ADirectionalLight* DirectionalLight = SkeletalMeshViewerWorld->SpawnActor<ADirectionalLight>();
    DirectionalLight->SetActorRotation(FRotator(45.f, 45.f, 0.f));
    DirectionalLight->GetComponentByClass<UDirectionalLightComponent>()->SetIntensity(4.0f);

    FViewportCamera* Camera = LevelEditor->GetActiveViewportClient()->GetPerspectiveCamera();
    CameraLocation = Camera->GetLocation();
    CameraRotation = Camera->GetRotation();
    
    Camera->SetRotation(FVector(0.0f, 30, 180));
    if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(MeshComp))
    {
        float FOV = LevelEditor->GetActiveViewportClient()->GetCameraFOV();

        // 로컬 바운딩 박스
        FBoundingBox Box = Primitive->GetBoundingBox();
        FVector LocalCenter = (Box.MinLocation + Box.MaxLocation) * 0.5f;
        FVector LocalExtents = (Box.MaxLocation - Box.MinLocation) * 0.5f;
        float Radius = LocalExtents.Length();
        
        FMatrix ComponentToWorld = Primitive->GetWorldMatrix();
        FVector WorldCenter = ComponentToWorld.TransformPosition(LocalCenter);

        // FOV 기반 거리 계산
        float VerticalFOV = FMath::DegreesToRadians(FOV);
        float Distance = Radius / FMath::Tan(VerticalFOV * 0.5f);

        // 카메라 위치 설정
        Camera->SetLocation(WorldCenter - Camera->GetForwardVector() * Distance);
    }

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_LOCAL);
    }
}

void UEditorEngine::BindEssentialObjects()
{
    for (const auto Iter: TObjectRange<APlayer>())
    {
        if (Iter->GetWorld() == ActiveWorld)
        {
            ActiveWorld->SetMainPlayer(Iter);
            break;
        }
    }
    
    //실수로 안만들면 넣어주기
    if (ActiveWorld->GetMainPlayer() == nullptr)
    {
        APlayer* TempPlayer = ActiveWorld->SpawnActor<APlayer>();
        TempPlayer->SetActorLabel(TEXT("OBJ_PLAYER"));
        TempPlayer->SetActorTickInEditor(false);
        ActiveWorld->SetMainPlayer(TempPlayer);
    }
    
    //무조건 PIE들어갈때 만들어주기
    APlayerController* PlayerController = ActiveWorld->SpawnActor<APlayerController>();
    PlayerController->SetActorLabel(TEXT("OBJ_PLAYER_CONTROLLER"));
    PlayerController->SetActorTickInEditor(false);
    ActiveWorld->SetPlayerController(PlayerController);
    
    ActiveWorld->GetPlayerController()->Possess(ActiveWorld->GetMainPlayer());
}

void UEditorEngine::EndPIE()
{
    if (PIEWorld)
    {
        this->ClearActorSelection(); // PIE World 기준 Select Actor 해제 
        WorldList.Remove(GetWorldContextFromWorld(PIEWorld));
        PIEWorld->Release();
        GUObjectArray.MarkRemoveObject(PIEWorld);
        PIEWorld = nullptr;

        // TODO: PIE에서 EditorWorld로 돌아올 때, 기존 선택된 Picking이 유지되어야 함. 현재는 에러를 막기위해 임시조치.
        DeselectActor(GetSelectedActor());
        DeselectComponent(GetSelectedComponent());
    }

    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    Handler->OnPIEModeEnd();
    // 다시 EditorWorld로 돌아옴.
    ActiveWorld = EditorWorld;
}

void UEditorEngine::EndSkeletalMeshViewer()
{
    if (SkeletalMeshViewerWorld)
    {
        this->ClearActorSelection();
        WorldList.Remove(GetWorldContextFromWorld(SkeletalMeshViewerWorld));
        SkeletalMeshViewerWorld->Release();
        GUObjectArray.MarkRemoveObject(SkeletalMeshViewerWorld);
        SkeletalMeshViewerWorld = nullptr;
        
        FViewportCamera* Camera = LevelEditor->GetActiveViewportClient()->GetPerspectiveCamera();
        Camera->SetLocation(CameraLocation);
        Camera->SetRotation(CameraRotation);
        
        DeselectActor(GetSelectedActor());
        DeselectComponent(GetSelectedComponent());
    }
    ActiveWorld = EditorWorld;

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_WORLD);
    }
}

void UEditorEngine::StartParticleSystemViewer()
{
    // Check Valid Particle Asset

    if (ParticleSystemViewerWorld)
    {
        UE_LOG(ELogLevel::Warning, TEXT("ParticleSystemViewerWorld already exists!"));
        return;
    }

    FWorldContext& WorldContext = CreateNewWorldContext(EWorldType::EditorPreview, EPreviewTypeBitFlag::ParticleSystem);

    ParticleSystemViewerWorld = UWorld::CreateWorld(this, EWorldType::EditorPreview, FString("ParticleSystemViewerWorld"));

    WorldContext.SetCurrentWorld(ParticleSystemViewerWorld);
    ActiveWorld = ParticleSystemViewerWorld;

    // ParticleSystem 액터 스폰
    AParticleSystemActor* ParticleActor = ParticleSystemViewerWorld->SpawnActor<AParticleSystemActor>();
    // @note ParticleActor의 생성자에서 기본적으로 설정해주고 있음, 추후 확인 필요
    //ParticleActor->SetActorTickInEditor(true);

    //UParticleSystemComponent* ParticleComponent = ParticleActor->AddComponent<UParticleSystemComponent>();

    //ParticleActor->SetRootComponent(ParticleComponent);
    ParticleActor->SetActorLabel(TEXT("OBJ_PARTICLESYSTEM"));

    auto EditorEngine = Cast<UEditorEngine>(GEngine);
    auto UnrealEd = EditorEngine->GetUnrealEditor();
    auto Panel = reinterpret_cast<ParticleSystemEmittersPanel*>(UnrealEd->GetEditorPanel("ParticleSystemEmittersPanel").get());
    if (Panel)
    {
        Panel->SetParticleSystemComponent(ParticleActor->ParticleSystemComponent);
    }

    ADirectionalLight* DirectionalLight = ParticleSystemViewerWorld->SpawnActor<ADirectionalLight>();
    DirectionalLight->SetActorRotation(FRotator(45.f, 45.f, 0.f));
    DirectionalLight->GetComponentByClass<UDirectionalLightComponent>()->SetIntensity(4.0f);

    FViewportCamera* Camera = LevelEditor->GetActiveViewportClient()->GetPerspectiveCamera();
    CameraLocation = Camera->GetLocation();
    CameraRotation = Camera->GetRotation();

    Camera->SetRotation(FVector(0.0f, 30, 180));
    //if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(ParticleComponent))
    //{
    //    float FOV = LevelEditor->GetActiveViewportClient()->GetCameraFOV();

    //    // 로컬 바운딩 박스
    //    FBoundingBox Box = Primitive->GetBoundingBox();
    //    FVector LocalCenter = (Box.MinLocation + Box.MaxLocation) * 0.5f;
    //    FVector LocalExtents = (Box.MaxLocation - Box.MinLocation) * 0.5f;
    //    float Radius = LocalExtents.Length();

    //    FMatrix ComponentToWorld = Primitive->GetWorldMatrix();
    //    FVector WorldCenter = ComponentToWorld.TransformPosition(LocalCenter);

    //    // FOV 기반 거리 계산
    //    float VerticalFOV = FMath::DegreesToRadians(FOV);
    //    float Distance = Radius / FMath::Tan(VerticalFOV * 0.5f);

    //    // 카메라 위치 설정
    //    Camera->SetLocation(WorldCenter - Camera->GetForwardVector() * Distance);
    //}

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_LOCAL);
    }
    uint32 width;
    uint32 height;
    GEngineLoop.GetClientSize(width, height);
    LevelEditor->GetActiveViewportClient()->GetViewport()->ResizeViewport(FRect(0.0f, 72.f, width * 0.4, height * 0.6));
}

void UEditorEngine::EndParticleSystemViewer()
{
    if (ParticleSystemViewerWorld)
    {
        this->ClearActorSelection();
        WorldList.Remove(GetWorldContextFromWorld(ParticleSystemViewerWorld));
        ParticleSystemViewerWorld->Release();
        GUObjectArray.MarkRemoveObject(ParticleSystemViewerWorld);
        ParticleSystemViewerWorld = nullptr;

        FViewportCamera* Camera = LevelEditor->GetActiveViewportClient()->GetPerspectiveCamera();
        Camera->SetLocation(CameraLocation);
        Camera->SetRotation(CameraRotation);

        DeselectActor(GetSelectedActor());
        DeselectComponent(GetSelectedComponent());
    }

    ActiveWorld = EditorWorld;

    uint32 width;
    uint32 height;
    GEngineLoop.GetClientSize(width, height);
    LevelEditor->GetActiveViewportClient()->GetViewport()->ResizeViewport(FRect(0.0f, 72.f, width, height));

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_WORLD);
    }
}

FWorldContext& UEditorEngine::GetEditorWorldContext(/*bool bEnsureIsGWorld*/)
{
    for (FWorldContext* WorldContext : WorldList)
    {
        if (WorldContext->WorldType == EWorldType::Editor)
        {
            return *WorldContext;
        }
    }
    return CreateNewWorldContext(EWorldType::Editor);
}

FWorldContext* UEditorEngine::GetPIEWorldContext(/*int32 WorldPIEInstance*/)
{
    for (FWorldContext* WorldContext : WorldList)
    {
        if (WorldContext->WorldType == EWorldType::PIE)
        {
            return WorldContext;
        }
    }
    return nullptr;
}

void UEditorEngine::SelectActor(AActor* InActor)
{
    if (InActor && CanSelectActor(InActor))
    {
        PrivateEditorSelection::GActorSelected = InActor;
    }
}

void UEditorEngine::DeselectActor(AActor* InActor)
{
    if (PrivateEditorSelection::GActorSelected == InActor && InActor)
    {
        PrivateEditorSelection::GActorSelected = nullptr;
        ClearComponentSelection();
    }
}

void UEditorEngine::ClearActorSelection()
{
    PrivateEditorSelection::GActorSelected = nullptr;
}

bool UEditorEngine::CanSelectActor(const AActor* InActor) const
{
    return InActor != nullptr && InActor->GetWorld() == ActiveWorld && !InActor->IsActorBeingDestroyed();
}

AActor* UEditorEngine::GetSelectedActor() const
{
    return PrivateEditorSelection::GActorSelected;
}

void UEditorEngine::HoverActor(AActor* InActor)
{
    if (InActor)
    {
        PrivateEditorSelection::GActorHovered = InActor;
    }
}

void UEditorEngine::NewLevel()
{
    ClearActorSelection();
    ClearComponentSelection();

    if (ActiveWorld->GetActiveLevel())
    {
        ActiveWorld->GetActiveLevel()->Release();
    }
}

void UEditorEngine::SelectComponent(USceneComponent* InComponent) const
{
    if (InComponent && CanSelectComponent(InComponent))
    {
        PrivateEditorSelection::GComponentSelected = InComponent;
    }
}

void UEditorEngine::DeselectComponent(USceneComponent* InComponent)
{
    // 전달된 InComponent가 현재 선택된 컴포넌트와 같다면 선택 해제
    if (PrivateEditorSelection::GComponentSelected == InComponent && InComponent != nullptr)
    {
        PrivateEditorSelection::GComponentSelected = nullptr;
    }
}

void UEditorEngine::ClearComponentSelection()
{
    PrivateEditorSelection::GComponentSelected = nullptr;
}

bool UEditorEngine::CanSelectComponent(const USceneComponent* InComponent) const
{
    return InComponent != nullptr && InComponent->GetOwner() && InComponent->GetOwner()->GetWorld() == ActiveWorld && !InComponent->GetOwner()->IsActorBeingDestroyed();
}

USceneComponent* UEditorEngine::GetSelectedComponent() const
{
    return PrivateEditorSelection::GComponentSelected;
}

void UEditorEngine::HoverComponent(USceneComponent* InComponent)
{
    if (InComponent)
    {
        PrivateEditorSelection::GComponentHovered = InComponent;
    }
}

AEditorPlayer* UEditorEngine::GetEditorPlayer() const
{
    return EditorPlayer;
}
