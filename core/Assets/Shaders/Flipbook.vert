#version 460

layout(location = 0) out vec2 uv;
layout(location = 1) out flat uint texIndex;

struct Vertex{
    float x;
    float y;
    float u;
    float v;
}

layout(binding = 0) readonly buffer vertices{
    Vertex data[];
}

layout(push_constant) uniform pushConstant {
    uint textureIndex;
}

void main(){
    Vertex vtx = vertices.data[gl_VertexIndex];
    gl_Position = vec4(vtx.x, vtx.y, 0.0, 0.0);
    uv = vec2(vtx.u, vtx.v);
    texIndex = pushConstant.textureIndex;
}