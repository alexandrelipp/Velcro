#version 460

// color defined when shader is compiled (from spirV -> machine). "'constant_id' : can only be applied to a scalar"
layout (constant_id = 0) const float R = 0.0;
layout (constant_id = 1) const float G = 0.0;
layout (constant_id = 2) const float B = 0.0;
layout (constant_id = 3) const float A = 0.0;

layout(location = 0) out vec4 color;

void main(){
    color = vec4(R, G, B, A);
}