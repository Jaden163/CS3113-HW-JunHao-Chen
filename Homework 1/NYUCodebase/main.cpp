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

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

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




int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    
    
    //intial set-up
    glViewport(0,0,640,360);
    
    // initializes texture program
    ShaderProgram program;
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    GLuint cardBack= LoadTexture(RESOURCE_FOLDER"cardBack_blue5.png");
    GLuint cardThree= LoadTexture(RESOURCE_FOLDER"cardClubs3.png");
    GLuint cardSeven= LoadTexture(RESOURCE_FOLDER"cardHearts7.png");
    GLuint cardJack= LoadTexture(RESOURCE_FOLDER"cardDiamondsJ.png");
    GLuint chip= LoadTexture(RESOURCE_FOLDER"chipRedWhite_side.png");
    
    //initializes untextured poly program
    ShaderProgram program1;
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
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.04f,0.4f,0.14f,1.0f);
        
        // initialize base vertices and tecture vertices
        float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        // draws first texture (cardback)
        glBindTexture(GL_TEXTURE_2D, cardBack);
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0f,0.45f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.4f,0.4f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // draws second texture (three)
        glBindTexture(GL_TEXTURE_2D,cardThree);
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(0.15f,0.45f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.4f,0.4f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draws third texture (Jack)
        glBindTexture(GL_TEXTURE_2D,cardJack);
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0f,-0.35f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.4f,0.4f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draws fourth texture (Seven)
        glBindTexture(GL_TEXTURE_2D,cardSeven);
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(0.15f,-0.35f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.4f,0.4f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draws fifth texture (First chip)
        glBindTexture(GL_TEXTURE_2D,chip);
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.3f,-0.05f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.15f,0.15f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // draws sixth texture (Second chip)
        glBindTexture(GL_TEXTURE_2D,chip);
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.27f,-0.05f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.15f,0.15f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // draws 7th untextured poly
        program1.SetColor(0.0f, 0.0f, 0.0f, 1.0f);
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.6f,0.1f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(1.5f,0.025f,1.0f));
        program1.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        SDL_GL_SwapWindow(displayWindow);
        
        
        
    }
    
    SDL_Quit();
    return 0;
}
