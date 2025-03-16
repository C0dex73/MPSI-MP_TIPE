#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define MATRIXWIDTH 1920
#define MATRIXHEIGTH 1010
#define CELLSIZE 1

struct Cell;
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, unsigned int VBO);
bool shouldBeAlive(unsigned int x, unsigned int y);

struct Cell {
    float x, y;
    unsigned char state; //rightmost bit as current state and the leftmost as oldest
};

const char *vShaderP = 
"#version 450 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in int state;\n"
"flat out float cellState;\n"
"void main()\n"
"{\n"
"   cellState = state & 2;\n"
"   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
"}\0";

const char *fShaderP = 
"#version 450 core\n"
"out vec4 FragColor;\n"
"flat in float cellState;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(cellState, cellState, cellState, 1.0f);\n"
"}\n\0";

struct Cell matrix[MATRIXWIDTH][MATRIXHEIGTH];
unsigned int vertexIndexes[MATRIXWIDTH-1][MATRIXHEIGTH-1][2][3];

double now, deltaTime, lastFrameTime;
const double fpsMax = 1.f/60.f;

int main() {
    srand(time(0));
    for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
        for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
            matrix[i][j].x = 2.f*i/(MATRIXWIDTH-1)-1.f;
            matrix[i][j].y = 2.f*j/(MATRIXHEIGTH-1)-1.f;
            matrix[i][j].state = (unsigned char)(rand() & 2);
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
    int  success;
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vertexIndexes), &vertexIndexes[0][0][0][0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct Cell), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(struct Cell), (void*)(offsetof(struct Cell, state)));
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
    //ESC to close window
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    //SPACE to step
    if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
            for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
                matrix[i][j].state += shouldBeAlive(i,j);
            }
        }
        for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
            for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
                matrix[i][j].state <<= 1;
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //ENTER to randomize
    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
            for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
                matrix[i][j].state += rand();
            }
        }
        for(unsigned int i = 0; i < MATRIXWIDTH; ++i) {
            for(unsigned int j = 0; j < MATRIXHEIGTH; ++j) {
                matrix[i][j].state <<= 1;
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(matrix), &matrix[0][0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

//when the window size changes, update glad config to new width and height
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

//tells wether a cell should be alive or not next gen
bool shouldBeAlive(unsigned int x, unsigned int y) {
    unsigned int sum = 0;
    for (int i = x-1; i <= x+1; ++i) {
        for (int j = y-1; j <= y+1; ++j) {
            if (i < 0 || i >= MATRIXWIDTH || j < 0 || j >= MATRIXHEIGTH || (i == x && j == y)) { continue; }
            sum += matrix[i][j].state & 2;
        }
    }
    sum >>= 1;
    if ((matrix[x][y].state & 2) == 0) { return sum == 3; }
    return sum == 2 || sum == 3;
}