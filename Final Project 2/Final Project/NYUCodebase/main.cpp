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
#include <random>
#include <math.h>
#include <SatCollision.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define LEVEL_WIDTH 55
#define LEVEL_HEIGHT 40
#define SPRITE_COUNT_X 32
#define SPRITE_COUNT_Y 32
#define FIXED_TIMESTEP 0.0166666f
#define TILE_SIZE 1/16.0f
#define THREE_TEXT_OFFSET 0.05f
#define TWO_TEXT_OFFSET 0.030f


using namespace std;



SDL_Window* displayWindow;

// initializes the texture program as a global variable
ShaderProgram program;
//initializes the untextured program
ShaderProgram program1;
//initialize keyboard
const Uint8 *keys=SDL_GetKeyboardState(NULL);
//initialize textsheet
GLuint characters;
GLuint mineCraft;
GLuint font;
GLuint weapons;
// initialize matrix
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);
// initialze map
FlareMap map;
// initialize who's turn it is
int turnIndex=0;



class SheetSprite;
class Entity;
std::vector <Entity> entities;
std::vector <Entity> combatElements;
//initialize sprite index from xml for character models
vector<SheetSprite*> character1;



float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}


class SheetSprite {
public:
    
    SheetSprite(){};
    
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size):textureID(textureID),u(u),v(v),width(width),height(height),size(size),actualVertSize(glm::vec3(0.5f*size*(width/height),0.5*size,1.0f)){}
    
    void Draw();
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
    
    glm::vec3 actualVertSize; // size of the textured coordinates for the texture's ACTUAL size.
};

void SheetSprite::Draw() {
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
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}

void DrawText(int fontTexture, std::string text, float size, float spacing) {
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
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}


enum EntityType {PLAYER1,PLAYER2,ART,BULLETS};
enum ModeType{MOVE,ATTACK,IDLE};
class Entity{
public:
    
    void render(){
        if (aliveFlag){
            modelMatrix = glm::mat4(1.0f);
            modelMatrix=glm::translate(modelMatrix,position);
            if (faceLeftFlag){
                modelMatrix=glm::scale(modelMatrix,glm::vec3(-TILE_SIZE,TILE_SIZE,1.0f));
            }else{
                modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE,TILE_SIZE,1.0f));
            }
            program.SetModelMatrix(modelMatrix);
            sprite.Draw();
            
            if (&entities[turnIndex]!=this ){
                string hpString=std::to_string(hp);
                modelMatrix = glm::mat4(1.0f);
                modelMatrix=glm::translate(modelMatrix,position);
                if (hpString.size()==3){
                    modelMatrix=glm::translate(modelMatrix,glm::vec3((-THREE_TEXT_OFFSET),0.075f,0.0f));
                }else if (hpString.size()==2){
                    modelMatrix=glm::translate(modelMatrix,glm::vec3((-TWO_TEXT_OFFSET),0.075f,0.0f));
                }else{
                    modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0f,0.075f,0.0f));
                }
                program.SetModelMatrix(modelMatrix);
                DrawText(font, hpString, TILE_SIZE, 0.0005);
            }
        }
        
        if (&entities[turnIndex]==this && currentMode==ATTACK){
            // draw direction arrow
            modelMatrix = glm::mat4(1.0f);
            combatElements[0].position=position;
            modelMatrix=glm::translate(modelMatrix,combatElements[0].position);
            modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0,0.085,0.0));
            modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0,-0.085,0.0));
            modelMatrix = glm::rotate(modelMatrix, combatElements[0].rotation, glm::vec3(0.0f, 0.0f, 1.0f));
            modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0,0.085,0.0));
            modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE,TILE_SIZE,1.0f));
            modelMatrix=glm::scale(modelMatrix,glm::vec3(2.0f,1.5f,1.0f));
            program.SetModelMatrix(modelMatrix);
            combatElements[0].sprite.Draw();
            
            //draw gun
            modelMatrix = glm::mat4(1.0f);
            combatElements[1].position=position;
            combatElements[1].faceLeftFlag=faceLeftFlag;
            modelMatrix=glm::translate(modelMatrix,combatElements[1].position);
            if (faceLeftFlag){
                modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.02,0.0,0.0));
                modelMatrix=glm::scale(modelMatrix,glm::vec3(-TILE_SIZE,TILE_SIZE,1.0f));
            }else{
                modelMatrix=glm::translate(modelMatrix,glm::vec3(0.02,0.0,0.0));
                modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE,TILE_SIZE,1.0f));
            }
            modelMatrix=glm::scale(modelMatrix,glm::vec3(0.45f,0.45f,1.0f));
            program.SetModelMatrix(modelMatrix);
            combatElements[1].sprite.Draw();
            
            // draw bullet
            modelMatrix = glm::mat4(1.0f);
            modelMatrix=glm::translate(modelMatrix,combatElements[2].position);
            modelMatrix = glm::rotate(modelMatrix, combatElements[2].rotation, glm::vec3(0.0f, 0.0f, 1.0f));
            if (faceLeftFlag){
                modelMatrix=glm::scale(modelMatrix,glm::vec3(-TILE_SIZE,TILE_SIZE,1.0f));
            }else{
                modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE,TILE_SIZE,1.0f));
            }
            modelMatrix=glm::scale(modelMatrix,glm::vec3(0.55f,0.55f,1.0f));
            program.SetModelMatrix(modelMatrix);
            combatElements[2].sprite.Draw();
            
        }
        
        

    }

    void update(float elapsed){
        
        if (entityType==PLAYER1 || entityType==PLAYER2){
            if (turnTimer<=0.0f){
                turnTimer=5.0f;
                if(currentMode==MOVE && attackFlag){
                    currentMode=ATTACK;
                } else if (!moveFlag && !attackFlag){
                    moveFlag=true;
                    attackFlag=true;
                    nextTurn();
                }
            }
            
            if (aliveFlag){
                // gravity affecting all entities
                velocity.y += gravity.y * elapsed;
                
                // check collision against other entioties
                for (int i=0;i<9;i++){
                    if(checkEntityCollision(entities[i])){
                        if ((position.y - entities[i].position.y) > size.y/2*TILE_SIZE){
                            //entities[i].position.x=100.0f;
                        }
                    }
                }
                
                // friction
                velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
                velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);
            
                if (&entities[turnIndex]==this && (moveFlag || currentMode==MOVE)){
                    if (keys[SDL_SCANCODE_UP] && botFlag){
                        //only jump if botFlag is set
                        velocity.y+=30.0f*elapsed;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    if (keys[SDL_SCANCODE_RIGHT]){
                        velocity.x+=0.5f*elapsed;
                        faceLeftFlag=false;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    
                    if (keys[SDL_SCANCODE_LEFT]){
                        velocity.x-=0.5f*elapsed;
                        faceLeftFlag=true;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    if (currentMode!=IDLE){
                        turnTimer-=elapsed;
                    }
                }else if(&entities[turnIndex]==this && currentMode==ATTACK){
                    if (keys[SDL_SCANCODE_RIGHT]){
                        combatElements[0].rotation -= 90 * (3.1415926f / 180.0f)*elapsed ;
                        faceLeftFlag=false;
                    }
                     if (keys[SDL_SCANCODE_LEFT]){
                         combatElements[0].rotation += 90 * (3.1415926f / 180.0f)*elapsed ;
                         faceLeftFlag=true;
                     }
                    if (keys[SDL_SCANCODE_SPACE]){
                        combatElements[2].position=combatElements[0].position;
                        combatElements[2].faceLeftFlag=faceLeftFlag;
                        combatElements[2].aliveFlag=true;
                        combatElements[2].rotation=combatElements[0].rotation;
                        
                        if (faceLeftFlag){
                            combatElements[2].position.x-=0.04;
                        }else{
                            combatElements[2].position.x+=0.04;
                        }
                    }
                }

                
                //position updates
                position.y+=velocity.y*elapsed;
                checkYCollisionMap();
                position.x+=velocity.x*elapsed;
                checkXCollisionMap();
                
                
                //moving animation stuff
                animationElapsed += elapsed;
                if(animationElapsed > 1.0/framesPerSecond) {
                    index++;
                    animationElapsed = 0.0;
                    if(index > 3) {
                        index = 0;
                    }
                }
                if (abs(velocity.x)<=0.025){
                    index=1;
                }
                sprite=*((*modelAnimation)[index]);
            }
            
        }else if(entityType==BULLETS && aliveFlag){
            if (faceLeftFlag){
                position.x+=0.25*elapsed*cos(rotation);
                position.y+=0.25*elapsed*sin(rotation);
                
            }else{
                position.x+=0.25*elapsed;
                position.y+=0.25*elapsed;

            }
            checkYCollisionMap();
            if (botFlag || topFlag){
                aliveFlag=false;
                position=glm::vec3(100);
            }
            checkXCollisionMap();
            if (leftFlag || rightFlag){
                aliveFlag=false;
                position=glm::vec3(100);
            }
            
        }
        
    }
    

    void checkYCollisionMap(){
        checkBotCollisionMap();
        checkTopCollisionMap();
    }
    
    void checkXCollisionMap(){
        checkRightCollisionMap();
        checkLeftCollisionMap();
    }
    
    bool checkEntityCollision( const Entity& someEntity){
        float yCollide=abs(position.y-someEntity.position.y)-((size.y/2+someEntity.size.y/2)*TILE_SIZE);
        float xCollide=abs(position.x-someEntity.position.x)-((size.x/2+someEntity.size.x/2)*TILE_SIZE);
        return (xCollide<=0.0f && yCollide<=0.0f);
    }
    
    EntityType entityType;
    SheetSprite sprite;
    
    vector<SheetSprite*>* modelAnimation;
    float animationElapsed = 0.0f;
    float framesPerSecond = 10.0f;
    int index=0;
    
    glm::vec3 position;
    glm::vec3 size=glm::vec3(1.0f);
    glm::vec3 velocity=glm::vec3(0.0f,0.0f,0.0f);
    glm::vec3 gravity=glm::vec3(0.0f,-0.5f,0.0f);
    glm::vec3 friction=glm::vec3(1.0f,0.0f,0.0f);
    
    int hp=100;
    float rotation;
    float turnTimer=3.0;
    bool moveFlag=true;
    bool attackFlag=true;
    bool aliveFlag=true;
    ModeType currentMode=IDLE;
    

    bool topFlag=false;
    bool botFlag=false;
    bool leftFlag=false;
    bool rightFlag=false;
    bool faceLeftFlag=false;
    
private:
    
    void checkBotCollisionMap(){
        int gridX = (int) (position.x / (TILE_SIZE));
        int gridY = (int)(position.y/(-TILE_SIZE)+size.y/2);
        if (map.mapData[gridY][gridX]==1 || map.mapData[gridY][gridX]==35){
            aliveFlag=false;
            checkVictory();
            nextTurn();
        }else if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=257 && map.mapData[gridY][gridX]!=203 ){
            float penetration=fabs(((-TILE_SIZE*gridY)-(position.y-(size.y/2)*TILE_SIZE)));
            position.y+=penetration;
            velocity.y=0;
            botFlag=true;
        }else{
            botFlag=false;
        }
    }
    
    void checkTopCollisionMap(){
        int gridX = (int) (position.x / (TILE_SIZE));
        int gridY = (int)(position.y/(-TILE_SIZE)-size.y/2);
        if (map.mapData[gridY][gridX]==1 || map.mapData[gridY][gridX]==35){
            aliveFlag=false;
            checkVictory();
            nextTurn();
        }else if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=257 && map.mapData[gridY][gridX]!=203 ){
            float penetration=fabs(((-TILE_SIZE*gridY)-(position.y+(size.y/2)*TILE_SIZE)));
            // needed a little bit more to completely resolve penetration without stuttering camera movement
            penetration-=0.05;
            position.y-=penetration;
            velocity.y=0;
            topFlag=true;
        }else{
            topFlag=false;
        }
    }
    
    void checkRightCollisionMap(){
        int gridX = (int) (position.x / (TILE_SIZE) + size.y/2);
        int gridY = (int)(position.y/(-TILE_SIZE));
        if (map.mapData[gridY][gridX]==1 || map.mapData[gridY][gridX]==35){
            aliveFlag=false;
            checkVictory();
            nextTurn();
        }else if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=257 && map.mapData[gridY][gridX]!=203 ){
            float penetration=fabs(((TILE_SIZE*gridX)-position.x-(size.x/2)*TILE_SIZE));
            position.x-=penetration;
            rightFlag=true;
            velocity.x=0;
        }else{
            rightFlag=false;
        }
    }
    
    void checkLeftCollisionMap(){
        int gridX = (int) (position.x / (TILE_SIZE) - size.y/2);
        int gridY = (int)(position.y/(-TILE_SIZE));
        if (map.mapData[gridY][gridX]==1 || map.mapData[gridY][gridX]==35){
            aliveFlag=false;
            checkVictory();
            nextTurn();
        }else if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=257 && map.mapData[gridY][gridX]!=203 ){
            float penetration=fabs(((TILE_SIZE*gridX)-position.x+(size.x/2)*TILE_SIZE));
            penetration-=0.06f;
            position.x+=penetration;
            leftFlag=true;
            velocity.x=0;
        }else{
            leftFlag=false;
        }
    }
    
    void checkVictory(){
        int aliveCount=0;
        for (Entity& someEntity:entities){
            if (someEntity.aliveFlag){
                aliveCount++;
            }
        }
        
    }
    
    void nextTurn(){
        if(&entities[turnIndex]==this){
            turnIndex++;
            if (turnIndex==4){
                turnIndex=0;
            }
            while (!entities[turnIndex].aliveFlag){
                if (turnIndex==4){
                    turnIndex=0;
                }
                turnIndex++;
            }
        }
    }
};


void convertFlareEntity(){
    // converts flare map entity to in-game entity.
    for (FlareMapEntity& someEntity:map.entities){
        float xPos=someEntity.x*TILE_SIZE;
        float yPos=someEntity.y*-TILE_SIZE;
        Entity newEntity;
        newEntity.position=glm::vec3(xPos,yPos,0.0f);
        newEntity.modelAnimation=&character1;
        newEntity.entityType=PLAYER1;
        newEntity.sprite=*((*newEntity.modelAnimation)[newEntity.index]);
        entities.push_back(newEntity);
    }
    
};

void drawMCSprite(int index, int spriteCountX,int spriteCountY) {
    float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
    float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
    float spriteWidth = 1.0/(float)spriteCountX;
    float spriteHeight = 1.0/(float)spriteCountY;
    float texCoords[] = {
        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,
        u+spriteWidth, v,
        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight
    };
    float vertices[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f,
        -0.5f, 0.5f, -0.5f};
    
    glBindTexture(GL_TEXTURE_2D, mineCraft);
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    
}


void renderUI(){
    //writes move left
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,entities[turnIndex].position);
    modelMatrix=glm::translate(modelMatrix, glm::vec3(-1.75f,0.95f,0.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "Moves Left:", TILE_SIZE, 0.0005);
    
    //render move icon
    if (entities[turnIndex].moveFlag){
        modelMatrix = glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,entities[turnIndex].position);
        modelMatrix=glm::translate(modelMatrix, glm::vec3(-1.20f,0.95f,0.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE));
        program.SetModelMatrix(modelMatrix);
        drawMCSprite(314, SPRITE_COUNT_X, SPRITE_COUNT_Y);
    }
    
    // render attack icon
    if (entities[turnIndex].attackFlag){
        modelMatrix = glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,entities[turnIndex].position);
        modelMatrix=glm::translate(modelMatrix, glm::vec3(-1.1f,0.95f,0.0f));
        modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE));
        program.SetModelMatrix(modelMatrix);
        drawMCSprite(59, SPRITE_COUNT_X, SPRITE_COUNT_Y);
    }
    
    // write timer
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,entities[turnIndex].position);
    modelMatrix=glm::translate(modelMatrix, glm::vec3(-1.75f,0.85f,0.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "Move Timer:", TILE_SIZE, 0.0005);
    
    //render time left
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,entities[turnIndex].position);
    modelMatrix=glm::translate(modelMatrix, glm::vec3(-1.20f,0.85f,0.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
    program.SetModelMatrix(modelMatrix);
    int timeInt=entities[turnIndex].turnTimer;
    string time = std::to_string(timeInt)+"s";
    DrawText(font, time, TILE_SIZE, 0.0005);
    
    
    //render "current HP"
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,entities[turnIndex].position);
    modelMatrix=glm::translate(modelMatrix, glm::vec3(-1.750f,0.75f,0.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "Current HP", TILE_SIZE, 0.0005);
    
    //render  HP
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,entities[turnIndex].position);
    modelMatrix=glm::translate(modelMatrix, glm::vec3(-1.2f,0.75f,0.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
    program.SetModelMatrix(modelMatrix);
    string health = std::to_string(entities[turnIndex].hp);
    DrawText(font, health, TILE_SIZE, 0.0005);
    
}

void randomSpawn(){
    //generate 4 player
    srand(time(0));
    vector <int> positions;
    for (int i=0;i<4;i++){
        //random x position
        int randomX=((rand())%39)+8;
        // dont want duplicate x positions for spawns
        while (find(positions.begin(),positions.end(),randomX)!=positions.end()){
            randomX=((rand())%39)+8;
        }
        positions.push_back(randomX);
        // random to choose probe from top to bot or bot to top
        int randomTop=(rand())%2;
        if (randomTop<5){
            for (int i=3;i<LEVEL_HEIGHT;i++){
                if(map.mapData[i][randomX]!=704 && map.mapData[i][randomX]!=203 && map.mapData[i][randomX]!=1 && map.mapData[i][randomX]!=35 && map.mapData[i][randomX]!=257 ){
                    FlareMapEntity someEntity;
                    someEntity.x=randomX;
                    someEntity.y=i-1;
                    map.entities.push_back(someEntity);
                    break;
                }
            }
//        }else{
//            cout<<"bot"<<endl;
//            for (int i=LEVEL_HEIGHT-1;i>0;i--){
//                if(map.mapData[i][randomX]==704 && map.mapData[i][randomX]==257 && map.mapData[i][randomX]==203){
//                    FlareMapEntity someEntity;
//                    someEntity.x=randomX;
//                    someEntity.y=i;
//                    map.entities.push_back(someEntity);
//                    break;
//                }
            }
        }

    convertFlareEntity();
}

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

void renderMap(){
    
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    
    for(int y=0; y < LEVEL_HEIGHT; y++) {
        for(int x=0; x < LEVEL_WIDTH; x++) {
            // 0 on my sprite sheet was a lava tile so I had to change the tile to an invisible tile since Tiles defaults invisible tile to 0
            if (map.mapData[y][x]==0){
                map.mapData[y][x]=704;
            }
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
    
    glBindTexture(GL_TEXTURE_2D, mineCraft);
    
    glm::mat4 modelMatrix=glm::mat4(1.0f);
    program.SetModelMatrix(modelMatrix);
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    glDrawArrays(GL_TRIANGLES, 0, LEVEL_WIDTH*LEVEL_HEIGHT*6);
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
}


void setUp(){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("XCOM Worms", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 576, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif

    //intial set-up
    glViewport(0,0,1024,576);
    
    
    //loads textured file in program
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    //loads untextured poly program
    program1.Load(RESOURCE_FOLDER"vertex.glsl",RESOURCE_FOLDER"fragment.glsl");
    
    
    glm::mat4 projectionMatrix=glm::mat4(1.0f);

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
    
    map.Load(RESOURCE_FOLDER"level1.txt");
    mineCraft= LoadTexture(RESOURCE_FOLDER"minecraft_Sprite.png");
    characters=LoadTexture(RESOURCE_FOLDER"sprites.png");
    font=LoadTexture(RESOURCE_FOLDER"font2.png");
    weapons=LoadTexture(RESOURCE_FOLDER"combat.png");
    renderMap();
    
    SheetSprite* model_1_1 = new SheetSprite(characters,0.0f/32.0f, 16.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_1);
    SheetSprite* model_1_2 = new SheetSprite(characters,10.0f/32.0f, 0.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_2);
    SheetSprite* model_1_3 = new SheetSprite(characters,10.0f/32.0f, 16.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_3);
    SheetSprite* model_1_4 = new SheetSprite(characters,0.0f/32.0f, 0.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_4);
    
    //arrow
    SheetSprite* arrowSprite= new SheetSprite(weapons,58.0f/128.0f, 0.0f/64.0f, 12.0f/128.0f, 46.0f/64.0f, 1.0f);
    Entity arrow;
    arrow.sprite=*arrowSprite;
    arrow.position=glm::vec3(100);
    arrow.friction=glm::vec3(0);
    arrow.entityType=ART;
    combatElements.push_back(arrow);
    
    //machine gun
    SheetSprite* machineGunSprite= new SheetSprite(weapons,0.0f/128.0f, 0.0f/64.0f, 58.0f/128.0f, 26.0f/64.0f, 1.0f);
    Entity machineGun;
    machineGun.sprite=*machineGunSprite;
    arrow.position=glm::vec3(100);
    machineGun.friction=glm::vec3(0);
    machineGun.entityType=ART;
    combatElements.push_back(machineGun);
    
    //bullet
    SheetSprite* bulletSprite= new SheetSprite(weapons,0.0f/128.0f, 26.0f/64.0f, 13.0f/128.0f, 38.0f/64.0f, 1.0f);
    Entity bullet;
    bullet.sprite=*bulletSprite;
    bullet.position=glm::vec3(100);
    bullet.friction=glm::vec3(0);
    bullet.entityType=BULLETS;
    bullet.aliveFlag=false;
    combatElements.push_back(bullet);

    
    randomSpawn();
    
}

void renderGameLevel(){
    renderMap();
    renderUI();
    for (Entity& someEntity:entities){
        someEntity.render();
    }
}

void update(float elapsed){
    for (Entity& someEntity:entities){
        someEntity.update(elapsed);
    }
    for (Entity& someEntity:combatElements){
        someEntity.update(elapsed);
    }
}

void cameraMovement(){
    viewMatrix = glm::mat4(1.0f);
    if (keys[SDL_SCANCODE_1]){
        turnIndex=1;
    }
    if (keys[SDL_SCANCODE_2]){
        turnIndex=2;
    }
    if (keys[SDL_SCANCODE_3]){
        turnIndex=3;
    }
    viewMatrix=glm::translate(viewMatrix,-entities[turnIndex].position);    
    program.SetViewMatrix(viewMatrix);
}



int main(int argc, char *argv[])
{
    
    setUp();
    renderGameLevel();

    float accumulator = 0.0f;
    float lastFrameTicks=0.0f;
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        //background as skyblue
        glClearColor(29/255.0, 41/255.0, 81/255.0, 1.0f);
        
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
            update(FIXED_TIMESTEP);
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        
        renderGameLevel();
        cameraMovement();
        SDL_GL_SwapWindow(displayWindow);
    }
    
    


    SDL_Quit();
    return 0;
}
