#version 460

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D texSampler;

vec3 colorD = vec3(0.0);

void main(){
//    float alpha = texture(texSampler, uv).r;
//    // TODO : set as 0 prob better
//    if (alpha < 0.5)
//        discard;


    color = vec4(vec3(texture(texSampler, uv).x), 1.0);
    //color = vec4(1.0, 1.0, 1.0, 1.0);
}