#define CY_NO_IMMINTRIN_H
#define VNAME(value) (#value)
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "cyCodeBase/cyGL.h"
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyMatrix.h"
#include "loadpng/lodepng.h"

//Window Width and Height
int width_S = 1600;
int height_S = 1000;

//Parameters for Camera Control
double distance = 100;
double phi = M_PI;
double theta = 0;

//Parameters for Light Control
double phi_L = 0;
double theta_L = M_PI;
cyVec3f lightDir = cyVec3f(0,-1,0);

//Parameters for Square Camera Control
double distance_S = 5;
double phi_S = M_PI;
double theta_S = 0.5 * M_PI;

//Parameters of Camera
cyVec3f camPos = cyVec3f(0,-distance,0);
cyVec3f camTarget = cyVec3f(0,0,-1);
cyVec3f camUp = cyVec3f (0,1,0);

//Parameters of Square Camera
cyVec3f camPos_S = cyVec3f(0, 0, distance_S);
cyVec3f camTarget_S = cyVec3f (0,0,-1);

//MVP Matrices
cy::Matrix4f projMatrix = cy::Matrix4f::Perspective(0.25 * M_PI,1.6f,0.1f,1000.0f);
cy::Matrix4f viewMatrix = cy::Matrix4f::View(camPos,camTarget,camUp);
cy::Matrix4f modelMatrix = cy::Matrix4f::Identity();

//MVP Matrices for Square
cy::Matrix4f viewMatrix_S = cy::Matrix4f::View(camPos_S, camTarget_S, camUp);
cy::Matrix4f modelMatrix_S = cyMatrix4f::Identity();

//Containers for mesh & program
cy::TriMesh mesh;
cy::GLSLProgram program;
cy::GLSLProgram program_S;

//Parameters for mouse control
int holdCount = 0;
bool firstMov = true;
double lastX;
double lastY;
bool ctrlPressed = false;
bool altPressed = false;

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

    cy::Matrix4f mv = viewMatrix * modelMatrix;
    cy::Matrix3f mvN = mv.GetInverse().GetTranspose().GetSubMatrix3();
    cy::Matrix4f mvp = projMatrix * viewMatrix * modelMatrix;

    program.SetUniformMatrix4("mv",mv.cell);
    program.SetUniformMatrix3("mvN",mvN.cell);
    program.SetUniformMatrix4("mvp",mvp.cell);

}

void UpdateSquare(){
    camTarget_S = CalculatePos(theta_S,phi_S);
    camPos_S = -distance_S * camTarget_S;
    viewMatrix_S = cyMatrix4f::View(camPos_S,camTarget_S,camUp);
    cyMatrix4f mvp = projMatrix * viewMatrix_S * modelMatrix_S;
    program_S.SetUniformMatrix4("mvp", mvp.cell);
}

void UpdateLight(){
    lightDir = (CalculatePos(theta_L,phi_L)).XYZ().GetNormalized();
    program.SetUniform("lightDir",lightDir.x,lightDir.y,lightDir.z);
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
        UpdateSquare();
    }
}

void CompileShader(){
    //Create Shader Programs
    program.BuildFiles("../shaders/shader.vert","../shaders/shader.frag");
    program_S.BuildFiles("../shaders/square.vert","../shaders/square.frag");

    //Set Up Aspect Ratio
    GLfloat aspectRatio = (float)width_S/(float)height_S;
    program_S.SetUniform("aspectRatio",aspectRatio);

    //Set MVP uniform
    UpdateCam();
    UpdateLight();
    UpdateSquare();



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

    if(altPressed){
        phi_S += offsetX / 180.0f * M_PI;
        theta_S -= offsetY / 180.f * M_PI;
        UpdateSquare();
        return;
    }

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

    if(altPressed){
        distance_S -= distance_Diff;
        if(distance_S <= 1) distance_S = 1;
        UpdateSquare();
        return;
    }

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
        std::cout << "Current Square Status:" << std::endl;
        std::cout << "Distance: " << distance_S << std::endl;
        std::cout << "Theta:    " << theta_S/M_PI << " * PI" << std::endl;
        std::cout << "Phi:      " << phi_S/M_PI << " * PI" << std::endl;
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
        case GLFW_KEY_LEFT_ALT:
            altPressed = action == GLFW_PRESS;
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
    GLFWwindow* pWindow = glfwCreateWindow(width_S, height_S, "Project 5 - Render Buffers", nullptr, nullptr);
    if(!pWindow) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetWindowMonitor(pWindow,glfwGetPrimaryMonitor(),0,0,width_S,height_S,60);
    glfwSetWindowMonitor(pWindow, nullptr,160,0,width_S,height_S,60);

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

    std::cout<< "Num of Faces           : " << mesh.NF() << std::endl;
    std::cout<< "Num of Materials       : " << mesh.NM() << std::endl;

    //Load Image Data from PNG
    std::string filename = "../models/";
    filename.append(argc == 1? argv[0]:argv[1]);
    filename.append("/");
    filename.append(mesh.M(0).map_Kd);

    std::string filename_S = "../models/";
    filename_S.append(argc == 1? argv[0]:argv[1]);
    filename_S.append("/");
    filename_S.append(mesh.M(0).map_Ks);

    std::vector<unsigned char> image;
    unsigned width, height;
    unsigned error = lodepng::decode(image, width, height, filename);

    if(error != 0) {
        std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
        return 1;
    }

    std::vector<unsigned char> image_S;
    unsigned error_S = lodepng::decode(image_S, width, height, filename_S);

    if(error_S != 0) {
        std::cout << "error " << error_S << ": " << lodepng_error_text(error_S) << std::endl;
        return 1;
    }
#pragma endregion LoadFlies

#pragma region BindTexture

    //Bind Texture Units
    GLuint texID[2];
    glGenTextures(2, &texID[0]);

    //For diffuse texture
    glBindTexture(GL_TEXTURE_2D, texID[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_REPEAT);

    //For specular texture
    glBindTexture(GL_TEXTURE_2D, texID[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_S.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_REPEAT);

#pragma endregion BindTexture

#pragma region VertexData

    std::cout<< "Num of Vertex Positions: " << mesh.NV() << std::endl;
    std::cout<< "Num of Vertex Normals  : " << mesh.NVN() << std::endl;
    std::cout<< "Num of Vertex TexCoords: " << mesh.NVT() << std::endl;
    std::cout<< "Diffuse Texture Name   : " << mesh.M(0).map_Kd << std::endl;
    std::cout<< "Specular Texture Name  : " << mesh.M(0).map_Ks << std::endl;


    //Prepare Vertex Buffer Data
    std::vector<cyVec3f> positionBufferData;
    std::vector<cyVec3f> normalBufferData;
    std::vector<cyVec2f> texCoordBufferData;
    std::vector<GLuint> indexBufferData;

    unsigned int max = cy::Max(mesh.NV(),mesh.NVN());
    for(int vi = 0; vi < max; vi++){
        positionBufferData.push_back(vi < mesh.NV() ? mesh.V(vi) : mesh.V(0));
        normalBufferData.push_back(vi < mesh.NVN() ? mesh.VN(vi) : mesh.VN(0));
        texCoordBufferData.push_back(mesh.VT(0).XY());
    }

    for(int i = 0; i < mesh.NF(); i++){
        //cy::TriMesh::TriFace face = mesh.F(i);
        //cyVec3f N = (mesh.V(face.v[1])-mesh.V(face.v[0])) ^ (mesh.V(face.v[2])-mesh.V(face.v[0]));
        for(int j = 0; j < 3; j++){
            //Set up triangle vertex buffer data
            unsigned int index = mesh.F(i).v[j];
            unsigned int indexN = mesh.FN(i).v[j];
            unsigned int indexT = mesh.FT(i).v[j];


            if(indexN == index){
                indexBufferData.push_back(index);
                texCoordBufferData.at(index).Set(mesh.VT(indexT).x, mesh.VT(indexT).y);
            }
            else {//we need to duplicate the Vertex
                bool added = false;
                for(unsigned int mi = max; mi < positionBufferData.size(); mi++){
                    if(positionBufferData.at(mi) == mesh.V(index) &&
                    normalBufferData.at(mi) == mesh.VN(indexN) &&
                    texCoordBufferData.at(mi) == mesh.VT(indexT).XY())
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
                    texCoordBufferData.push_back(mesh.VT(indexT).XY());
                    indexBufferData.push_back(newIndex);
                }
            }
        }
    }


    float efficiency = (float)(positionBufferData.size()*sizeof(cyVec3f) + normalBufferData.size()*sizeof(cyVec3f) + texCoordBufferData.size() * sizeof(cyVec2f) + indexBufferData.size() * sizeof(GLuint)) / (float)(mesh.NF() * 3 * (2 * sizeof(cyVec3f)+ sizeof(cyVec2f)));

    std::cout << "Size of position buffer: " << positionBufferData.size() << std::endl;
    std::cout << "Size of normal buffer  : " << normalBufferData.size() << std::endl;
    std::cout << "Size of texCoord buffer: " << texCoordBufferData.size() << std::endl;
    std::cout << "Size of index buffer   : " << indexBufferData.size() << std::endl;
    std::cout << "Optimized Memory Ratio : " << efficiency*100 << "%" << std::endl;

#pragma endregion VertexData

#pragma region Shader

    //Compile The Shader Program for the first time
    CompileShader();

    //Send Texture Data to GPU
    program.SetUniform("tex",0);
    program.SetUniform("texS", 1);
    program_S.SetUniform("tex",2);

#pragma endregion Shader

#pragma region VertexBuffer

    //Generate and bind a VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //Generate and bind a VBO for the Pos
    GLuint vbo[3];
    glGenBuffers(3,&vbo[0]);

    //Link buffer data to vertex shader
    GLuint pos  = program.AttribLocation("iPos");
    GLuint posN = program.AttribLocation("iNormal");
    GLuint posT = program.AttribLocation("iTexCoord");

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

    //Similar Operation for the texCoord buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cyVec2f) * texCoordBufferData.size(), texCoordBufferData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(posT, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posT);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //Generate a EBO/IBO for indexing
    GLuint ibo;
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexBufferData.size(), indexBufferData.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

#pragma endregion VertexBuffer

#pragma region Square
    //vao
    GLuint vao_square;
    glGenVertexArrays(1, &vao_square);
    glBindVertexArray(vao_square);

    //vertex data
    static const GLfloat squareVertexPos[] = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f,  1.0f, 0.0f,
    };

    GLuint pos_square = program_S.AttribLocation("iPos");

    //vbo
    GLuint vbo_square;
    glGenBuffers(1, &vbo_square);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_square);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertexPos), squareVertexPos, GL_STATIC_DRAW);
    glVertexAttribPointer(pos_square, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(pos_square);
    glBindBuffer(GL_ARRAY_BUFFER, 0);


    glBindVertexArray(0);

#pragma endregion Square

#pragma region RenderToTexture

    int width_R = 1024;
    int height_R = 1024;

    cyGLRenderTexture2D renderBuffer;
    renderBuffer.Initialize(true,3, width_R, height_R);
    renderBuffer.SetTextureFilteringMode(GL_LINEAR,GL_LINEAR_MIPMAP_LINEAR);

#pragma endregion RenderToTexture

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
        //SetClearColor();

        //Set frame buffer target and render
        renderBuffer.Bind();
        program.Bind();
        glBindVertexArray(vao);
        glClearColor(0.1,0.1,0.1,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,texID[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,texID[1]);
        glDrawElements(GL_TRIANGLES, indexBufferData.size(), GL_UNSIGNED_INT, nullptr);

        //Set frame buffer target to back buffer
        renderBuffer.Unbind();
        renderBuffer.BuildTextureMipmaps();

        //Draw the Square
        program_S.Bind();
        glBindVertexArray(vao_square);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, renderBuffer.GetTextureID());
        glDrawArrays(GL_TRIANGLES,0,6);

        //Swap Buffers to render
        glfwSwapBuffers(pWindow);
    }

    //Delete Buffers


    //Terminate and Exit
    glfwTerminate();
    exit(EXIT_SUCCESS);
#pragma endregion GLFWLifeCycle

}