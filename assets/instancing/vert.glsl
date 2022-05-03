#version 450
#pragma shader_stage(vertex)

layout (location = 0) in vec2 v_Position;
layout (location = 1) in vec3 v_Color;
layout (location = 2) in vec2 v_TexCoord;

layout (location = 3) in vec3 inRotnScale; // rotation float, scale float[2]
layout (location = 4) in vec2 inPosition;
layout (location = 5) in vec2 intexCoord00;
layout (location = 6) in vec2 intexCoordDelta;
layout (location = 7) in float intexID;

layout (binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out float texID;

void main() {
    float x = v_Position.x, y = v_Position.y;
    x = x * cos(inRotnScale[0]) - y * sin(inRotnScale [0]);
    y = x * sin(inRotnScale[0]) + y * cos(inRotnScale [0]);
    x *= inRotnScale[1];
    y *= inRotnScale[2];
    x += inPosition.x;
    y += inPosition.y;
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(vec2(x, y), 0.0, 1.0);

    fragColor = v_Color;
    fragTexCoord = intexCoord00 + v_TexCoord * intexCoordDelta;
    texID = intexID;
}