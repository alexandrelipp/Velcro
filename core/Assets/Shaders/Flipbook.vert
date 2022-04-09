#version 460

layout(location = 0) out vec2 uv;
layout(location = 1) out flat uint texIndex;

struct Vertex{
    float x;
    float y;
    float u;
    float v;
};

layout(binding = 0) readonly buffer Vertices{
    Vertex vertices[];
};

layout(push_constant) uniform PushConstant {
    uint textureIndex;
} pushConstant;

void main(){
    Vertex vtx = vertices[gl_VertexIndex];
    gl_Position = vec4(vtx.x, vtx.y, 0.0, 1.0);
    uv = vec2(vtx.u, vtx.v);
    texIndex = pushConstant.textureIndex;
}