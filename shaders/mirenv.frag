# version 410 core
in vec3 oDir;
out vec4 color;

uniform vec3 camPos;
uniform vec3 mirNorm;
uniform samplerCube skybox;

void main(){
    vec3 viewDir = normalize(camPos - oDir);
    vec3 reflectDir = reflect(-viewDir,mirNorm);
    vec4 C_Env = texture(skybox, reflectDir);
    color = C_Env;
}