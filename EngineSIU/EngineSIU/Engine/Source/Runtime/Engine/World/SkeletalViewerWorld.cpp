#include "SkeletalViewerWorld.h"
#include "Classes/Animation/SkeletalMeshActor.h"
#include "Classes/Engine/AssetManager.h"
#include "Engine/EditorEngine.h"

USkeletalViewerWorld* USkeletalViewerWorld::CreateWorld(UObject* InOuter, const EWorldType InWorldType, const FString& InWorldName)
{
    USkeletalViewerWorld* NewWorld = FObjectFactory::ConstructObject<USkeletalViewerWorld>(InOuter);
    NewWorld->WorldName = InWorldName;
    NewWorld->WorldType = InWorldType;
    NewWorld->InitializeNewWorld();
    NewWorld->SelectBoneIndex = 0;

    return NewWorld;
}

void USkeletalViewerWorld::Tick(float DeltaTime)
{
    UWorld::Tick(DeltaTime);

    //TODO: 임시로 SkeletalMeshComponent을 강제로 셀렉트 함
    Cast<UEditorEngine>(GEngine)->SelectActor(SkeletalMeshComponent->GetOwner());
    Cast<UEditorEngine>(GEngine)->SelectComponent(SkeletalMeshComponent);
}
