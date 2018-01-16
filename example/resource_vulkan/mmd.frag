#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNor;
layout (location = 2) in vec2 inUV;

layout (binding = 1) uniform UBO 
{
	vec3	Diffuse;
	float	Alpha;
	vec3	Ambient;
	vec3	Specular;
	float	SpecularPower;
	vec3	LightColor;
	vec3	LightDir;

	vec4	TexMulFactor;
	vec4	TexAddFactor;

	vec4	ToonTexMulFactor;
	vec4	ToonTexAddFactor;

	vec4	SphereTexMulFactor;
	vec4	SphereTexAddFactor;

	ivec4	TextureModes;
} ubo;

layout (binding = 2) uniform sampler2D Tex;
layout (binding = 3) uniform sampler2D ToonTex;
layout (binding = 4) uniform sampler2D SphereTex;

layout (location = 0) out vec4 outFragColor;

vec3 ComputeTexMulFactor(vec3 texColor, vec4 factor)
{
	vec3 ret = texColor * factor.rgb;
	return mix(vec3(1.0, 1.0, 1.0), ret, factor.a);
}

vec3 ComputeTexAddFactor(vec3 texColor, vec4 factor)
{
	vec3 ret = texColor + (texColor - vec3(1.0)) * factor.a ;
	ret = clamp(ret, vec3(0.0), vec3(1.0))+ factor.rgb;
	return ret;
}

void main() 
{
	vec3 eyeDir = normalize(inPos);
	vec3 lightDir = normalize(-ubo.LightDir);
	vec3 nor = normalize(inNor);
	float ln = dot(nor, lightDir);
	ln = clamp(ln + 0.5, 0.0, 1.0);
	vec3 color = vec3(0.0, 0.0, 0.0);
	float alpha = ubo.Alpha;
	vec3 diffuseColor = ubo.Diffuse * ubo.LightColor;
	color = diffuseColor;
	color += ubo.Ambient;
	color = clamp(color, 0.0, 1.0);
    int TexMode = ubo.TextureModes.x;
    int ToonTexMode = ubo.TextureModes.y;
    int SphereTexMode = ubo.TextureModes.z;

    if (TexMode != 0)
    {
		vec4 texColor = texture(Tex, inUV);
		texColor.rgb = ComputeTexMulFactor(texColor.rgb, ubo.TexMulFactor);
		texColor.rgb = ComputeTexAddFactor(texColor.rgb, ubo.TexAddFactor);
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
		vec2 spUV = vec2(0.0);
		spUV.x = nor.x * 0.5 + 0.5;
		spUV.y = nor.y * 0.5 + 0.5;
		vec3 spColor = texture(SphereTex, spUV).rgb;
		spColor = ComputeTexMulFactor(spColor, ubo.SphereTexMulFactor);
		spColor = ComputeTexAddFactor(spColor, ubo.SphereTexAddFactor);
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
		vec3 toonColor = texture(ToonTex, vec2(0.0, 1.0 - ln)).rgb;
		toonColor = ComputeTexMulFactor(toonColor, ubo.ToonTexMulFactor);
		toonColor = ComputeTexAddFactor(toonColor, ubo.ToonTexAddFactor);
		color *= toonColor;
	}

	vec3 specular = vec3(0.0);
	if (ubo.SpecularPower > 0)
	{
		vec3 halfVec = normalize(eyeDir + lightDir);
		vec3 specularColor = ubo.Specular * ubo.LightColor;
		specular += pow(max(0.0, dot(halfVec, nor)), ubo.SpecularPower) * specularColor;
	}

	color += specular;
	
	outFragColor = vec4(color, alpha);
}
