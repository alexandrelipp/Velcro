#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 uv;
layout(location = 1) in flat uint texIndex;

layout(location = 0) out vec4 color;

layout(binding = 1) uniform sampler2D textures[];

void main(){
    color = texture(textures[nonuniformEXT(texIndex)], uv);
    //color = vec4(uv, 0.0, 1.0);
}