#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL2_rotozoom.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL2_gfxPrimitives_font.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define LOG(msg)                (add_log(msg))

//Max string length, for char buffers.
#define MSL                         512

//In pixels.
#define SCREEN_WIDTH                800
#define SCREEN_HEIGHT               600

//Standard tile size.
#define TILE_SIZE                   32

//Used specifically for the 3 background tiles.
#define BG_WIDTH                    128
#define BG_HEIGHT                   256

//ship_1.png
#define PLAYER_WIDTH                48
#define PLAYER_HEIGHT               48

//Player speed/maneuvering
#define PLAYER_SPEED                300.0f
#define PLAYER_MAX_ROLL             90.0f
#define PLAYER_ROLL_SPEED           180.0f

#define AFTERBURNER_MAX 100.0f
#define AFTERBURNER_DEPLETION_RATE 30.0f // Units per second
#define AFTERBURNER_RECHARGE_RATE 10.0f // Units per second
#define AFTERBURNER_SPEED_MULTIPLIER 1.5f
#define MAX_AFTERBURNER_PARTICLES 50

#define MAX_PARTICLES       100
#define PARTICLE_LIFETIME   60

//Enemies
#define MAX_ENEMIES 10


//Player weapons
#define MAX_WEAPONS                 5

#define WPN_LASER           0
#define WPN_PLASMA_GUN      1
#define WPN_RAILGUN         2
#define WPN_RAPID_FIRE      3
#define WPN_MISSILE         4

#define MAX_PROJECTILES     120

#define WEAPON_SWITCH_COOLDOWN 150 

//Player power ups
#define MAX_POWERUPS 10
#define POWERUP_SIZE 32
//#define POWERUP_SPEED 10
#define POWERUP_SPAWN_CHANCE 0.005f
#define POWERUP_DURATION 10000  // 10 seconds
//#define POWERUP_COUNT 4

//Game specific
#define FPS 60
#define FRAME_TARGET_TIME (1000 / FPS)
#define SCROLL_SPEED (PLAYER_SPEED)

#define MAX_PLANETS 24 //Needs to match how many planet .png files we have
#define PLANET_SPAWN_CHANCE 0.005 // Adjust this value to control how often planets appear


//Scroll speed of planets.
#define MIN_PLANET_SPEED 50.0f
#define MAX_PLANET_SPEED 200.0f


//Data structs used in game

//Particles
typedef struct 
{
    float x, y;
    float vx, vy;
    int lifetime;
    SDL_Color color;
} Particle;

typedef struct 
{
    SDL_Texture* texture;
    SDL_Rect position;
    float scale;
    float speed;
    bool active;
    float radius;
    float x;
    float y;    
} Planet;

// Power-up types
typedef enum {
    POWERUP_SPEED,
    POWERUP_FIRE_RATE,
    POWERUP_SHIELD,
    POWERUP_AMMO    
} PowerUpType;

// Power-up structure
typedef struct {
    PowerUpType type;
    SDL_Texture* texture;
    SDL_Rect position;
    float velocity_y;
    bool active;
} PowerUp;

typedef struct 
{
    SDL_Texture* textures[2];
    int scroll_y;
} Background;

typedef struct 
{
    float x, y;
    float vx, vy;
    float angle;
    float speed;
    int damage;
    bool active;
    int type;
    SDL_Texture* texture;
    SDL_Rect dest_rect;
    SDL_Point center;
    bool is_enemy_projectile;
} Projectile;

typedef struct 
{
    int max_ammo;
    int damage;
    float fire_rate;
    int bullet_speed;
    Uint32 cooldown;
    char* name;    
    int offset_x;     //for positioning with shooter
    int offset_y;
    int width;
    int height;
} WeaponType;

typedef struct 
{
    WeaponType type;
    int ammo;
} Weapon;

typedef struct 
{
    SDL_Texture* texture;
    SDL_Rect position;

    float velocity_x;
    float velocity_y;
    float bonus_velocity;
    float roll_angle;
    Uint32 last_shot_time;
    Uint32 last_weapon_switch_time;

    float afterburner;
    bool is_afterburner_active;
    int hit_points;
    int max_hp;
    Weapon weapons[MAX_WEAPONS];
    int current_weapon;
    int score;
} Player;

typedef struct {
    SDL_Texture* texture;
    SDL_Rect position;
    float velocity_x;
    float velocity_y;
    int hit_points;
    int max_hp;
    int defense;
    int damage;
    int current_weapon;
    Uint32 last_shot_time;
    bool active;
} Enemy;

typedef struct 
{
    SDL_Renderer* renderer;
    SDL_Window* window;
    Background background;
    bool is_running;
    Player player;
    SDL_Texture * weapon_textures[MAX_WEAPONS];
    
    Planet planets[MAX_PLANETS];
    SDL_Texture* planet_textures[MAX_PLANETS];
    Uint32 last_frame_time;

    float current_game_speed;
    Projectile projectiles[MAX_PROJECTILES];
    TTF_Font* font;

    Particle afterburner_particles[MAX_AFTERBURNER_PARTICLES];



    Enemy enemies[MAX_ENEMIES];
    SDL_Texture* enemy_texture;

    PowerUp powerups[MAX_POWERUPS];
    SDL_Texture* powerup_texture;
    Uint32 powerup_end_times[4];
} Game;




//Function declarations shared program wide.
void add_log(char * message);
void apply_powerup(Game* game, PowerUpType type);
bool check_collision(SDL_Rect a, SDL_Rect b);
void render_powerups(Game* game);
void spawn_powerup(Game* game);
void update_powerup_effects(Game* game);
void update_powerups(Game* game, float delta_time);

//Globals
extern Particle particles[MAX_PARTICLES];



