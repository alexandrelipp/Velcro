#version 460

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2  o_uv;

layout(push_constant) uniform PushConstant{
    vec3 dim;
};

void main(){
    vec2 position = (pos + dim.xy) * dim.z;
    gl_Position = vec4(position, 0.0, 1.0);
    o_uv = uv;
}