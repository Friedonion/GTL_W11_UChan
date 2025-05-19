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
    // 라이팅 계산 제거, 단순히 텍스처와 색상 곱하기
    float4 diffuseColor = MaterialTextures[0].Sample(MaterialSamplers[0], input.TexCoord);
    
    float4 finalColor = diffuseColor;
    
    return finalColor;
}
