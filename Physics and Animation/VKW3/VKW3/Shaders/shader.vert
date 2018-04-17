#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;


layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;
out gl_PerVertex
{
	vec4 gl_Position;
};


layout(push_constant) uniform PushConstants
{
	uint modelIndex;
	uint red;
	uint green;
	uint blue;
} pushConstants;


layout(binding = 0) uniform Camera
{
	mat4 view;
	mat4 projection;
} camera;
layout(binding = 1) uniform ModelMatrices
{
	mat4 matrix[3];
} model;
layout(binding = 2) uniform BoneMatrices
{
	mat4 matrix[64];
} bones;


void main() {
	gl_Position = camera.projection * camera.view * model.matrix[pushConstants.modelIndex] * vec4(inPosition, 1.0);
	outUV = inUV;
	outColor = vec4(pushConstants.red, pushConstants.green, pushConstants.blue, 1.0f);
}
