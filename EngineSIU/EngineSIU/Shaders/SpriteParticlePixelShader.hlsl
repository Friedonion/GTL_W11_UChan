#include "ShaderRegisters.hlsl"

Texture2D ParticleTexture : register(t0);
SamplerState SamplerLinear : register(s0);

struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

float4 mainPS(PSInput input) : SV_TARGET
{
    float4 texColor = ParticleTexture.Sample(SamplerLinear, input.TexCoord);
    return texColor * input.Color;
}

