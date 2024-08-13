#include "main.h"


const struct SDL_Color POWERUP_COLORS[4] = 
{
    {0,255,255,255},            //speed
    {255,0,255,255},            //fire_rate
    {0,255,0,255},              //shield
    {255,255,0,255}             //ammo
};

void init_powerups(Game* game) {
    // Create a surface for the diamond shape
    SDL_Surface* surface = SDL_CreateRGBSurface(0, POWERUP_SIZE, POWERUP_SIZE, 32, 0, 0, 0, 0);
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 255, 255, 255, 255));

    // Create a diamond shape
    Sint16 vx[] = {POWERUP_SIZE/2, POWERUP_SIZE, POWERUP_SIZE/2, 0};
    Sint16 vy[] = {0, POWERUP_SIZE/2, POWERUP_SIZE, POWERUP_SIZE/2};
    filledPolygonColor(game->renderer, vx, vy, 4, 0xFFFFFF);
        

    // Create texture from surface
    game->powerup_texture = SDL_CreateTextureFromSurface(game->renderer, surface);
    SDL_FreeSurface(surface);

    // Initialize powerups
    for (int i = 0; i < MAX_POWERUPS; i++) {
        game->powerups[i].active = false;
    }

    // Initialize powerup end times
    for (int i = 0; i < 4; i++) {
        game->powerup_end_times[i] = 0;
    }
}

void update_powerups(Game* game, float delta_time) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (game->powerups[i].active) {
            game->powerups[i].position.y += game->powerups[i].velocity_y * delta_time;

            // Check if powerup is off-screen
            if (game->powerups[i].position.y > SCREEN_HEIGHT) {
                game->powerups[i].active = false;
            }

            // Check collision with player
            if (check_collision(game->powerups[i].position, game->player.position)) {
                apply_powerup(game, game->powerups[i].type);
                game->powerups[i].active = false;
            }
        }
    }

    // Spawn new powerup
    if ((float)rand() / RAND_MAX < POWERUP_SPAWN_CHANCE) {
        spawn_powerup(game);
    }

    update_powerup_effects(game);
}

void render_powerups(Game* game) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (game->powerups[i].active) {
            SDL_SetTextureColorMod(game->powerup_texture, 
                POWERUP_COLORS[game->powerups[i].type].r,
                POWERUP_COLORS[game->powerups[i].type].g,
                POWERUP_COLORS[game->powerups[i].type].b);
            SDL_RenderCopy(game->renderer, game->powerup_texture, NULL, &game->powerups[i].position);
        }
    }
}

void spawn_powerup(Game* game) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!game->powerups[i].active) {
            game->powerups[i].active = true;
            game->powerups[i].type = rand() % 4;
            game->powerups[i].position.w = POWERUP_SIZE;
            game->powerups[i].position.h = POWERUP_SIZE;
            game->powerups[i].position.x = rand() % (SCREEN_WIDTH - POWERUP_SIZE);
            game->powerups[i].position.y = -POWERUP_SIZE;
            game->powerups[i].velocity_y = POWERUP_SPEED;
            break;
        }
    }
}

void apply_powerup(Game* game, PowerUpType type) {
    game->powerup_end_times[type] = SDL_GetTicks() + POWERUP_DURATION;

    switch (type) {
        case POWERUP_SPEED:
            game->player.bonus_velocity = PLAYER_SPEED * 0.5f;
            break;
        case POWERUP_FIRE_RATE:
            // The effect will be applied in the shooting logic
            break;
        case POWERUP_SHIELD:
            game->player.hit_points += 50;  // Add shield points
            if (game->player.hit_points > game->player.max_hp) {
                game->player.hit_points = game->player.max_hp;
            }
            break;
        case POWERUP_AMMO:
            for (int i = 0; i < MAX_WEAPONS; i++) {
                game->player.weapons[i].ammo = game->player.weapons[i].type.max_ammo;
            }
            break;
    }
}

void update_powerup_effects(Game* game) {
    Uint32 current_time = SDL_GetTicks();

    // Speed powerup
    if (current_time > game->powerup_end_times[POWERUP_SPEED]) {
        game->player.bonus_velocity = 0;
    }

    // Other powerups don't need to be updated here as they are checked in their respective systems
}


void draw_shield(Game* game, int x, int y, int radius,/* int thickness,*/ Uint32 remaining_time, Uint32 total_time) 
{
    // Calculate the alpha based on remaining time
    Uint8 alpha = (Uint8)(255 * remaining_time / (float)total_time);
    
    // Draw the shield circle
    //thickCircleColor(game->renderer, x, y, radius, thickness, 0, 255, 255, alpha);
    aacircleRGBA(game->renderer, x, y, radius, 0, 255, 255, alpha);
}