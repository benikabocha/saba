#version 140

in vec3 in_Pos;
in vec3 in_Nor;

uniform mat4 u_WV;
uniform mat4 u_WVP;
uniform vec2 u_ScreenSize;
uniform float u_EdgeSize;

void main()
{
    vec3 nor = mat3(u_WV) * in_Nor;
    vec4 pos = u_WVP * vec4(in_Pos, 1.0);
    vec2 screenNor = normalize(vec2(nor));
    pos.xy += screenNor * vec2(1.0) / (u_ScreenSize *0.5) * u_EdgeSize * pos.w;
    gl_Position = pos;
}
