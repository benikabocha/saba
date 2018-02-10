#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNor;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 wv;
	mat4 wvp;
} ubo;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNor;
layout (location = 2) out vec2 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
	gl_Position = ubo.wvp * vec4(inPos.xyz, 1.0);
	outPos = (ubo.wv * vec4(inPos.xyz, 1.0)).xyz;
	outNor = mat3(ubo.wv) * inNor;
	outUV = vec2(inUV.x, 1.0 - inUV.y);
}
