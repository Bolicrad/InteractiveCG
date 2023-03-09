# version 410 core

in vec3 worldPos;
in vec3 worldNormal;

out vec4 color;

uniform vec3 camPos;
uniform vec3 lightDir;
uniform samplerCube skybox;

void main(){
    vec3 normal = normalize(worldNormal);
    vec3 viewDir = normalize(camPos - worldPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    vec3 reflectDir = reflect(-viewDir,normal);
    vec4 C_Env = texture(skybox, reflectDir);

    vec3 Kd = vec3(1,0,0);
    vec3 Ks = vec3(1,1,1);
    int alpha = 10;
    float Intensity = 1;
    float Intensity_A = 0.2;

    vec3 C_Ambient = Intensity_A * Kd;
    vec3 C_Diffuse = Intensity * max(dot(normal, lightDir),0.0)* Kd;
    vec3 C_Specular = Intensity * pow(max(dot(normal, halfDir), 0.0), alpha) * Ks;
    vec3 C_Blinn = C_Ambient + C_Diffuse + C_Specular;

    color = C_Env*vec4(Ks,1)+vec4(C_Specular,1);
}