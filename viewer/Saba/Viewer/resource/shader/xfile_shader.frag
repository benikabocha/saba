#version 140

// Input
in vec3	vs_Pos;
in vec3	vs_Nor;
in vec2	vs_UV;

uniform vec4	u_Diffuse;
uniform vec3	u_Specular;
uniform float	u_SpecularPower;
uniform vec3	u_Emissive;
uniform vec3	u_LightDir;
uniform vec3	u_LightColor;

uniform int u_TexMode;
uniform sampler2D	u_Tex;

uniform int u_SphereTexMode;
uniform sampler2D u_SphereTex;

// Output
out vec4 fs_Color;

vec4 BlendVec4(vec4 color, vec4 tex)
{
	return color * tex;
}

void main()
{
	vec3 lightDir = normalize(u_LightDir);
	vec3 nor = normalize(vs_Nor);

	// Diffuse
	vec4 diffuse = u_Diffuse;

	float ln = dot(-lightDir, nor);
	if (ln < 0.0)
	{
		ln = -ln;
		nor = -nor;
	}
	ln = clamp(ln + 0.5, 0.0, 1.0);
	diffuse.rgb *= ln;
	diffuse.rgb *= u_LightColor;

	// Emissive
	vec3 emissive = u_Emissive;

	vec4 color = vec4(diffuse.rgb + emissive, diffuse.a);
	color = clamp(color, 0.0, 1.0);
	if (u_TexMode != 0)
	{
		vec4 texColor = texture(u_Tex, vs_UV);
		color = BlendVec4(color, texColor);
	}

	// SpTexture
	if (u_SphereTexMode != 0)
	{
		vec2 spUV = nor.xy * 0.5 + 0.5;
		spUV.y = 1.0 - spUV.y;
		vec3 spColor = texture(u_SphereTex, spUV).rgb;
		if (u_SphereTexMode == 1)
		{
			color.rgb *= spColor;
		}
		else if (u_SphereTexMode == 2)
		{
			color.rgb += spColor;
		}
	}

	// Specular
	vec3 specular = u_Specular;
	if (u_SpecularPower != 0.0 && ln > 0.0)
	{
		vec3 rn = reflect(-lightDir, nor);
		float cosAngle = max(0.0, dot(normalize(vs_Pos), rn));
		specular *= pow(cosAngle, u_SpecularPower);
		specular *= u_LightColor;
	}

	color.rgb += specular;

	fs_Color = color;
}
