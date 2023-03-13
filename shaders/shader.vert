#version 410 core
layout(location=0)in vec3 iPos;
layout(location=1)in vec3 iNormal;

out vec3 worldPos;
out vec3 worldNormal;

uniform mat4 m;
uniform mat3 mN;
uniform mat4 mvp;

void main()
{
    worldPos = vec3(m * vec4(iPos, 1));
    worldNormal = mN * iNormal;
    gl_Position = mvp * vec4(iPos, 1);
}