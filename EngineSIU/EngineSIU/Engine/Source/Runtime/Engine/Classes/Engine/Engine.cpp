#include "Engine.h"

#include "EditorEngine.h"
#include "UnrealEd/SceneManager.h"
#include "UObject/Casts.h"
#include "World/World.h"

UEngine* GEngine = nullptr;

void UEngine::Init()
{
    // 컴파일 타임에 확정되지 못한 타입을 런타임에 검사
    UStruct::ResolvePendingProperties();
}

FWorldContext* UEngine::GetWorldContextFromWorld(const UWorld* InWorld)
{
    for (FWorldContext* WorldContext : WorldList)
    {
        if (WorldContext->World() == InWorld)
        {
            return WorldContext;
        }
    }
    return nullptr;
}

FWorldContext* UEngine::GetWorldContextFromHandle(const FName WorldContextHandle)
{
    for (FWorldContext* WorldContext : WorldList)
    {
        if (WorldContext->ContextHandle == WorldContextHandle)
        {
            return WorldContext;
        }
    }
    return nullptr;
}

FWorldContext& UEngine::CreateNewWorldContext(EWorldType InWorldType, EPreviewTypeBitFlag InPreviewType)
{
    FWorldContext* NewWorldContext = new FWorldContext;
    WorldList.Add(NewWorldContext);
    NewWorldContext->WorldType = InWorldType;
    NewWorldContext->PreviewType = InPreviewType;
    NewWorldContext->ContextHandle = FName(*FString::Printf(TEXT("WorldContext_%d"), NextWorldContextHandle++));

    return *NewWorldContext;
}


void UEngine::LoadLevel(const FString& FileName) const
{
    SceneManager::LoadSceneFromJsonFile(*FileName, *ActiveWorld);
}

void UEngine::SaveLevel(const FString& FileName) const
{
    SceneManager::SaveSceneToJsonFile(*FileName, *ActiveWorld);
}
