#version 410 core
layout(location=0)in vec3 iPos;
layout(location=1)in vec2 iTexCoord;

out vec3 worldPos;
out vec4 lightViewPos;
out vec2 texCoord;

uniform mat4 m;
uniform mat4 mvp;
uniform mat4 matrixShadow;

void main()
{
    worldPos = vec3(m * vec4(iPos, 1));
    lightViewPos = matrixShadow * vec4(iPos, 1);
    texCoord = iTexCoord;
    gl_Position = mvp * vec4(iPos, 1);
}