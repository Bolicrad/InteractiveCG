# CS 6610 Project 6 Environment Mapping

## ScreenShot
![Project6Step1](assets/Project6Step1.png)![Project6Step2](assets/Project6Step2.png)![Project6Step3](assets/Project6Step3.png)
## What you implemented
1. Created cube map texture from the images.
2. Utilized a single triangle to render the skybox.
3. Wrote fragment Shader for the teapot to calculate both environment mapping and specular light in World Space.
4. (CS 6610 requirement)Created a plane to receive both environment mapping and object mapping, by rendering to texture.

## Additional functionalities beyond project requirements
There is a platform bug on Apple Silicon Macs: if you tried to load multiple samplers in a single pass, the system will only load one texture. To avoid that problem without losing the desired effect, I seperate the render to plane operation into two passes:

```glsl
//mirror.frag
# version 410 core
in vec4 gl_FragCoord;
out vec4 color;

uniform sampler2DRect tex;

void main(){
    vec4 C_Tex = texture(tex, gl_FragCoord.xy);
    if(C_Tex.w < 0.1f)discard;
    color = C_Tex;
}

//mirenv.frag
# version 410 core
in vec3 oDir;
out vec4 color;

uniform vec3 camPos;
uniform vec3 mirNorm;
uniform samplerCube skybox;

void main(){
    vec3 viewDir = normalize(camPos - oDir);
    vec3 reflectDir = reflect(-viewDir, mirNorm);
    vec4 C_Env = texture(skybox, reflectDir);
    color = C_Env;
}
```
The first pass samples the texture from previously rendered texture, and discard and fragment that cannot pass alpha test, which only renders the reflected teapot. The Second pass render the plane again, with only environment mapping colors.
## How to use the implementation

This project is now a Clion project, so we need to run it under this IDE, or others that support cmake.

After download and setup the environment, then click Run in your IDE, and you will see a 16:10 window appear on your screen, contains a teapot and a plane, both reflecting the environment lights.

### List of Inputs

* Hold mouse left and drag, to rotate the view of the camera;
* Hold mouse left and drag, to rotate the light direction when ```ctrl``` is pressed; 
* Hold mouse right and drag, to zoom in/out the camera.
* Press ```Esc``` to exit; 

## Envrionment, OS, External Libraries and Additional Requirements
I developed and tested this project on Latest MacOS 13.2.1, and the architecture is Apple Silicon (Arm64). 

### To setup environment:

1. install [HomeBrew](https://brew.sh).
2. In Terminal, run ```$ brew install glfw ```;
3. Go to [GLAD](https://glad.dav1d.de) online service, choose as this picture. Then click generate. ![](assets/GLAD.jpg)


4. Download the zip flie, copy the two folders inside the include folder to /opt/homebrew/include. 

The environment is now set up and ready for debugging the project you copied from me.
### To create a empty project under this environment:

1. Create a empty Clion Project. (Or other IDE supports Cmake)
2. Copy the src folder in glad to the root of project. 
3. Drag the main.cpp into src folder.
4. Under the root of project, create a folder named "include". For any external libraries, put in this folder. 
> The best practice for adding include libraries is to use ```git submodule add repo_url include/repo_name``` to have those external libraries installed if you are using git.
5. Then Modify the CMakeLists.txt like this:
```cmake
cmake_minimum_required(VERSION 3.24)
project(Your-Project-Name)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_OSX_ARCHITECTURES  "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

set(GLFW_LINK /opt/homebrew/lib/libglfw.3.dylib)
link_libraries(${OPENGL} ${GLFW_LINK})

add_executable(Your-Executable-Name src/glad.c src/main.cpp)
target_include_directories(Your-Executable-Name PUBLIC /opt/homebrew/include)
target_include_directories(Your-Executable-Name PRIVATE ${PROJECT_SOURCE_DIR}/include)

if (APPLE)
    target_link_libraries(Your-Executable-Name "-framework OpenGL")
    target_link_libraries(Your-Executable-Name "-framework GLUT")
endif ()
```

### Miscs:

1. I also Included the cyCodeBase, but it's buggy on Apple Silicon Mac unless you disable immintrin.h by adding the first line on the top of main.cpp, as well as the second line to override gluErrorString function, before include cyGL.h:
    ```cpp
    #define CY_NO_IMMINTRIN_H
    #define gluErrorString(value) (#value)
    ```
2. For any included library, if it has any .cpp files, you need to add it to compile list by modfying the CMakeLists.txt like this:
    ```cmake
    add_executable(Your-Executable-Name src/glad.c [library cpp directories] src/main.cpp)
    ```