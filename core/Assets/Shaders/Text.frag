#version 460

layout (constant_id = 0) const float UNIT_RANGE_X = 0.0;
layout (constant_id = 1) const float UNIT_RANGE_Y = 0.0;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 color;

layout(binding = 2) uniform sampler2D msdf;

float median(vec3 col) {
    return max(min(col.r, col.g), min(max(col.r, col.g), col.b));
}

/*
TAKEN from https://github.com/Chlumsky/msdfgen
 Here, screenPxRange() represents the distance field range in output screen pixels. For example, if the pixel range was set to 2
 when generating a 32x32 distance field, and it is used to draw a quad that is 72x72 pixels on the screen, it should return 4.5
 (because 72/32 * 2 = 4.5). For 2D rendering, this can generally be replaced by a precomputed uniform value.

 For rendering in a 3D perspective only, where the texture scale varies across the screen, you may want to implement this
 function with fragment derivatives in the following way. I would suggest precomputing unitRange as a uniform variable
 instead of pxRange for better performance.*/
float screenPxRange() {
    float pxRange = 5.0;
    //vec2 unitRange = vec2(pxRange)/vec2(textureSize(msdf, 0));
    const vec2 unitRange = vec2(UNIT_RANGE_X, UNIT_RANGE_Y);
    vec2 screenTexSize = vec2(1.0)/fwidth(texCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

vec3 colorD = vec3(0.0);

void main(){
    vec3 msd = texture(msdf, texCoord).rgb;
    float sd = median(msd);

    #if 0
    float alpha = smoothstep(0.48, 0.52, sd);
    //float alpha = step(0.5, sd);
    color = vec4(1.0, 1.0, 1.0, alpha);
    #else
    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    color = vec4(1.0, 1.0, 1.0, opacity);
    #endif
}