# version 410 core

in vec3 oPos;
in vec3 oNormal;
in vec2 oTexCoord;

out vec4 color;

uniform vec3 lightDir;
uniform sampler2D tex;
uniform sampler2D texS;

void main(){
    vec3 normal = normalize(oNormal);
    vec3 viewDir = normalize(-oPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    vec3 Kd = texture(tex, oTexCoord).rgb;
    vec3 Ks = texture(texS, oTexCoord).rgb;
    int alpha = 10;
    float Intensity = 1;
    float Intensity_A = 0.2;

    vec3 C_Ambient = Intensity_A * Kd;
    vec3 C_Diffuse = Intensity * max(dot(normal, lightDir),0.0)* Kd;
    vec3 C_Specular = Intensity * pow(max(dot(normal, halfDir), 0.0), alpha) * Ks;
    vec3 C_Blinn = C_Ambient + C_Diffuse + C_Specular;

    color = vec4(C_Blinn,1);
}