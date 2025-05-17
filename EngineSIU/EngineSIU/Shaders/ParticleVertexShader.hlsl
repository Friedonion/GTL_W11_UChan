#include "ShaderRegisters.hlsl"

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float4 World0 : WORLD0;
    float4 World1 : WORLD1;
    float4 World2 : WORLD2;
    float4 World3 : WORLD3;
    float4 Color : COLOR;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;

    matrix World = matrix(input.World0, input.World1, input.World2, input.World3);
    float3 camRight = float3(InvViewMatrix._11, InvViewMatrix._12, InvViewMatrix._13);
    float3 camUp = float3(InvViewMatrix._21, InvViewMatrix._22, InvViewMatrix._23);

    float3 localOffset = input.Position.x * camRight + input.Position.y * camUp;
    float4 worldPos = mul(float4(localOffset, 1.0f), World);

    output.Pos = mul(worldPos, ViewMatrix);
    output.Pos = mul(output.Pos, ProjectionMatrix);
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;

    return output;
}
