//LIBS
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "config.h"
#include "shaders.h"
#include "matrix.h"
#include "random.h"
#include "utils.h"


//DEFS
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window, unsigned int VBO);
float growth(int x, int y);
void doStep(unsigned int VBO);
float neighbourSum(int x, int y);
void genKernel();
float kernelF(float radius);


/********************** C **********************/
float kernel[2*(int)KERNELRAD+1][2*(int)KERNELRAD+1];

double now, deltaTime, lastFrameTime;
const double fpsMax = 1/60.f;
bool step;
bool rpress;
bool dstep = false;
bool dpress;

//useless functions to use with gdb : 'b debug'
void debug(){ float r = 3; r += 5; return; }


/************************* MAIN  *************************/
#ifndef TIPE_SHOWKERNEL
int main() {

    genKernel();
    initRandom();

    debug();

    //init the matrix with random values
#ifdef SHOWKERNEL
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            matrix[i][j].x = 2.f*(i+.5f)/(MATRIXWIDTH)-1.f;
            matrix[i][j].y = 1.f-2.f*(j+.5f)/(MATRIXHEIGTH);

            matrix[i][j].oldState = kernel[i][j];
            matrix[i][j].state = kernel[i][j];
        }
    }
#else
    randomizeMatrix();
#endif

    debug();

    // glfw init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(MATRIXWIDTH*CELLSIZE, MATRIXHEIGTH*CELLSIZE, "TIPE SIM", NULL, NULL);

    if (window == NULL) { //checks if window was properly created 
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    
    //window setup
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    //glad init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    //shaders init
    unsigned int vShader, fShader, pShader;
    vShader = glCreateShader(GL_VERTEX_SHADER);
    fShader = glCreateShader(GL_FRAGMENT_SHADER);
    pShader = glCreateProgram();
    int success;
    char infoLog[512];

    //vShader compile
    glShaderSource(vShader, 1, &vShaderP, NULL);
    glCompileShader(vShader);
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
    }

    //fShader compile
    glShaderSource(fShader, 1, &fShaderP, NULL);
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", infoLog);
    }

    //shader linking
    glAttachShader(pShader, vShader);
    glAttachShader(pShader, fShader);
    glLinkProgram(pShader);
    glGetProgramiv(pShader, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(pShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s", infoLog);
    }

    //shader cleanup
    glDeleteShader(vShader);
    glDeleteShader(fShader); 

    //buffers init
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    //send matrix data to gpu to display
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(struct Cell), (void*)0); //this is here to tell the gpu how to manage the given data
    glEnableVertexAttribArray(0);

    //buffer cleanup
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //setup opengl to display
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glUseProgram(pShader);
    glBindVertexArray(VAO);

    // MAIN LOOP
    while (!glfwWindowShouldClose(window)) {
        lastFrameTime = glfwGetTime();
        now = glfwGetTime();

        //fps cap
        while ((now - lastFrameTime) < fpsMax) { now = glfwGetTime(); }

        processInput(window, VBO);

        if(step) { doStep(VBO); }

        //create new frame
        glDrawArrays(GL_POINTS, 0, sizeof(matrix)/sizeof(struct Cell));

        //display
        glfwSwapBuffers(window);
        glfwPollEvents();

        lastFrameTime = now;
    }

    //close glfw, exit
    glfwTerminate();
    return 0;
}
#endif


/************************* FUNCTIONS  *************************/


//input handling
void processInput(GLFWwindow *window, unsigned int VBO) {

    step = dstep;

    //ESC to close window
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    //SPACE to play-pause
    bool ndpress = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if(ndpress && !dpress) {
        dstep = !dstep;
    }
    dpress = ndpress;

    step = dstep;
    //RIGHT ARROW to step
    bool nrpress = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    if(nrpress && !rpress) {
        step = true;
    }
    rpress = nrpress;

    //ENTER to randomize
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        clearMatrix();
        randomizeMatrix();
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

//when the window size changes, update glad config to new width and height
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

//tells wether a cell should be alive or not next gen
float growth(int x, int y) {
    float sum = neighbourSum(x, y);

    //GAUSSIAN
    float a = 2.f;
    float b = .15f;
    float c = .017f;
    float d = -1.f;
    float res = a * expf(-(sum-b)*(sum-b)/(2*c*c))+d;

    return res*DT;
}

//calculates the neighbour sum ponderated by their kernel value relative the the cell in (x,y)
float neighbourSum(int x, int y) {
    float sum = .0f;
    float kSum = .0f;
    for (int i = x-KERNELRAD ; i <= x+KERNELRAD ; ++i){
        for (int j = y-KERNELRAD ; j <= y + KERNELRAD ; ++j) {
            kSum += kernel[i-x+(int)KERNELRAD][j-y+(int)KERNELRAD];
            sum += kernel[i-x+(int)KERNELRAD][j-y+(int)KERNELRAD] * matrix[loopback(i, MATRIXWIDTH-1)][loopback(j, MATRIXHEIGTH-1)].oldState;
        }
    }
    return sum/kSum;
}

//generates the kernel matrix, containing the weight of each cells in the neighbour sum
void genKernel() {
    for (int i = -KERNELRAD ; i <= KERNELRAD ; ++i){
        for (int j = -KERNELRAD ; j <= KERNELRAD ; ++j) {
            float r = sqrtf(i*i+j*j)/KERNELRAD;
            if (r > 1 || r == 0) { continue; }
            kernel[i+(int)KERNELRAD][j+(int)KERNELRAD] = kernelF(r);
        }
    }
}

//for debugging purposes, displays the hoovered cell's value
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    int x = (int)(xpos/CELLSIZE/MATRIXWIDTH*(MATRIXWIDTH));
    int y = (int)(ypos/CELLSIZE/MATRIXHEIGTH*(MATRIXHEIGTH));
    char title[255];
    snprintf(title, 255, "TIPE SIM - Cell's value : %f | dV : %f", matrix[(int)(x)][(int)(y)].oldState, matrix[(int)(x)][(int)(y)].oldState);
    glfwSetWindowTitle(window, title);
}

//simulation step
void doStep(unsigned int VBO) {
    //calculate state from oldState
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            //DEBUG printf("%f => ", matrix[i][j].state);
            matrix[i][j].state += growth(i, j);
            //DEBUG printf("%f => ", matrix[i][j].state);
            if(matrix[i][j].state > 1.f) { matrix[i][j].state = 1.f; }
            if(matrix[i][j].state < 0.f) { matrix[i][j].state = 0.f; }
            //DEBUG printf("%f\n", matrix[i][j].state);
        }
    }
    //switch them
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            matrix[i][j].oldState = matrix[i][j].state;
        }
    }
    //send data to gpu to display
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}