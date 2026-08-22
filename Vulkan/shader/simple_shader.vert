#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPosition;
layout(location = 4) out vec4 fragCameraPos; 

layout(location = 3) in float inFlow;
layout(location = 3) out float fragFlow;


void main() {
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0); 
    fragCameraPos = ubo.cameraPos;
    fragFlow = inFlow;
    gl_Position = pos;
    fragColor = inColor;
    fragNormal = normalize(inNormal);
    fragWorldPosition =   (ubo.model * vec4(inPosition, 1.0) ).xyz;
}