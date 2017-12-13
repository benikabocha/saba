cbuffer VSData : register(b0)
{
    float4x4 WVP;
};

cbuffer PSData : register(b1)
{
    float4 ShadowColor;
};

struct VSInput
{
    float3 Pos : POSITION;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
    VSOutput vsOut;
    vsOut.Position = mul(WVP, float4(input.Pos, 1.0));
    return vsOut;
}

float4 PSMain(VSOutput vsOut) : SV_TARGET0
{
    return ShadowColor;
}
