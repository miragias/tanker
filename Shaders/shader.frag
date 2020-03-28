#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float gamma;
} ubo;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragTexCoord, 0.0 , 1.0);
    outColor = texture(texSampler, fragTexCoord * 1.0);
    outColor.r = pow(outColor.r , ubo.gamma);
    outColor.g = pow(outColor.g , ubo.gamma);
    outColor.b = pow(outColor.b , ubo.gamma);
}
