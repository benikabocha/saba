#version 140
#if ENABLE_CODE
in vec3 in_Pos;
void main()
{
	gl_Position = vec4(in_Pos, 1.0);
}
#else
test
#endif
