#version 140
#if ENABLE_CODE
out vec4 fs_Color;
void main()
{
	fs_Color = vec4(1.0, 1.0, 0.0, 1.0);
}
#else
test
#endif
