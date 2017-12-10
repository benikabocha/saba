cbuffer VSData : register(b0)
{
    float4x4 WV;
    float4x4 WVP;
};

cbuffer PSData : register(b1)
{
    float   Alpha;
    float3  Diffuse;
    float3  Ambient;
    float3  Specular;
    float   SpecularPower;
    float3  LightColor;
    float3  LightDir;

    float4  TexMulFactor;
    float4  TexAddFactor;

    float4  ToonTexMulFactor;
    float4  ToonTexAddFactor;

    float4  SphereTexMulFactor;
    float4  SphereTexAddFactor;

    int4    TextureModes;
}

Texture2D Tex : register(t0);
Texture2D ToonTex : register(t1);
Texture2D SphereTex : register(t2);
sampler TexSampler : register(s0);
sampler ToonTexSampler : register(s1);
sampler SphereTexSampler : register(s2);

struct VSInput
{
    float3 Pos : POSITION;
    float3 Nor : NORMAL;
    float2 UV : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 Pos : TEXCOORD0;
    float3 Nor : TEXCOORD1;
    float2 UV : TEXCOORD2;
};

VSOutput VSMain(VSInput input)
{
    VSOutput vsOut;
    vsOut.Position = mul(WVP, float4(input.Pos, 1.0));
    vsOut.Pos = mul(WV, float4(input.Pos, 1.0)).xyz;
    vsOut.Nor = mul((float3x3)WV, input.Nor);
    vsOut.UV = float2(input.UV.x, 1.0 - input.UV.y);
    return vsOut;
}

float3 ComputeTexMulFactor(float3 texColor, float4 factor)
{
    float3 ret = texColor * factor.rgb;
    return lerp(float3(1.0, 1.0, 1.0), ret, factor.a);
}

float3 ComputeTexAddFactor(float3 texColor, float4 factor)
{
    float3 ret = texColor + (texColor - (float3)1.0) * factor.a;
    ret = clamp(ret, (float3)0.0, (float3)1.0)+ factor.rgb;
    return ret;
}

float4 PSMain(VSOutput vsOut) : SV_TARGET0
{
    float3 eyeDir = normalize(vsOut.Pos);
    float3 lightDir = normalize(-LightDir);
    float3 nor = normalize(vsOut.Nor);
    float ln = dot(nor, lightDir);
    ln = clamp(ln + 0.5, 0.0, 1.0);
    float3 color = float3(0.0, 0.0, 0.0);
    float alpha = Alpha;
    float3 diffuseColor = Diffuse * LightColor;
    color = diffuseColor;
    color += Ambient;
    color = clamp(color, 0.0, 1.0);
    int TexMode = TextureModes.x;
    int ToonTexMode = TextureModes.y;
    int SphereTexMode = TextureModes.z;

    if (TexMode != 0)
    {
        float4 texColor = Tex.Sample(TexSampler, vsOut.UV);
        texColor.rgb = ComputeTexMulFactor(texColor.rgb, TexMulFactor);
        texColor.rgb = ComputeTexAddFactor(texColor.rgb, TexAddFactor);
        color *= texColor.rgb;
        if (TexMode == 2)
        {
            alpha *= texColor.a;
        }
    }
    if (alpha == 0.0)
    {
        discard;
    }

    if (SphereTexMode != 0)
    {
        float2 spUV = (float2)0.0;
        spUV.x = nor.x * 0.5 + 0.5;
        spUV.y = nor.y * 0.5 + 0.5;
        float3 spColor = SphereTex.Sample(SphereTexSampler, spUV).rgb;
        spColor = ComputeTexMulFactor(spColor, SphereTexMulFactor);
        spColor = ComputeTexAddFactor(spColor, SphereTexAddFactor);
        if (SphereTexMode == 1)
        {
            color *= spColor;
        }
        else if (SphereTexMode == 2)
        {
            color += spColor;
        }
    }

    if (ToonTexMode != 0)
    {
        float3 toonColor = ToonTex.Sample(ToonTexSampler, float2(0.0, 1.0 - ln)).rgb;
        toonColor = ComputeTexMulFactor(toonColor, ToonTexMulFactor);
        toonColor = ComputeTexAddFactor(toonColor, ToonTexAddFactor);
        color *= toonColor;
    }

    float3 specular = (float3)0.0;
    if (SpecularPower > 0.0)
    {
        float3 halfVec = normalize(eyeDir + lightDir);
        float3 specularColor = Specular * LightColor;
        specular += pow(max(0.0, dot(halfVec, nor)), SpecularPower) * specularColor;
    }
    color += specular;

    return float4(color, alpha);
}
