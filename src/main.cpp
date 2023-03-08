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
double phi = M_PI;
double theta = 0;

//Parameters of Camera
cyVec3f camPos = cyVec3f(0,-distance,0);
cyVec3f camTarget = cyVec3f(0,0,-1);
cyVec3f camUp = cyVec3f (0,1,0);

//MVP Matrices
cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(0.25 * M_PI,1.6f,0.1f,1000.0f);
cy::Matrix4f viewMatrix = cy::Matrix4f::View(camPos,camTarget,camUp);
cy::Matrix4f modelMatrix = cy::Matrix4f::Identity();

//Containers for mesh & program
cy::TriMesh mesh;
cy::GLSLProgram program;

//Parameters for mouse control
int holdCount = 0;
bool firstMov = true;
double lastX;
double lastY;

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
    camTarget = CalculatePos(theta,phi);
    camPos = - distance * camTarget;

    viewMatrix = cy::Matrix4f::View(camPos,camTarget,camUp);

    cy::Matrix4f mvp = projMatrix * viewMatrix * modelMatrix;

    program.SetUniformMatrix4("mvp",mvp.cell);
}

#pragma endregion CameraControl

void CheckCenter(){
    if(mesh.IsBoundBoxReady()){
        cyVec3f center = (mesh.GetBoundMax() + mesh.GetBoundMin())/2;
        modelMatrix.SetTranslation(-center);
        program.SetUniformMatrix4("m", modelMatrix.cell);
        centered = true;

        //Update Camera
        UpdateCam();
    }
}

void CompileShader(){
    //Create Shader Programs
    program.BuildFiles("../shaders/shader.vert","../shaders/shader.frag");

    //Set MVP uniform
    UpdateCam();

    //Bind Program
    program.Bind();
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
    GLFWwindow* pWindow = glfwCreateWindow(1600, 1000, "Project 2 - Transformations", nullptr, nullptr);
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

#pragma region Shader

    //Compile The Shader Program for the first time
    CompileShader();

#pragma endregion Shader

#pragma region VertexBuffer

    //Generate and bind a VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //Generate and bind a VBO for the Pos
    GLuint vbo;
    glGenBuffers(1,&vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    //Load the vertex data into the VBO
    glBufferData(GL_ARRAY_BUFFER,sizeof(cy::Vec3f)*mesh.NV(),&mesh.V(0),GL_STATIC_DRAW);

    //Link buffer data to vertex shader
    GLuint pos  = program.AttribLocation("iPos");
    glEnableVertexAttribArray(pos);

    //define VBO format
    glVertexAttribPointer(pos,3,GL_FLOAT,GL_FALSE,0,(GLvoid*)0);
    //bind vbo again and unbind vao for later use
    glBindBuffer(GL_ARRAY_BUFFER, vbo);


#pragma endregion VertexBuffer

#pragma region GLFWLifeCycle
    //Bind GLFW Callbacks
    glfwSetKeyCallback(pWindow,cb_Key);
    glfwSetMouseButtonCallback(pWindow,cb_MouseButton);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    //GLFW main loop
    while(!glfwWindowShouldClose(pWindow)){
        if(!centered){
            CheckCenter();
        }
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_POINTS,0,mesh.NV());

        //Swap Buffers to render
        glfwSwapBuffers(pWindow);
    }

    //Terminate and Exit
    glfwTerminate();
    exit(EXIT_SUCCESS);
#pragma endregion GLFWLifeCycle

}