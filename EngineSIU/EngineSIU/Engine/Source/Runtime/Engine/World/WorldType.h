#pragma once

enum class EWorldType
{
    None,
    Game,
    Editor,
    PIE,
    EditorPreview,
    GamePreview,
    GameRPC,
    Inactive
};

enum class EWorldTypeBitFlag
{
    None = 0,
    Game = 1 << 0,
    Editor = 1 << 1,
    PIE = 1 << 2,
    EditorPreview = 1 << 3,
    GamePreview = 1 << 4,
    GameRPC = 1 << 5,
    Inactive = 1 << 6
};

inline EWorldTypeBitFlag operator|(EWorldTypeBitFlag A, EWorldTypeBitFlag B)
{
    return static_cast<EWorldTypeBitFlag>(
        static_cast<uint32>(A) | static_cast<uint32>(B)
    );
}

inline EWorldTypeBitFlag operator&(EWorldTypeBitFlag A, EWorldTypeBitFlag B)
{
    return static_cast<EWorldTypeBitFlag>(
        static_cast<uint32>(A) & static_cast<uint32>(B)
    );
}

inline bool HasFlag(EWorldTypeBitFlag Mask, EWorldTypeBitFlag Flag)
{
    return (static_cast<uint32>(Mask) & static_cast<uint32>(Flag)) != 0;
}

enum class EPreviewTypeBitFlag
{
    None = 0,
    StaticMesh = 1 << 0,
    SkeletalMesh = 1 << 1,
    Skeleton = 1 << 2,
    AnimSequence = 1 << 3,
    ParticleSystem = 1 << 4,
};

inline EPreviewTypeBitFlag operator|(EPreviewTypeBitFlag A, EPreviewTypeBitFlag B)
{
    return static_cast<EPreviewTypeBitFlag>(
        static_cast<uint32>(A) | static_cast<uint32>(B)
        );
}

inline EPreviewTypeBitFlag operator&(EPreviewTypeBitFlag A, EPreviewTypeBitFlag B)
{
    return static_cast<EPreviewTypeBitFlag>(
        static_cast<uint32>(A) & static_cast<uint32>(B)
        );
}

inline bool HasFlag(EPreviewTypeBitFlag Mask, EPreviewTypeBitFlag Flag)
{
    return (static_cast<uint32>(Mask) & static_cast<uint32>(Flag)) != 0;
}
