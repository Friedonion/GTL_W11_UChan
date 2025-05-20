#include "ShaderRegisters.hlsl"

Texture2D ParticleTexture : register(t0);
SamplerState SamplerLinear : register(s0);

struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
    float Distance : DISTANCE;
};

float4 mainPS(PSInput input) : SV_TARGET
{
    float4 texColor = ParticleTexture.Sample(SamplerLinear, input.TexCoord);
    
    float threshold = 0.01;
    float luminance = texColor.r + texColor.g + texColor.b;
    clip(luminance - threshold);
    
    return texColor * input.Color;
}

