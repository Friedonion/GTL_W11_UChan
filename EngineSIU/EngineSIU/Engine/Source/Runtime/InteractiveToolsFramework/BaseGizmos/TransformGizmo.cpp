#include "TransformGizmo.h"
#include "GizmoArrowComponent.h"
#include "Define.h"
#include "GizmoCircleComponent.h"
#include "Actors/Player.h"
#include "GizmoRectangleComponent.h"
#include "ReferenceSkeleton.h"
#include "Animation/Skeleton.h"
#include "Engine/EditorEngine.h"
#include "World/World.h"
#include "Engine/FObjLoader.h"
#include "Engine/SkeletalMesh.h"

ATransformGizmo::ATransformGizmo()
{
    static int a = 0;
    UE_LOG(ELogLevel::Error, "Gizmo Created %d", a++);
    FObjManager::CreateStaticMesh("Assets/GizmoTranslationX.obj");
    FObjManager::CreateStaticMesh("Assets/GizmoTranslationY.obj");
    FObjManager::CreateStaticMesh("Assets/GizmoTranslationZ.obj");
    FObjManager::CreateStaticMesh("Assets/GizmoRotationX.obj");
    FObjManager::CreateStaticMesh("Assets/GizmoRotationY.obj");
    FObjManager::CreateStaticMesh("Assets/GizmoRotationZ.obj");
    FObjManager::CreateStaticMesh("Assets/GizmoScaleX.obj");
    FObjManager::CreateStaticMesh("Assets/GizmoScaleY.obj");
    FObjManager::CreateStaticMesh("Assets/GizmoScaleZ.obj");

    SetRootComponent(
        AddComponent<USceneComponent>()
    );

    UGizmoArrowComponent* LocationX = AddComponent<UGizmoArrowComponent>();
    LocationX->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoTranslationX.obj"));
    LocationX->SetupAttachment(RootComponent);
    LocationX->SetGizmoType(UGizmoBaseComponent::ArrowX);
    ArrowArr.Add(LocationX);

    UGizmoArrowComponent* LocationY = AddComponent<UGizmoArrowComponent>();
    LocationY->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoTranslationY.obj"));
    LocationY->SetupAttachment(RootComponent);
    LocationY->SetGizmoType(UGizmoBaseComponent::ArrowY);
    ArrowArr.Add(LocationY);

    UGizmoArrowComponent* LocationZ = AddComponent<UGizmoArrowComponent>();
    LocationZ->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoTranslationZ.obj"));
    LocationZ->SetupAttachment(RootComponent);
    LocationZ->SetGizmoType(UGizmoBaseComponent::ArrowZ);
    ArrowArr.Add(LocationZ);

    UGizmoRectangleComponent* ScaleX = AddComponent<UGizmoRectangleComponent>();
    ScaleX->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoScaleX.obj"));
    ScaleX->SetupAttachment(RootComponent);
    ScaleX->SetGizmoType(UGizmoBaseComponent::ScaleX);
    RectangleArr.Add(ScaleX);

    UGizmoRectangleComponent* ScaleY = AddComponent<UGizmoRectangleComponent>();
    ScaleY->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoScaleY.obj"));
    ScaleY->SetupAttachment(RootComponent);
    ScaleY->SetGizmoType(UGizmoBaseComponent::ScaleY);
    RectangleArr.Add(ScaleY);

    UGizmoRectangleComponent* ScaleZ = AddComponent<UGizmoRectangleComponent>();
    ScaleZ->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoScaleZ.obj"));
    ScaleZ->SetupAttachment(RootComponent);
    ScaleZ->SetGizmoType(UGizmoBaseComponent::ScaleZ);
    RectangleArr.Add(ScaleZ);

    UGizmoCircleComponent* CircleX = AddComponent<UGizmoCircleComponent>();
    CircleX->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoRotationX.obj"));
    CircleX->SetupAttachment(RootComponent);
    CircleX->SetGizmoType(UGizmoBaseComponent::CircleX);
    CircleArr.Add(CircleX);

    UGizmoCircleComponent* CircleY = AddComponent<UGizmoCircleComponent>();
    CircleY->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoRotationY.obj"));
    CircleY->SetupAttachment(RootComponent);
    CircleY->SetGizmoType(UGizmoBaseComponent::CircleY);
    CircleArr.Add(CircleY);

    UGizmoCircleComponent* CircleZ = AddComponent<UGizmoCircleComponent>();
    CircleZ->SetStaticMesh(FObjManager::GetStaticMesh(L"Assets/GizmoRotationZ.obj"));
    CircleZ->SetupAttachment(RootComponent);
    CircleZ->SetGizmoType(UGizmoBaseComponent::CircleZ);
    CircleArr.Add(CircleZ);
}

void ATransformGizmo::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Editor 모드에서만 Tick. SkeletalMeshViewer모드에서도 tick
    if (GEngine->ActiveWorld->WorldType != EWorldType::Editor and GEngine->ActiveWorld->WorldType != EWorldType::EditorPreview)
    {
        return;
    }

    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        return;
    }
    AEditorPlayer* EditorPlayer = Engine->GetEditorPlayer();
    if (!EditorPlayer)
    {
        return;
    }
    
    USceneComponent* SelectedComponent = Engine->GetSelectedComponent();
    AActor* SelectedActor = Engine->GetSelectedActor();

    USceneComponent* TargetComponent = nullptr;

    if (SelectedComponent != nullptr)
    {
        TargetComponent = SelectedComponent;
    }
    else if (SelectedActor != nullptr)
    {
        TargetComponent = SelectedActor->GetRootComponent();
    }

    if (TargetComponent)
    {
        SetActorLocation(TargetComponent->GetComponentLocation());
        if (EditorPlayer->GetCoordMode() == ECoordMode::CDM_LOCAL || EditorPlayer->GetControlMode() == EControlMode::CM_SCALE)
        {
            SetActorRotation(TargetComponent->GetComponentRotation());
        }
        else
        {
            SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));
        }

        //본 부착용
        if (GEngine->ActiveWorld->WorldType == EWorldType::EditorPreview)
        {
            USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(TargetComponent);
            if (SkeletalMeshComp)
            {
                int BoneIndex = Engine->SkeletalMeshViewerWorld->SelectBoneIndex;
                TArray<FMatrix> GlobalBoneMatrices;
                SkeletalMeshComp->GetCurrentGlobalBoneMatrices(GlobalBoneMatrices);

                FTransform GlobalBoneTransform = FTransform(GlobalBoneMatrices[BoneIndex]);

                AddActorLocation(GlobalBoneTransform.Translation);
                if (EditorPlayer->GetCoordMode() == ECoordMode::CDM_LOCAL || EditorPlayer->GetControlMode() == EControlMode::CM_SCALE)
                {
                    AddActorRotation(GlobalBoneTransform.Rotation);
                }
            
            }
        }
    }


    //

}

void ATransformGizmo::Initialize(FEditorViewportClient* InViewport)
{
    AttachedViewport = InViewport;
}
