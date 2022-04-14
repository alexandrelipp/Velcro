#version 460

layout(location = 0) out vec4 color;

layout(binding = 0) uniform Uniform{
    mat4 mvp[2];
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
    gl_Position = mvp[gl_InstanceIndex] * vec4(vtx.x, vtx.y, 0.0, 1.0);
    if (gl_InstanceIndex == 0)
        color = vec4(1.0, 0.0, 0.0, 1.0);
    else
        color = vec4(0.0, 0.0, 1.0, 1.0);
}