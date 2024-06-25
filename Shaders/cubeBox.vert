#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

//layout(location = 0) out vec3 fragColor;
//layout(location = 1) out vec2 TexCoords;

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProjection;
    vec3 pos;
    vec3 lightPos;
} ubo;

layout(binding = 2) uniform second{
    vec3 cameraPos;
} sh;

layout(location = 0) out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 LightPos;
    vec3 CameraPos;
    vec3 inColor;
    vec2 TexCoords;
} vs_out;

void main() {
    vs_out.FragPos = inPosition;
    vs_out.Normal = inNormal;
    vs_out.TexCoords = inTexCoord;
    vs_out.LightPos = ubo.lightPos;//sh.cameraPos;
    vs_out.CameraPos = ubo.pos;
    vs_out.inColor = inColor;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}