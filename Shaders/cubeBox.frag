#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 TexCoords;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D depthTexture;

void main() {
    outColor = vec4(fragColor, 1.0);

    //float depthValue = texture(depthTexture, TexCoords).r;
    //outColor = vec4(vec3(depthValue), 1.0);
}