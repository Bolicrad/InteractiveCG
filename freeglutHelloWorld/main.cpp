//
//  main.cpp
//  freeglutHelloWorld
//
//  Created by Lei Zhao on 2023/1/19.
//

#include <iostream>
#include <GL/freeglut.h>

void HSV2RGB(int H, float S, float V, GLclampf& r, GLclampf& g, GLclampf& b){
    if(S>100)S=100;
    if(S<0)S=0;
    if(V>100)V=100;
    if(V<0)V=0;
    GLclampf s = S/100;
    GLclampf v = V/100;
    
    int i = H/60;
    int difs = H%60;
    
    float min, max;
    max = v;
    min = max*(1-s);
    
    float adj = (max-min)*difs/60.0f;
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

void display(void) {
    //Display Function
    
    //Clear the viewport
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    
    //Swap Buffers
    glutSwapBuffers();
    
}

void idle(void){
    //allocate color channel variables
    GLclampf r;
    GLclampf g;
    GLclampf b;
    
    //Change the Hue Through Time
    int hue = (glutGet(GLUT_ELAPSED_TIME) / 30) % 360;
    
    //Convert Color from HSV to RGB(A)
    HSV2RGB(hue, 80, 100, r, g, b);
    
    //Reset Clear Color
    glClearColor(r, g, b, 1);
    
    //Redraw
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y){
    //Keyboard CallBacks
    switch(key){
        case 27: //ESC
            glutLeaveMainLoop();
            break;
    }
}


int main(int argc, char * argv[]) {

    //Init
    glutInit(&argc, argv);
    
    //Init Configs
    glutInitWindowSize(1600, 1000);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    
    //Create Window
    glutCreateWindow("Hello freeGLUT");
    
    //Register CallBacks
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    
    //OpenGL intializations
    glClearColor(0, 0, 0, 1);
    
    //Start Main Loop
    glutMainLoop();
    
    return 0;
}
