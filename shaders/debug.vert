#version 410 core
layout(location=0)in vec3 iPos;

out vec2 texCoord;

uniform mat4 mvp;
uniform float length;

void main()
{
    texCoord = vec2(iPos.x, iPos.y)/length + vec2(0.5,0.5);
    gl_Position = mvp * vec4(iPos, 1);
}