#version 140

// Input
in vec3	in_Pos;
in vec3	in_Nor;
in vec2	in_UV;

// Uniform
uniform	mat4	u_WV;
uniform	mat4	u_WVP;
uniform mat3	u_WVIT;

// Output
out vec3	vs_Pos;
out vec3	vs_Nor;
out vec2	vs_UV;


void main()
{
	gl_Position = u_WVP * vec4(in_Pos, 1.0);
	vs_Pos = (u_WV * vec4(in_Pos, 1.0)).xyz;
	vs_Nor = u_WVIT * in_Nor;
	vs_UV = in_UV;
}
