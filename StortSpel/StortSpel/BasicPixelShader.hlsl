#define MAX_LIGHTS 8

struct ps_in
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
};




float4 main(ps_in input) : SV_TARGET
{
	return float4(1, 0, 0, 1);
}