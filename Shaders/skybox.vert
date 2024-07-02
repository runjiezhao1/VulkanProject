#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProjection;
    vec3 pos;
    vec3 lightPos;
} ubo;

layout (location = 0) out vec2 texCoords;
layout (location = 1) out vec3 transColor;

void main()
{
    texCoords = inTexCoord;
    transColor = inColor;
    
	//mat4 viewMat = mat4(mat3(ubo.model));
	//gl_Position = ubo.proj * viewMat * vec4(aPos, 1.0);

    vec4 pos = ubo.proj * mat4(mat3(ubo.view)) * vec4(aPos.x, -aPos.y, aPos.z, 1.0);
    //gl_Position = vec4(pos.xy,1,1);
    gl_Position = pos.xyww;
    //gl_Position = vec4(aPos.xy,1,1);
}