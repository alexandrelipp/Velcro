#version 450

layout(location = 0) out vec4 color;

layout(binding = 0) uniform UniformBuffer{
    mat4 mvp;
} ubo;

struct Vertex{
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
    float a;
};

layout(binding = 1) readonly buffer Vertices{
    Vertex vertices[];
};

void main(){
    Vertex vtx = vertices[gl_VertexIndex];
    gl_Position = ubo.mvp * vec4(vtx.x, vtx.y, vtx.z, 1.0);
    color = vec4(vtx.r, vtx.g, vtx.b, vtx.a);
}