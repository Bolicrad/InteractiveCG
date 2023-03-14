#version 410 core

in vec2 texCoord;
out vec4 color;

uniform sampler2D shadow;

void main(){
    float depthvalue = texture(shadow,texCoord).r;
    color = vec4(vec3(depthvalue),1.0);
}