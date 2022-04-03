#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

//layout(binding = 3) uniform sampler2D texSampler;

const vec3 lightPos = vec3(1.0);

void main() {

    //outColor = texture(texSampler, uv);
    vec3 L = normalize(lightPos - worldPos);
    vec3 N = normalize(normal);

    float ambient = 0.2;
    float diffuse = max(0.0, dot(L, N));

    float total = ambient + diffuse;


    outColor = vec4(total, total, total, 1.0);
}