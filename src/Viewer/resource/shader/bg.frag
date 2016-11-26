#version 140

in vec2 vs_Pos;

uniform	vec3 u_Color1;
uniform vec3 u_Color2;

out vec4 fs_Color;

void main()
{
	fs_Color.rgb = mix(u_Color2, u_Color1, (vs_Pos.y + 1.0) * 0.5);
	fs_Color.a = 0.0;
}
