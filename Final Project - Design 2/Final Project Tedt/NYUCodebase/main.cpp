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
#define START_BOX_Left 0.26655f
#define START_BOX_TOP -0.422222f
#define START_BOX_BOTTOM -0.8f
#define START_BOX_RIGHT 1.8603


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
//initialize mouse positions
float mouseX;
float mouseY;
int setUpCount=0;




class SheetSprite;
class Entity;
std::vector <Entity> entities;
std::vector <Entity> combat;
//initialize sprite index from xml for character models
vector<SheetSprite*> character1;



enum GameMode {MAIN_MENU,MAP_SELECT,GAME_LEVEL,GAME_OVER,PAUSE};
GameMode mode=MAIN_MENU;



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
            

//            string hpString=std::to_string(hp);
//            modelMatrix = glm::mat4(1.0f);
//            modelMatrix=glm::translate(modelMatrix,position);
//            if (hpString.size()==3){
//                modelMatrix=glm::translate(modelMatrix,glm::vec3((-THREE_TEXT_OFFSET),0.075f,0.0f));
//            }else if (hpString.size()==2){
//                modelMatrix=glm::translate(modelMatrix,glm::vec3((-TWO_TEXT_OFFSET),0.075f,0.0f));
//            }else{
//                modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0f,0.075f,0.0f));
//            }
//            program.SetModelMatrix(modelMatrix);
//            DrawText(font, hpString, TILE_SIZE, 0.0005);

        
            //draw gun
            modelMatrix = glm::mat4(1.0f);
            combat[0].position=position;
            combat[0].faceLeftFlag=faceLeftFlag;
            modelMatrix=glm::translate(modelMatrix,combat[0].position);
            if (faceLeftFlag){
                modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.02,0.0,0.0));
                modelMatrix=glm::scale(modelMatrix,glm::vec3(-TILE_SIZE,TILE_SIZE,1.0f));
            }else{
                modelMatrix=glm::translate(modelMatrix,glm::vec3(0.02,0.0,0.0));
                modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE,TILE_SIZE,1.0f));
            }
            modelMatrix=glm::scale(modelMatrix,glm::vec3(0.45f,0.45f,1.0f));
            program.SetModelMatrix(modelMatrix);
            combat[0].sprite.Draw();
        }
        
        // draw bullet
        modelMatrix = glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,combat[1].position);
        float rotation= 90*(3.14159/180);
        modelMatrix=glm::rotate(modelMatrix, rotation , glm::vec3(0.0f,0.0f,1.0f));
        if (faceLeftFlag){
            modelMatrix=glm::scale(modelMatrix,glm::vec3(-TILE_SIZE,TILE_SIZE,1.0f));
        }else{
            modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE,TILE_SIZE,1.0f));
        }
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.55f,0.55f,1.0f));
        program.SetModelMatrix(modelMatrix);
        combat[1].sprite.Draw();
        
        modelMatrix = glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,combat[2].position);
        modelMatrix=glm::rotate(modelMatrix, rotation , glm::vec3(0.0f,0.0f,1.0f));
        if (faceLeftFlag){
            modelMatrix=glm::scale(modelMatrix,glm::vec3(-TILE_SIZE,TILE_SIZE,1.0f));
        }else{
            modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE,TILE_SIZE,1.0f));
        }
        modelMatrix=glm::scale(modelMatrix,glm::vec3(0.55f,0.55f,1.0f));
        program.SetModelMatrix(modelMatrix);
        combat[2].sprite.Draw();
        
    }

    void update(float elapsed){
        
        checkVictory();
        if (entityType==PLAYER1 || entityType==PLAYER2){
            
            if (aliveFlag){
                // gravity affecting all entities
                velocity.y += gravity.y * elapsed;
                
                // friction
                velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
                velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);
            
                if (entityType==PLAYER2){
                    
                    
                    if (keys[SDL_SCANCODE_I] && botFlag){
                        //only jump if botFlag is set
                        velocity.y+=30.0f*elapsed;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    if (keys[SDL_SCANCODE_L]){
                        velocity.x+=0.5f*elapsed;
                        faceLeftFlag=false;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    
                    if (keys[SDL_SCANCODE_J]){
                        velocity.x-=0.5f*elapsed;
                        faceLeftFlag=true;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    if (keys[SDL_SCANCODE_RETURN] && combat[2].shotFlag){
                        combat[2].shotFlag=false;
                        combat[2].position=position;
                        combat[2].faceLeftFlag=faceLeftFlag;
                        combat[2].aliveFlag=true;
                        
                        if (faceLeftFlag){
                            combat[2].position.x-=0.04;
                        }else{
                            combat[2].position.x+=0.04;
                        }
                    }
                }else if (entityType==PLAYER1){
                    if (keys[SDL_SCANCODE_W] && botFlag){
                        //only jump if botFlag is set
                        velocity.y+=30.0f*elapsed;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    if (keys[SDL_SCANCODE_D]){
                        velocity.x+=0.5f*elapsed;
                        faceLeftFlag=false;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    
                    if (keys[SDL_SCANCODE_A]){
                        velocity.x-=0.5f*elapsed;
                        faceLeftFlag=true;
                        moveFlag=false;
                        currentMode=MOVE;
                    }
                    if (keys[SDL_SCANCODE_SPACE] && combat[1].shotFlag){
                        combat[1].shotFlag=false;
                        combat[1].position=position;
                        combat[1].faceLeftFlag=faceLeftFlag;
                        combat[1].aliveFlag=true;
                        
                        if (faceLeftFlag){
                            combat[1].position.x-=0.04;
                        }else{
                            combat[1].position.x+=0.04;
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
            if (this==&combat[2]){
                if(checkEntityCollision(entities[0])){
                    entities[0].aliveFlag=false;
                    checkVictory();
                }
            }else if (this==&combat[1]){
                if(checkEntityCollision(entities[1])){
                    entities[1].aliveFlag=false;
                    checkVictory();
                }
            }
            
            if (faceLeftFlag){
                position.x-=1.0*elapsed;
                
            }else{
                position.x+=1.0*elapsed;
            }
            checkYCollisionMap();
            if (botFlag || topFlag){
                aliveFlag=false;
                position=glm::vec3(100);
            }
            if (aliveFlag){
                checkXCollisionMap();
                if (leftFlag || rightFlag){
                    aliveFlag=false;
                    position=glm::vec3(100);
                }
            }
        }
    }

    void checkYCollisionMap(){
        checkVictory();
        checkBotCollisionMap();
        checkTopCollisionMap();
    }
    
    void checkXCollisionMap(){
        checkVictory();
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
    
    bool shotFlag=true;
    int lives=2;
    int hp=100;
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
        if (aliveFlag){
            int gridX = (int) (position.x / (TILE_SIZE));
            int gridY = (int)(position.y/(-TILE_SIZE)+size.y/2);
            // dispawn if past world borders
            if (entityType==BULLETS){
                if (gridX<=0 && gridX>=LEVEL_WIDTH){
                    aliveFlag=false;
                    shotFlag=true;
                }
                if (gridY<=0 && gridY>=LEVEL_HEIGHT){
                    aliveFlag=false;
                    shotFlag=true;
                }
            }
            // set death if collided with world hazards
            if (map.mapData[gridY][gridX]==1 || map.mapData[gridY][gridX]==35){
                aliveFlag=false;
            // collisions against other tiles
            }else if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=257 && map.mapData[gridY][gridX]!=203 ){
                float penetration=fabs(((-TILE_SIZE*gridY)-(position.y-(size.y/2)*TILE_SIZE)));
                position.y+=penetration;
                velocity.y=0;
                botFlag=true;
                shotFlag=true;
            }else{
                botFlag=false;
            }
        }
    }
    
    void checkTopCollisionMap(){
        if (aliveFlag){
            int gridX = (int) (position.x / (TILE_SIZE));
            int gridY = (int)(position.y/(-TILE_SIZE)-size.y/2);
            if (entityType==BULLETS){
                if (gridX<=0 && gridX>=LEVEL_WIDTH){
                    aliveFlag=false;
                    shotFlag=true;
                }
                if (gridY<=0 && gridY>=LEVEL_HEIGHT){
                    aliveFlag=false;
                    shotFlag=true;
                }
            }
            if (map.mapData[gridY][gridX]==1 || map.mapData[gridY][gridX]==35){
                aliveFlag=false;
            }else if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=257 && map.mapData[gridY][gridX]!=203 ){
                float penetration=fabs(((-TILE_SIZE*gridY)-(position.y+(size.y/2)*TILE_SIZE)));
                // needed a little bit more to completely resolve penetration without stuttering camera movement
                penetration-=0.05;
                position.y-=penetration;
                velocity.y=0;
                topFlag=true;
                shotFlag=true;
            }else{
                topFlag=false;
            }
        }
    }
    
    void checkRightCollisionMap(){
        if (aliveFlag){
            int gridX = (int) (position.x / (TILE_SIZE) + size.y/2);
            int gridY = (int)(position.y/(-TILE_SIZE));
            if (entityType==BULLETS){
                if (gridX<=0 && gridX>=LEVEL_WIDTH){
                    aliveFlag=false;
                    shotFlag=true;
                }
                if (gridY<=0 && gridY>=LEVEL_HEIGHT){
                    aliveFlag=false;
                    shotFlag=true;
                }
            }
            if (map.mapData[gridY][gridX]==1 || map.mapData[gridY][gridX]==35){
                aliveFlag=false;
            }else if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=257 && map.mapData[gridY][gridX]!=203 ){
                float penetration=fabs(((TILE_SIZE*gridX)-position.x-(size.x/2)*TILE_SIZE));
                position.x-=penetration;
                rightFlag=true;
                velocity.x=0;
                shotFlag=true;
            }else{
                rightFlag=false;
            }
        }
    }
    
    void checkLeftCollisionMap(){
        if (aliveFlag){
            int gridX = (int) (position.x / (TILE_SIZE) - size.y/2);
            int gridY = (int)(position.y/(-TILE_SIZE));
            if (entityType==BULLETS){
                if (gridX<=0 && gridX>=LEVEL_WIDTH){
                    aliveFlag=false;
                    shotFlag=true;
                }
                if (gridY<=0 && gridY>=LEVEL_HEIGHT){
                    aliveFlag=false;
                    shotFlag=true;
                }
            }
            if (map.mapData[gridY][gridX]==1 || map.mapData[gridY][gridX]==35){
                aliveFlag=false;
            }else if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=257 && map.mapData[gridY][gridX]!=203 ){
                float penetration=fabs(((TILE_SIZE*gridX)-position.x+(size.x/2)*TILE_SIZE));
                penetration-=0.06f;
                position.x+=penetration;
                leftFlag=true;
                velocity.x=0;
                shotFlag=true;
            }else{
                leftFlag=false;
            }
        }
    }
    
    void checkVictory(){
        if (!entities[0].aliveFlag){
            modelMatrix = glm::mat4(1.0f);
            modelMatrix=glm::translate(modelMatrix,entities[0].position);
            modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
            program.SetModelMatrix(modelMatrix);
            DrawText(font, "Player 2 Wins", TILE_SIZE, 0.0005);
        }
        else if (!entities[1].aliveFlag){
            modelMatrix = glm::mat4(1.0f);
            modelMatrix=glm::translate(modelMatrix,entities[0].position);
            modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
            program.SetModelMatrix(modelMatrix);
            DrawText(font, "Player 1 Wins", TILE_SIZE, 0.0005);
        }
    }

};

void convertFlareEntity(){
    // converts flare map entity to in-game entity.
    int count=0;
    for (FlareMapEntity& someEntity:map.entities){
        float xPos=someEntity.x*TILE_SIZE;
        float yPos=someEntity.y*-TILE_SIZE;
        Entity newEntity;
        newEntity.position=glm::vec3(xPos,yPos,0.0f);
        newEntity.modelAnimation=&character1;
        if (count==0){
            newEntity.entityType=PLAYER1;
        }else{
            newEntity.entityType=PLAYER2;
        }
        count++;
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
//    //writes move left
//    modelMatrix = glm::mat4(1.0f);
//    modelMatrix=glm::translate(modelMatrix,combat[3].position);
//    modelMatrix=glm::translate(modelMatrix, glm::vec3(-1.75f,0.95f,0.0f));
//    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
//    program.SetModelMatrix(modelMatrix);
//    DrawText(font, "Lives", TILE_SIZE, 0.0005);
//
//    modelMatrix = glm::mat4(1.0f);
//    modelMatrix=glm::translate(modelMatrix,combat[3].position);
//    modelMatrix=glm::translate(modelMatrix, glm::vec3(1.55f,0.95f,0.0f));
//    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.75f,1.0f,1.0f));
//    program.SetModelMatrix(modelMatrix);
//    DrawText(font, "Lives", TILE_SIZE, 0.0005);
    
}

void intialSpawn(){
    //generate 4 player
    srand(time(0));
    vector <int> positions;
    for (int i=0;i<2;i++){
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
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl",RESOURCE_FOLDER"fragment_textured.glsl");
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

    SheetSprite* model_1_1 = new SheetSprite(characters,0.0f/32.0f, 16.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_1);
    SheetSprite* model_1_2 = new SheetSprite(characters,10.0f/32.0f, 0.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_2);
    SheetSprite* model_1_3 = new SheetSprite(characters,10.0f/32.0f, 16.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_3);
    SheetSprite* model_1_4 = new SheetSprite(characters,0.0f/32.0f, 0.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_4);
    
    
    //machine gun
    SheetSprite* machineGunSprite= new SheetSprite(weapons,0.0f/128.0f, 0.0f/64.0f, 58.0f/128.0f, 26.0f/64.0f, 1.0f);
    Entity machineGun;
    machineGun.sprite=*machineGunSprite;
    machineGun.position=glm::vec3(100);
    machineGun.friction=glm::vec3(0);
    machineGun.entityType=ART;
    combat.push_back(machineGun);
    
    //bullet
    SheetSprite* bulletSprite= new SheetSprite(weapons,0.0f/128.0f, 26.0f/64.0f, 13.0f/128.0f, 38.0f/64.0f, 1.0f);
    Entity bullet;
    bullet.sprite=*bulletSprite;
    bullet.position=glm::vec3(100);
    bullet.friction=glm::vec3(0);
    bullet.size=glm::vec3(0.5);
    bullet.entityType=BULLETS;
    bullet.aliveFlag=false;
    combat.push_back(bullet);
    combat.push_back(bullet);
    
    //camera entity
    Entity camera;
    camera.position.x=28*TILE_SIZE;
    camera.position.y=20*-TILE_SIZE;
    camera.entityType=ART;
    combat.push_back(camera);
}

void cameraMovement(){
    viewMatrix = glm::mat4(1.0f);
    viewMatrix=glm::translate(viewMatrix,-combat[3].position);
    viewMatrix=glm::translate(viewMatrix,glm::vec3(0.0,-0.35,0.0));
    viewMatrix=glm::scale(viewMatrix,glm::vec3(1.0f,0.85,1.0f));
    program.SetViewMatrix(viewMatrix);
}

void gameSetUp(){
    renderMap();
    intialSpawn();
    setUpCount++;
}

void renderGameLevel(){
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(29/255.0, 41/255.0, 81/255.0, 1.0f);
    if (setUpCount==0){
        gameSetUp();
    }
    renderMap();
    cameraMovement();
    renderUI();
    for (Entity& someEntity:entities){
        someEntity.render();
    }
}

void renderGameMenu(){
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0, 0, 0, 1.0f);
    
    float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
    glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program1.positionAttribute);
    
    program1.SetColor(0.824f,0.839f,0.851f, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::scale(modelMatrix,glm::vec3(1.0f,0.25f,1.0f));
    program1.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.6f,0.5f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "One-Hit K.O", 0.25f,0.0005f);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.25f,0.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "Start", 0.25f,0.0005f);
    
    
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    glDisableVertexAttribArray(program1.positionAttribute);
}

void renderMapSelect(){
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0, 0, 0, 1.0f);
    
    float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
    glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program1.positionAttribute);
    
    program1.SetColor(0.824f,0.839f,0.851f, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::scale(modelMatrix,glm::vec3(1.0f,0.25f,1.0f));
    program1.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.80f,0.5f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "Select Your Map", 0.25f,0.0005f);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.25f,0.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "Start", 0.25f,0.0005f);
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    glDisableVertexAttribArray(program1.positionAttribute);
}


void Render(){
    switch(mode){
        case MAIN_MENU:
            renderGameMenu();
            break;
        case MAP_SELECT:
            renderMapSelect();
            break;
        case GAME_LEVEL:
            renderGameLevel();
            break;
        case PAUSE:
            //renderPause();
            break;
        case GAME_OVER:
            //renderGameOver();
            break;
    }
}

void update(float elapsed){
    for (Entity& someEntity:entities){
        someEntity.update(elapsed);
    }
    for (Entity& someEntity:combat){
        someEntity.update(elapsed);
    }
}


int main(int argc, char *argv[])
{
    
    setUp();
    //renderGameLevel();

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
            //cout<<mouseX<<"-"<<mouseY<<endl;
            if (event.type==SDL_MOUSEBUTTONDOWN && mode==MAIN_MENU) {
                if(mouseX>START_BOX_Left && mouseX<START_BOX_RIGHT && mouseY>START_BOX_BOTTOM && mouseY<START_BOX_TOP){
                    mode=MAP_SELECT;
                }
            }
            else if (event.type==SDL_MOUSEBUTTONDOWN && mode==MAP_SELECT) {
                if(mouseX>START_BOX_Left && mouseX<START_BOX_RIGHT && mouseY>START_BOX_BOTTOM && mouseY<START_BOX_TOP){
                    mode=GAME_LEVEL;
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
            if (mode==GAME_LEVEL && setUpCount!=0){
                update(FIXED_TIMESTEP);
            }
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        Render();
        SDL_GL_SwapWindow(displayWindow);
    }
    
    


    SDL_Quit();
    return 0;
}
