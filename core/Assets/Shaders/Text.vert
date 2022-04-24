#version 460

layout(location = 0) in vec2 pos;

layout(location = 0) out vec2 o_uv;

layout(binding = 0) readonly buffer MVPs{
    mat4 mvps[];
};

layout(binding = 1) readonly buffer TexCoords{
    vec2 coords[];
};

void main(){
    gl_Position = mvps[gl_InstanceIndex] * vec4(pos, 0.0, 1.0);
    //o_uv = uv;
    o_uv = coords[gl_InstanceIndex * 4 + gl_VertexIndex];
}