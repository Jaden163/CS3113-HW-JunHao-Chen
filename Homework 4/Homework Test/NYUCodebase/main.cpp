#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x2.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "FlareMap.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define LEVEL_WIDTH 128
#define LEVEL_HEIGHT 20
#define SPRITE_COUNT_X 32
#define SPRITE_COUNT_Y 32
#define TILE_SIZE 1/16.0f



SDL_Window* displayWindow;

// initializes the texture program as a global variable
ShaderProgram program;
//initializes the untextured program
ShaderProgram program1;
//initialize textsheet
GLuint characters;
GLuint mineCraft;

// initialze map
FlareMap map;


GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
}

void setUp(){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Mario Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif

    //intial set-up
    glViewport(0,0,640,360);
    
    //loads textured file in program
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    //loads untextured poly program
    program1.Load(RESOURCE_FOLDER"vertex.glsl",RESOURCE_FOLDER"fragment.glsl");
    
    //creates the 3 matrixes
    glm::mat4 projectionMatrix=glm::mat4(1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);

    
    //sets up ortho and use the programs
    projectionMatrix= glm::ortho(-1.777f,1.777f,-1.0f,1.0f,-1.0f,1.0f);
    glUseProgram(program.programID);
    glUseProgram(program1.programID);
    
    // only needs to set Projection & View once
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    program1.SetProjectionMatrix(projectionMatrix);
    program1.SetViewMatrix(viewMatrix);
    
    // only needs to set Blend once
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    map.Load(RESOURCE_FOLDER"marioClone.txt");
    mineCraft= LoadTexture(RESOURCE_FOLDER"minecraft_Sprite.png");


    
}

void renderMap(){
    
}


int main(int argc, char *argv[])
{
    
    setUp();
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        std::vector<float> vertexData;
        std::vector<float> texCoordData;
        
        for(int y=0; y < LEVEL_HEIGHT; y++) {
            for(int x=0; x < LEVEL_WIDTH; x++) {
                if(map.mapData[y][x] != 0) {
                float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
                float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
                float spriteWidth = 1.0f/(float)SPRITE_COUNT_X;
                float spriteHeight = 1.0f/(float)SPRITE_COUNT_Y;
                vertexData.insert(vertexData.end(), {
                    TILE_SIZE * x, -TILE_SIZE * y,
                    TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    TILE_SIZE * x, -TILE_SIZE * y,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
                });
                texCoordData.insert(texCoordData.end(), {
                    u, v,
                    u, v+(spriteHeight),
                    u+spriteWidth, v+(spriteHeight),
                    u, v,
                    u+spriteWidth, v+(spriteHeight),
                    u+spriteWidth, v
                });
                }
            }
        }
        
        glBindTexture(GL_TEXTURE_2D, mineCraft);
        
        glm::mat4 modelMatrix=glm::mat4(1.0f);
        program.SetModelMatrix(modelMatrix);
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
        glEnableVertexAttribArray(program.positionAttribute);
        
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
        glEnableVertexAttribArray(program.texCoordAttribute);
        int tiles=LEVEL_WIDTH*LEVEL_HEIGHT;
        glDrawArrays(GL_TRIANGLES, 0, 6*tiles);
        
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        SDL_GL_SwapWindow(displayWindow);

    }
    
    SDL_Quit();
    return 0;
}
