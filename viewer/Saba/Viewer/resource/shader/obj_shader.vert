#version 140

// Input
in vec3	in_Pos;
in vec3	in_Nor;
#ifdef USE_UV
in vec2	in_UV;
#endif // USE_UV

// Uniform
uniform	mat4	u_WV;
uniform	mat4	u_WVP;
uniform mat3	u_WVIT;

// Output
out vec3	vs_Pos;
out vec3	vs_Nor;
#ifdef USE_UV
out vec2	vs_UV;
#endif // USE_UV


void main()
{
	gl_Position = u_WVP * vec4(in_Pos, 1.0);
	vs_Pos = (u_WV * vec4(in_Pos, 1.0)).xyz;
	vs_Nor = u_WVIT * in_Nor;
	#ifdef USE_UV
	vs_UV = in_UV;
	#endif // USE_UV
}
