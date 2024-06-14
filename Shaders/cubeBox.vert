#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 TexCoords;

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProjection;
    vec3 pos;
    vec3 lightPos;
} ubo;


vec3 BlingPhong();
vec3 TestNormal();
vec3 OriginalColor();

void main() {
    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);

    vec3 specular = BlingPhong();

    fragColor = specular;
}

vec3 TestNormal(){
    return abs(inNormal);
}

vec3 OriginalColor(){
    return inColor;
}

vec3 BlingPhong(){
    vec3 lightDir = normalize(ubo.lightPos - inPosition);
    vec3 viewDir = normalize(ubo.pos - inPosition);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float shininess = 0.9f;

    float spec = pow(max(dot(inNormal, halfwayDir),0), shininess);

    vec3 color = abs(inNormal) * spec;

    return color;
}