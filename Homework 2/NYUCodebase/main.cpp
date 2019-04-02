#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <iostream>
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

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("PONG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(0,0,640,360);
    
    ShaderProgram program;
    program.Load(RESOURCE_FOLDER"vertex.glsl",RESOURCE_FOLDER"fragment.glsl");
    
    glm::mat4 projectionMatrix=glm::mat4(1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    
    //sets up ortho and use the programs

    projectionMatrix= glm::ortho(-1.777f,1.777f,-1.0f,1.0f,-1.0f,1.0f);
    glUseProgram(program.programID);
    
    // only needs to set Projection & View once
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);

    float lastFrameTicks = 0.0f;

    // positions for leftBar and rightBar
    float leftBarYPos=0.0f;
    float rightBarYPos=0.0f;
    float leftBarXPos=-1.412f;
    float rightBarXPos=1.412f;
    
    // initial positions for the ball
    float ballX_position=0.0f;
    float ballY_position=0.0f;
    
    // default ball speed
    float ballXSpeed=1.5f;
    float ballYSpeed=1.5f;
    
    // width and length of borders
    float borderWidth=1*0.15f;
    float borderLength=1*2.9f;
    
    // width and length of ball
    float ballWidth=1*.075f;
    float ballLength=1*.075f;
    
    // width and length of left & right bar
    float barWidth=1*0.3f;
    float barLength=1*.075f;
    
    // initial positions of left & righr bar
    float barXpos=0.0f;
    float barYpos=1.0f;
    
    
    SDL_Event event;
    
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        
        // game tick system
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;

        // get keyboard state
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        
        //right bar controls
        if (keys[SDL_SCANCODE_DOWN]){
            //limits the bar movement so it cannot exceed top and bot border
            if(rightBarYPos-barWidth/2>-1.0f+borderWidth/2){
            rightBarYPos-=1*elapsed;
            }
        }else if (keys[SDL_SCANCODE_UP]){
            if(rightBarYPos+barWidth/2<1.0f-borderWidth/2){
            rightBarYPos+=1*elapsed;
            }
        }

        //left bar controls
        if (keys[SDL_SCANCODE_S]){
            //limits the bar movement so it cannot exceed top and bot border
            if(leftBarYPos-barWidth/2>-1.0f+borderWidth/2){
            leftBarYPos-=1*elapsed;
            }
        }else if (keys[SDL_SCANCODE_W]){
            if(leftBarYPos+barWidth/2<1.0f-borderWidth/2){
            leftBarYPos+=1*elapsed;
            }
        }
        
        // collision detection for left,right bar and top,bottom borderd
        float detectRightBarX=abs(ballX_position-rightBarXPos) - ((ballLength+barLength)/2);
        float detectRightBarY=abs(ballY_position-rightBarYPos) - ((ballWidth+barWidth)/2);
        
        float detectLeftBarX=abs(ballX_position-leftBarXPos) - ((ballLength+barLength)/2);
        float detectLeftBarY=abs(ballY_position-leftBarYPos) - ((ballWidth+barWidth)/2);
        
        float detectTopBorderX=abs(ballX_position-barXpos) - ((ballLength+borderLength)/2);
        float detectTopBorderY=abs(ballY_position-barYpos) - ((ballWidth+borderWidth)/2);
        
        float detectBotBorderX=abs(ballX_position-barXpos) - ((ballLength+borderLength)/2);
        float detectBotBorderY=abs(ballY_position+barYpos) - ((ballWidth+borderWidth)/2);
    
        
        // if it is less than zero, reflect the ball in X-direction
        if (detectRightBarX<0.0 && detectRightBarY<0.0){
            ballXSpeed=(-ballXSpeed);
            //reflect the ball in Y-irection depending where it hits top half of bar or lower half
            if (ballY_position>rightBarYPos){
                ballYSpeed=(-ballYSpeed);
            }
        }else if (detectTopBorderX<0.0 && detectTopBorderY<0.0){
            //only needs to reflect Y-direction if it hits top or bottom border
            ballYSpeed=(-ballYSpeed);
        }else if (detectLeftBarX<0.0 && detectLeftBarY<0.0){
            ballXSpeed=(-ballXSpeed);
            if (ballY_position>leftBarYPos){
                ballYSpeed=(-ballYSpeed);
            }
        }else if (detectBotBorderX<0.0 && detectBotBorderY<0.0){
            ballYSpeed=(-ballYSpeed);
        }

        // changing ball's position
        ballX_position+=(ballXSpeed*elapsed);
        ballY_position+=(ballYSpeed*elapsed);
        
        // checks if someone scores by changing background color and resetting the ball.
        if (ballX_position<leftBarXPos){
            glClearColor(0.5f,0.0f,0.0f,1.0f);
            ballX_position=0.0f;
            ballY_position=0.0f;
        }else if( ballX_position>rightBarXPos){
            glClearColor(0.0f,0.0f,0.5f,1.0f);
            ballX_position=0.0f;
            ballY_position=0.0f;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        
        //initialize vertex
        float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);

        //draws top border
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0f,1.0f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(2.9f,0.15f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draws bottom border
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0f,-1.0f,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(2.9f,0.15f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draws left bar
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(-1.412f,leftBarYPos,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.075f,0.30f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draws right bar
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(1.412f,rightBarYPos,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.075f,0.30f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draws "ball"
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(ballX_position,ballY_position,1.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.075f,0.075f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draws "net"
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.025f,2.0f,1.0f));
        program.SetModelMatrix(modelMatrix);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
