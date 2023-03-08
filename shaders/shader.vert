#version 410 core
layout(location=0)in vec3 iPos;
layout(location=1)in vec3 iNormal;

out vec3 oPos;
out vec3 oNormal;

uniform mat4 mv;
uniform mat3 mvN;
uniform mat4 mvp;

void main()
{
    oPos = vec3(mv * vec4(iPos, 1));
    oNormal = mvN * iNormal;
    gl_Position = mvp * vec4(iPos, 1);
}