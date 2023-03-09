#version 410 core
layout(location=0)in vec3 iPos;
layout(location=1)in vec3 iNormal;

out vec3 worldPos;
out vec3 worldNormal;

uniform mat4 m;
uniform mat3 mN;
uniform mat4 vp;
uniform vec3 mirPos;
uniform vec3 mirNorm;

void main()
{
    worldPos = vec3(m * vec4(iPos, 1));
    worldNormal = mN * iNormal;

    vec3 posM = worldPos - mirPos;
    vec3 rPosM = reflect(posM, mirNorm);
    vec3 rPos = rPosM + mirPos;

    gl_Position = vp * vec4(rPos, 1);
}