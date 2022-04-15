#version 460

layout(binding = 0) uniform Uniform{
    mat4 vp;
};

// even though we don't use all of the attributes, we declared them so the vertex data is compatible with the multiMeshLayer
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

layout(binding = 3) readonly buffer Transforms{
    mat4 transforms[];
};

layout(push_constant) uniform PC {
    float factor;
};

void main(){
    // get vertex using PVP
    uint idx = indices[gl_VertexIndex];
    Vertex vtx = vertices[idx];

    // calculate position using factor and instance index
    gl_Position = vp * transforms[gl_InstanceIndex] * vec4(vtx.x * factor, vtx.y * factor, vtx.z * factor, 1.0);
}