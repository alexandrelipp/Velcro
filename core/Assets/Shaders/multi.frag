#version 460

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

//layout(binding = 3) uniform sampler2D texSampler;

void main() {
    //outColor = texture(texSampler, uv);
    outColor = vec4(1.0);
}