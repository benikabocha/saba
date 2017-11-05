#version 140

#define NUM_SHADOWMAP 4

in vec3 vs_Pos;
in vec3 vs_Nor;
in vec2 vs_UV;

in vec4 vs_shadowMapCoord[NUM_SHADOWMAP];

out vec4 out_Color;

uniform float u_Alpha;
uniform vec3 u_Diffuse;
uniform vec3 u_Ambient;
uniform vec3 u_Specular;
uniform float u_SpecularPower;
uniform vec3 u_LightColor;
uniform vec3 u_LightDir;

uniform int u_TexMode;
uniform sampler2D u_Tex;
uniform vec4 u_TexMulFactor;
uniform vec4 u_TexAddFactor;

uniform int u_ToonTexMode;
uniform sampler2D u_ToonTex;
uniform vec4 u_ToonTexMulFactor;
uniform vec4 u_ToonTexAddFactor;

uniform int u_SphereTexMode;
uniform sampler2D u_SphereTex;
uniform vec4 u_SphereTexMulFactor;
uniform vec4 u_SphereTexAddFactor;

// ShadowMap
uniform float u_ShadowMapSplitPositions[NUM_SHADOWMAP + 1];
uniform sampler2DShadow u_ShadowMap0;
uniform sampler2DShadow u_ShadowMap1;
uniform sampler2DShadow u_ShadowMap2;
uniform sampler2DShadow u_ShadowMap3;
uniform int u_ShadowMapEnabled;

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
	vec3 eyeDir = normalize(vs_Pos);
	vec3 lightDir = normalize(-u_LightDir);
	vec3 nor = normalize(vs_Nor);
	float ln = dot(nor, lightDir);
	ln = clamp(ln + 0.5, 0.0, 1.0);
	vec3 color = vec3(0.0, 0.0, 0.0);
	float alpha = u_Alpha;
	vec3 diffuseColor = u_Diffuse * u_LightColor;
	color = diffuseColor;
	color += u_Ambient;
	color = clamp(color, 0.0, 1.0);

	if (u_ShadowMapEnabled != 0)
	{
		float z = -vs_Pos.z;
		float visibility = 1.0;
		if (u_ShadowMapSplitPositions[0] <= z && z < u_ShadowMapSplitPositions[0 + 1])
		{
			visibility = textureProj(u_ShadowMap0, vs_shadowMapCoord[0]);
		}
		else if (u_ShadowMapSplitPositions[1] <= z && z < u_ShadowMapSplitPositions[1 + 1])
		{
			visibility = textureProj(u_ShadowMap1, vs_shadowMapCoord[1]);
		}
		else if (u_ShadowMapSplitPositions[2] <= z && z < u_ShadowMapSplitPositions[2 + 1])
		{
			visibility = textureProj(u_ShadowMap2, vs_shadowMapCoord[2]);
		}
		else if (u_ShadowMapSplitPositions[3] <= z && z < u_ShadowMapSplitPositions[3 + 1])
		{
			visibility = textureProj(u_ShadowMap3, vs_shadowMapCoord[3]);
		}
		ln *= (1.0 - visibility);
	}

    if (u_TexMode != 0)
    {
		vec4 texColor = texture(u_Tex, vs_UV);
		texColor.rgb = ComputeTexMulFactor(texColor.rgb, u_TexMulFactor);
		texColor.rgb = ComputeTexAddFactor(texColor.rgb, u_TexAddFactor);
        color *= texColor.rgb;
		if (u_TexMode == 2)
		{
			alpha *= texColor.a;
		}
    }
	if (alpha == 0.0)
	{
		discard;
	}

	if (u_SphereTexMode != 0)
	{
		vec2 spUV = vec2(0.0);
		spUV.x = nor.x * 0.5 + 0.5;
		spUV.y = 1.0 - (nor.y * 0.5 + 0.5);
		vec3 spColor = texture(u_SphereTex, spUV).rgb;
		spColor = ComputeTexMulFactor(spColor, u_SphereTexMulFactor);
		spColor = ComputeTexAddFactor(spColor, u_SphereTexAddFactor);
		if (u_SphereTexMode == 1)
		{
			color *= spColor;
		}
		else if (u_SphereTexMode == 2)
		{
			color += spColor;
		}
	}

	if (u_ToonTexMode != 0)
	{
		vec3 toonColor = texture(u_ToonTex, vec2(0.0, ln)).rgb;
		toonColor = ComputeTexMulFactor(toonColor, u_ToonTexMulFactor);
		toonColor = ComputeTexAddFactor(toonColor, u_ToonTexAddFactor);
		color *= toonColor;
	}

	vec3 specular = vec3(0.0);
	if (u_SpecularPower > 0)
	{
		vec3 halfVec = normalize(eyeDir + lightDir);
		vec3 specularColor = u_Specular * u_LightColor;
		specular += pow(max(0.0, dot(halfVec, nor)), u_SpecularPower) * specularColor;
	}

	color += specular;
	
	out_Color = vec4(color, alpha);
}
