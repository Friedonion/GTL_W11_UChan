#pragma once

#include "WorldType.h"
#include "UObject/NameTypes.h"

class UWorld;

/*
    FWorldContext
    UWorld에 대한 정보를 저장해두는 Context

*/
struct FWorldContext
{
    EWorldType WorldType;
    EPreviewTypeBitFlag PreviewType;
    FName ContextHandle;

    void SetCurrentWorld(UWorld* InWorld)
    {
        ThisCurrentWorld = InWorld;
    }

    UWorld* World() const
    {
        return ThisCurrentWorld;
    }

    FWorldContext()
        : WorldType(EWorldType::None)
        , PreviewType(EPreviewTypeBitFlag::None)
        , ContextHandle(TEXT("None"))
        , ThisCurrentWorld(nullptr)
    {}

private:
    UWorld* ThisCurrentWorld;
};
