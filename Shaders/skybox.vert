#version 450

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) out vec3 texCoords;

void main()
{
    texCoords = aPos;
    vec4 pos = ubo.proj * ubo.view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}