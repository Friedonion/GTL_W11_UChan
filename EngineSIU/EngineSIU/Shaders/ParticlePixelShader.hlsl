#include "ShaderRegisters.hlsl"

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}

