#version 140

#define NUM_SHADOWMAP 4

in vec3 in_Pos;
in vec3 in_Nor;
in vec2 in_UV;

out vec3 vs_Pos;
out vec3 vs_Nor;
out vec2 vs_UV;

out vec4 vs_shadowMapCoord[NUM_SHADOWMAP];

uniform mat4 u_WV;
uniform mat4 u_WVP;
uniform mat4 u_LightWVP[NUM_SHADOWMAP];

void main()
{
    gl_Position = u_WVP * vec4(in_Pos, 1.0);
    vs_Pos = (u_WV * vec4(in_Pos, 1.0)).xyz;
    vs_Nor = mat3(u_WV) * in_Nor;
    vs_UV = in_UV;

    for (int i = 0; i < NUM_SHADOWMAP; i++)
    {
        vs_shadowMapCoord[i] = u_LightWVP[i] * vec4(in_Pos, 1.0);
    }
}
