#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in flat uint materialIndex;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushPos{
    vec3 cameraWorldPos;
} push;

// TODO : use vecs!!!
struct Material{
    float ax;
    float ay;
    float az;
    float dx;
    float dy;
    float dz;
    float sx;
    float sy;
    float sz;
};

layout(binding = 5) readonly buffer Materials{
    Material materials[];
};

// TODO : not be hard coded!
const vec3 lightPos = vec3(10.0);

void main() {
    Material material = materials[materialIndex];

    //outColor = texture(texSampler, uv);
    vec3 L = normalize(lightPos - worldPos);
    vec3 N = normalize(normal);

    // calculate reflection over normal from world to camera pos vector
    vec3 view = normalize(push.cameraWorldPos - worldPos);
    vec3 rView = reflect(-view, N);

    vec3 ambient = vec3(material.ax, material.ay, material.az);
    vec3 diffuse = vec3(material.dx, material.dy, material.dz) * max(0.0, dot(L, N));
    vec3 specular = vec3(material.sx, material.sy, material.sz) * pow(max(dot(L, rView), 0.0), 32);

    vec3 total = ambient + diffuse + specular;


    outColor = vec4(total, 1.0);
}