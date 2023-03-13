# version 410 core

in vec3 worldPos;
in vec3 worldNormal;

out vec4 color;

uniform vec3 camPos;
uniform vec3 spotDir;
uniform vec3 lightPos;
uniform float lightFovRad;

void main(){
    vec3 normal = normalize(worldNormal);
    vec3 viewDir = normalize(camPos - worldPos);
    vec3 lightDir = normalize(lightPos - worldPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    vec3 Kd = vec3(1,0,0);
    float Intensity_A = 0.2;
    vec3 C_Ambient = Intensity_A * Kd;

    float angle = max(acos(dot(spotDir, -lightDir)),0);
    if(angle > lightFovRad){
        //out of the range of the spot light;
        color = vec4(C_Ambient,1);
        return;
    };

    vec3 Ks = vec3(1,1,1);
    int alpha = 10;
    float Intensity = 1;

    vec3 C_Diffuse = Intensity * max(dot(normal, lightDir),0.0)* Kd;
    vec3 C_Specular = Intensity * pow(max(dot(normal, halfDir), 0.0), alpha) * Ks;
    vec3 C_Blinn = C_Ambient + C_Diffuse + C_Specular;

    color = vec4(C_Blinn,1);
}