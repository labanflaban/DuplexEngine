
//struct vs_in
//{
//    float3 pos : POSITION;
//    float3 color : COLOR;
//};

struct vs_in
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct vs_out
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

cbuffer perModel : register(b0)
{
    float4x4 wvpMatrix;
};


vs_out main(vs_in input)
{
    vs_out output;
    output.pos = mul(float4(input.pos, 1), wvpMatrix);
    output.uv = input.uv;
	output.normal = input.normal;
    output.tangent = input.tangent;
    output.bitangent = input.bitangent;
    
    return output;
}