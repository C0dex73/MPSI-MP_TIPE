/*  Written by BAILLOUX Thomas and BAZIN Olivier    */


/********************** PREPROCESSOR **********************/

//LIBS
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <glad.h>
#include <glfw3.h>
#include <dimensions.h>

//#define SHOWKERNEL


//DEFS
struct Cell;
struct Dimension;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, unsigned int VBO);
void char_callback(GLFWwindow* window, unsigned int codepoint);

int init();
int openfile(const char * mode);


/********************** C **********************/
struct Dimension* dim;

const char *vShaderP = 
"#version 450 core\n"
"layout (location = 0) in vec4 cell;\n"
"flat out vec2 cellState;\n"
"void main()\n"
"{\n"
"   cellState = vec2(cell.z, cell.w);\n"
"   gl_Position = vec4(cell.x, cell.y, 0.0, 1.0);\n"
"   gl_PointSize = 2;\n"
"}\0";

const char *fShaderP = 
"#version 450 core\n"
"out vec4 FragColor;\n"
"flat in vec2 cellState;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(cellState.y*cellState.x*1.0f, cellState.x*1.0f, cellState.x*1.0f, 1.0f);\n"
"}\n\0";

double now, deltaTime, lastFrameTime;
const double fpsMax = 1/60.f;
bool step;
bool rpress;
bool dstep = false;
bool dpress;
char filename[255] = "";
char oldFilename[255] = "";
bool filenameReady = false;

unsigned int vShader, fShader, pShader, VAO, VBO;
GLFWwindow* window;
FILE* fp;

//useless functions to use with gdb : 'b debug'
void debug(){ return; }


/************************* MAIN  *************************/
int main() {

    int err = init();
    if(err != 0) {
        return err;
    }

    // MAIN LOOP
    while (!glfwWindowShouldClose(window)) {
        lastFrameTime = glfwGetTime();
        now = glfwGetTime();

        //fps cap
        while ((now - lastFrameTime) < fpsMax) { now = glfwGetTime(); }

        processInput(window, VBO);

        if(step) {
            doStep(dim);
            //send data to gpu to display
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(struct Cell)*getMatrixLength(dim), getMatrixPointer(dim), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        //create new frame
        glDrawArrays(GL_POINTS, 0, getMatrixLength(dim));

        //display
        glfwSwapBuffers(window);
        glfwPollEvents();

        lastFrameTime = now;
    }

    //close glfw, exit
    glfwTerminate();
    return 0;
}


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
        
        randomizeDimensionByKernel(dim);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(struct Cell)*getMatrixLength(dim), getMatrixPointer(dim), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_POINTS, 0, getMatrixLength(dim));

    }

    //R to reset
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        memcpy(dim->matrix, dim->matrixInit, sizeof(struct Cell)*getMatrixLength(dim));

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(struct Cell)*getMatrixLength(dim), getMatrixPointer(dim), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_POINTS, 0, getMatrixLength(dim));
    }

    //s to save matrixInit
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        openfile("wb");
        fwrite(getMatrixInitPointer(dim), sizeof(struct Cell), getMatrixLength(dim), fp);
        fclose(fp);
    }

    //l to load matrixInit
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        openfile("rb");
        fread(getMatrixPointer(dim), sizeof(struct Cell), getMatrixLength(dim), fp);
        fclose(fp);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(struct Cell)*getMatrixLength(dim), getMatrixPointer(dim), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

//when the window size changes, update glad config to new width and height
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glfwSetWindowShouldClose(window, true);
}

//initializes all necessary components
int init() {
    dim = CreateDimension(255, 255, 3, 13, .1f, 0.5f);

    //init the matrix with random values
    randomizeDimensionByKernel(dim);
    
    // glfw init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(getMatrixWidth(dim)*getDimensionCellSize(dim), getMatrixHeight(dim)*getDimensionCellSize(dim), "TIPE SIM", NULL, NULL);

    if (window == NULL) { //checks if window was properly created 
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }
    
    //window setup
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //glad init
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return 1;
    }

    //shaders init
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
        fprintf(stderr, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
        return 1;
    }

    //fShader compile
    glShaderSource(fShader, 1, &fShaderP, NULL);
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", infoLog);
        return 1;
    }

    //shader linking
    glAttachShader(pShader, vShader);
    glAttachShader(pShader, fShader);
    glLinkProgram(pShader);
    glGetProgramiv(pShader, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(pShader, 512, NULL, infoLog);
        fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s", infoLog);
        return 1;
    }

    //shader cleanup
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    //buffers init
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    //send matrix data to gpu to display
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(struct Cell)*getMatrixLength(dim), getMatrixPointer(dim), GL_DYNAMIC_DRAW);
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

    return 0;
}

int openfile(const char * mode) {
    bool enter = false;
    bool oldEnter = false;
    bool backspace = false;
    bool oldBackspace = false;
    glfwSetCharCallback(window, char_callback);
    glfwSetCursorPosCallback(window, NULL);
    char title[511];
    for(int i = 0; i < 249; ++i) {
        filename[i] = *"";
        oldFilename[i] = *"";
    }
    sprintf(title, "File name : %s.blob - Type file name and press enter (%ld/255)", filename, strlen(filename)+6);
    glfwSetWindowTitle(window, title);
    for(;;) {
        glfwPollEvents();
        oldEnter = enter;
        oldBackspace = backspace;
        enter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        backspace = glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;

        if(!enter && oldEnter) {
            break;
        }

        if(!backspace && oldBackspace) {
            if(strlen(filename) != 0) { filename[strlen(filename)-1] = ""[0]; }
            if(strlen(oldFilename) != 0) { oldFilename[strlen(oldFilename)-1] = ""[0]; }
            sprintf(title, "File name : %s.blob - Type file name and press enter (%ld/255)", filename, strlen(filename)+6);
            glfwSetWindowTitle(window, title);
        }

        if(strlen(oldFilename) != strlen(filename)) {
            memcpy(oldFilename, filename, strlen(filename)+1);
            sprintf(title, "File name : ./saves/%s.blob - Type file name and press enter (%ld/255)", filename, strlen(filename)+6);
            glfwSetWindowTitle(window, title);
        }
    }
    debug();
    glfwSetCharCallback(window, NULL);
    glfwSetWindowTitle(window, "TIPE SIM");
    sprintf(title, "./saves/%s.blob", filename); //using title as buffer for path bcause it is useless now
    fp = fopen(title , mode);
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file \"%s\"\n", title);
        return 1;
    }
    return 0;
}

void char_callback(GLFWwindow* window, unsigned int codepoint) {
    debug();
    if(strlen(filename) == 249) { return; } //if filename buffer full (255 = 249 + 5 for .blob\0) don't do anything
    char input[2] = { (char)codepoint, *"" };
    memcpy(&filename[strlen(filename)], input, 1);
}