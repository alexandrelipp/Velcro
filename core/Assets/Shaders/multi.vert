#version 460

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 normal;

layout(binding = 0) uniform UniformBuffer{
    mat4 vp;
} ubo;

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

layout(binding = 3) readonly buffer Xforms{
    mat4 transforms[];
};

void main() {
    // get vertex using PVP
    uint idx = indices[gl_VertexIndex];
    Vertex vertex = vertices[idx];

    // get model transform using baseInstance (defined in VK_DRAW_INDIRECT)
    mat4 model = transforms[gl_BaseInstance];

    // calculate normal (transpose + inverse for non uniform scale)
    // mat3 test = inverse(transpose(mat3(model))); // Is this better ??
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    normal = normalMatrix * vec3(vertex.nx, vertex.ny, vertex.nz);

    // calculate tex coords + vertex pos
    uv = vec2(vertex.u, vertex.v);
    gl_Position = ubo.vp * model * vec4(vertex.x, vertex.y, vertex.z, 1.0);
}