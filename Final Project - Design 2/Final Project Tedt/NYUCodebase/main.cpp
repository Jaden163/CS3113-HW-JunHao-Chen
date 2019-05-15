#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "glm/mat4x2.hpp"
#include <SDL_mixer.h>
#include "glm/gtc/matrix_transform.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "FlareMap.h"
#include <random>


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

//UI interface data
#define START_BOX_Left 0.26655f
#define START_BOX_TOP -0.422222f
#define START_BOX_BOTTOM -0.8f
#define START_BOX_RIGHT 1.8603
#define TILED_BOX_LEFT1 0.26655f
#define TILED_BOX_RIGHT1 1.8603
#define TILED_BOX_BOT1 -0.738889
#define TILED_BOX_TOP1 -0.333333f
#define TILED_BOX_LEFT2 0.26655f
#define TILED_BOX_RIGHT2 1.8603f
#define TILED_BOX_BOT2  -1.2f
#define TILED_BOX_TOP2 -0.8111111f
#define MAP1_LEFT -0.294316
#define MAP1_RIGHT 0.505334
#define MAP1_TOP -0.2f
#define MAP1_BOT -1.01111




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
GLuint character2;
GLuint mineCraft;
GLuint font;
GLuint weapons;
GLuint map1;
GLuint map2;
GLuint map3;
// initialize matrix
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);
// initialze map
FlareMap map;
//initialize mouse positions
float mouseX;
float mouseY;
//misc
int setUpCount=0;
int winner;
int mapSelect=0;
float startTimer=4.0;
//audio stuff
int audio=Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
Mix_Music* menuMusic;
Mix_Music* gameMusic;
Mix_Chunk* shootBullet;


class SheetSprite;
class Entity;
std::vector <Entity> entities;
std::vector <Entity> combat;
//initialize sprite index from xml for character animation
vector<SheetSprite*> character1;
vector<SheetSprite*> character2Vec;



enum GameMode {MAIN_MENU,MAP_SELECT,GAME_LEVEL,GAME_OVER};
GameMode mode=MAIN_MENU;



float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

//particle stuff
class Particle {
public:
    Particle() {};
    glm::vec3 position;
    glm::vec3 velocity;
    float lifetime;
    float sizeDeviation;
    float rotation;
};

class ParticleEmitter {
public:
    ParticleEmitter(unsigned int particleCount) {};
    ParticleEmitter() {};
    ~ParticleEmitter() {};
    
    void Update(float elapsed);
    void Render();
    
    glm::vec3 position;
    glm::vec3 velocity=glm::vec3(0.0f, 2.0f, 0.0f);
    glm::vec3 velocityDeviation=glm::vec3(1.0f,0.0f,0.0f);
    glm::vec3 gravity=glm::vec3(0.0f, -4.0f, 0.0f);
    
    float startSize=0.01;
    float endSize=0.05;
    float sizeDeviation=0.01;
    
    float maxLifetime=1;
    std::vector<Particle> particles;
};

void ParticleEmitter::Render() {
    std::vector<float> vertices;
    std::vector<float> particleColors;
    for(int i=0; i < particles.size(); i++) {
        float m = (particles[i].lifetime/maxLifetime);
        float size = lerp(startSize, endSize, m) + particles[i].sizeDeviation;
        float cosTheta = cosf(particles[i].rotation);
        float sinTheta = sinf(particles[i].rotation);
        float TL_x = cosTheta * -size - sinTheta * size;
        float TL_y = sinTheta * -size + cosTheta * size;
        float BL_x = cosTheta * -size - sinTheta * -size;
        float BL_y = sinTheta * -size + cosTheta * -size;
        float BR_x = cosTheta * size - sinTheta * -size;
        float BR_y = sinTheta * size + cosTheta * -size;
        float TR_x = cosTheta * size - sinTheta * size;
        float TR_y = sinTheta * size + cosTheta * size;
        vertices.insert(vertices.end(), {
            particles[i].position.x + TL_x, particles[i].position.y + TL_y,
            particles[i].position.x + BL_x, particles[i].position.y + BL_y,
            particles[i].position.x + TR_x, particles[i].position.y + TR_y,
            particles[i].position.x + TR_x, particles[i].position.y + TR_y,
            particles[i].position.x + BL_x, particles[i].position.y + BL_y,
            particles[i].position.x + BR_x, particles[i].position.y + BR_y
        });
        particles[i].rotation += 0.1;
    }
    glUseProgram(program1.programID);
    glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program1.positionAttribute);
    modelMatrix = glm::mat4(1.0f);
    program1.SetModelMatrix(modelMatrix);
    program1.SetColor(1.0f,0.8f,0.80f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size()/2);
    glUseProgram(program.programID);
}

void ParticleEmitter::Update(float elapsed) {
    srand(time(0));
    //random lifetime,velocity,size
    float random=0;
    for (Particle &particle : particles) {
        random=rand()%100;
        if (int(random)%2==0){
            random=-random;
        }
        random=random/100;
        particle.velocity.x += gravity.x * elapsed;
        particle.velocity.y += gravity.y * elapsed;
        particle.position.x += particle.velocity.x * elapsed;
        particle.position.y += particle.velocity.y * elapsed;
        if (particle.lifetime > maxLifetime) {
            particle.position = position;
            particle.lifetime -= maxLifetime;
            particle.velocity.x = velocity.x + random;
            particle.velocity.y = velocity.y;
        }
        particle.lifetime += elapsed;
    }
}

//initialize a particle emitter
ParticleEmitter particleSources[1];

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
            
            //draw gun attached to character model
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
        
        // draw bullets
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
            
                if (entityType==PLAYER2 && startTimer<1.0f){
        
                    if (keys[SDL_SCANCODE_I] && botFlag){
                        //only jump if botFlag is set
                        velocity.y+=30.0f*elapsed;
                    }
                    if (keys[SDL_SCANCODE_L]){
                        velocity.x+=0.5f*elapsed;
                        faceLeftFlag=false;
                    }
                    
                    if (keys[SDL_SCANCODE_J]){
                        velocity.x-=0.5f*elapsed;
                        faceLeftFlag=true;
                    }
                    if (keys[SDL_SCANCODE_RETURN] && combat[2].shotFlag){
                        Mix_VolumeChunk(shootBullet, 10);
                        Mix_PlayChannel( 1, shootBullet, 0);
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
                }else if (entityType==PLAYER1 && startTimer<1.0f){
                    if (keys[SDL_SCANCODE_W] && botFlag){
                        //only jump if botFlag is set
                        velocity.y+=30.0f*elapsed;
                    }
                    if (keys[SDL_SCANCODE_D]){
                        velocity.x+=0.5f*elapsed;
                        faceLeftFlag=false;
                    }
                    
                    if (keys[SDL_SCANCODE_A]){
                        velocity.x-=0.5f*elapsed;
                        faceLeftFlag=true;
                    }
                    if (keys[SDL_SCANCODE_SPACE] && combat[1].shotFlag){
                        Mix_VolumeChunk(shootBullet, 10);
                        Mix_PlayChannel( 1, shootBullet, 0);
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
            
        }
        
        if(entityType==BULLETS && aliveFlag){
            //check bullet against characters
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
                position.x-=1.5*elapsed;
                
            }else{
                position.x+=1.5*elapsed;
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
    bool aliveFlag=true;

    

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
            mode=GAME_OVER;
            winner=1;
        }
        else if (!entities[1].aliveFlag){
            mode=GAME_OVER;
            winner=0;
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
        if (count==0){
            newEntity.modelAnimation=&character1;
            newEntity.entityType=PLAYER1;
        }else{
            newEntity.entityType=PLAYER2;
            newEntity.modelAnimation=&character2Vec;

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


void intialSpawn(){
    //generate 2 player
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
    displayWindow = SDL_CreateWindow("One-Hit", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 576, SDL_WINDOW_OPENGL);
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
    
    mineCraft= LoadTexture(RESOURCE_FOLDER"minecraft_Sprite.png");
    characters=LoadTexture(RESOURCE_FOLDER"sprites.png");
    font=LoadTexture(RESOURCE_FOLDER"font2.png");
    weapons=LoadTexture(RESOURCE_FOLDER"combat.png");
    map1=LoadTexture(RESOURCE_FOLDER"map1.png");
    map2=LoadTexture(RESOURCE_FOLDER"map2.png");
    map3=LoadTexture(RESOURCE_FOLDER"map3.png");
    menuMusic=Mix_LoadMUS("menuMusic.mp3");
    gameMusic=Mix_LoadMUS("gameMusic.mp3");
    shootBullet=Mix_LoadWAV("awpSound.wav");
    character2=LoadTexture(RESOURCE_FOLDER"character2.png");
    
    
    
    SheetSprite* model_1_1 = new SheetSprite(characters,0.0f/32.0f, 16.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_1);
    SheetSprite* model_1_2 = new SheetSprite(characters,10.0f/32.0f, 0.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_2);
    SheetSprite* model_1_3 = new SheetSprite(characters,10.0f/32.0f, 16.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_3);
    SheetSprite* model_1_4 = new SheetSprite(characters,0.0f/32.0f, 0.0f/32.0f, 10.0f/32.0f, 16.0f/32.0f, 1.0f);
    character1.push_back(model_1_4);
    
    SheetSprite* model_2_1 = new SheetSprite(character2,12.0f/32.0f, 16.0f/32.0f, 11.0f/32.0f, 16.0f/32.0f, 1.0f);
    character2Vec.push_back(model_2_1);
    SheetSprite* model_2_2 = new SheetSprite(character2,0.0f/32.0f, 0.0f/32.0f, 12.0f/32.0f, 16.0f/32.0f, 1.0f);
    character2Vec.push_back(model_2_2);
    SheetSprite* model_2_3 = new SheetSprite(character2,12.0f/32.0f, 0.0f/32.0f, 11.0f/32.0f, 16.0f/32.0f, 1.0f);
    character2Vec.push_back(model_2_3);
    SheetSprite* model_2_4 = new SheetSprite(character2,0.0f/32.0f, 16.0f/32.0f, 12.0f/32.0f, 16.0f/32.0f, 1.0f);
    character2Vec.push_back(model_2_4);
    
    
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
    
    srand(time(0));
    float random;
    for (int i = 0; i < 50; i++) {
        random=rand()%100;
        Particle p;
        p.lifetime = lerp(0.0, particleSources[0].maxLifetime,random/100.0);
        p.sizeDeviation = lerp(-particleSources[0].sizeDeviation, particleSources[0].sizeDeviation,random/100.0);
        particleSources[0].particles.push_back(p);
    }
    
    //camera entity
    Entity camera;
    camera.position.x=28*TILE_SIZE;
    camera.position.y=20*-TILE_SIZE;
    camera.entityType=ART;
    combat.push_back(camera);
}

void cameraMovement(){
    if (mapSelect==1){
        viewMatrix = glm::mat4(1.0f);
        viewMatrix=glm::translate(viewMatrix,-combat[3].position);
        viewMatrix=glm::translate(viewMatrix,glm::vec3(0.0,-0.35,0.0));
        viewMatrix=glm::scale(viewMatrix,glm::vec3(1.0f,0.85,1.0f));
        program.SetViewMatrix(viewMatrix);
    }else if(mapSelect!=1){
        viewMatrix = glm::mat4(1.0f);
        viewMatrix=glm::translate(viewMatrix,-combat[3].position);
        viewMatrix=glm::translate(viewMatrix,glm::vec3(0.0,-0.15,0.0));
        viewMatrix=glm::scale(viewMatrix,glm::vec3(1.0f,0.85,1.0f));
        program.SetViewMatrix(viewMatrix);
    }
}

void gameSetUp(){
    if (mapSelect==1){
        map.Load(RESOURCE_FOLDER"level1.txt");
    }else if (mapSelect==2){
        map.Load(RESOURCE_FOLDER"level2.txt");
    }else if (mapSelect==3){
        map.Load(RESOURCE_FOLDER"level3.txt");
    }
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
    for (Entity& someEntity:entities){
        someEntity.render();
    }

    if (startTimer>1.0f){
        string timeString=std::to_string(int(startTimer));
        modelMatrix = glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,combat[3].position);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(0.0,1.0f,1.0f));
        program.SetModelMatrix(modelMatrix);
        DrawText(font, timeString, 0.25, 0.0005);
    }
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    glDisableVertexAttribArray(program1.positionAttribute);
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
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.4f,0.5f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "One-Hit", 0.25f,0.0005f);
    
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
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, map1);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.60f,0.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindTexture(GL_TEXTURE_2D, map2);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.0f,0.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindTexture(GL_TEXTURE_2D, map3);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(0.60f,0.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.80f,0.5f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(0.5f,0.5f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font, "Select Your Map", 0.25f,0.0005f);
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    glDisableVertexAttribArray(program1.positionAttribute);
}

void renderPause(){

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(29/255.0, 41/255.0, 81/255.0, 1.0f);
    renderMap();
    cameraMovement();
    for (Entity& someEntity:entities){
        someEntity.render();
    }
    
    float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
    glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program1.positionAttribute);
    
    program1.SetColor(0.8,0.95,0.58, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,combat[3].position);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-1.75,1.3f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(1.0f,0.25f,1.0f));
    program1.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    program1.SetColor(1.0,0.44,0.38, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,combat[3].position);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-1.75,1.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(1.0f,0.25f,1.0f));
    program1.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,combat[3].position);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.35,0.25f,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font,"Restart", 0.075, 0.0005);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,combat[3].position);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.1,-0.10,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font,"Quit", 0.075, 0.0005);
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    glDisableVertexAttribArray(program1.positionAttribute);
}

void renderGameOver(){
    float offset=0;
    if (mapSelect!=1){
        offset=0.25f;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(29/255.0, 41/255.0, 81/255.0, 1.0f);
    
    renderMap();
    
    cameraMovement();
    for (Entity& someEntity:entities){
        someEntity.render();
    }
    
    float vertices[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
    glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program1.positionAttribute);
    
    program1.SetColor(0.8,0.95,0.58, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,combat[3].position);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-1.75,1.3f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(1.0f,0.25f,1.0f));
    program1.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    program1.SetColor(1.0,0.44,0.38, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,combat[3].position);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-1.75,1.0f,1.0f));
    modelMatrix=glm::scale(modelMatrix,glm::vec3(1.0f,0.25f,1.0f));
    program1.SetModelMatrix(modelMatrix);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,combat[3].position);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.35,0.25f-offset,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font,"Play Again", 0.075, 0.0005);
    
    modelMatrix = glm::mat4(1.0f);
    modelMatrix=glm::translate(modelMatrix,combat[3].position);
    modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.1,-0.10-offset,1.0f));
    program.SetModelMatrix(modelMatrix);
    DrawText(font,"Quit", 0.075, 0.0005);
    
    if (winner==1){
        modelMatrix = glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,combat[3].position);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.5f,1.0f,1.0f));
        program.SetModelMatrix(modelMatrix);
        DrawText(font,"Player 2 Wins!", 0.075, 0.0005);
    }else{
        modelMatrix = glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,combat[3].position);
        modelMatrix=glm::translate(modelMatrix,glm::vec3(-0.5f,1.2f,1.0f));
        program.SetModelMatrix(modelMatrix);
        DrawText(font,"Player 1 Wins!", 0.075, 0.0005);
    }
    
    particleSources[0].position = combat[3].position;
    particleSources[0].position.x-=1.76f;
    particleSources[0].position.y+=1.4f;
    particleSources[0].Render();
    
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
        case GAME_OVER:
            renderGameOver();
            break;
    }
}

void update(float elapsed){
    if (startTimer>1.0f){
        startTimer-=elapsed;
    }
    for (Entity& someEntity:entities){
        someEntity.update(elapsed);
    }
    particleSources[0].position = combat[3].position;
    particleSources[0].position.x-=2.0f;
    particleSources[0].Update(elapsed);
    
    for (Entity& someEntity:combat){
        someEntity.update(elapsed);
    }
}

void reset(){
    startTimer=4.0f;
    setUpCount=0;
    viewMatrix = glm::mat4(1.0f);
    program.SetViewMatrix(viewMatrix);
    program1.SetViewMatrix(viewMatrix);
    character1.clear();
    entities.clear();
    map.entities.clear();
    combat[1].position=glm::vec3(100);
    combat[2].position=glm::vec3(100);
    combat[1].aliveFlag=false;
    combat[2].aliveFlag=false;
    combat[1].shotFlag=true;
    combat[2].shotFlag=true;
}


int main(int argc, char *argv[])
{
    setUp();
    Mix_PlayMusic(menuMusic, -1);


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
                if(mouseX>MAP1_LEFT && mouseX<MAP1_RIGHT && mouseY>MAP1_BOT && mouseY<MAP1_TOP){
                    mapSelect=1;
                    mode=GAME_LEVEL;
                }else if(mouseX>MAP1_LEFT+0.95f && mouseX<MAP1_RIGHT+0.95f && mouseY>MAP1_BOT && mouseY<MAP1_TOP){
                    mapSelect=2;
                    mode=GAME_LEVEL;
                }
                else if(mouseX>MAP1_LEFT+1.9f && mouseX<MAP1_RIGHT+1.9f && mouseY>MAP1_BOT && mouseY<MAP1_TOP){
                    mapSelect=3;
                    mode=GAME_LEVEL;
                }
                Mix_HaltMusic();
                Mix_PlayMusic(gameMusic, -1);
                Mix_VolumeMusic(70);
            }else if (event.type==SDL_MOUSEBUTTONDOWN && mode==GAME_OVER) {
                Mix_HaltMusic();
                Mix_PlayMusic(menuMusic, -1);
                Mix_VolumeMusic(90);
                if(mouseX>TILED_BOX_LEFT1 && mouseX<TILED_BOX_RIGHT1 && mouseY>TILED_BOX_BOT1 && mouseY<TILED_BOX_TOP1){
                    reset();
                    mode=MAP_SELECT;
                }else if(mouseX>TILED_BOX_LEFT2 && mouseX<TILED_BOX_RIGHT2 && mouseY>TILED_BOX_BOT2 && mouseY<TILED_BOX_TOP2){
                    Mix_FreeMusic(menuMusic);
                    Mix_FreeMusic(gameMusic);
                    Mix_FreeChunk(shootBullet);
                    SDL_Quit();
                    return 0;
                }
            }else if (event.type==SDL_MOUSEBUTTONDOWN && mode==GAME_OVER) {
                
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
            continue;
        }
        
        while(elapsed >= FIXED_TIMESTEP) {
            if ((mode==GAME_LEVEL) && setUpCount!=0){
                update(FIXED_TIMESTEP);
            }else if(mode==GAME_OVER){
                particleSources[0].Update(FIXED_TIMESTEP);
            }
            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        Render();
        SDL_GL_SwapWindow(displayWindow);
    }
    
    Mix_FreeMusic(menuMusic);
    Mix_FreeMusic(gameMusic);
    Mix_FreeChunk(shootBullet);
    SDL_Quit();
    return 0;
}
