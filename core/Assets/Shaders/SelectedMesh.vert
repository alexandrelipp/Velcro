#version 460

layout(binding = 0) uniform Uniform{
    mat4 mvp;
};

struct Vertex{
    float x;
    float y;
    float u;
    float v;
};

layout(binding = 1) readonly buffer Vertices{
    Vertex vertices[];
};

void main(){
    Vertex vtx = vertices[gl_VertexIndex];
    gl_Position = mvp * vec4(vtx.x, vtx.y, 0.0, 1.0);
}