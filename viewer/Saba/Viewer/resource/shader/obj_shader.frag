#version 140

// Input
in vec3	vs_Pos;
in vec3	vs_Nor;
#ifdef USE_UV
in vec2	vs_UV;
#endif // USE_UV

uniform vec3	u_Ambient;
uniform vec3	u_Diffuse;
uniform vec3	u_Specular;
uniform float	u_SpecularPower;
uniform float	u_Transparency;
uniform vec3	u_LightDir;
uniform vec3	u_LightColor;

#ifdef	USE_AMBIENT_TEX
uniform sampler2D	u_AmbientTex;
#endif	// USE_AMBIENT_TEX

#ifdef	USE_DIFFUSE_TEX
uniform sampler2D	u_DiffuseTex;
#endif	// USE_DIFFUSE_TEX

#ifdef	USE_SPECULAR_TEX
uniform sampler2D	u_SpecularTex;
#endif	// USE_SPECULAR_TEX

#ifdef	USE_TRANSPARENCY_TEX
uniform sampler2D	u_TransparencyTex;
#endif	// USE_TRANSPARENCY_TEX

// Output
out vec4 fs_Color;

vec3 BlendVec3(vec3 color, vec3 tex)
{
#ifdef	USE_BLEND_TEX_COLOR
	return color * tex;
#else	// USE_BLEND_TEX_COLOR
	return tex;
#endif	// USE_BLEND_TEX_COLOR
}

void main()
{
	vec3 lightDir = normalize(u_LightDir);
	vec3 nor = normalize(vs_Nor);

	// Ambient
	vec3 ambient = u_Ambient;
	#ifdef	USE_AMBIENT_TEX
	vec3 ambientTex = texture(u_AmbientTex, vs_UV).rgb;
	ambient = BlendVec3(ambient, ambientTex);
	#endif	// USE_AMBIENT_TEX

	// Diffuse
	vec3 diffuse = u_Diffuse;
	#ifdef	USE_DIFFUSE_TEX
	vec3 diffuseTex = texture(u_DiffuseTex, vs_UV).rgb;
	diffuse = BlendVec3(diffuse, diffuseTex);
	#endif	// USE_DIFFUSE_TEX

	float ln = dot(-lightDir, nor);
	diffuse *= max(0.0, ln);
	diffuse *= u_LightColor;

	// Specular
	vec3 specular = u_Specular;
	#ifdef	USE_SPECULAR_TEX
	vec3 specularTex = texture(u_SpecularTex, vs_UV).rgb;
	specular = BlendVec3(specular, specularTex);
	#endif	// USE_SPECULAR_TEX
	if (ln > 0.0)
	{
		vec3 rn = reflect(-lightDir, nor);
		float cosAngle = max(0.0, dot(normalize(vs_Pos), rn));
		specular *= pow(cosAngle, u_SpecularPower);
		specular *= u_LightColor;
	}

	fs_Color.rgb = ambient + diffuse + specular;
	fs_Color.a = 1.0;
}
