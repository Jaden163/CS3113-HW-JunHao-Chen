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
#define SPRITE_CHARACTER_COUNT_X 16
#define SPRITE_CHARACTER_COUNT_Y 8
#define FIXED_TIMESTEP 0.0166666f
#define TILE_SIZE 1/16.0f
#define CAMERA_X_ADJUST 1.3f
#define CAMERA_Y_ADJUST 0.45f
#define MAX_RIGHT_CAMERA 4.92f
#define MAX_LEFT_CAMERA 0.479

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
// initialize matrix
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);
// initialze map
FlareMap map;





class Entity;
std::vector <Entity> entities;



float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}


enum EntityType {ENEMY,PLAYER};
class Entity{
public:
    
    void render(){
        float u = (float)(((int)spriteIndex) % SPRITE_CHARACTER_COUNT_X) / (float) SPRITE_CHARACTER_COUNT_X;
        float v = (float)(((int)spriteIndex) / SPRITE_CHARACTER_COUNT_X) / (float) SPRITE_CHARACTER_COUNT_Y;
        float spriteWidth = 1.0/(float)SPRITE_CHARACTER_COUNT_X;
        float spriteHeight = 1.0/(float)SPRITE_CHARACTER_COUNT_Y;
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
        
        glBindTexture(GL_TEXTURE_2D, characters);
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        modelMatrix=glm::mat4(1.0f);
        modelMatrix=glm::translate(modelMatrix,position);
        
        // render the character model to face the right direction
        if (faceLeftFlag){
            modelMatrix=glm::scale(modelMatrix,glm::vec3(-TILE_SIZE,TILE_SIZE,1.0f));
        }else{
            modelMatrix=glm::scale(modelMatrix,glm::vec3(TILE_SIZE,TILE_SIZE,1.0f));
        }
        program.SetModelMatrix(modelMatrix);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
    }

    void update(float elapsed){
        
        // gravity affecting all entities
        velocity.y += gravity.y * elapsed;
        
        if(entityType==PLAYER){
        
            // check collision against other entioties
            for (int i=0;i<9;i++){
                if(checkEntityCollision(entities[i])){
                    if ((position.y - entities[i].position.y) > size.y/2*TILE_SIZE){
                        entities[i].position.x=100.0f;
                    }else{
                        restart();
                    }
                }
            }
            
            // friction
            velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
            velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);
            
            if (keys[SDL_SCANCODE_UP] && botFlag){
                //only jump if botFlag is set
                velocity.y+=30.0f*elapsed;
            }
            if (keys[SDL_SCANCODE_RIGHT]){
                velocity.x+=0.5f*elapsed;
                faceLeftFlag=false;
            }
            if (keys[SDL_SCANCODE_LEFT]){
                velocity.x-=0.5f*elapsed;
                faceLeftFlag=true;
            }
            
            position.x+=velocity.x*elapsed;
        }
        
        
        else if(entityType==ENEMY){
            // enemies have a timer based movement system. Didn't have time to code a system based on collision
            cout<<timer<<endl;
            if (timer<=0){
                Enemyvelocity.x=-Enemyvelocity.x;
                timer=1.5f;
                faceLeftFlag=!faceLeftFlag;
            }
            position.x+=Enemyvelocity.x*elapsed;
            timer-=elapsed;
        }
        
        position.y+=velocity.y*elapsed;
        checkAllCollisionMap();
        
    }
    
    void restart(){
        // restart the game by resetting everyone's position to starting position
        for (Entity& someEntity:entities){
            someEntity.position=someEntity.startingPos;
        }
    }
    
    void checkAllCollisionMap(){
        // checks all collisiona against "solid" tile maps
        int gridY = (int)(position.y/(-TILE_SIZE)+size.y/2);
        // detect if you fall  into "death tiles" which is any tile greater than 18

        if (gridY>18 && entityType==PLAYER){
            position=startingPos;
            restart();
        }
        checkBotCollisionMap();
        checkTopCollisionMap();
        checkRightCollisionMap();
        checkLeftCollisionMap();
    }
    
    bool checkEntityCollision( const Entity& someEntity){
        float yCollide=abs(position.y-someEntity.position.y)-((size.y/2+someEntity.size.y/2)*TILE_SIZE);
        float xCollide=abs(position.x-someEntity.position.x)-((size.x/2+someEntity.size.x/2)*TILE_SIZE);
        return (xCollide<=0.0f && yCollide<=0.0f);
    }
    
    EntityType entityType;
    
    glm::vec3 position;
    glm::vec3 startingPos;
    glm::vec3 tilePos;
    glm::vec3 size=glm::vec3(1.0f);
    glm::vec3 velocity=glm::vec3(0.0f,0.0f,0.0f);
    // wanted another velocity for enemies only so player dont have starting velocity as well
    glm::vec3 Enemyvelocity=glm::vec3(0.1f,0.0f,0.0f);
    glm::vec3 gravity=glm::vec3(0.0f,-0.5f,0.0f);
    glm::vec3 friction=glm::vec3(1.0f,0.0f,0.0f);
    
    float timer=1.5f;
    bool topFlag=false;
    bool botFlag=false;
    bool leftFlag=false;
    bool rightFlag=false;
    bool faceLeftFlag=false;
    
    int spriteIndex;
    
private:
    
    void checkBotCollisionMap(){
        int gridX = (int) (position.x / (TILE_SIZE));
        int gridY = (int)(position.y/(-TILE_SIZE)+size.y/2);
        if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=130 && map.mapData[gridY][gridX]!=204 ){
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
        if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=130 && map.mapData[gridY][gridX]!=204 ){
            float penetration=fabs(((-TILE_SIZE*gridY)-(position.y+(size.y/2)*TILE_SIZE)));
            // needed a little bit more to completely adjust penetration without stuttering camera movement
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
        if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=130 && map.mapData[gridY][gridX]!=204 ){
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
        if (map.mapData[gridY][gridX]!=704 && map.mapData[gridY][gridX]!=130 && map.mapData[gridY][gridX]!=204 ){
            float penetration=fabs(((TILE_SIZE*gridX)-position.x+(size.x/2)*TILE_SIZE));
            penetration-=0.06f;
            position.x+=penetration;
            leftFlag=true;
            velocity.x=0;
        }else{
            leftFlag=false;
        }
    }
    

    
};


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

void convertFlareEntity(){
    // converts flare map entity to in-game entity.
    for (FlareMapEntity& someEntity:map.entities){
        float xPos=someEntity.x*TILE_SIZE;
        float yPos=someEntity.y*-TILE_SIZE;
        Entity newEntity;
        newEntity.position=glm::vec3(xPos,yPos,0.0f);
        if (someEntity.type=="ENEMY"){
            newEntity.entityType=ENEMY;
            newEntity.spriteIndex=80;
        }else if(someEntity.type=="PLAYER"){
            newEntity.entityType=PLAYER;
            newEntity.spriteIndex=99;
        }
        newEntity.startingPos=newEntity.position;
        entities.push_back(newEntity);
    }
    
};

void setUp(){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Mario Clone", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 576, SDL_WINDOW_OPENGL);
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
    
    //creates the 3 matrixes
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
    map.Load(RESOURCE_FOLDER"marioClone.txt");
    mineCraft= LoadTexture(RESOURCE_FOLDER"minecraft_Sprite.png");
    characters=LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    convertFlareEntity();
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


void renderGameLevel(){
    renderMap();
    for (Entity& someEntity:entities){
        someEntity.render();
    }
}

void update(float elapsed){
    for (Entity& someEntity:entities){
        someEntity.update(elapsed);
    }
}

void cameraMovement(){
    viewMatrix = glm::mat4(1.0f);
    // wanted the camera to stop moving if it reaches a certain x position but still allow character to still move
    // the CAMERA_ADJUST values are to make the camera frame better so it doesn't show too much empty space on bottom
    if (entities[9].position.x > MAX_LEFT_CAMERA && entities[9].position.x<MAX_RIGHT_CAMERA){
        viewMatrix=glm::translate(viewMatrix,-entities[9].position);
        viewMatrix=glm::translate(viewMatrix,glm::vec3(-CAMERA_X_ADJUST,-CAMERA_Y_ADJUST,0.0f));
    }else if (entities[9].position.x <MAX_LEFT_CAMERA){
        viewMatrix=glm::translate(viewMatrix,glm::vec3(-MAX_LEFT_CAMERA,-entities[9].position.y,0.0f));
        viewMatrix=glm::translate(viewMatrix,glm::vec3((-CAMERA_X_ADJUST),(-CAMERA_Y_ADJUST),0.0f));
    }else if (entities[9].position.x >MAX_RIGHT_CAMERA){
        viewMatrix=glm::translate(viewMatrix,glm::vec3(-MAX_RIGHT_CAMERA,-entities[9].position.y,0.0f));
        viewMatrix=glm::translate(viewMatrix,glm::vec3(-CAMERA_X_ADJUST,-CAMERA_Y_ADJUST,0.0f));
    }
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
        glClearColor(144/255.0, 221/255.0, 223/255.0, 1.0f);
        
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
