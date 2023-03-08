#version 410 core
layout(location=0)in vec3 iPos;
uniform mat4 mvp;
void main()
{
    gl_Position = mvp * vec4(iPos, 1);
}