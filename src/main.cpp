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
double distance = 150;
double phi = 1.2 * M_PI;
double theta = 0.35 * M_PI;

//Parameters for Light Control
double distance_L = 50;
double phi_L = 1.2* M_PI;
double theta_L = 0.4 * M_PI;

//Parameters of Camera
cyVec3f camPos = cyVec3f(0,-distance,0);
cyVec3f camTarget = cyVec3f(0,0,0);
cyVec3f camUp = cyVec3f (0,1,0);
float camFOV = 0.25*M_PI;

//parameters of Spotlight
cyVec3f lightPos = cyVec3f(0,distance,0);
cyVec3f lightTarget = cyVec3f(0,0,0);
cyVec3f lightUp = cyVec3f(0,1,0);
float lightFOV = 0.3*M_PI;

//MVP Matrices
cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(camFOV,1.6f,0.1f,1000.0f);
cy::Matrix4f viewMatrix = cy::Matrix4f::View(camPos,camTarget,camUp);
cy::Matrix4f modelMatrix = cy::Matrix4f::Identity();

//LightCam Matrices
cy::Matrix4f lightProjMatrix = cy::Matrix4f::Perspective(lightFOV,1.0f,0.1f,1000.0f);
cy::Matrix4f lightMatrix = cy::Matrix4f::View(lightPos,lightTarget,lightUp);

//Containers for mesh & program
cy::TriMesh mesh;
cy::GLSLProgram program;
cy::GLSLProgram program_shadow;
cy::GLSLProgram program_hint;

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
    camPos = distance * CalculatePos(theta,phi);

    viewMatrix = cy::Matrix4f::View(camPos,camTarget,camUp);
    cy::Matrix4f vp = projMatrix * viewMatrix * modelMatrix;
    cy::Matrix4f mvp = vp * modelMatrix;

    program.SetUniformMatrix4("mvp",mvp.cell);
    program.SetUniform("camPos",camPos.x,camPos.y,camPos.z);

    program_hint.SetUniformMatrix4("vp", vp.cell);
}

void UpdateLight(){
    cyVec3f spotDir = - CalculatePos(theta_L,phi_L);
    lightPos = - distance_L * spotDir;

    program.SetUniform("spotDir",spotDir.x,spotDir.y,spotDir.z);
    program.SetUniform("lightPos",lightPos.x,lightPos.y,lightPos.z);

    lightMatrix = cy::Matrix4f::View(lightPos,lightTarget,lightUp);
    cyMatrix4f mvp = lightProjMatrix * lightMatrix * modelMatrix;
    program_shadow.SetUniformMatrix4("mvp", mvp.cell);

    float shadowBias = 0.00003f;
    cyMatrix4f mShadow = cyMatrix4f::Translation(cyVec3f(0.5f,0.5f,0.5f - shadowBias))
            * cyMatrix4f::Scale(0.5f)
            * mvp;

    program.SetUniformMatrix4("matrixShadow",mShadow.cell);

    cyMatrix4f hintModel = cyMatrix4f::Translation(lightPos)
            * cyMatrix4f::Rotation(cyVec3f(0,1,0),spotDir);
    program_hint.SetUniformMatrix4("m", hintModel.cell);

}

#pragma endregion CameraControl

#pragma region Shader
void SetUpUniforms(){
    program.SetUniformMatrix4("m",modelMatrix.cell);
    cy::Matrix3f mN = modelMatrix.GetInverse().GetTranspose().GetSubMatrix3();
    program.SetUniformMatrix3("mN",mN.cell);
    program.SetUniform("lightFovRad",lightFOV);
}

void CompileShader(){
    //Create Shader Programs
    program.BuildFiles("../shaders/shader.vert","../shaders/shader.frag");
    program_shadow.BuildFiles("../shaders/shadow.vert", "../shaders/shadow.frag");
    program_hint.BuildFiles("../shaders/debug.vert", "../shaders/debug.frag");
    //Set up "Constant" Uniforms
    SetUpUniforms();

    //Set up "Varying" Uniforms
    UpdateCam();
    UpdateLight();
}

// Check and translate the model to the Center
bool centered = false;
void CheckCenter(){
    if(mesh.IsBoundBoxReady()){
        cyVec3f center = (mesh.GetBoundMax() + mesh.GetBoundMin())/2;
        modelMatrix.SetTranslation(-center);
        SetUpUniforms();
        centered = true;

        //Update Camera
        UpdateCam();
        UpdateLight();
    }
}

#pragma endregion Shader

#pragma region InputCallbacks

//Parameters for Input Callbacks
int holdCount = 0;
bool firstMov = true;
double lastX;
double lastY;
bool ctrlPressed = false;

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
        theta_L += offsetY / 180.0f * M_PI;
        UpdateLight();
        return;
    }

    phi += offsetX / 180.0f * M_PI;
    theta += offsetY / 180.0f * M_PI;
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

    if(ctrlPressed){
        distance_L -= distance_Diff;
        if(distance_L <= 10) distance_L = 10;
        UpdateLight();
        return;
    }

    distance -= distance_Diff;
    if(distance <= 10) distance = 10;
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
        std::cout << "Pos:      (" << lightPos.x << ", " << lightPos.y << ", " << lightPos.z << ")" << std::endl;
        std::cout << "Distance: " << distance_L << std::endl;
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
    GLFWwindow* pWindow = glfwCreateWindow(1600, 1000, "Project 7 - Shadow Mapping", nullptr, nullptr);
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

#pragma region InitShader

    //Compile The Shader Program for the first time
    CompileShader();

#pragma endregion InitShader

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

#pragma region Plane

    //vao
    GLuint vao_square;
    glGenVertexArrays(1, &vao_square);
    glBindVertexArray(vao_square);

    //vertex data
    float squareSize = 128.0f;
    static const GLfloat squareVertexPos[] = {
            -squareSize/2.0f, 0.0f, -squareSize/2.0f,
            squareSize/2.0f, 0.0f, -squareSize/2.0f,
            -squareSize/2.0f, 0.0f, squareSize/2.0f,
            squareSize/2.0f,  0.0f,squareSize/2.0f,
    };
    static const GLfloat squareVertexNorm[] = {
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,
    };

    GLuint pos_square = program.AttribLocation("iPos");
    GLuint posN_square = program.AttribLocation("iNormal");

    //vbo
    GLuint vbo_square[2];
    glGenBuffers(2, vbo_square);

    //Position
    glBindBuffer(GL_ARRAY_BUFFER, vbo_square[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertexPos), squareVertexPos, GL_STATIC_DRAW);
    glVertexAttribPointer(pos_square, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_square);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //Normal
    glBindBuffer(GL_ARRAY_BUFFER, vbo_square[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertexNorm), squareVertexNorm, GL_STATIC_DRAW);
    glVertexAttribPointer(posN_square, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posN_square);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

#pragma endregion Plane

#pragma region hintObject

    //vao
    GLuint vao_hint;
    glGenVertexArrays(1, &vao_hint);
    glBindVertexArray(vao_hint);

    //vertex data
    float height_hint = 2.0f;
    float width_hint = sqrt(2.0f)*height_hint*tan(lightFOV/2);

    static const GLfloat hintVertexPos[] = {
            0.0f, 0.0f, 0.0f,
            -width_hint/2.0f, height_hint, -width_hint/2.0f,
            width_hint/2.0f, height_hint, -width_hint/2.0f,
            width_hint/2.0f, height_hint, width_hint/2.0f,
            -width_hint/2.0f, height_hint, width_hint/2.0f,
            -width_hint/2.0f, height_hint, -width_hint/2.0f,
    };

    GLuint pos_hint = program_hint.AttribLocation("iPos");

    //vbo
    GLuint vbo_hint;
    glGenBuffers(1, &vbo_hint);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_hint);
    glBufferData(GL_ARRAY_BUFFER, sizeof(hintVertexPos), hintVertexPos, GL_STATIC_DRAW);
    glVertexAttribPointer(pos_hint, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_hint);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

#pragma endregion hintObject

#pragma region RenderToTexture

    int width_R = 1024;
    int height_R = 1024;

    cyGLRenderDepth2D shadowMap;
    shadowMap.Initialize(true, width_R, height_R);
    shadowMap.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);
    shadowMap.SetTextureWrappingMode(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);

    program.SetUniform("shadow",0);

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

        //Render the shadow Camera
        shadowMap.Bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        program_shadow.Bind();
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexBufferData.size(), GL_UNSIGNED_INT, nullptr);
        shadowMap.Unbind();

        //Render the world Camera
        program.Bind();
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadowMap.GetTextureID());
        glDrawElements(GL_TRIANGLES, indexBufferData.size(), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(vao_square);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //Render the Hint Object
        program_hint.Bind();
        glBindVertexArray(vao_hint);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

        //Swap Buffers to render
        glfwSwapBuffers(pWindow);
    }

    //Terminate and Exit
    glfwTerminate();
    exit(EXIT_SUCCESS);
#pragma endregion GLFWLifeCycle

}