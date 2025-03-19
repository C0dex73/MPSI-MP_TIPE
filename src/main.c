#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define MATRIXWIDTH 800
#define MATRIXHEIGTH 600
#define CELLSIZE 1

struct Cell;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window, unsigned int VBO);
float growth(unsigned int x, unsigned int y);
void doStep(unsigned int VBO);

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
"}\0";

const char *fShaderP = 
"#version 450 core\n"
"out vec4 FragColor;\n"
"flat in float cellState;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(cellState*1.0f, cellState*1.0f, cellState*1.0f, 1.0f);\n"
"}\n\0";

struct Cell matrix[MATRIXWIDTH][MATRIXHEIGTH];
unsigned int vertexIndexes[MATRIXWIDTH-1][MATRIXHEIGTH-1][2][3];

double now, deltaTime, lastFrameTime;
const double fpsMax = 1.f/60.f;
bool step;
bool rpress;

int main() {
    srand(time(0));
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            matrix[i][j].x = 2.f*i/(MATRIXWIDTH-1)-1.f;
            matrix[i][j].y = 1.f-2.f*j/(MATRIXHEIGTH-1);
            matrix[i][j].oldState = ((float)rand()/(float)(RAND_MAX));
            matrix[i][j].state = ((float)rand()/(float)(RAND_MAX));
            if(i == MATRIXWIDTH-1 || j == MATRIXHEIGTH-1) { continue; }
            unsigned int cellIndex[2][3] = 
            {
                {(1+j)+(1+i)*MATRIXHEIGTH, j+(1+i)*MATRIXHEIGTH, j+i*MATRIXHEIGTH },
                {(1+j)+(1+i)*MATRIXHEIGTH, (1+j)+i*MATRIXHEIGTH, j+i*MATRIXHEIGTH }
            };
            memcpy(&vertexIndexes[i][j], &cellIndex[0][0], sizeof(cellIndex));
        }
    }

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
    unsigned int VAO, VBO, EBO, vShader, fShader, pShader;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
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

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexIndexes), &vertexIndexes[0][0][0][0], GL_DYNAMIC_DRAW);

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
        glDrawElements(GL_TRIANGLES, sizeof(vertexIndexes)/sizeof(int), GL_UNSIGNED_INT, 0);

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

    step = false;

    //ESC to close window
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    //SPACE to play-pause
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        step = true;
    }

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

//tells wether a cell should be alive or not next gen
float growth(unsigned int x, unsigned int y) {
    float sum = 0.f;
    for (int i = x-1; i <= x+1; ++i) {
        for (int j = y-1; j <= y+1; ++j) {
            if (i < 0 || i >= MATRIXWIDTH || j < 0 || j >= MATRIXHEIGTH || (i == x && j == y)) { continue; }
            sum += matrix[i][j].oldState;
        }
    }
    sum /= 8.f;
    float a = 2.f;
    float b = .375f;
    float c = .075f;
    float d = -1.f;
    float res = a * expf(-powf(sum-b, 2)/(2*powf(c, 2)))+d;
    // DEBUG : printf("%f => %f\n", sum, res);
    return res;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    int x = (int)(xpos/CELLSIZE/MATRIXWIDTH*(MATRIXWIDTH-1));
    int y = (int)(ypos/CELLSIZE/MATRIXHEIGTH*(MATRIXHEIGTH-1));
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