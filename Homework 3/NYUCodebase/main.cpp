#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <vector>
#include "ShaderProgram.h"
#include "glm/mat4x2.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;


#define MAX_BULLETS 5
#define FIXED_TIMESTEP 0.0166666f
#define LEFTWALL_POS -1.777f
#define RIGHTWALL_POS 1.777f
#define START_BOX_Left -0.5f
#define START_BOX_TOP 0.122f
#define START_BOX_BOTTOM -0.122f
#define START_BOX_RIGHT 0.5f
#define WINNING_SCORE 2100


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

class Entity;

struct GameState{
    std::vector<Entity>spaceShips;
    std::vector<Entity>bullets;
    int bulletIndex=0;
    int score=0;
};

enum GameMode {MAIN_MENU,GAME_LEVEL,GAME_OVER};

GameMode mode=MAIN_MENU;
GameState state;



// initializes the texture program as a global variable
ShaderProgram program;
//initializes the untextured program
ShaderProgram program1;
//initialize textsheet
GLuint textSheet;
// initialize keyboard state as a global variable
const Uint8 *keys=SDL_GetKeyboardState(NULL);
//initialize mouse positions
float mouseX;
float mouseY;

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


class SheetSprite {
    public:
    
    SheetSprite(){};

    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size):textureID(textureID),u(u),v(v),width(width),height(height),size(size),actualVertSize(glm::vec3(0.5f*size*(width/height),0.5*size,1.0f)){}

    void Draw(ShaderProgram &program);
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
    
    glm::vec3 actualVertSize; // size of the textured coordinates for the texture's ACTUAL size.
};

void SheetSprite::Draw(ShaderProgram &program) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLfloat texCoords[] = {
        u, v+height,
        u+width, v,
        u, v,
        u+width, v,
        u, v+height,
        u+width, v+height
    };
    float aspect = width / height;
    float vertices[] = {
        -0.5f * size * aspect, -0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, 0.5f * size,
        0.5f * size * aspect, 0.5f * size,
        -0.5f * size * aspect, -0.5f * size ,
        0.5f * size * aspect, -0.5f * size};
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

enum EntityType {ENEMY,PLAYER,BULLETS};
class Entity {
public:
    
    void Update(float elapsed){
        // enemies control - only check if enemy is alive
        if (entityType==ENEMY && aliveFlag){
            //checks collision with any bullets
            for (int i=0;i<MAX_BULLETS;i++){
                if (checkCollision(state.bullets[i])){
                    aliveFlag=false;
                    state.bullets[i].position.x=200.0f;
                    state.score+=100;
                    if (state.score==2100){
                        mode=GAME_OVER;
                    }
                    break;
                }
            }
            // make the enemies move down a set period of time(verticalMovetimer) before reversing if something has hit a wall
            if( collisionRight && verticalMoveTimer>0.0f){
                verticalMoveTimer-=elapsed;
                position.y-=velocity.y*elapsed;
                // after moving down, reset collision detection so enemies can continiue moving horizontally
                if (verticalMoveTimer<=0.0f){
                    for (Entity& someEntity:state.spaceShips){
                        if (someEntity.entityType==ENEMY){
                        someEntity.collisionRight=false;
                        someEntity.verticalMoveTimer=0.6f;
                        }
                    }
                }
            }else if(collisionLeft && verticalMoveTimer>0.0f){
                    verticalMoveTimer-=elapsed;
                    position.y-=velocity.y*elapsed;
                    if (verticalMoveTimer<=0.0f){
                        for (Entity& someEntity:state.spaceShips){
                            if (someEntity.entityType==ENEMY){
                            someEntity.collisionLeft=false;
                            someEntity.verticalMoveTimer=0.6f;
                            }
                        }
                    }
            }else{
                // enemies move normally (horziontal)
                position.x+=velocity.x*elapsed;
            }
            
            //if one enemy has hit the right wall, reverse every other enemy
            if (position.x+size.x>=RIGHTWALL_POS && aliveFlag){

                for (Entity& someEntity:state.spaceShips){
                    if (someEntity.entityType==ENEMY){
                    someEntity.collisionRight=true;
                    someEntity.velocity.x=-someEntity.velocity.x;

                    }
                }
            }
            //if one enemy has hit the left wall, reverse every other enemy
            if(position.x-size.x<=LEFTWALL_POS && aliveFlag){
                for (Entity& someEntity:state.spaceShips){
                    if (someEntity.entityType==ENEMY){
                    someEntity.collisionLeft=true;
                    someEntity.velocity.x=-someEntity.velocity.x;
                    }
                }
            }
            
        // player controls
        }else if(entityType==PLAYER){
            if (keys[SDL_SCANCODE_RIGHT]){
                if(position.x+(size.x)<=RIGHTWALL_POS){
                    position.x+=velocity.x*elapsed;
                }
            }
            if (keys[SDL_SCANCODE_LEFT]){
                if(position.x-(size.x)>=LEFTWALL_POS){
                    position.x-=velocity.x*elapsed;
                }
            }
            if (keys[SDL_SCANCODE_SPACE]){
                //cool down for shooting bullets
                if (shootCoolDown<=0.0f){
                    shootBullet();
                    shootCoolDown=0.125f;
                }else{
                    shootCoolDown-=elapsed;
                }
            }
           
        // bullet movements
        }else if(entityType==BULLETS){
            position.y+=elapsed*velocity.y;
        }
    }
    
    // collision detection of entity vs entity
    bool checkCollision( const Entity& someEntity){
        float yCollide=abs(position.y-someEntity.position.y)-((size.y+someEntity.size.y));
        float xCollide=abs(position.x-someEntity.position.x)-((size.x+someEntity.size.x));
        return (xCollide<=0.0f && yCollide<=0.0f);
    }
    
    void Render(ShaderProgram & program){
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,position);
        program.SetModelMatrix(modelMatrix);
        sprite.Draw(program);
    }
    
    void shootBullet(){
        //shoot from where the ship is by taking size into account so laser don't come from the middle of ship
        state.bullets[state.bulletIndex].position.x=position.x;
        state.bullets[state.bulletIndex].position.y=position.y+size.y+state.bullets[state.bulletIndex].size.y;
        state.bulletIndex++;
        if(state.bulletIndex > MAX_BULLETS-1) {
            state.bulletIndex = 0;
        }
    }
    
    SheetSprite sprite;
    EntityType entityType;
    
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 size;
    
    float shootCoolDown=0.0f;
    float verticalMoveTimer=0.6f;
    
    bool aliveFlag=true;
    bool collisionRight=false;
    bool collisionLeft=false;

};

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
    float character_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x + character_size, texture_y + character_size,
            texture_x + character_size, texture_y,
            texture_x, texture_y + character_size,
        }); }
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    // draw this data (use the .data() method of std::vector to get pointer to data)
    // draw this yourself, use text.size() * 6 or vertexData.size()/2 to get number of vertices
    


        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
        glEnableVertexAttribArray(program.positionAttribute);
        
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6*text.size());
    
    

}


void renderGameLevel(GameState& state){
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    for ( Entity& someEntity:state.spaceShips){
        if(someEntity.aliveFlag){
            someEntity.Render(program);
        }
    }
    for(int i=0; i < MAX_BULLETS; i++) {
        state.bullets[i].Render(program);
    }
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-1.65f,-0.85f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.25f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    string scoreST=std::to_string(state.score);
    DrawText(program, textSheet, scoreST , 0.5f,0.0005f);
    
}

void renderGameMenu(){
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
    glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program1.positionAttribute);

    program1.SetColor(1.0f, 0.0f, 0.0f, 1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::scale(modelMatrix,glm::vec3(1.0f,0.25f,1.0f));
    program1.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.25f,0.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(program, textSheet, "Start", 0.25f,0.0005f);

}

void renderGameOver(){
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
    glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program1.positionAttribute);
    
    program1.SetColor(1.0f, 0.0f, 0.0f, 1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::scale(modelMatrix,glm::vec3(1.0f,0.25f,1.0f));
    program1.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.385f,0.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.35f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(program, textSheet, "Play Again", 0.25f,0.0005f);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.45f,0.25f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.45f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    if (state.score<WINNING_SCORE){
        DrawText(program, textSheet, "You Lose!", 0.25f,0.0005f);
    }else if(state.score==WINNING_SCORE){
        DrawText(program, textSheet, "You Win!", 0.25f, 0.0005f);
    }
}

void Render(){
    switch(mode){
        case MAIN_MENU:
            renderGameMenu();
            break;
        case GAME_LEVEL:
            renderGameLevel(state);
            break;
        case GAME_OVER:
            renderGameOver();
    }
}

void updateGameLevel(GameState & state,float elapsed){
    for (int i=0;i<MAX_BULLETS;i++){
        state.bullets[i].Update(FIXED_TIMESTEP);
    }
    for (Entity& someEntity:state.spaceShips){
        someEntity.Update(FIXED_TIMESTEP);
    }
}



void updateMainMenu(){
    
    // The check for button down was not working in this function. I was not sure why. Moved to the the while loop
//    SDL_Event mouse;
//        while (SDL_PollEvent(&mouse)) {
//            if (mouse.type==SDL_MOUSEBUTTONDOWN) {
//                mode=GAME_LEVEL;
//            }
//        }
}

void Update(float elapsed){
    switch(mode){
        case MAIN_MENU:
            updateMainMenu();
            break;
        case GAME_LEVEL:
            updateGameLevel(state, elapsed);
    }
}

void setUp(GameState& state){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Space Invader", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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
    
    //create enemy entities and load them into a vector
    float x_pos=-1.1f;
    float y_pos=0.8f;
    GLuint spaceSheet=LoadTexture(RESOURCE_FOLDER"sheet.png");
    textSheet=LoadTexture(RESOURCE_FOLDER"font1.png");
    for(int i=0; i < 21; i++) {
        if (x_pos>1.1f){
            // if x>1.1f, create enemies on the next row
            x_pos=-1.1f;
            y_pos-=0.25f;
        }
        Entity myEntity;
        myEntity.sprite = SheetSprite(spaceSheet, 423.0f/1024.0f, 728.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, 0.20f);
        myEntity.position=glm::vec3(x_pos,y_pos,1.0f);
        myEntity.entityType=ENEMY;
        myEntity.velocity=glm::vec3(0.25f,0.1f,1.0f);
        myEntity.size=myEntity.sprite.actualVertSize;
        x_pos+=0.35f; // x-coord space between enemies
        state.spaceShips.push_back(myEntity);
    }
    
    //create player ship
    Entity playerShip;
    playerShip.sprite=SheetSprite(spaceSheet,0.0f/1024.0f, 941.0f/1024.0f, 112.0f/1024.0f, 75.0f/1024.0f, 0.20f);
    playerShip.position=glm::vec3(0.0f,-0.75f,1.0f);
    playerShip.velocity=glm::vec3(0.75f,0.0f,1.0f);
    playerShip.size=playerShip.sprite.actualVertSize;
    playerShip.entityType=PLAYER;
    state.spaceShips.push_back(playerShip);
    
    // create bullets
    for(int i=0; i < MAX_BULLETS; i++) {
        Entity bullet;
        state.bullets.push_back(bullet);
        state.bullets[i].sprite=SheetSprite(spaceSheet, 856.0f/1024.0f, 421.0f/1024.0f, 9.0f/1024.0f, 54.0f/1024.0f, 0.20f);
        state.bullets[i].position=glm::vec3(2000.0f,0.0f,1.0f);
        state.bullets[i].velocity=glm::vec3(0.0f,1.0f,1.0f);
        state.bullets[i].size=state.bullets[i].sprite.actualVertSize;
        state.bullets[i].entityType=BULLETS;
    }
    
}

int main(int argc, char *argv[])
{
    setUp(state);

    float accumulator = 0.0f;
    float lastFrameTicks=0.0f;
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
            
            if(event.type == SDL_MOUSEMOTION) {
                 mouseX = (((float)event.motion.x / 640.0f) * 3.554f ) - 1.777f;
                 mouseY = (((float)(360-event.motion.y) / 360.0f) * 2.0f ) - 1.0f;
            }
            
//            cout<<mouseX<<"-"<<mouseY<<endl;
            // switch mode on click
            if (event.type==SDL_MOUSEBUTTONDOWN && mode==MAIN_MENU) {
                if(mouseX>START_BOX_Left && mouseX<START_BOX_RIGHT && mouseY>START_BOX_BOTTOM && mouseY<START_BOX_TOP){
                mode=GAME_LEVEL;
                }
            }
            if (event.type==SDL_MOUSEBUTTONDOWN && mode==GAME_OVER) {
                if(mouseX>START_BOX_Left && mouseX<START_BOX_RIGHT && mouseY>START_BOX_BOTTOM && mouseY<START_BOX_TOP){
                    mode=GAME_LEVEL;
                    float x_pos=-1.1f;
                    float y_pos=0.8f;
                    for(int i=0; i < 21; i++) {
                        if (x_pos>1.1f){
                            // if x>1.1f, create enemies on the next row
                            x_pos=-1.1f;
                            y_pos-=0.25f;
                        }
                        state.spaceShips[i].position=glm::vec3(x_pos,y_pos,1.0f);
                        state.spaceShips[i].aliveFlag=true;
                        x_pos+=0.35f;
                    }
                    state.spaceShips[21].position=glm::vec3(0.0f,-0.75f,1.0f);
                    state.score=0;
                    for(int i=0; i < MAX_BULLETS; i++) {
                        state.bullets[i].position=glm::vec3(2000.0f,0.0f,1.0f);
                    }
                }
            }
        }
        
        //game tick system
        float ticks=(float)SDL_GetTicks()/1000.0f;
        float elapsed=ticks-lastFrameTicks;
        lastFrameTicks=ticks;
        
        //better collision system with Fixed timestep
        elapsed += accumulator;
        if(elapsed < FIXED_TIMESTEP) {
            accumulator = elapsed;
            continue; }
        
        while(elapsed >= FIXED_TIMESTEP) {
            Update(FIXED_TIMESTEP);
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        Render();
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        glDisableVertexAttribArray(program1.positionAttribute);
        SDL_GL_SwapWindow(displayWindow);
    }

    
    SDL_Quit();
    return 0;
}
