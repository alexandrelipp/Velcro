#version 460

layout(location = 0) out vec4 color;

layout(binding = 0) uniform Uniform{
    mat4 mvp[2];
};

struct Vertex{
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    float u;
    float v;
};

layout(binding = 1) readonly buffer Vertices{
    Vertex vertices[];
};

layout(binding = 2) readonly buffer Indices{
    uint indices[];
};

void main(){
    // get vertex using PVP
    uint idx = indices[gl_VertexIndex];
    Vertex vtx = vertices[idx];

    gl_Position = mvp[gl_InstanceIndex] * vec4(vtx.x, vtx.y, vtx.z, 1.0);
   //gl_Position = mvptest * vec4(vtx.x, vtx.y, 0.0, 1.0);
    if (gl_InstanceIndex == 0)
        color = vec4(1.0, 0.0, 0.0, 1.0);
    else
        color = vec4(0.0, 0.0, 1.0, 1.0);
}