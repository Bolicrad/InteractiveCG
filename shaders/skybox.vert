#version 410 core
layout(location=0)in vec3 iPos;
out vec3 oDir;
uniform mat4 vp;

void main()
{
    oDir = iPos;
    gl_Position = vp * vec4(iPos, 1);
}