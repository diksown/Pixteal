#include <iostream>
#include <string>
#include "../include/engine/engine.h"
#include "../include/engine/texture.h"
#include "../include/engine/font.h"
#include "../include/engine/text.h"
#include "../include/sprite.h"
#include "../include/player.h"
#include "../include/world.h"
#include "../include/enemy.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 568
#define MAX_ENEMIES 5

// Engine
Engine *engine;

// Variáveis do jogo
World *world;
Player *player;
Enemy *enemy[MAX_ENEMIES];
pthread_t enemy_thread[MAX_ENEMIES];

// Texturas
Texture *texture_1px;

// Fonte e texto
Font *font;
Text *text;


// Thread principal
void render(){
    // Renderizando terreno
    world->render();

    // Renderizando inimigos
    for(int i = 0;i < MAX_ENEMIES;i++){
        enemy[i]->render();
    }

    // Renderizando jogador
    player->render();

    // Renderizando texto dos personagens
    for(int i = 0;i < MAX_ENEMIES;i++){
        enemy[i]->renderName();
    }
    player->renderName();

    // Renderizando interface
    texture_1px->setColor(34, 32, 52);
    texture_1px->setWidth(WINDOW_WIDTH);
    texture_1px->setHeight(88);
    texture_1px->render();

    // Renderizando pontuação do jogador
    text->setColor(251, 242, 54);
    text->setText("Thread 1: "+std::to_string(player->getPoints()));
    text->setPos(10, 10);
    text->render();

    text->setColor(255, 255, 255);
    // Renderizando pontuação dos inimigos
    for(int i = 0;i < MAX_ENEMIES;i++){
        text->setText("Thread "+std::to_string(i+2)+": "+std::to_string(enemy[i]->getPoints()));
        text->setPos(12 + (i+1)/3*128, 10 + 27*((i+1)%3));
        text->render();
    }
}

// Thread principal
void update(){

    // Tratando eventos
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0){
        // Comando para fechar o programa
        if (e.type == SDL_QUIT) {
            engine->stop();
            return;
        }

        // Tecla para baixo ou para baixo
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP){
            SDL_Keycode code = e.key.keysym.sym;

            if (code == SDLK_UP || code == SDLK_w) player->setMovingUp(e.type == SDL_KEYDOWN);
            if (code == SDLK_RIGHT || code == SDLK_d) player->setMovingRight(e.type == SDL_KEYDOWN);
            if (code == SDLK_DOWN || code == SDLK_s) player->setMovingDown(e.type == SDL_KEYDOWN);
            if (code == SDLK_LEFT || code == SDLK_a) player->setMovingLeft(e.type == SDL_KEYDOWN);
        }
    }

    // Atualizando Jogador
    player->update();

    // Atualizando mapa
    world->update();
}

// Popula o 'world' com o jogador principal e os inimígos
void populate(){
    SDL_Point p;

    // Criando jogador principal
    p = world->randomPoint();
    player = new Player(world, text, rand(), p.x, p.y);
    world->setOccupied(p.x, p.y, true);

    // Criando inimigos
    for(int i = 0;i < MAX_ENEMIES;i++){
        p = world->randomPoint();
        enemy[i] = new Enemy(world, text, "Thread "+std::to_string(i+2), rand(), p.x, p.y);
        world->setOccupied(p.x, p.y, true);
    }
}

// Carrega todos os recursos necessários
void load(){
    // Carregando textura dos personagens
    Sprite::loadTexture(new Texture(engine, "gfx/sprites.png"));

    // Carregando fonte e objeto para renderização de texto
    font = new Font("gfx/font.ttf", 16);
    text = new Text(engine, font, " ");

    // Carregando objetos do jogo
    world = new World(engine, "gfx/ground.png", "gfx/ores.png");
    populate();

    // Carregando texturas
    texture_1px = new Texture(engine, "gfx/1px.png");
}

void unload(){
    delete world;
    delete player;
    for(int i = 0;i < MAX_ENEMIES;i++) delete enemy[i];
    delete font;
    delete text;
    delete texture_1px;
    Sprite::unloadTexture();
    delete engine;
}

// Thread destinada ao processamento dos personagens inimigos
void *enemyThread(void *arg){
    Enemy *enemy = (Enemy*)arg;

    while(engine->isRunning()){
        unsigned int t0 = SDL_GetTicks();

        // Atualizando inimigo
        enemy->update();

        // Ajustando FPS conforme o limite especificado
        Engine::sleep(t0);
    }

    pthread_exit(NULL);
}

void createEnemiesThreads(){
    // Criando atributo para definir as threads como JOINABLE
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(int i = 0;i < MAX_ENEMIES;i++){
        pthread_create(&enemy_thread[i], &attr, enemyThread, (void*)enemy[i]);
    }

    pthread_attr_destroy(&attr);
}

void joinEnemiesThreads(){
    for(int i = 0;i < MAX_ENEMIES;i++){
        pthread_join(enemy_thread[i], NULL);
    }
}

int main(){

    // Setando semente para a geração de número aleatórios
    srand(time(NULL));

    // Criando engine
    engine = new Engine(WINDOW_WIDTH, WINDOW_HEIGHT, "Pixteal");

    // Carregando recursos
    load();

    // Criando threads dos inimigos
    createEnemiesThreads();

    // Iniciando engine
    engine->start(render, update);

    // Esperando as threads finalizarem
    joinEnemiesThreads();

    // Destruindo componentes
    unload();

    return 0;
}
