#version 140

in vec3 in_Pos;
in vec3 in_Color;

out vec3 vs_Color;

uniform	mat4 u_WVP;

void main()
{
	gl_Position = u_WVP * vec4(in_Pos, 1.0);
	vs_Color = in_Color;
}
