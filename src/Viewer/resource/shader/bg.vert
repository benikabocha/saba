#version 140

out vec2 vs_Pos;

void main()
{
	float x = (gl_VertexID & 0x1) == 0 ? 0.0 : 1.0;
	float y = (gl_VertexID & 0x2) == 0 ? 0.0 : 1.0;
	vs_Pos = vec2(x, y);
	gl_Position = vec4(vec2(x, y) * 2.0 - 1.0, 1.0, 1.0);
}
