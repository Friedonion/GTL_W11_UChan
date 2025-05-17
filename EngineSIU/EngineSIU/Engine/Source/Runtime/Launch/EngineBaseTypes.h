#pragma once
#include "HAL/PlatformType.h"


enum class EViewModeIndex : uint8
{
    /**
     * 이 enum 수정 시에는 CompositingShader.hlsl도 수정해야 합니다.
     */
    VMI_Lit_Gouraud = 0,
    VMI_Lit_Lambert,
    VMI_Lit_BlinnPhong,
    VMI_LIT_PBR,
    VMI_Unlit, // Lit 모드들은 이 위에 적어주세요.
    VMI_Wireframe,
    VMI_SceneDepth,
    VMI_WorldNormal,
    VMI_WorldTangent,
    VMI_LightHeatMap,
    VMI_MAX,
};


enum ELevelViewportType : uint8
{
    LVT_Perspective = 0,

    /** Top */
    LVT_OrthoXY = 1,
    /** Bottom */
    LVT_OrthoNegativeXY,
    /** Left */
    LVT_OrthoYZ,
    /** Right */
    LVT_OrthoNegativeYZ,
    /** Front */
    LVT_OrthoXZ,
    /** Back */
    LVT_OrthoNegativeXZ,

    LVT_MAX,
    LVT_None = 255,
};

enum EPrimitiveType
{
    // Topology that defines a triangle N with 3 vertex extremities: 3*N+0, 3*N+1, 3*N+2.
    PT_TriangleList,

    // Topology that defines a triangle N with 3 vertex extremities: N+0, N+1, N+2.
    PT_TriangleStrip,

    // Topology that defines a line with 2 vertex extremities: 2*N+0, 2*N+1.
    PT_LineList,

    // Topology that defines a quad N with 4 vertex extremities: 4*N+0, 4*N+1, 4*N+2, 4*N+3.
    // Supported only if GRHISupportsQuadTopology == true.
    PT_QuadList,

    // Topology that defines a point N with a single vertex N.
    PT_PointList,

    // Topology that defines a screen aligned rectangle N with only 3 vertex corners:
    //    3*N + 0 is upper-left corner,
    //    3*N + 1 is upper-right corner,
    //    3*N + 2 is the lower-left corner.
    // Supported only if GRHISupportsRectTopology == true.
    PT_RectList,

    PT_Num,
    PT_NumBits = 3
};
