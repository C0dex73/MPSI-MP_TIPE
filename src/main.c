#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define MATRIXWIDTH 200
#define MATRIXHEIGTH 200
#define CELLSIZE 2
#define STR_(X) #X
#define STR(X) STR_(X)
#define KERNELRAD 13.0f
//#define SHOWKERNEL

#ifdef SHOWKERNEL
#undef MATRIXWIDTH
#define MATRIXWIDTH (2*(int)KERNELRAD+1)
#undef MATRIXHEIGTH
#define MATRIXHEIGTH (2*(int)KERNELRAD+1)
#undef CELLSIZE
#define CELLSIZE 10
#endif

struct Cell;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window, unsigned int VBO);
float growth(int x, int y);
void doStep(unsigned int VBO);
int loopback(int value, int max);
float neighbourSum(int x, int y);
void genKernel();
float kernelF(float radius);

struct Cell {
    float x, y;
    float state;
    float oldState;
};

const char *vShaderP = 
"#version 450 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in float state;\n"
"flat out float cellState;\n"
"void main()\n"
"{\n"
"   cellState = state;\n"
"   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
"   gl_PointSize = "STR(CELLSIZE-1)";\n"
"}\0";

const char *fShaderP = 
"#version 450 core\n"
"out vec4 FragColor;\n"
"flat in float cellState;\n"
"void main()\n"
"{\n"
#ifdef SHOWKERNEL
"   FragColor = vec4(cellState*1.0f, cellState*0.0f, cellState*0.0f, 1.0f);\n"
#else
"   FragColor = vec4(cellState*1.0f, cellState*1.0f, cellState*1.0f, 1.0f);\n"
#endif
"}\n\0";

struct Cell matrix[MATRIXWIDTH][MATRIXHEIGTH];
float kernel[2*(int)KERNELRAD+1][2*(int)KERNELRAD+1];

double now, deltaTime, lastFrameTime;
const double fpsMax = 1/60.f;
bool step;
bool rpress;
bool dstep = false;
bool dpress;

void debug(){ float r = 3; r += 5; return; }

int main() {

    genKernel();

    srand(time(0));
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

    //buffers init
    unsigned int VAO, VBO, vShader, fShader, pShader;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    vShader = glCreateShader(GL_VERTEX_SHADER);
    fShader = glCreateShader(GL_FRAGMENT_SHADER);
    pShader = glCreateProgram();
    int success;
    char infoLog[512];

    glShaderSource(vShader, 1, &vShaderP, NULL);
    glCompileShader(vShader);
    glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s", infoLog);
    }

    glShaderSource(fShader, 1, &fShaderP, NULL);
    glCompileShader(fShader);
    glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s", infoLog);
    }

    glAttachShader(pShader, vShader);
    glAttachShader(pShader, fShader);
    glLinkProgram(pShader);

    glGetProgramiv(pShader, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(pShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s", infoLog);
    }

    
    glDeleteShader(vShader);
    glDeleteShader(fShader); 


    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct Cell), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(struct Cell), (void*)(offsetof(struct Cell, oldState)));
    glEnableVertexAttribArray(1);


    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);


    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);

    glUseProgram(pShader);
    glBindVertexArray(VAO);

    // MAIN LOOP
    while (!glfwWindowShouldClose(window)) {
        lastFrameTime = glfwGetTime();
        now = glfwGetTime();
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
                matrix[i][j].state = ((float)rand()/(float)(RAND_MAX));;
                matrix[i][j].oldState = ((float)rand()/(float)(RAND_MAX));;
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

//when the window size changes, update glad config to new width and height
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int loopback(int value, int max) {
    if (value > max) return value - max - 1;
    if (value < 0) return max + value + 1;
    return value;
}

//tells wether a cell should be alive or not next gen
float growth(int x, int y) {    
    float sum = neighbourSum(x, y);

    //GAUSSIAN
    float a = 2.f;
    float b = .3f;
    float c = .13f;
    float d = -1.f;
    float res = a * expf(-(sum-b)*(sum-b)/(2*c*c))+d;

    /* ORIGINAL GOL
    if (sum >= 2.9f && sum <= 3.1f) { return 1.f; }
    if (sum >= 2.f && sum <= 3.f) { return 0.f; }
    */
    return res;
}

float neighbourSum(int x, int y) {
    float sum = .0f;
    float kSum = .0f;
    for (int i = x-KERNELRAD ; i <= x+KERNELRAD ; ++i){
        for (int j = y-KERNELRAD ; j <= y + KERNELRAD ; ++j) {
            kSum += kernel[i-x+(int)KERNELRAD][j-y+(int)KERNELRAD];
            sum += kernel[i-x+(int)KERNELRAD][j-y+(int)KERNELRAD] * matrix[loopback(i, MATRIXWIDTH-1)][loopback(j, MATRIXHEIGTH-1)].oldState;
        }
    }
    //printf("%f/%f ", sum, kSum);
    return sum/kSum;
}

void genKernel() {
    for (int i = -KERNELRAD ; i <= KERNELRAD ; ++i){
        for (int j = -KERNELRAD ; j <= KERNELRAD ; ++j) {
            float r = sqrtf(i*i+j*j)/KERNELRAD;
            if (r > 1 || r == 0) { continue; }
            kernel[i+(int)KERNELRAD][j+(int)KERNELRAD] = kernelF(r);
        }
    }
}

float kernelF(float radius) {
    if (radius > .25f && radius < .75f) { return 1.f; }
    else { return 0.f; }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    int x = (int)(xpos/CELLSIZE/MATRIXWIDTH*(MATRIXWIDTH));
    int y = (int)(ypos/CELLSIZE/MATRIXHEIGTH*(MATRIXHEIGTH));
    float value = matrix[(int)(x)][(int)(y)].oldState;
    char title[255];
    snprintf(title, 255, "TIPE SIM - Cell's value : %f", value);
    glfwSetWindowTitle(window, title);
}

void doStep(unsigned int VBO) {
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
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            matrix[i][j].oldState = matrix[i][j].state;
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}