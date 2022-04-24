#version 460

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 color;

layout(binding = 2) uniform sampler2D msdf;

float median(vec3 col) {
    return max(min(col.r, col.g), min(max(col.r, col.g), col.b));
}

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange() {
    float pxRange = 5.0;
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(msdf, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(texCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

vec3 colorD = vec3(0.0);

void main(){
    vec4 bgColor = vec4(0.0);
    vec4 fgColor = vec4(1.0);

    vec3 msd = texture(msdf, texCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);

    #if 0
    float alpha = smoothstep(0.48, 0.52, sd);
    //float alpha = step(0.5, sd);
    color = vec4(1.0, 1.0, 1.0, alpha);
    #else
    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    //float alpha = smoothstep(0.3, 0.7, screenPxDistance);
    //color = vec4(1.0, 1.0, 1.0, alpha);
    color = mix(bgColor, fgColor, opacity);
    #endif


//    vec3 msd = texture(texSampler, uv).rgb;
//    //float sd = median(texture(texSampler, uv).rgb);
//    float sd = median(msd.r, msd.g, msd.b);
//    float alpha = smoothstep(0.47, 0.53, sd);
//    color = vec4(1.0, 1.0, 1.0, alpha);
//        if (sd < 0.5)
//            discard;
//
//
//
//        color = vec4(1.0, 1.0, 1.0, 1.0);
//    float screenPxDistance = screenPxRange()*(sd - 0.5);
//    float opacity  = clamp(screenPxDistance + 0.5, 0.0, 1.0);
//    float alpha = smoothstep(0.47, 0.53, opacity);
//    color = vec4(1.0, 1.0, 1.0, alpha);
    //color = texture(texSampler, uv).rgba;

//    float alpha = smoothstep(0.47, 0.53, value);
//    color = vec4(1.0, 1.0, 1.0, alpha);
    // TODO : set as 0 prob better
//    if (alpha < 0.5)
//        discard;
//
//
//    //color = vec4(vec3(texture(texSampler, uv).x), 1.0);
//    color = vec4(1.0, 1.0, 1.0, 1.0);
}