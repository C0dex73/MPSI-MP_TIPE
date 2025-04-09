/*  Written by BAILLOUX Thomas and BAZIN Olivier    */


/********************** PREPROCESSOR **********************/

//LIBS
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


// presets : 120x120|5  /   960x505|2
#define MATRIXWIDTH 120
#define MATRIXHEIGTH 120
#define CELLSIZE 5
#define STR_(X) #X
#define STR(X) STR_(X)
#define KERNELRAD 13.0f
#define DT .1f
#define RDMDENSITY 5


//#define SHOWKERNEL


#ifdef SHOWKERNEL
#undef MATRIXWIDTH
#define MATRIXWIDTH (2*(int)KERNELRAD+1)
#undef MATRIXHEIGTH
#define MATRIXHEIGTH (2*(int)KERNELRAD+1)
#undef CELLSIZE
#define CELLSIZE 10
#endif


//DEFS
struct Cell;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window, unsigned int VBO);
void char_callback(GLFWwindow* window, unsigned int codepoint);
float growth(int x, int y);
void doStep(unsigned int VBO);
int loopback(int value, int max);
float neighbourSum(int x, int y);
void genKernel();
float kernelF(float radius);
int init();
int openfile(const char * mode);
void printM();


/********************** C **********************/
struct Cell {
    float x, y;
    float state, oldState;
};

const char *vShaderP = 
"#version 450 core\n"
"layout (location = 0) in vec4 cell;\n"
"flat out vec2 cellState;\n"
"void main()\n"
"{\n"
"   cellState = vec2(cell.z, cell.w);\n"
"   gl_Position = vec4(cell.x, cell.y, 0.0, 1.0);\n"
"   gl_PointSize = "STR(CELLSIZE-1)";\n"
"}\0";

const char *fShaderP = 
"#version 450 core\n"
"out vec4 FragColor;\n"
"flat in vec2 cellState;\n"
"void main()\n"
"{\n"
#ifdef SHOWKERNEL
"   FragColor = vec4(cellState.y*1.0f, cellState.y*0.0f, cellState.y*0.0f, 1.0f);\n"
#else
"   FragColor = vec4(cellState.y*cellState.x*1.0f, cellState.x*1.0f, cellState.x*1.0f, 1.0f);\n"
#endif
"}\n\0";

struct Cell matrix[MATRIXWIDTH][MATRIXHEIGTH];
struct Cell matrixInit[MATRIXWIDTH][MATRIXHEIGTH];
float kernel[2*(int)KERNELRAD+1][2*(int)KERNELRAD+1];
float kSum = .0f;

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
void debug(){ float r = 3; r += 5; return; }


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

        if(step) { doStep(VBO); }

        //create new frame
        glDrawArrays(GL_POINTS, 0, sizeof(matrix)/sizeof(struct Cell));processInput(window, VBO);

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
        for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
            for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
                matrix[i][j].oldState = .0f;
                matrix[i][j].state = .0f;
                matrixInit[i][j].oldState = .0f;
                matrixInit[i][j].state = .0f;
            }
        }

        for(unsigned int i = 0; i <= MATRIXWIDTH/(KERNELRAD*RDMDENSITY); ++i) {
            unsigned int x = ((float)rand()/(float)(RAND_MAX))*((float)MATRIXWIDTH), y = ((float)rand()/(float)(RAND_MAX))*((float)MATRIXHEIGTH);
            for(int i = x-KERNELRAD; i <= x+KERNELRAD; ++i) {
                for (int j = y-KERNELRAD ; j <= y+KERNELRAD ; ++j) {
                    matrix[loopback(i, MATRIXWIDTH-1)][loopback(j, MATRIXHEIGTH-1)].state = ((float)rand()/(float)(RAND_MAX));
                    matrix[loopback(i, MATRIXWIDTH-1)][loopback(j, MATRIXHEIGTH-1)].oldState = ((float)rand()/(float)(RAND_MAX));
                }
            }
        }
        memcpy(matrixInit, matrix, sizeof(matrix));
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_POINTS, 0, sizeof(matrix)/sizeof(struct Cell));

    }

    //s to save matrixInit
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        openfile("wb");
        fwrite(matrixInit, sizeof(struct Cell), MATRIXHEIGTH*MATRIXWIDTH, fp);
        fclose(fp);
    }

    //l to load matrixInit
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        openfile("rb");
        fread(matrix, sizeof(struct Cell), MATRIXHEIGTH*MATRIXWIDTH, fp);
        fclose(fp);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

//when the window size changes, update glad config to new width and height
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glfwSetWindowShouldClose(window, true);
}

//calculate a new index as if the arrays were looping end <=> start
int loopback(int index, int len) {
    if (index > len) return index - len - 1;
    if (index < 0) return len + index + 1;
    return index;
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
    for (int i = x-KERNELRAD ; i <= x+KERNELRAD ; ++i){
        for (int j = y-KERNELRAD ; j <= y + KERNELRAD ; ++j) {
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
            float k = kernelF(r);
            kernel[i+(int)KERNELRAD][j+(int)KERNELRAD] = k;
            kSum += k;
        }
    }
}

//The function to apply to a cell's radius to get its kernel factor
float kernelF(float radius) {
    return expf(4*(1-1/(4*radius*(1-radius))));
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

//initializes all necessary components
int init() {
    genKernel();

    //init the matrix with random values
    srand(time(0));
    debug();
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            matrix[i][j].x = 2.f*(i+.5f)/(MATRIXWIDTH)-1.f;
            matrix[i][j].y = 1.f-2.f*(j+.5f)/(MATRIXHEIGTH);
#ifdef SHOWKERNEL
            matrix[i][j].oldState = kernel[i][j];
            matrix[i][j].state = kernel[i][j];
#else
            matrix[i][j].oldState = ((float)rand()/(float)(RAND_MAX));
            matrix[i][j].state = ((float)rand()/(float)(RAND_MAX));
#endif
        }
    }
    memcpy(matrixInit, matrix, sizeof(matrix));

    // glfw init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(MATRIXWIDTH*CELLSIZE, MATRIXHEIGTH*CELLSIZE, "TIPE SIM", NULL, NULL);

    if (window == NULL) { //checks if window was properly created 
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }
    
    //window setup
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

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
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetWindowTitle(window, "TIPE SIM");
    sprintf(title, "./saves/%s.blob", filename); //using title as buffer for path bcause it is useless now
    fp = fopen(title , mode);
    if (fp == NULL) {
        fprintf(stderr, "Failed to open file \"%s\"\n", title);
        return 1;
    }
    return 0;
}

void printM() {
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            printf("%f,%f=>%f/%f ", matrix[i][j].x, matrix[i][j].y, matrix[i][j].oldState, matrix[i][j].state);
        }
        printf("\n");
    }
}

void char_callback(GLFWwindow* window, unsigned int codepoint) {
    debug();
    if(strlen(filename) == 249) { return; } //if filename buffer full (255 = 249 + 5 for .blob\0) don't do anything
    char input[2] = { (char)codepoint, *"" };
    memcpy(&filename[strlen(filename)], input, 1);
}