#include "main.h"

void render_gradient_bar(SDL_Renderer* renderer, int x, int y, int width, int height, float percentage, SDL_Color start_color, SDL_Color end_color);


Particle particles[MAX_PARTICLES];

void init_particles() 
{
    for (int i = 0; i < MAX_PARTICLES; i++)     
        particles[i].lifetime = 0;
    
}

void create_explosion(float x, float y) 
{
    for (int i = 0; i < MAX_PARTICLES; i++) 
    {
        if (particles[i].lifetime <= 0) 
        {
            particles[i].x = x;
            particles[i].y = y;
            float angle = (float)rand() / RAND_MAX * 2 * M_PI;
            float speed = (float)rand() / RAND_MAX * 2 + 1;
            particles[i].vx = cosf(angle) * speed;
            particles[i].vy = sinf(angle) * speed;
            particles[i].lifetime = PARTICLE_LIFETIME;
            particles[i].color = (SDL_Color){255, 100 + rand() % 155, 0, 255};
        }
    }
}

void update_particles() 
{
    for (int i = 0; i < MAX_PARTICLES; i++) 
    {
        if (particles[i].lifetime > 0) 
        {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].lifetime--;
            particles[i].color.a = 255 * particles[i].lifetime / PARTICLE_LIFETIME;
        }
    }
}

void render_particles(SDL_Renderer *renderer) 
{
    for (int i = 0; i < MAX_PARTICLES; i++) 
    {
        if (particles[i].lifetime > 0) 
        {
            SDL_SetRenderDrawColor(renderer, particles[i].color.r, particles[i].color.g, particles[i].color.b, particles[i].color.a);
            SDL_RenderDrawPoint(renderer, (int)particles[i].x, (int)particles[i].y);
        }
    }
}


void update_afterburner_particles(Game* game, float delta_time) 
{
    for (int i = 0; i < MAX_AFTERBURNER_PARTICLES; i++) 
    {        
        Particle* p = &game->afterburner_particles[i];
        
        if (p->lifetime > 0) {
            p->x += p->vx * delta_time;
            p->y += p->vy * delta_time;
            p->lifetime--;
            p->color.a = (Uint8)((float)p->lifetime / 30 * 255);
        }
    }

    if (game->player.is_afterburner_active) {
        for (int i = 0; i < 2; i++) {
            int index = -1;
            for (int j = 0; j < MAX_AFTERBURNER_PARTICLES; j++) {
                if (game->afterburner_particles[j].lifetime <= 0) {
                    index = j;
                    break;
                }
            }
            if (index != -1) {
                Particle* p = &game->afterburner_particles[index];
                p->x = game->player.position.x + game->player.position.w / 2;
                p->y = game->player.position.y + game->player.position.h - 5;
                p->vx = (float)(rand() % 20 - 10) * 5;
                p->vy = (float)(rand() % 10 + 20) * 5;
                p->lifetime = 30;
                p->color = (SDL_Color){0, 100 + rand() % 155, 200 + rand() % 55, 255};
            }
        }
    }
}

void render_afterburner_particles(Game* game) 
{
    for (int i = 0; i < MAX_AFTERBURNER_PARTICLES; i++) 
    {
        Particle* p = &game->afterburner_particles[i];
        if (p->lifetime > 0) {
            SDL_SetRenderDrawColor(game->renderer, p->color.r, p->color.g, p->color.b, p->color.a);
            SDL_Rect rect = {(int)p->x - 1, (int)p->y - 1, 3, 3};
            SDL_RenderFillRect(game->renderer, &rect);
        }
    }
}

//Draw afterburner meter to the screen.
void render_afterburner_meter(Game* game) 
{
    int meter_width = 100;
    int meter_height = 10;
    int x = (SCREEN_WIDTH - meter_width) / 2;
    int y = SCREEN_HEIGHT - meter_height - 10;

    SDL_Color start_color = {0, 100, 255, 255};
    SDL_Color end_color = {0, 200, 255, 255};
    float percentage = game->player.afterburner / AFTERBURNER_MAX;

    render_gradient_bar(game->renderer, x, y, meter_width, meter_height, percentage, start_color, end_color);
}