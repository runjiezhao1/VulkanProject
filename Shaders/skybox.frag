#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 texCoords;

layout (location = 1) in vec3 transColor;

layout (binding = 1) uniform sampler2D skybox;

void main()
{    
    //outColor = vec4(transColor,1.0);
    outColor = texture(skybox, texCoords);
}