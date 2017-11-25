#version 140

uniform vec4	u_ShadowColor;

// Output
out vec4 fs_Color;

void main()
{
	fs_Color = u_ShadowColor;
}
