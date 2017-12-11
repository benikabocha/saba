cbuffer VSData : register(b0)
{
    float4x4 WV;
    float4x4 WVP;
    float2 ScreenSize;
};

cbuffer VSEdgeData : register(b1)
{
    float EdgeSize;
};

cbuffer PSData : register(b2)
{
    float4 EdgeColor;
};

struct VSInput
{
    float3 Pos : POSITION;
    float3 Nor : NORMAL;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
    VSOutput vsOut;
    float3 nor = mul((float3x3)WV, input.Nor);
    float4 pos = mul(WVP, float4(input.Pos, 1.0));
    float2 screenNor = normalize((float2)nor);
    pos.xy += screenNor * float2(1.0, 1.0) / (ScreenSize * 0.5) * EdgeSize * pos.w;
    vsOut.Position = pos;
    return vsOut;
}

float4 PSMain(VSOutput vsOut) : SV_TARGET0
{
    return EdgeColor;
}
