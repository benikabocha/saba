#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNor;

layout (binding = 0) uniform VSModelUBO
{
	mat4 wv;
	mat4 wvp;
	vec2 screenPos;
} modelUBO;

layout (binding = 1) uniform VSMaterialUBO
{
	float edgeSize;
} matUBO;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    vec3 nor = mat3(modelUBO.wv) * inNor;
    vec4 pos = modelUBO.wvp * vec4(inPos, 1.0);
    vec2 screenNor = normalize(vec2(nor));
    pos.xy += screenNor * vec2(1.0) / (modelUBO.screenPos *0.5) * matUBO.edgeSize * pos.w;
    gl_Position = pos;
}
