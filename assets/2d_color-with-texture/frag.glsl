#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;

// binding 1 used for transformation data
layout (binding = 1) uniform sampler2D textureSampler;

layout (location = 0) out vec4 outColor;

void main() {
    vec4 tex_color = texture(textureSampler, fragTexCoord);
    vec4 frag_color = vec4(fragColor, 1.0);
    outColor = tex_color * frag_color; // vec4(fragTexCoord, 0.0, 1.0); // vec4(fragColor, 1.0);
}