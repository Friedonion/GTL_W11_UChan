#include "ShaderRegisters.hlsl"


struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPosition : POSITION1;
    float2 TexCoord : TEXCOORD;    
    float4 Color : COLOR;
};

float4 mainPS(PS_INPUT input) : SV_TARGET
{
    float4 texColor = MaterialTextures[0].Sample(MaterialSamplers[0], input.TexCoord);

    // 알파가 0일 경우 input.Color 사용
    float t = step(0.0001, texColor.a);
    float4 finalColor = lerp(input.Color, texColor, t);

    return finalColor;
}

