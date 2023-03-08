# version 410 core

in vec3 oPos;
in vec2 oTexCoord;

out vec4 color;

uniform sampler2D tex;

void main(){
    color = texture(tex, oTexCoord);
}