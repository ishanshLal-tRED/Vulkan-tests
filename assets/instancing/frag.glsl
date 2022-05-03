#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;
layout (location = 2) flat in float texID;

// binding 0 used for transformation data
layout (binding = 1) uniform sampler2D textureSampler;
layout (binding = 2) uniform sampler2D atlasSampler;

layout (location = 0) out vec4 outColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}
float pxRange = 2;
float screenPxRange() {
    vec2 unitRange = vec2 (pxRange)/vec2 (textureSize(atlasSampler, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(fragTexCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}
void main() {
    if (texID < 0.5) {
         vec4 msd = texture(atlasSampler, fragTexCoord);
        float sd = median(msd.r, msd.g, msd.b);
        float screenPxDistance = screenPxRange()*(sd - 0.5);
        float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
        vec3 bgColor = vec3 (0);
        outColor = vec4(mix(bgColor, fragColor, opacity), msd.a);
    } else {
        outColor = texture(textureSampler, fragTexCoord) * vec4(fragColor, 1.0);
    }
    
}
