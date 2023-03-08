#version 410 core
layout(location=0)in vec3 iPos;

out vec2 oTexCoord;

uniform mat4 mvp;
uniform float aspectRatio;

void main()
{
    oTexCoord = vec2(iPos.x/aspectRatio,iPos.y)/2 + vec2(0.5, 0.5);
    gl_Position = mvp * vec4(iPos, 1);
}