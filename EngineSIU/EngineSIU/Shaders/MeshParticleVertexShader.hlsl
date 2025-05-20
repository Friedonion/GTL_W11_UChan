#include "ShaderRegisters.hlsl"

struct VS_INPUT
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoord : TEXCOORD;
    float4 World0 : WORLD0;
    float4 World1 : WORLD1;
    float4 World2 : WORLD2;
    float4 World3 : WORLD3;
    float Distance : DISTANCE;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : POSITION1;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    // 인스턴스별 월드 변환 행렬 구성
    matrix WorldMatrix = matrix(input.World0, input.World1, input.World2, input.World3);
    float4 worldPos = mul(float4(input.Position, 1.0f), WorldMatrix);
    output.Position = mul(worldPos, ViewMatrix);
    output.Position = mul(output.Position, ProjectionMatrix);
    output.WorldPosition = worldPos.xyz;
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
    
    return output;
}
