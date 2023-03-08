#version 410 core
layout(location=0)in vec3 iPos;
layout(location=1)in vec3 iNormal;
layout(location=2)in vec2 iTexCoord;

out vec3 oPos;
out vec3 oNormal;
out vec2 oTexCoord;

uniform mat4 mv;
uniform mat3 mvN;
uniform mat4 mvp;

void main()
{
    oPos = vec3(mv * vec4(iPos, 1));
    oNormal = mvN * iNormal;
    oTexCoord = iTexCoord;
    gl_Position = mvp * vec4(iPos, 1);
}