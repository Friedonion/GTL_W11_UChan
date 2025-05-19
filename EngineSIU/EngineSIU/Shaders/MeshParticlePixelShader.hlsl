#include "ShaderRegisters.hlsl"

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
SamplerState SamplerLinear : register(s0);

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
    float4 diffuseColor = DiffuseTexture.Sample(SamplerLinear, input.TexCoord);
    
    // 텍스처가 로드되지 않은 경우를 위한 디폴트 색상
    if (all(diffuseColor.rgb == 0))
    {
        diffuseColor = float4(1, 1, 1, 1);
    }
    
    // 단순하게 텍스처 색상과 인스턴스 색상만 곱함
    float4 finalColor = diffuseColor * input.Color;
    
    return finalColor;
}
