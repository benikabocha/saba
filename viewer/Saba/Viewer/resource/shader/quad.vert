#version 140

out vec2 vs_UV;

void main()
{
    const vec2 positions[4] = vec2[4](
        vec2(-1.0, 1.0f),
        vec2(-1.0, -1.0f),
        vec2(1.0, 1.0f),
        vec2(1.0, -1.0f)
    );

    const vec2 uvs[4] = vec2[4](
        vec2(0.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0)
    );

	gl_Position = vec4(positions[gl_VertexID % 4], 0.0, 1.0);
	vs_UV = uvs[gl_VertexID % 4];
}
