#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 texCoords;

layout (binding = 1) uniform sampler2D skybox[6];

void main()
{    
    outColor = texture(skybox[0], texCoords.xy);
}