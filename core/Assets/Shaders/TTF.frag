#version 460

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D texSampler;

vec3 colorD = vec3(0.0);

void main(){
    color = vec4(colorD, texture(texSampler, uv).x);
}