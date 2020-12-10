struct ps_in
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float4 worldPos : POSITION;
};

Texture2D diffuseTexture : TEXTURE : register(t0);
SamplerState sampState : SAMPLER : register(s0);

float4 main(ps_in input) : SV_TARGET
{
    float4 color = diffuseTexture.Sample(sampState, input.uv);
    return color;
}