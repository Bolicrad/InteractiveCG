# version 410 core
in vec3 oDir;
in vec4 gl_FragCoord;
out vec4 color;

uniform sampler2DRect tex;

void main(){
    vec4 C_Tex = texture(tex, gl_FragCoord.xy);
    if(C_Tex.w < 0.1f)discard;
    color = C_Tex;
}