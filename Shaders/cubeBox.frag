#version 450

//layout(location = 0) in vec4 fragColor;
//layout(location = 1) in vec2 TexCoords;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D depthTexture;

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 LightPos;
    vec3 CameraPos;
    vec3 inColor;
    vec2 TexCoords;
} fs_in;

void main() {
    vec3 color = fs_in.inColor;
    vec3 ambient = 0.01f * color;
    vec3 lightDir = normalize(fs_in.LightPos - fs_in.FragPos);
    vec3 normal = normalize(fs_in.Normal);
    
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;

    vec3 viewDir = normalize(fs_in.CameraPos - fs_in.FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = 0.0;
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.3f) * spec;
    outColor = vec4(diffuse, 1.0f);
}