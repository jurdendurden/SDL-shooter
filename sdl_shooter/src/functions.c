#include "main.h"

///Generates a random number within the given range.
int rnd_num(int min, int max)
{ 
    int num = 0;

    if (min == 0 && max == 0)
        return 0;    

    num = (min + rand() % (max+1 - min));

    return num;
    
}

//Prints errors to the console and logs them into game.log
void add_log(char * message)
{
    printf("%s", message);
    FILE * fp;

    if ((fp = fopen("../game.log", "a")) == NULL)
    {
        printf("Error writing to log file.");
        return;
    }
    

    fprintf(fp, "%s", message);
    fclose(fp);
}


bool check_collision(SDL_Rect a, SDL_Rect b) 
{
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}

void check_planet_collision(Game* game) 
{
    char buf[MSL];

    for (int i = 0; i < MAX_PLANETS; i++) 
    {
        Planet* planet = &game->planets[i];
        
        // Simple circle collision detection
        float dx = game->player.position.x + game->player.position.w / 2 - planet->x;
        float dy = game->player.position.y + game->player.position.h / 2 - planet->y;
        float distance = sqrt(dx * dx + dy * dy);
        
        if (distance < (game->player.position.w / 2 + planet->radius)) {
            // Collision detected, calculate damage
            int damage = (int)(planet->radius * game->current_game_speed * 0.1f);
            game->player.hit_points -= damage;
            sprintf(buf, "Player took %d damage from planet collision!\n", damage);
            LOG(buf);


            if (game->player.hit_points < 0) {
                game->player.hit_points = 0;
                // Implement game over logic here
            }
            
            // Implement visual feedback for collision (screen shake, particle effects)
        }
    }
}