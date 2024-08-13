
#include "main.h"


//SDL Colors
const SDL_Color CLR_WHITE =         {255, 255, 255, 255};
const SDL_Color CLR_RED =           {255, 0, 0, 255};
const SDL_Color CLR_GREEN =         {0, 255, 0, 255};
const SDL_Color CLR_YELLOW =        {255, 255, 0, 255};
const SDL_Color CLR_LIME_GREEN =    {50, 205, 50, 255};
const SDL_Color CLR_CYAN =          {0, 255, 255, 255};
const SDL_Color CLR_MAGENTA =       {255, 0, 255, 255};

const WeaponType WEAPON_TYPES[] = 
{//max_ammo,damage, fire_rate,  bullet_speed,   cooldown,name,              offx,   offy,   width,  height
    {100,   10,     0.5,        6,            250,     "Laser",           1,      4,      2,      12},
    {40,    20,     1.0,        5,            500,     "Plasma Gun",      2,      4,      4,      8},
    {250,   25,     300,        4,            400,     "Railgun",         8,      8,      16,     16},
    {500,   2,      600,        2.5,          150,     "Rapid Fire",      1,      1,      2,      2},
    {20,    50,     200,        15,           2000,    "Missile",         8,      8,      16,     16}
};

//function prototypes
void change_weapon(Game * game, int direction);

void check_planet_collision(Game * game);

void cleanup(Game* game);

void create_explosion(float x, float y);

void draw_shield(Game* game, int x, int y, int radius, Uint32 remaining_time, Uint32 total_time);

void handle_events(Game* game);
void handle_input(Game* game);

void init_enemies(Game* game);
bool init_game(Game* game);
void init_particles();
void init_planets(Game* game);
void init_powerups(Game* game);

bool load_background(Game* game);
bool load_planet_textures(Game* game);
bool load_player(Game* game);
bool load_weapon_textures(Game * game);

int rnd_num(int min, int max);

void render(Game* game);
void render_afterburner_meter(Game* game);
void render_afterburner_particles(Game* game);
void render_gradient_bar(SDL_Renderer* renderer, int x, int y, int width, int height, float percentage, SDL_Color start_color, SDL_Color end_color);
void render_score(Game* game);
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color);
void render_enemies(Game* game);
void render_particles(SDL_Renderer *renderer);

void shoot_projectile(Game* game, Enemy * enemy, Player * player);

void spawn_enemy(Game* game);

void update(Game* game);
void update_afterburner_particles(Game* game, float delta_time);
void update_enemies(Game* game, float delta_time);
void update_particles();
void update_planets(Game* game, float delta_time);
void update_player(Game* game, float delta_time);
void update_projectiles(Game* game, float delta_time);

//Main game loop.
int main(int argc, char* argv[]) 
{
    Game game;
    
    if (argc > 0)
    {
        if (argv[1])
            LOG(argv[1]);
    }

    if (!init_game(&game)) 
        return 1;    

    game.last_frame_time = SDL_GetTicks();

    while (game.is_running)     
    {
        Uint32 frame_start = SDL_GetTicks();

        handle_events(&game);
        update(&game);
        render(&game);

        Uint32 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_TARGET_TIME) 
            SDL_Delay(FRAME_TARGET_TIME - frame_time);        
    }

    cleanup(&game);
    return 0;
}

//Initializes everything for the game.
bool init_game(Game* game) 
{
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
    {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    game->window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (game->window == NULL) 
    {
        SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    game->renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED);
    if (game->renderer == NULL) 
    {
        SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    if (TTF_Init() == -1) 
    {
        fprintf(stderr, "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }

    game->font = TTF_OpenFont("../fonts/SpaceFrigateItalic.ttf", 14); 
    if (game->font == NULL) 
    {
        fprintf(stderr, "Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }

    game->background.scroll_y = 0;
    game->is_running = true;
    
    //Player init
    game->player.last_shot_time = 0;
    game->player.afterburner = AFTERBURNER_MAX;
    game->player.is_afterburner_active = false;    
    game->player.max_hp = 1000;
    game->player.hit_points = game->player.max_hp;
    game->player.score = 0;
    game->player.bonus_velocity = 0;
    game->player.velocity_x = 0;
    game->player.velocity_y = 0;
    
    for (int i = 0; i < MAX_AFTERBURNER_PARTICLES; i++)     
        game->afterburner_particles[i].lifetime = 0;
    
    for (int i = 0; i < MAX_WEAPONS; i++) 
    {
        game->player.weapons[i].type = WEAPON_TYPES[i];
        game->player.weapons[i].ammo = WEAPON_TYPES[i].max_ammo;
    }

    game->player.current_weapon = WPN_LASER;

    if (!load_background(game)) 
        return false;

    //Load player
    if (!load_player(game)) 
        return false;

    //Initialize projectiles and their textures
    if (!load_weapon_textures(game))
        return false;

    for (int i = 0; i < MAX_PROJECTILES; i++)     
        game->projectiles[i].active = false;        
    
    if (!load_planet_textures(game)) 
        return false;

    init_planets(game);    
    init_enemies(game);
    init_powerups(game);
    init_particles();

    return true;
}

void init_planets(Game* game) 
{
    for (int i = 0; i < MAX_PLANETS; i++) 
        game->planets[i].active = false;    
}

//Load background images.
bool load_background(Game* game) 
{
    const char* bg_files[] = {"../img/space_bg1.png", "../img/space_bg2.png"};
    
    for (int i = 0; i < 2; i++) 
    {
        SDL_Surface* surface = IMG_Load(bg_files[i]);
        if (surface == NULL) 
        {
            SDL_Log("Unable to load background image %s! SDL_Error: %s\n", bg_files[i], SDL_GetError());
            return false;
        }
        game->background.textures[i] = SDL_CreateTextureFromSurface(game->renderer, surface);
        SDL_FreeSurface(surface);

        if (game->background.textures[i] == NULL) 
        {
            SDL_Log("Unable to create texture from background image %s! SDL_Error: %s\n", bg_files[i], SDL_GetError());
            return false;
        }
    }

    game->background.scroll_y = 0;
    return true;
}

bool load_player(Game* game) 
{
    SDL_Surface* surface = IMG_Load("../img/Player/ship_1.png");
    if (surface == NULL) 
    {
        SDL_Log("Unable to load player image! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    game->player.texture = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    if (game->player.texture == NULL) 
    {
        SDL_Log("Unable to create texture from player image! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    // Set initial position for the player
    game->player.position.w = PLAYER_WIDTH;
    game->player.position.h = PLAYER_HEIGHT;
    game->player.position.x = (SCREEN_WIDTH - PLAYER_WIDTH) / 2;
    game->player.position.y = SCREEN_HEIGHT - PLAYER_HEIGHT - 50; // 50 pixels from the bottom

    game->player.velocity_x = 0;
    game->player.roll_angle = 0;

    return true;
}

bool load_weapon_textures(Game * game) 
{
    for (int i = 0; i < MAX_WEAPONS; i++) 
    {
        char filename[64];
        snprintf(filename, sizeof(filename), "../img/Projectiles/%s.png", WEAPON_TYPES[i].name);
        SDL_Surface* surface = IMG_Load(filename);
        if (surface) 
        {
            game->weapon_textures[i] = SDL_CreateTextureFromSurface(game->renderer, surface);            
            SDL_FreeSurface(surface);
            if (!game->weapon_textures[i]) 
            {
                printf("Failed to create texture for %s: %s\n", WEAPON_TYPES[i].name, SDL_GetError());
                return false;                
            }
        } 
        else         
        {
            fprintf(stderr, "Failed to load texture for %s: %s\n", WEAPON_TYPES[i].name, IMG_GetError());        
            return false;
        }
    }
    return true;
}

bool load_planet_textures(Game* game) 
{

    const char* planet_files[] = 
    {
        "../img/Planets/planet_1.png", //0
        "../img/Planets/planet_2.png",
        "../img/Planets/planet_3.png",
        "../img/Planets/planet_4.png",
        "../img/Planets/planet_5.png",
        "../img/Planets/planet_6.png", //5
        "../img/Planets/planet_7.png",
        "../img/Planets/planet_8.png",
        "../img/Planets/planet_9.png",
        "../img/Planets/planet_10.png",
        "../img/Planets/planet_11.png", //10
        "../img/Planets/planet_12.png",
        "../img/Planets/planet_13.png",
        "../img/Planets/planet_14.png",
        "../img/Planets/planet_15.png",
        "../img/Planets/planet_16.png", //15
        "../img/Planets/planet_17.png",
        "../img/Planets/planet_18.png",
        "../img/Planets/nebula_1.png",
        "../img/Planets/Black_hole.png",
        "../img/Planets/Ice.png", //20
        "../img/Planets/Lava.png",
        "../img/Planets/Terran.png",
        "../img/Planets/starburst.png",
        "../img/Planets/supernova.png"
    };

    for (int i = 0; i < MAX_PLANETS; i++) 
    {
        SDL_Surface* surface = IMG_Load(planet_files[i]);
        if (surface == NULL) 
        {
            SDL_Log("Unable to load planet image %s! SDL_Error: %s\n", planet_files[i], SDL_GetError());
            return false;
        }
        game->planet_textures[i] = SDL_CreateTextureFromSurface(game->renderer, surface);
        SDL_FreeSurface(surface);

        if (game->planet_textures[i] == NULL) 
        {
            SDL_Log("Unable to create texture from planet image %s! SDL_Error: %s\n", planet_files[i], SDL_GetError());
            return false;
        }

        printf("Loaded planet texture #%d\n", i);
    }

    return true;
}

void handle_events(Game* game) 
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) 
    {
        if (event.type == SDL_QUIT)         
            game->is_running = false;        
    }
    
    handle_input(game);
}

void handle_input(Game* game) 
{
    const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
    
    if (keyboard_state[SDL_SCANCODE_LEFT] || keyboard_state[SDL_SCANCODE_A])     
        game->player.velocity_x = - 1 - game->player.bonus_velocity;
    else if (keyboard_state[SDL_SCANCODE_RIGHT] || keyboard_state[SDL_SCANCODE_D])     
        game->player.velocity_x = 1 + game->player.bonus_velocity;     
    /*else if (keyboard_state[SDL_SCANCODE_UP] || keyboard_state[SDL_SCANCODE_W])     
        game->player.velocity_y = - 1 - game->player.bonus_velocity;
    else if (keyboard_state[SDL_SCANCODE_DOWN] || keyboard_state[SDL_SCANCODE_S])     
        game->player.velocity_y = 1 + game->player.bonus_velocity;     */
    else     
    {
        game->player.velocity_x = 0;
        game->player.velocity_y = 0;
    }
    
    game->player.is_afterburner_active = (keyboard_state[SDL_SCANCODE_LCTRL] || keyboard_state[SDL_SCANCODE_RCTRL]) && game->player.afterburner > 0;

    if (keyboard_state[SDL_SCANCODE_SPACE]) 
        shoot_projectile(game, NULL, &game->player);

    if (keyboard_state[SDL_SCANCODE_Q])
        change_weapon(game, -1);
    if (keyboard_state[SDL_SCANCODE_E])
        change_weapon(game, 1);

    if (keyboard_state[SDL_SCANCODE_ESCAPE])    
        game->is_running = false;            
}

void change_weapon(Game* game, int direction) 
{

    if (!game)
    {
        LOG("Null game in change_weapon()");
        return;
    }

    Uint32 current_time = SDL_GetTicks();

    if (current_time - game->player.last_weapon_switch_time < WEAPON_SWITCH_COOLDOWN)
        // Cooldown hasn't elapsed, don't switch weapon
        return;    

    int num_weapons = MAX_WEAPONS;
    game->player.current_weapon = (game->player.current_weapon + direction + num_weapons) % num_weapons;
    game->player.last_weapon_switch_time = current_time;
}

void shoot_projectile(Game * game, Enemy * enemy, Player * player)
{
    Uint32 current_time = SDL_GetTicks();
    Uint32 last_shot_time = 0;
    int cur_weapon = 0;
    float cooldown_multiplier = (current_time < game->powerup_end_times[POWERUP_FIRE_RATE]) ? 0.5f : 1.0f;

    if (!enemy && !player)
    {
        LOG("NULL enemy and player in shoot_projectile()");
        return;
    }

    if (!game)
    {
        LOG("NULL game object in shoot_projectile()");
        return;
    }

    if (enemy)
    {
        cur_weapon = enemy->current_weapon;
        last_shot_time = enemy->last_shot_time;
        /*if (enemy->cur_weapon.ammo < 1)
            return;*/
    }
    else
    {
        cur_weapon = player->current_weapon;
        last_shot_time = player->last_shot_time;    
        if (player->weapons[cur_weapon].ammo < 1)
            return;
    }

    if (cur_weapon < 0 || cur_weapon >= MAX_WEAPONS) 
    {
        LOG("Error: current_weapon out of bounds in shoot_projectile()");
        return;
    }

    if (current_time - last_shot_time < WEAPON_TYPES[cur_weapon].cooldown * cooldown_multiplier)     
        return; // Don't shoot if cooldown hasn't elapsed
    

    for (int i = 0; i < MAX_PROJECTILES; i++) 
    {
        if (!game->projectiles[i].active) 
        {            
            if (enemy)
            {
                game->projectiles[i].x = enemy->position.x + enemy->position.w / 2;
                game->projectiles[i].y = enemy->position.y + enemy->position.h;
                game->projectiles[i].is_enemy_projectile = true;
                game->projectiles[i].angle = 180; // Shooting down toward player
            }
            else
            {
                game->projectiles[i].x = player->position.x + player->position.w / 2;
                game->projectiles[i].y = player->position.y;
                game->projectiles[i].is_enemy_projectile = false;
                game->projectiles[i].angle = game->player.roll_angle;
            }
            
            game->projectiles[i].speed = WEAPON_TYPES[cur_weapon].bullet_speed;
            game->projectiles[i].damage = WEAPON_TYPES[cur_weapon].damage;
            game->projectiles[i].active = true;
            game->projectiles[i].type = cur_weapon;
            game->projectiles[i].texture = game->weapon_textures[cur_weapon];
            
            // Calculate velocity components based on angle
            float rad_angle = game->projectiles[i].angle * M_PI / 180.0f;
            game->projectiles[i].vx = sinf(rad_angle) * game->projectiles[i].speed;
            game->projectiles[i].vy = -cosf(rad_angle) * game->projectiles[i].speed;

             // Increase the size of the projectile for visibility
            int projectile_width = WEAPON_TYPES[cur_weapon].width * 2;  // Double the width
            int projectile_height = WEAPON_TYPES[cur_weapon].height * 2;  // Double the height

            game->projectiles[i].dest_rect = (SDL_Rect)
            {                
                (int)game->projectiles[i].x - WEAPON_TYPES[cur_weapon].width / 2,
                (int)game->projectiles[i].y - WEAPON_TYPES[cur_weapon].height / 2,
                projectile_width,
                projectile_height                
            };

            game->projectiles[i].center = (SDL_Point)
            {
                WEAPON_TYPES[cur_weapon].width / 2,
                WEAPON_TYPES[cur_weapon].height / 2
            };            

            /*printf("Projectile created: x=%f, y=%f, angle=%f, speed=%f\n", 
            game->projectiles[i].x, game->projectiles[i].y, 
            game->projectiles[i].angle, game->projectiles[i].speed);*/


            if (enemy)
            {
                enemy->last_shot_time = current_time;
                //enemy->weapons[cur_weapon].ammo --;
            }
            else
            {
                player->last_shot_time = current_time;
                player->weapons[cur_weapon].ammo --;
            }
            break;
        }
    }

} 


void update(Game* game) 
{    
    Uint32 current_time = SDL_GetTicks();
    float delta_time = (current_time - game->last_frame_time) / 1000.0f;
    game->last_frame_time = current_time;

    // Scroll background
    game->background.scroll_y += SCROLL_SPEED * delta_time;
    if (game->background.scroll_y >= BG_HEIGHT)     
        game->background.scroll_y -= BG_HEIGHT;
    
    update_enemies(game, delta_time);
    update_afterburner_particles(game, delta_time);
    update_planets(game, delta_time);
    update_player(game, delta_time);    
    update_projectiles(game, delta_time);
    update_particles();
    update_powerups(game, delta_time);
    //update_powerup_effects(); 
    check_planet_collision(game);
}

void update_player(Game* game, float delta_time) 
{
    float current_speed = PLAYER_SPEED;

    if (game->player.is_afterburner_active) 
    {
        current_speed *= AFTERBURNER_SPEED_MULTIPLIER;
        game->player.afterburner -= AFTERBURNER_DEPLETION_RATE * delta_time;
        if (game->player.afterburner < 0) game->player.afterburner = 0;
    } 
    else 
    {
        game->player.afterburner += AFTERBURNER_RECHARGE_RATE * delta_time;
        if (game->player.afterburner > AFTERBURNER_MAX) 
            game->player.afterburner = AFTERBURNER_MAX;
    }

    // Update player position
     game->player.position.x += (int)(game->player.velocity_x * current_speed * delta_time);
     game->player.position.y += (int)(game->player.velocity_y * current_speed * delta_time);

    // Clamp player position to screen bounds
    if (game->player.position.x < 0)     
        game->player.position.x = 0;
    else if (game->player.position.x > SCREEN_WIDTH - game->player.position.w) 
        game->player.position.x = SCREEN_WIDTH - game->player.position.w;    
    /*else if (game->player.position.y < 0)     
        game->player.position.y = 0;
    else if (game->player.position.y > SCREEN_HEIGHT - game->player.position.y) 
        game->player.position.y = SCREEN_HEIGHT - game->player.position.y;    */

    // Update roll angle
    float target_roll = 0;
    if (game->player.velocity_x < 0) 
        target_roll = -PLAYER_MAX_ROLL;
    else if (game->player.velocity_x > 0) 
        target_roll = PLAYER_MAX_ROLL;
    
    float roll_change = PLAYER_ROLL_SPEED * delta_time;
    if (game->player.roll_angle < target_roll)     
        game->player.roll_angle = fmin(game->player.roll_angle + roll_change, target_roll);     
    else if (game->player.roll_angle > target_roll)     
        game->player.roll_angle = fmax(game->player.roll_angle - roll_change, target_roll);
}

void update_projectiles(Game* game, float delta_time) 
{
    for (int i = 0; i < MAX_PROJECTILES; i++) 
    {
        if (game->projectiles[i].active) 
        {            
            game->projectiles[i].x += game->projectiles[i].vx;
            game->projectiles[i].y += game->projectiles[i].vy;
            
            game->projectiles[i].dest_rect.x = (int)game->projectiles[i].x - game->projectiles[i].dest_rect.w / 2;
            game->projectiles[i].dest_rect.y = (int)game->projectiles[i].y - game->projectiles[i].dest_rect.h / 2;
            
            // Deactivate projectile if it goes off screen
            if (game->projectiles[i].y  < 0 ||
                game->projectiles[i].y > SCREEN_HEIGHT ||
                game->projectiles[i].x  < 0 ||
                game->projectiles[i].x > SCREEN_WIDTH) 
            {
                game->projectiles[i].active = false;
                //printf("Projectile deactivated: x=%f, y=%f\n", game->projectiles[i].x, game->projectiles[i].y);
            }        
            
            // Check collision with enemies
            SDL_Rect projectile_rect = 
            {
                (int)game->projectiles[i].x - 2,
                (int)game->projectiles[i].y - 2,
                4,
                4
            };
            
            for (int j = 0; j < MAX_ENEMIES; j++) 
            {
                if (game->enemies[j].active && check_collision(projectile_rect, game->enemies[j].position)) 
                {
                    int damage = game->projectiles[i].damage - game->enemies[j].defense;
                    if (damage > 0) 
                    {
                        game->enemies[j].hit_points -= damage;
                        if (game->enemies[j].hit_points <= 0) 
                        {
                            game->enemies[j].active = false;
                            game->player.score += game->enemies[j].max_hp;                            
                            create_explosion(game->enemies[j].position.x, game->enemies[j].position.y);
                        }
                    }
                    game->projectiles[i].active = false;
                    break;
                }
                
                // Check collision with player (for enemy projectiles)
                if (game->projectiles[i].angle == 180 && check_collision(projectile_rect, game->player.position)) {
                    game->player.hit_points -= game->projectiles[i].damage;
                    game->projectiles[i].active = false;
                    // Add player hit effect here
                }
            }
        }
    }
}


void update_planets(Game* game, float delta_time) 
{
    // Spawn new planets
    if ((float)rand() / RAND_MAX < PLANET_SPAWN_CHANCE) 
    {
        for (int i = 0; i < MAX_PLANETS; i++) 
        {
            if (!game->planets[i].active) 
            {
                game->planets[i].active = true;
                game->planets[i].scale = (float)(rand() % 50 + 25) / 100.0f; // Random scale between 0.25 and .75
                game->planets[i].position.w = (int)(96 * game->planets[i].scale); // Assuming original size is 48x48
                game->planets[i].position.h = (int)(96 * game->planets[i].scale);
                game->planets[i].position.x = rand() % (SCREEN_WIDTH - game->planets[i].position.w);
                game->planets[i].position.y = -game->planets[i].position.h;

                float speed_factor = 1.0f - (game->planets[i].scale - 0.25f) / 0.5f; // 0 for largest, 1 for smallest
                game->planets[i].speed = MIN_PLANET_SPEED + speed_factor * (MAX_PLANET_SPEED - MIN_PLANET_SPEED);

                //printf("Planet spawned [%d] @ x: %d, y: %d [speed: %.2f, scale: %.2f, delta_time: %.4f\n", 
                    //i, game->planets[i].position.x, game->planets[i].position.y, game->planets[i].speed, game->planets[i].scale, delta_time);
                break;
            }
        }
    }

    // Move planets
    for (int i = 0; i < MAX_PLANETS; i++) 
    {
        if (game->planets[i].active) 
        {
            float movement = (int)(game->planets[i].speed * delta_time);
            
            game->planets[i].position.y += movement;
            /*printf("Planet [%d] movement: %.2f, speed: %.2f, delta_time: %.4f\n", 
                   i, movement, game->planets[i].speed, delta_time);            */
            if (game->planets[i].position.y > SCREEN_HEIGHT)            
                game->planets[i].active = false;
        }       
    }
}

void render_current_weapon(SDL_Renderer* renderer, Game* game) 
{
    int wtype = game->player.current_weapon;
    
    // Position for weapon display (bottom right)
    int display_width =  32;  // Adjust as needed
    int display_height = 32;  // Adjust as needed
    int spacing = 8;
    int start_x = SCREEN_WIDTH - (MAX_WEAPONS * (display_width + spacing));
    int y = SCREEN_HEIGHT - display_height - 32;  // 32 pixels from bottom edge

    if (renderer == NULL || game == NULL) 
    {
        LOG("Error: Renderer or game is NULL in render_current_weapon");
        return;
    }

    if (game->font == NULL) 
    {
        fprintf(stderr, "Error: Font is NULL in render_current_weapon\n");
        return;
    }

    for (int i = 0; i < MAX_WEAPONS; i++) 
    {
        SDL_Rect dest_rect = 
        {
            start_x + i * (display_width + spacing),
            y,
            display_width,
            display_height
        };
    
        // Draw weapon icon
        if (game->weapon_textures[wtype])        
            SDL_RenderCopy(renderer, game->weapon_textures[wtype], NULL, &dest_rect);        
    
        // Highlight current weapon
        if (i == game->player.current_weapon) 
        {
            SDL_SetRenderDrawColor(renderer, 9, 255, 255, 255);  // Yellow highlight
            SDL_RenderDrawRect(renderer, &dest_rect);
        }

        // Draw ammo count
        char ammo_count[8];
        snprintf(ammo_count, sizeof(ammo_count), "%d", game->player.weapons[i].ammo);
        render_text(renderer, game->font, ammo_count, dest_rect.x, dest_rect.y + dest_rect.h, CLR_LIME_GREEN);

    }
    // Draw current weapon name
    char weapon_name[64];
    snprintf(weapon_name, sizeof(weapon_name), "%s", WEAPON_TYPES[game->player.current_weapon].name);
    render_text(renderer, game->font, weapon_name, start_x, y - 20, CLR_LIME_GREEN);
}

void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) 
{
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (surface) 
    {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) 
        {
            SDL_Rect dest = {x, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

void render_health_bar(Game* game) 
{
    int meter_width = 100;
    int meter_height = 10;
    int x = (SCREEN_WIDTH - meter_width) / 2;
    int y = SCREEN_HEIGHT - meter_height - 25;

    SDL_Color start_color = {255, 0, 0, 255};
    SDL_Color end_color = {0, 255, 0, 255};
    float percentage = (float)game->player.hit_points / game->player.max_hp;

    render_gradient_bar(game->renderer, x, y, meter_width, meter_height, percentage, start_color, end_color);
}

void render_projectiles(Game* game) 
{
    for (int i = 0; i < MAX_PROJECTILES; i++) 
    {
        // Debug: Draw a colored rectangle around the projectile
        //SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);  // Red color
        //SDL_RenderDrawRect(game->renderer, &game->projectiles[i].dest_rect);

        if (game->projectiles[i].active) 
        {
            SDL_RenderCopyEx(
                game->renderer,
                game->projectiles[i].texture,
                NULL,
                &game->projectiles[i].dest_rect,
                game->projectiles[i].angle,
                &game->projectiles[i].center,
                SDL_FLIP_NONE
            );         

            // Debug output
            /*printf("Rendering projectile: x=%d, y=%d, w=%d, h=%d, angle=%f\n", 
                   game->projectiles[i].dest_rect.x, game->projectiles[i].dest_rect.y,
                   game->projectiles[i].dest_rect.w, game->projectiles[i].dest_rect.h,
                   game->projectiles[i].angle);*/
        }
    }
}

void render_planets(Game* game) 
{
    for (int i = 0; i < MAX_PLANETS; i++) 
    {
        if (game->planets[i].active) 
        {
            SDL_RenderCopy(game->renderer, game->planet_textures[i], NULL, &game->planets[i].position);

            /*printf("Rendered planet [%d] @ x: %d, y: %d, w: %d, h: %d\n", 
                   i, game->planets[i].position.x, game->planets[i].position.y, 
                   game->planets[i].position.w, game->planets[i].position.h);*/
        }
    }
}

void render_gradient_bar(SDL_Renderer* renderer, int x, int y, int width, int height, float percentage, SDL_Color start_color, SDL_Color end_color) 
{
    SDL_Rect bg_rect = {x, y, width, height};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &bg_rect);

    int fill_width = (int)(percentage * width);
    for (int i = 0; i < fill_width; i++) 
    {
        float t = (float)i / width;
        SDL_Color color = 
        {
            start_color.r + (end_color.r - start_color.r) * t,
            start_color.g + (end_color.g - start_color.g) * t,
            start_color.b + (end_color.b - start_color.b) * t,
            255
        };
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer, x + i, y, x + i, y + height - 1);
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &bg_rect);
}

void render_enemies(Game* game) 
{
    //char buf[MSL];

    for (int i = 0; i < MAX_ENEMIES; i++) 
    {
        if (game->enemies[i].active) 
        {
            SDL_RenderCopy(game->renderer, game->enemy_texture, NULL, &game->enemies[i].position);        
            //sprintf(buf, "Enemy %d: x=%d, y=%d\n", i, game->enemies[i].position.x, game->enemies[i].position.y);
            //LOG(buf);
        }
    }
}

void render_score(Game* game) 
{
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "Score: %d", game->player.score);    

    SDL_Rect dest_rect = 
    {
        SCREEN_WIDTH - 160, 
        10, // 10 pixels from the top
        100,
        10
    };

    render_text(game->renderer, game->font, score_text, dest_rect.x, dest_rect.y + dest_rect.h, CLR_LIME_GREEN);
}

void render(Game* game) 
{
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
    SDL_RenderClear(game->renderer);

    // Render scrolling background
    for (int y = -BG_HEIGHT + game->background.scroll_y; y < SCREEN_HEIGHT; y += BG_HEIGHT) 
    {
        for (int x = 0; x < SCREEN_WIDTH; x += BG_WIDTH) 
        {
            SDL_Rect dest_rect = {x, y, BG_WIDTH, BG_HEIGHT};

            int texture_index = rand() % 2;
            SDL_RenderCopy(game->renderer, game->background.textures[texture_index], NULL, &dest_rect);
        }
    }

    // Render player with rotation
    SDL_Rect src_rect = {0, 0, game->player.position.w, game->player.position.h};
    SDL_RenderCopyEx(game->renderer, game->player.texture, &src_rect, &game->player.position, 
                     game->player.roll_angle, NULL, SDL_FLIP_NONE);


    render_planets(game);
    render_afterburner_particles(game);
    render_afterburner_meter(game);
    render_enemies(game);
    render_score(game);
    render_health_bar(game);
    render_projectiles(game);
    render_current_weapon(game->renderer, game);
    render_particles(game->renderer);
    render_powerups(game);

    // Check if shield power-up is active
    if (game->powerup_end_times[POWERUP_SHIELD] > SDL_GetTicks()) 
    {
        Uint32 remaining_time = game->powerup_end_times[POWERUP_SHIELD] - SDL_GetTicks();
        Uint32 total_time = 10000; // Assuming shield lasts for 10 seconds        
        int shield_radius = game->player.position.w / 2 + 10; // Adjust as needed
        //int max_thickness = 10; // Maximum thickness of the shield
        //int current_thickness = (int)(max_thickness * remaining_time / (float)total_time);
        
        draw_shield(game, 
                    game->player.position.x + game->player.position.w / 2, 
                    game->player.position.y + game->player.position.h / 2, 
                    shield_radius, 
                    //current_thickness, 
                    remaining_time, 
                    total_time);
    }
    
    SDL_RenderPresent(game->renderer);
}

void init_enemies(Game* game) 
{
    for (int i = 0; i < MAX_ENEMIES; i++) 
        game->enemies[i].active = false;    
    
    // Load enemy texture
    SDL_Surface* surface = IMG_Load("../img/Enemies/enemy-green-01.png");
    if (surface == NULL)
    {
        SDL_Log("Unable to load enemy image! SDL_Error: %s\n", SDL_GetError());
        return;
    }
    game->enemy_texture = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);
}

void update_enemies(Game* game, float delta_time) 
{
    Uint32 current_time = SDL_GetTicks();
    for (int i = 0; i < MAX_ENEMIES; i++) 
    {
        Enemy* enemy = &game->enemies[i];
        if (enemy->active) 
        {
            // Move enemy
            enemy->position.x += enemy->velocity_x * delta_time;
            enemy->position.y += enemy->velocity_y * delta_time;
            
            // Check if enemy is off-screen
            if (enemy->position.y > SCREEN_HEIGHT) 
                enemy->active = false;
            
            
            // Enemy shooting
            if (current_time - enemy->last_shot_time >= WEAPON_TYPES[enemy->current_weapon].cooldown)            
                shoot_projectile(game, enemy, NULL);
            
            // Check collision with projectiles
            for (int j = 0; j < MAX_PROJECTILES; j++) 
            {
                if (game->projectiles[j].active && !game->projectiles[j].is_enemy_projectile &&
                    check_collision(game->projectiles[j].dest_rect, enemy->position)) 
                {
                    enemy->hit_points -= game->projectiles[j].damage;
                    game->projectiles[j].active = false;

                    if (enemy->hit_points <= 0) 
                    {
                        game->player.score += enemy->max_hp;                        
                        enemy->active = false;
                        // Add explosion effect here
                    }
                    break;
                }
            }

            // Check collision with player
            if (check_collision(enemy->position, game->player.position)) 
            {
                game->player.hit_points -= enemy->damage;
                enemy->active = false;
                // Add explosion effect here
            }
        }
    }
    
    // Spawn new enemies
    if (rnd_num(0, 200) < 2)     
        spawn_enemy(game);    
}

void spawn_enemy(Game* game) 
{
    for (int i = 0; i < MAX_ENEMIES; i++) 
    {
        if (!game->enemies[i].active) 
        {
            Enemy* enemy = &game->enemies[i];
            enemy->active = true;
            enemy->position.w = 48; // Adjust based on your enemy sprite size
            enemy->position.h = 48;
            enemy->position.x = rnd_num(0, SCREEN_WIDTH - enemy->position.w);
            enemy->position.y = -enemy->position.h;
            enemy->velocity_x = rnd_num(-50, 50);
            enemy->velocity_y = rnd_num(50, 100);
            enemy->max_hp = rnd_num(10,40);
            enemy->hit_points = enemy->max_hp;
            enemy->defense = rnd_num(0, 5);
            enemy->damage = rnd_num(10, 20);
            enemy->current_weapon = rnd_num(0, MAX_WEAPONS - 1);
            enemy->last_shot_time = SDL_GetTicks();

            Uint32 current_time = SDL_GetTicks();
            
            for (int j = 0; j < MAX_WEAPONS; j++) 
                enemy->last_shot_time = current_time;            

            break;
        }
    }
}

void cleanup(Game* game) 
{
    SDL_DestroyTexture(game->background.textures[0]);
    SDL_DestroyTexture(game->background.textures[1]);

    for (int i = 0; i < MAX_PLANETS; i++)    
        SDL_DestroyTexture(game->planet_textures[i]);
    
    for (int i = 0; i < MAX_WEAPONS; i++)
        SDL_DestroyTexture(game->weapon_textures[i]);
    
    SDL_DestroyTexture(game->powerup_texture);

    SDL_DestroyRenderer(game->renderer);
    SDL_DestroyWindow(game->window);
    SDL_Quit();
}