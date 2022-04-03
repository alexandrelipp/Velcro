#version 460

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 worldPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushPos{
    vec3 cameraWorldPos;
    float specularS; // TODO : remove?
} push;

//layout(binding = 3) uniform sampler2D texSampler;

// TODO : not be hard coded!
const vec3 lightPos = vec3(1.0);

void main() {

    //outColor = texture(texSampler, uv);
    vec3 L = normalize(lightPos - worldPos);
    vec3 N = normalize(normal);

    // calculate reflection over normal from world to camera pos vector
    vec3 view = normalize(push.cameraWorldPos - worldPos);
    vec3 rView = reflect(-view, N);

    float ambient = 0.2;
    float diffuse = max(0.0, dot(L, N));
    float specular = push.specularS * pow(max(dot(L, rView), 0.0), 32); // specular color as material instead of white?

    float total = ambient + diffuse + specular;


    outColor = vec4(total, total, total, 1.0);
}