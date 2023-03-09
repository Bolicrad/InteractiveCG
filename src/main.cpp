#define CY_NO_IMMINTRIN_H
#define gluErrorString(value) (#value)
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "cyCodeBase/cyGL.h"
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyMatrix.h"
#include "loadpng/lodepng.h"

//Parameters for Camera Control
double distance = 100;
double phi = 1.25*M_PI;
double theta = 0.5*M_PI;

//Parameters for Light Control
double phi_L = 0;
double theta_L = M_PI;
cyVec3f lightDir = cyVec3f(0,-1,0);

//Parameters of Camera
cyVec3f camPos = cyVec3f(0,-distance,0);
cyVec3f camTarget = cyVec3f(0,0,0);
cyVec3f camUp = cyVec3f (0,1,0);

//MVP Matrices
cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(0.25 * M_PI,1.6f,0.1f,1000.0f);
cy::Matrix4f viewMatrix = cy::Matrix4f::View(camPos,camTarget,camUp);
cy::Matrix4f modelMatrix = cy::Matrix4f::Identity();

//Containers for mesh & program
cy::TriMesh mesh;
cy::GLSLProgram program;
cy::GLSLProgram program_Skybox;
cy::GLSLProgram program_MirCam;
cy::GLSLProgram program_MirEnv;
cy::GLSLProgram program_Mirror;

//index for skybox data
GLuint vao_skybox;
GLuint vbo_skybox;

//Parameters for mouse control
int holdCount = 0;
bool firstMov = true;
double lastX;
double lastY;
bool ctrlPressed = false;

// Check and translate the model to the Center
bool centered = false;

#pragma region ClearColor
void HSV2RGB(int H, float S, float V, GLclampf& r, GLclampf& g, GLclampf& b){
    if(S>100)S=100;
    if(S<0)S=0;
    if(V>100)V=100;
    if(V<0)V=0;
    GLclampf s = S/100;
    GLclampf v = V/100;

    int i = H/60;
    int diff = H % 60;

    float min, max;
    max = v;
    min = max*(1-s);

    float adj = (max-min) * (diff / 60.0f);
    switch(i){
        case 0:
            r = max;
            g = min + adj;
            b = min;
            break;
        case 1:
            r = max - adj;
            g = max;
            b = min;
            break;
        case 2:
            r = min;
            g = max;
            b = min + adj;
            break;
        case 3:
            r = min;
            g = max - adj;
            b = max;
            break;
        case 4:
            r = min + adj;
            g = min;
            b = max;
            break;
        default:
            r = max;
            g = min;
            b = max - adj;
            break;
    }
}

void SetClearColor()
{
    GLclampf r,g,b;
    int hue = ((int)(glfwGetTime()*100/3))%360;
    HSV2RGB(hue,80,100,r,g,b);
    glClearColor(r, g, b, 1.0f);
}

#pragma endregion ClearColor

#pragma region CameraControl

cyVec3f CalculatePos(double &iTheta, double &iPhi){
    //Theta: (0,PI]
    if(iTheta > M_PI)iTheta = M_PI;
    if(iTheta <= 0)iTheta = 0.0001;

    //Phi: (0, 2*PI]
    if(iPhi > 2 * M_PI)iPhi -= 2 * M_PI;
    if(iPhi <= 0) iPhi += 2 * M_PI;

    return cy::Vec3f(
            sin(iTheta) * sin(iPhi),
            cos(iTheta),
            sin(iTheta) * cos(iPhi)
            );
}


void UpdateCam(){

    //Update World Camera
    camPos = - distance  *  CalculatePos(theta,phi);
    program.SetUniform("camPos",camPos.x,camPos.y,camPos.z);

    viewMatrix = cy::Matrix4f::View(camPos,camTarget,camUp);
    cy::Matrix4f mvp = projMatrix * viewMatrix * modelMatrix;
    program.SetUniformMatrix4("mvp",mvp.cell);


    //Update SkyBox
    cy::Matrix4f vp = projMatrix * viewMatrix;
    program_Skybox.SetUniformMatrix4("vp", vp.cell);

    cy::Matrix4f ivp = vp.GetInverse();
    cyVec3f skyboxVertices[3];
    skyboxVertices[0] = (ivp*cyVec3f(-1,-1,0.999)).XYZ()*1000;
    skyboxVertices[1] = (ivp*cyVec3f(-1,3,0.999)).XYZ()*1000;
    skyboxVertices[2] = (ivp*cyVec3f(3,-1,0.999)).XYZ()*1000;

    glBindBuffer(GL_ARRAY_BUFFER, vbo_skybox);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cyVec3f)*3, skyboxVertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //Update MirCam
    program_MirCam.SetUniform("camPos", camPos.x,camPos.y,camPos.z);
    program_MirCam.SetUniformMatrix4("vp",vp.cell);

    //Update MirEnv
    program_MirEnv.SetUniform("camPos", camPos.x, camPos.y, camPos.z);
    program_MirEnv.SetUniformMatrix4("vp", vp.cell);

    //Update Mirror
    program_Mirror.SetUniformMatrix4("vp", vp.cell);
}

void UpdateLight(){
    lightDir = (CalculatePos(theta_L,phi_L)).XYZ().GetNormalized();
    program.SetUniform("lightDir",lightDir.x,lightDir.y,lightDir.z);
    program_MirCam.SetUniform("lightDir",lightDir.x, lightDir.y, lightDir.z);
}

#pragma endregion CameraControl

void CheckCenter(){
    if(mesh.IsBoundBoxReady()){
        cyVec3f center = (mesh.GetBoundMax() + mesh.GetBoundMin())/2;
        modelMatrix.SetTranslation(-center);
        cy::Matrix3f mN = modelMatrix.GetInverse().GetTranspose().GetSubMatrix3();
        program.SetUniformMatrix4("m", modelMatrix.cell);
        program.SetUniformMatrix3("mN",mN.cell);
        program_MirCam.SetUniformMatrix4("m", modelMatrix.cell);
        program_MirCam.SetUniformMatrix3("mN", mN.cell);
        centered = true;

        //Update Camera
        UpdateCam();
    }
}

void CompileShader(){
    //Create Shader Programs
    program_MirCam.BuildFiles("../shaders/mircam.vert", "../shaders/shader.frag");
    program.BuildFiles("../shaders/shader.vert","../shaders/shader.frag");
    program_MirEnv.BuildFiles("../shaders/skybox.vert", "../shaders/mirenv.frag");
    program_Mirror.BuildFiles("../shaders/skybox.vert","../shaders/mirror.frag");
    program_Skybox.BuildFiles("../shaders/skybox.vert", "../shaders/skybox.frag");
    //Set MVP uniform
    UpdateCam();
    UpdateLight();
}

#pragma region InputCallbacks

void cb_MouseIdle(GLFWwindow* window, double posX, double posY){
    //if(firstMov)firstMov = false;
}

void cb_MouseAngles(GLFWwindow* window, double posX, double posY){
    //Change the Angle of camera
    if(firstMov){
        lastX = posX;
        lastY = posY;
        firstMov = false;
    }

    float offsetX = 0.1*(posX - lastX);
    float offsetY = 0.1*(posY - lastY);

    lastX = posX;
    lastY = posY;

    if(ctrlPressed){
        phi_L += offsetX / 180.0f * M_PI;
        theta_L -= offsetY / 180.0f * M_PI;
        UpdateLight();
        return;
    }

    phi += offsetX / 180.0f * M_PI;
    theta -= offsetY / 180.0f * M_PI;
    UpdateCam();

}

void cb_MouseDistance(GLFWwindow* window, double posX, double posY){
    if(firstMov){
        lastY = posY;
        firstMov = false;
    }

    float offsetY = 0.1*(posY - lastY);
    lastY = posY;

    float distance_Diff = offsetY;

    distance -= distance_Diff;
    if(distance <= 1) distance = 1;
    if(distance >= 149) distance = 149;
    UpdateCam();

}

void cb_MouseButton(GLFWwindow* window, int button, int action, int mods){
    if(button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT){
        if(action == GLFW_PRESS){
            if(holdCount == 0){
                switch(button){
                    case GLFW_MOUSE_BUTTON_LEFT:
                        glfwSetCursorPosCallback(window, cb_MouseAngles);
                        break;
                    case GLFW_MOUSE_BUTTON_RIGHT:
                        glfwSetCursorPosCallback(window, cb_MouseDistance);
                        break;
                }
            }
            holdCount++;
        }
        if(action == GLFW_RELEASE){
            holdCount--;
            if(holdCount <= 0){
                glfwSetCursorPosCallback(window, cb_MouseIdle);
                holdCount = 0;
            }
            else {
                switch(button){
                    case GLFW_MOUSE_BUTTON_RIGHT:
                        glfwSetCursorPosCallback(window, cb_MouseAngles);
                        break;
                    case GLFW_MOUSE_BUTTON_LEFT:
                        glfwSetCursorPosCallback(window, cb_MouseDistance);
                        break;
                }
            }
            UpdateLight();
            UpdateCam();
            lastX = 0;
            lastY = 0;
            firstMov = true;
        }
    }
    if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS){
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Current Cam Status:" << std::endl;
        std::cout << "POS:      (" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")" << std::endl;
        std::cout << "Distance: " << distance << std::endl;
        std::cout << "Theta:    " << theta/M_PI << " * PI" << std::endl;
        std::cout << "Phi:      " << phi/M_PI << " * PI" << std::endl;
        std::cout << "Current Light Status:" << std::endl;
        std::cout << "Dir:      (" << lightDir.x << ", " << lightDir.y << ", " << lightDir.z << ")" << std::endl;
        std::cout << "Theta:    " << theta_L/M_PI << " * PI" << std::endl;
        std::cout << "Phi:      " << phi_L/M_PI << " * PI" << std::endl;
    }
}

void cb_Key(GLFWwindow* window, int key, int scancode, int action, int mods){
    switch(key){
        case GLFW_KEY_ESCAPE://Exit
            if(action == GLFW_PRESS){
                glfwSetWindowShouldClose(window,GL_TRUE);
            }
            break;
        case GLFW_KEY_F6: //Recompile Shaders
            if(action == GLFW_PRESS){
                CompileShader();
            }
            break;
        case GLFW_KEY_LEFT_CONTROL:
            ctrlPressed = action == GLFW_PRESS;
            break;
    }
}

#pragma endregion InputCallbacks

int main(int argc, const char * argv[]) {

#pragma region InitEnv
    //Init GLFW
    for(int i=0;i<argc;i++){
        std::cout<< argv[i] << std::endl;
    }
    if(!glfwInit()) return -1;

    //Specify OpenGL Version 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT,GL_TRUE);

    //Create a GLFW window
    GLFWwindow* pWindow = glfwCreateWindow(1600, 1000, "Project 6 - Environment Mapping", nullptr, nullptr);
    if(!pWindow) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowMonitor(pWindow,glfwGetPrimaryMonitor(),0,0,1600,1000,60);
    glfwSetWindowMonitor(pWindow, nullptr,160,0,1600,1000,60);

    //Set Context to Current window
    glfwMakeContextCurrent(pWindow);

    //Init GLAD
    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    //Debug log OpenGL version
    std::cout << glGetString(GL_VERSION) << std::endl;

#pragma endregion InitEnv

#pragma region LoadFlies
    //Load mesh from obj
    std::string modelPath = "../models/";
    modelPath.append(argc == 1? argv[0]:argv[1]);
    modelPath.append("/");
    modelPath.append(argc == 1? argv[0]:argv[1]);
    modelPath.append(".obj");

    bool success = mesh.LoadFromFileObj(modelPath.c_str());
    if(!success){
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    //Calculate mesh information
    mesh.ComputeBoundingBox();

#pragma endregion LoadFlies

#pragma region LoadCubeMap
    //Assign & Bind cube map texId
    GLuint cubeMapTexId;
    glGenTextures(1,&cubeMapTexId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexId);

    //Load Files
    unsigned cubeMapWidth = 2048, cubeMapHeight = 2048;
    std::vector<unsigned char> cubeMapPOSX, cubeMapNEGX, cubeMapPOSY, cubeMapNEGY, cubeMapPOSZ, cubeMapNEGZ;
    lodepng::decode(cubeMapPOSX, cubeMapWidth, cubeMapHeight, "../cubeMaps/cubemap_posx.png");
    lodepng::decode(cubeMapNEGX, cubeMapWidth, cubeMapHeight, "../cubeMaps/cubemap_negx.png");
    lodepng::decode(cubeMapPOSY, cubeMapWidth, cubeMapHeight, "../cubeMaps/cubemap_posy.png");
    lodepng::decode(cubeMapNEGY, cubeMapWidth, cubeMapHeight, "../cubeMaps/cubemap_negy.png");
    lodepng::decode(cubeMapPOSZ, cubeMapWidth, cubeMapHeight, "../cubeMaps/cubemap_posz.png");
    lodepng::decode(cubeMapNEGZ, cubeMapWidth, cubeMapHeight, "../cubeMaps/cubemap_negz.png");

    //Write data to cube map buffer
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X,0,GL_RGBA,cubeMapWidth,cubeMapHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,cubeMapPOSX.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,0,GL_RGBA,cubeMapWidth,cubeMapHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,cubeMapNEGX.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,0,GL_RGBA,cubeMapWidth,cubeMapHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,cubeMapPOSY.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,0,GL_RGBA,cubeMapWidth,cubeMapHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,cubeMapNEGY.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,0,GL_RGBA,cubeMapWidth,cubeMapHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,cubeMapPOSZ.data());
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,0,GL_RGBA,cubeMapWidth,cubeMapHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,cubeMapNEGZ.data());

    //Specify Cube Map filters
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

#pragma endregion LoadCubeMap

#pragma region VertexData

    std::cout<< "Num of Vertex Positions: " << mesh.NV() << std::endl;
    std::cout<< "Num of Vertex Normals  : " << mesh.NVN() << std::endl;

    //Prepare Vertex Buffer Data
    std::vector<cyVec3f> positionBufferData;
    std::vector<cyVec3f> normalBufferData;
    std::vector<GLuint> indexBufferData;

    unsigned int max = cy::Max(mesh.NV(),mesh.NVN());
    for(int vi = 0; vi < max; vi++){
        positionBufferData.push_back(vi < mesh.NV() ? mesh.V(vi) : mesh.V(0));
        normalBufferData.push_back(vi < mesh.NVN() ? mesh.VN(vi) : mesh.VN(0));
    }

    for(int i = 0; i < mesh.NF(); i++){
        for(int j = 0; j < 3; j++){
            //Set up triangle vertex buffer data
            unsigned int index = mesh.F(i).v[j];
            unsigned int indexN = mesh.FN(i).v[j];

            if(indexN == index){
                indexBufferData.push_back(index);
            }
            else {//we need to duplicate the Vertex
                bool added = false;
                for(unsigned int mi = max; mi < positionBufferData.size(); mi++){
                    if(positionBufferData.at(mi) == mesh.V(index) &&
                    normalBufferData.at(mi) == mesh.VN(indexN))
                    {
                        //This Duplicated vertex is already added, do not add again
                        added = true;
                        indexBufferData.push_back(mi);
                        break;
                    }
                }
                if(!added){
                    unsigned int newIndex = positionBufferData.size();
                    positionBufferData.push_back(mesh.V(index));
                    normalBufferData.push_back(mesh.VN(indexN));
                    indexBufferData.push_back(newIndex);
                }
            }
        }
    }

    float efficiency = (float)(positionBufferData.size()*sizeof(cyVec3f) + normalBufferData.size()*sizeof(cyVec3f) + indexBufferData.size() * sizeof(GLuint)) / (float)(mesh.NF() * 3 * (2 * sizeof(cyVec3f)+ sizeof(cyVec2f)));

    std::cout << "Size of position buffer: " << positionBufferData.size() << std::endl;
    std::cout << "Size of normal buffer  : " << normalBufferData.size() << std::endl;
    std::cout << "Size of index buffer   : " << indexBufferData.size() << std::endl;
    std::cout << "Optimized Memory Ratio : " << efficiency*100 << "%" << std::endl;

#pragma endregion VertexData

#pragma region Shader

    //Compile The Shader Program for the first time
    CompileShader();
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexId);
    program.SetUniform("skybox",1);
    program_Skybox.SetUniform("skybox", 1);
    program_MirCam.SetUniform("skybox", 1);
    program_MirEnv.SetUniform("skybox",1);
    program_Mirror.SetUniform("tex",2);


#pragma endregion Shader

#pragma region VertexBuffer

    //Generate and bind a VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //Generate and bind a VBO for the Pos
    GLuint vbo[2];
    glGenBuffers(2,&vbo[0]);

    //Link buffer data to vertex shader
    GLuint pos  = program.AttribLocation("iPos");
    GLuint posN = program.AttribLocation("iNormal");

    //Init & Bind the position buffer to vbo[0] and vao
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cyVec3f) * positionBufferData.size(), positionBufferData.data(), GL_STATIC_DRAW);
    //Set up the format of position buffer
    glVertexAttribPointer(pos, 3, GL_FLOAT,GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //Similar Operation for the normal buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cyVec3f) * normalBufferData.size(), normalBufferData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(posN, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posN);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //Generate a EBO/IBO for indexing
    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexBufferData.size(), indexBufferData.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

#pragma endregion VertexBuffer

#pragma region SkyBox
    //vao for skybox
    glGenVertexArrays(1, &vao_skybox);
    glBindVertexArray(vao_skybox);

    //vbo for skybox
    glGenBuffers(1, &vbo_skybox);

    GLuint pos_skybox = program_Skybox.AttribLocation("iPos");

    //Link Buffer Data to vbo_skybox
    glBindBuffer(GL_ARRAY_BUFFER, vbo_skybox);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cyVec3f)*3, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(pos_skybox, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_skybox);
    glBindBuffer(GL_ARRAY_BUFFER,0);

    glBindVertexArray(0);

#pragma endregion SkyBox

#pragma region Mirror
    float mirrorHeight = -8.0f;
    program_MirCam.SetUniform("mirPos", 0.0f,mirrorHeight,0.0f);
    program_MirCam.SetUniform("mirNorm",0.0f,1.0f,0.0f);
    program_MirEnv.SetUniform("mirNorm",0.0f,1.0f,0.0f);

    //vao
    GLuint vao_mirror;
    glGenVertexArrays(1, &vao_mirror);
    glBindVertexArray(vao_mirror);

    //vertex data
    float mirrorLength = 80.0f;
    static const GLfloat squareVertexPos[] = {
            -mirrorLength / 2.0f, mirrorHeight, -mirrorLength / 2.0f,
            -mirrorLength / 2.0f, mirrorHeight, mirrorLength / 2.0f,
            mirrorLength / 2.0f, mirrorHeight, -mirrorLength / 2.0f,
            mirrorLength / 2.0f, mirrorHeight, mirrorLength / 2.0f,
    };

    GLuint pos_square = program_Mirror.AttribLocation("iPos");

    //vbo
    GLuint vbo_mirror;
    glGenBuffers(1, &vbo_mirror);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_mirror);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertexPos), squareVertexPos, GL_STATIC_DRAW);
    glVertexAttribPointer(pos_square, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_square);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

#pragma endregion Mirror

#pragma region RenderToTexture

    int width_R = 3200;
    int height_R = 2000;

    cyGLRenderTextureRect renderBuffer;
    renderBuffer.Initialize(true,4, width_R, height_R);
    renderBuffer.SetTextureFilteringMode(GL_LINEAR,GL_LINEAR);

#pragma endregion RenderToTexture
#pragma region GLFWLifeCycle
    //Bind GLFW Callbacks
    glfwSetKeyCallback(pWindow,cb_Key);
    glfwSetMouseButtonCallback(pWindow,cb_MouseButton);

    glEnable(GL_DEPTH_TEST);

    //GLFW main loop
    while(!glfwWindowShouldClose(pWindow)){
        if(!centered){
            CheckCenter();
        }
        glfwPollEvents();
        //Pass 1: render the object in mirror camera to mirror frame buffer
        renderBuffer.Bind();
        program_MirCam.Bind();
        glBindVertexArray(vao);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indexBufferData.size(), GL_UNSIGNED_INT, nullptr);
        renderBuffer.Unbind();

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        //Pass 2: render the object in world camera to default frame buffer
        program.Bind();
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexBufferData.size(), GL_UNSIGNED_INT, nullptr);

        //Pass 3: render the plane in world camera with the texture from pass 1;
        glEnable(GL_CULL_FACE);
        program_Mirror.Bind();
        glBindVertexArray(vao_mirror);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_RECTANGLE, renderBuffer.GetTextureID());
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_CULL_FACE);

        //Pass 4： render the plane in world camera with environment reflection;
        program_MirEnv.Bind();
        glBindVertexArray(vao_mirror);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexId);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //Pass 5: render the skybox(triangle) with Cube Map texture in world camera;
        glDepthMask(GL_FALSE);
        program_Skybox.Bind();
        glBindVertexArray(vao_skybox);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexId);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDepthMask(GL_TRUE);

        //Swap Buffers to render
        glfwSwapBuffers(pWindow);
    }

    //Terminate and Exit
    glfwTerminate();
    exit(EXIT_SUCCESS);
#pragma endregion GLFWLifeCycle

}