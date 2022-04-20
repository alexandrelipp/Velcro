#version 460

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D texSampler;

vec3 colorD = vec3(0.0);

void main(){
    // TODO what alpha do we use ??
    color = vec4(vec3(texture(texSampler, uv).x), 1.0);
    //color = vec4(1.0, 0.0, 0.0, texture(texSampler, uv).x);

    //vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texSampler, uv).r);
    //color = vec4(0.0, 0.0, 0.0 , 1.0) * sampled;

    //color = vec4(1.0, 1.0, 1.0, 0.2);
}