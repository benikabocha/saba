#version 140

in vec3 in_Pos;
in vec3 in_Nor;
in vec2 in_UV;

out vec3 vs_Pos;
out vec3 vs_Nor;
out vec2 vs_UV;

uniform mat4 u_WV;
uniform mat4 u_WVP;

void main()
{
    gl_Position = u_WVP * vec4(in_Pos, 1.0);
    vs_Pos = (u_WV * vec4(in_Pos, 1.0)).xyz;
    vs_Nor = mat3(u_WV) * in_Nor;
    vs_UV = in_UV;
}
