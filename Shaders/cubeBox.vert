#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 pos;
    vec3 lightPos;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition+vec3(0,0,0), 1.0);
    
    vec3 lightDir = normalize(ubo.lightPos - inPosition);
    vec3 viewDir = normalize(ubo.pos - inPosition);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float shininess = 2.f;

    float spec = pow(max(dot(inNormal, halfwayDir),0), shininess);

    vec3 specular = inColor * spec;

    fragColor = specular;
}