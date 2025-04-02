#include "shaders.h"
#include "config.h"

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