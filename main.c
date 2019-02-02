#include "SDL.h"
#include "SDL_timer.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define HEIGHT_1 163
#define WIDTH_1 175

#define HEIGHT_MACH 163
#define WIDTH_MACH 520

#define HEIGHT_DP 260
#define WIDTH_DP 460

#define HEIGHT_1_p2 161
#define WIDTH_1_p2 114

#define HEIGHT_2_p2 165
#define WIDTH_2_p2 171

#define HEIGHT_3_p2 165
#define WIDTH_3_p2 202

#define FRICTION 0.5
#define GRAVITY 0.4

#define MAX_HP 300
#define MAX_STAMINA 300
#define DAMAGE_MULTIPLIER 5

#define INPUT_CONDITION_2 (player->state != jumping )&& (player->state != range_attack )&&( player->state != falling )&&( player->state != uppercutting && player->state != slashing && player->state != double_kicking)

int debug = 0, debug_time;
Uint8 *key_input;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Rect rect, rect_p2;
SDL_Rect hp_bar1 = { 120, 15, MAX_HP, 30 }, hp_bar2 = { 1000 - 420 , 15, MAX_HP, 30 }, stamina_bar1 = { 120, 45, MAX_STAMINA, 30 }, stamina_bar2 = { 1000 - 420 , 45, MAX_STAMINA, 30 },flame_rect = {0, 0, 412, 78 };
SDL_Rect portrait1_rect = { 10, 10, 100, 100 }, portrait2_rect = { 1000 - 110 , 10, 100, 100 };

Mix_Chunk *damage, *KO_sfx, *hit, *hit_hard, *hit_medium, *mach_punch_sfx, *kick1_sfx, *kick2_sfx, *dynamic_punch_sfx, *flame_thrower_sfx, *slash_sfx, *guard_sfx, *victory_sfx, *battle_music;

SDL_Texture* loadTexture(char filename[]);
// collision and hitbox
bool check_collision(SDL_Rect A, SDL_Rect B);
void update_hurtbox_breloom(struct playable *player);
void update_hitbox_breloom(struct playable *player);
void update_hurtbox_combusken(struct playable2 *player);
void update_hitbox_combusken(struct playable2 *player);

//prototype of breloom
SDL_Texture* animate_breloom(struct playable *player, struct playable2 *other_player, long now);
void get_rect_p1(struct playable *player);

//prototype of combusken
void get_rect_p2(struct playable2 *player);
SDL_Texture* animate_combusken(struct playable2 *player, struct playable *other_player, long now);


enum { idle = 1, walking_forward = 2, walking_backward = 3, jumping = 4, falling = 5, mach_punch = 6, blocking = 7 ,dynamic_punch = 8
	, kicking = 9, range_attack = 10, jump_kicking = 11, uppercutting = 12, slashing = 13, hurt = 14, double_kicking = 15, knocked_down = 16, back_flipping = 17};
enum {normal = 1, wide = 2, jump = 3, DynamicPunch = 4, kick_rect = 5, low_rect = 6, slash_rect = 7, hurt_rect_1 = 8, double_kick_rect = 9, flip_rect= 10};
enum { attack_channel_1 = 1, attack_channel_2 = 2, hit_channel_1 = 3, hit_channel_2 = 4, music_channel = 5};
bool check_collision(SDL_Rect A, SDL_Rect B)
{
	//The sides of the rectangles
	int leftA, leftB;
	int rightA, rightB;
	int topA, topB;
	int bottomA, bottomB;

	//Calculate the sides of rect A
	leftA = A.x;
	rightA = A.x + A.w;
	topA = A.y;
	bottomA = A.y + A.h;

	//Calculate the sides of rect B
	leftB = B.x;
	rightB = B.x + B.w;
	topB = B.y;
	bottomB = B.y + B.h;
	//If any of the sides from A are outside of B
	if (bottomA <= topB)
	{
		return false;
	}

	if (topA >= bottomB)
	{
		return false;
	}

	if (rightA <= leftB)
	{
		return false;
	}

	if (leftA >= rightB)
	{
		return false;
	}

	//If none of the sides from A are outside B
	return true;
}

struct playable{
	int x, y;
	int state;
	float dy;
	long last_update, current_frame;
	int using_move, still_frame, hit_available;
	int rect_type;

	int hp, stamina;

	SDL_Texture *standing[8];
	SDL_Texture *walking[10];
	SDL_Texture *walking_back[10];
	SDL_Texture *jump[6];
	SDL_Texture *mach_punch[23];
	SDL_Texture *guard;
	SDL_Texture *dynamic_punch[15];
	SDL_Texture *kick[9];
	SDL_Texture *jump_kick[6];
	SDL_Texture *back_flip[10];

	SDL_Texture *hurt[4];
	SDL_Texture *knock_down[7];

	SDL_Rect hurtbox;
	SDL_Rect hitbox;

};
struct playable2 {
	int x, y;
	int state;
	float dy;
	long last_update, current_frame, hit_available;
	int using_move, still_frame;
	int rect_type;

	int hp, stamina;

	SDL_Texture *standing[8];
	SDL_Texture *walking[10];
	SDL_Texture *walking_back[10];
	SDL_Texture *jump[6];
	SDL_Texture *guard[7];
	SDL_Texture *range[6];
	SDL_Texture *uppercut[9];
	SDL_Texture *slash[12];
	SDL_Texture *double_kick[24];

	SDL_Texture *hurt[4];
	SDL_Texture *knock_down[7];


	SDL_Rect hurtbox;
	SDL_Rect hitbox;

};
struct resources{
	SDL_Texture *bg, *red_box, *green_box, *hollow_red, *hollow_green, *blue_box;
	SDL_Texture *frame, *frame_p2;
	SDL_Texture *portrait1, *portrait2;
	SDL_Texture *flamethrower[10];
};
void init()
{

	if (SDL_Init(SDL_INIT_EVERYTHING | SDL_INIT_AUDIO) != 0)
		printf("SDL Initialization failed");


	SDL_Window *win = SDL_CreateWindow("HALF LIFE 3", SDL_WINDOWPOS_CENTERED
		, SDL_WINDOWPOS_CENTERED, 1000, 600, 0);

	if (win == NULL)
		printf("Error initializing");

	Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
	renderer = SDL_CreateRenderer(win, -1, render_flags);
	if (!renderer) {
		printf("Error creating renderer");
		SDL_Quit();
	}

	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
	}

	mach_punch_sfx = Mix_LoadWAV("Sound_effect\\mach_punch.wav");
	dynamic_punch_sfx = Mix_LoadWAV("Sound_effect\\dynamic_punch.wav");
	flame_thrower_sfx = Mix_LoadWAV("Sound_effect\\fire.wav");
	slash_sfx = Mix_LoadWAV("Sound_effect\\swing_01.wav");
	hit_hard = Mix_LoadWAV("Sound_effect\\hit_hard.wav");
	hit_medium = Mix_LoadWAV("Sound_effect\\hit_m.wav");
	guard_sfx = Mix_LoadWAV("Sound_effect\\guard.wav");
	KO_sfx = Mix_LoadWAV("Sound_effect\\KO.wav");
	battle_music = Mix_LoadWAV("Sound_effect\\battle_music.wav");
	victory_sfx = Mix_LoadWAV("Sound_effect\\victory_sound.wav");
	kick1_sfx = Mix_LoadWAV("Sound_effect\\swing_02.wav");
	kick2_sfx = Mix_LoadWAV("Sound_effect\\swing_06.wav");

}
void init_assets(struct resources *data) {
	data->bg = loadTexture("assets\\background_brock.png");
	data->red_box = loadTexture("assets\\red_s.jpg");
	data->green_box = loadTexture("assets\\green.png");
	data->blue_box = loadTexture("assets\\blue.jpg");
	data->portrait1 = loadTexture("assets\\breloom_portrait.jpg");
	data->portrait2 = loadTexture("assets\\combusken_portrait.png");
	data->hollow_red = loadTexture("assets\\hollow_red.png");
	data->hollow_green = loadTexture("assets\\hollow_green.png");
	// init flamethrower
	data->flamethrower[0] = loadTexture("Flamethrower\\fire00.png");
	data->flamethrower[1] = loadTexture("Flamethrower\\fire01.png");
	data->flamethrower[2] = loadTexture("Flamethrower\\fire02.png");
	data->flamethrower[3] = loadTexture("Flamethrower\\fire03.png");
	data->flamethrower[4] = loadTexture("Flamethrower\\fire04.png");
	data->flamethrower[5] = loadTexture("Flamethrower\\fire05.png");
	data->flamethrower[6] = loadTexture("Flamethrower\\fire06.png");
	data->flamethrower[7] = loadTexture("Flamethrower\\fire07.png");
	data->flamethrower[8] = loadTexture("Flamethrower\\fire08.png");
	data->flamethrower[9] = loadTexture("Flamethrower\\fire09.png");

}
SDL_Texture* loadTexture(char filename[])
{
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(filename);
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", filename, IMG_GetError());
	}
	else
	{
		if (loadedSurface == NULL)
		{
			printf("Unable to load image %s! SDL_image Error: %s\n", filename, IMG_GetError());
		}
		else
		{
			//Color key image
			SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0x00, 0x00, 0x00));
		}
		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", filename, SDL_GetError());
		}


		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	return newTexture;
}

void update_render(struct playable *player1, struct playable2 *player2, long now, struct resources *asset) {

	SDL_Texture *new_frame, *new_frame_p2;
	//Clear screen
	SDL_RenderClear(renderer);
	//Render background to screen
	SDL_RenderCopy(renderer, asset->bg, NULL, NULL);

	//get frame for both players
	new_frame = animate_breloom(player1, player2, now);
	new_frame_p2 = animate_combusken(player2, player1, now);

	//update hurtbox
	update_hurtbox_breloom(player1);
	update_hurtbox_combusken(player2);
	
	//update hitbox
	update_hitbox_breloom(player1);
	update_hitbox_combusken(player2);
	

	if (new_frame != NULL) {
		
		get_rect_p1(player1);
		SDL_RenderCopy(renderer, new_frame, NULL, &rect);
		asset->frame = new_frame;
	}
	else {
		get_rect_p1(player1);
		SDL_RenderCopy(renderer, asset->frame, NULL, &rect);
	}
	// hurt box test
	
	if (debug) {
		SDL_RenderCopy(renderer, asset->hollow_red, NULL, &player1->hurtbox);
		SDL_RenderCopy(renderer, asset->hollow_green, NULL, &player1->hitbox);
	}

	
	if (new_frame_p2 != NULL) {
		get_rect_p2(player2);
		SDL_RenderCopyEx(renderer, new_frame_p2, NULL, &rect_p2, 0, NULL, SDL_FLIP_HORIZONTAL);
		asset->frame_p2 = new_frame_p2;

	}
	else {
		get_rect_p2(player2);
		SDL_RenderCopyEx(renderer, asset->frame_p2, NULL, &rect_p2, 0, NULL, SDL_FLIP_HORIZONTAL);
	}
	if (debug) {
		SDL_RenderCopy(renderer, asset->hollow_red, NULL, &player2->hurtbox);
		SDL_RenderCopy(renderer, asset->hollow_green, NULL, &player2->hitbox);
	}

	if (player2->state == range_attack ) {
		flame_rect.x = player2->x - 400;
		flame_rect.y = player2->y + 20;

		SDL_RenderCopy(renderer, asset->flamethrower[player2->current_frame], NULL, &flame_rect);
	}

	// HUD rendering
	SDL_RenderCopy(renderer, asset->red_box, NULL, &hp_bar1);
	SDL_RenderCopy(renderer, asset->red_box, NULL, &hp_bar2);
	SDL_RenderCopy(renderer, asset->blue_box, NULL, &stamina_bar1);
	SDL_RenderCopy(renderer, asset->blue_box, NULL, &stamina_bar2);
	SDL_RenderCopy(renderer, asset->portrait1, NULL, &portrait1_rect);
	SDL_RenderCopy(renderer, asset->portrait2, NULL, &portrait2_rect);

	//Update screen
	SDL_RenderPresent(renderer);
	
}

SDL_Texture* animate_breloom(struct playable *player, struct playable2 *other_player, long now) {
	//Player motion equation
	if (player->x < -40)
		player->x = -40;
	player->y += player->dy;
	if (player->y < 400)
		player->dy += GRAVITY;
	if (player->y > 400)
		player->y = 400;
	if ((player->state == jumping || player->state == jump_kicking || player->state == falling) && player->y == 400) {
		player->still_frame = -1;
		player->using_move = 0;
		player->state = idle;
	}
	if(player->state == jump_kicking)
		if (!(player->x + WIDTH_1 - 50 > other_player->x))
			player->x += 6;
	if (player->state == idle || player->state == walking_backward || player->state == walking_forward || player->state == jumping)
		if (player->stamina < MAX_STAMINA)
			player->stamina += 1;
	//printf("player %f %d\n", player->dy, player->y);
	
	if ((now - player->last_update) > 80 && player->state == idle && player->using_move <= 0) {
		player->rect_type = normal;
		player->last_update = now;
		player->current_frame = (player->current_frame + 1) % 8;
		return player->standing[player->current_frame];

		//SDL_RenderCopy(renderer, player->standing[player->current_frame], NULL, &rect);
	}
	else if ((now - player->last_update) > 50 && player->state == walking_forward && player->using_move <= 0) {
		player->rect_type = normal;
		player->last_update = now;
		player->current_frame = (player->current_frame + 1) % 8;
		player->state = idle;

		return player->walking[player->current_frame];
	}
	else if ((now - player->last_update) > 50 && player->state == walking_backward && player->using_move <= 0) {
		player->rect_type = normal;
		player->last_update = now;
		player->current_frame = (player->current_frame + 1) % 8;
		player->state = idle;

		return player->walking_back[player->current_frame];
	}
	else if ((now - player->last_update) > 110 && player->state == jumping ) {
		player->rect_type = normal;
		player->last_update = now;

		if (6 - player->using_move-1 >= 5) {
			return NULL;
		}

		return player->jump[6 - player->using_move--];
	}
	else if ((now - player->last_update) > 15 && player->state == mach_punch) {
		player->hit_available = 0;
		player->rect_type = wide;
		player->last_update = now;
		if (23 - player->still_frame == 10)
			player->hit_available = 1;
		if (23 - player->still_frame - 1 >= 22) {
			
			player->state = idle;
			return NULL;
		}
		return player->mach_punch[23 - player->still_frame--];
		//return player->mach_punch[5];
	}
	else if ((now - player->last_update) > 100 && player->state == blocking) {
		player->rect_type = normal;
		player->last_update = now;
		
		player->state = idle;
		return player->guard;
		
	}
	else if ((now - player->last_update) > 65 && player->state == dynamic_punch) {
		player->hit_available = 0;
		player->rect_type = DynamicPunch;
		player->last_update = now;
		if (15 - player->still_frame - 1 >= 14) {
			player->state = idle;
			return NULL;
		}
		if (player->still_frame % 2 == 0)
			player->hit_available = 1;
		//printf(" still frame = %d", player->still_frame);
		return player->dynamic_punch[15 - player->still_frame--];
		
	}
	else if ((now - player->last_update) > 40 && player->state == kicking) {
		player->hit_available = 0;
		player->rect_type = kick_rect;
		player->last_update = now;

		if (10 - player->still_frame - 1 >= 9) {
			player->state = idle;
			return NULL;
		}
		if (player->still_frame == 6)
			player->hit_available = 1;
		return player->kick[9 - player->still_frame--];
		//return player->mach_punch[5]

	}
	else if ((now - player->last_update) > 50 && player->state == jump_kicking) {
		player->rect_type = normal;
		player->last_update = now;
		if (7 - player->still_frame  >= 6) {
			return NULL;
		}
		if (player->still_frame == 4)
			player->hit_available = 1;
		return player->jump_kick[6 - player->still_frame--];

	}

	else if ((now - player->last_update) > 20 && player->state == hurt) {
		Mix_HaltChannel(attack_channel_1);
		player->rect_type = hurt_rect_1;
		player->last_update = now;
		if (5 - player->still_frame >= 4) {
			player->still_frame = 0;
			player->using_move = 0;
			player->state = idle;
			return NULL;
		}
		return player->hurt[4 - player->still_frame--];

	}	
	else if ((now - player->last_update) > 70 && player->state == knocked_down) {
	player->rect_type = hurt_rect_1;
	player->last_update = now;
	if (7 - player->still_frame >= 7) {
		player->still_frame = 0;
		player->using_move = 0;
		return NULL;
	}

	return player->knock_down[7 - player->still_frame--];

	}
	return NULL;
}

SDL_Texture* animate_combusken(struct playable2 *player, struct playable *other_player,long now) {
	//Player motion equation
	if (player->x + WIDTH_1_p2> 1020)
		player->x = 1020 - WIDTH_1_p2;
	player->y += player->dy;

	if (player->y < 400) {
		if (player->state == uppercutting) {
			player->dy += GRAVITY + 0.2;
		}
		else if (player->state == falling)
			player->dy += GRAVITY + 0.4;
		else	
			player->dy += GRAVITY;
	}
	if (player->stamina < 0 && player->state == blocking) {
		player->state = idle;
		player->still_frame = 0;
		player->using_move = 0;
	}
	if (player->y > 400)
		player->y = 400;
	if ((player->state == jumping || player->state == falling) &&  player->y == 400) {
		player->state = idle;
	}
	if (player->state == idle || player->state == walking_backward || player->state == walking_forward || player->state == jumping)
		if (player->stamina < MAX_STAMINA) {
			player->stamina += 1;
			stamina_bar2.x = 580 + (MAX_HP - player->stamina);
		}

	
	if ((now - player->last_update) > 100 && player->state == idle && player->using_move <= 0) {
		player->rect_type = normal;
		player->last_update = now;
		player->current_frame = (player->current_frame + 1) % 8;
		return player->standing[player->current_frame];

	}
	else if ((now - player->last_update) > 50 && player->state == walking_forward && player->using_move <= 0 ) {
		player->rect_type = normal;
		//player->x += 8;
		player->last_update = now;
		player->current_frame = (player->current_frame + 1) % 8;
		//SDL_RenderCopy(renderer, player->walking[player->current_frame], NULL, &rect);
		player->state = idle;
		return player->walking[player->current_frame];
	}
	
	else if ((now - player->last_update) > 50 && player->state == walking_backward && player->using_move <= 0) {
		player->rect_type = normal;
		//player->x -= 8;
		player->last_update = now;
		player->current_frame = (player->current_frame + 1) % 8;
		//SDL_RenderCopy(renderer, player->walking_back[player->current_frame], NULL, &rect);
		player->state = idle;
		return player->walking_back[player->current_frame];
	}
	
	else if ((now - player->last_update) > 100 && player->state == jumping) {
		player->rect_type = jump;
		player->last_update = now;
		//player->current_frame = (player->current_frame + 1) % 5;

		//SDL_RenderCopy(renderer, player->walking_back[1], NULL, &rect);
		if (6 - player->using_move-1 >= 5) {
			return NULL;
		}
		return player->jump[6 - player->using_move--];
	}
	else if ((now - player->last_update) > 40 && player->state == blocking) {
		player->rect_type = jump;
		player->last_update = now;

		if (7 - player->still_frame -1 >= 6) {
			return NULL;
		}
		return player->guard[7 - player->still_frame--];
		
	}
	
	else if ((now - player->last_update) > 50 && player->state == range_attack) {
		player->hit_available = 0;
		player->rect_type = wide;
		player->last_update = now;
		player->current_frame = 9 - player->still_frame--;

		//player->current_frame = (player->current_frame + 1) % 5;
		//SDL_RenderCopy(renderer, player->walking_back[1], NULL, &rect);
		if (10 - player->still_frame-1 >= 10) {
			player->hitbox.w = 0;
			player->hitbox.h = 0;
			player->state = idle;
			return NULL;
		}
		if (player->still_frame == 2) {
			player->hit_available = 1;
		}
		
		if (player->current_frame == 0 || player->current_frame == 9)
			return player->range[5];
		return player->range[4];
	}

	else if ((now - player->last_update) > 60 && player->state == uppercutting) {
		player->hit_available = 0;
		player->rect_type = normal;
		player->last_update = now;
		if (player->still_frame > 4) {
			player->rect_type = low_rect;
			if(!(player->x < other_player->x + WIDTH_1 - 50))
				player->x -= 45;
		}
		if (player->still_frame == 4) {
			player->dy = -14;
			Mix_PlayChannel(attack_channel_2, kick2_sfx, 0);
		}

		if (player->still_frame < 4 && player->still_frame > 0) {
			player->hit_available = 1;
		}

		//player->current_frame = (player->current_frame + 1) % 5;
		//SDL_RenderCopy(renderer, player->walking_back[1], NULL, &rect);
		if (10 - player->still_frame - 1 >= 9) {
			
			player->state = falling;
			return NULL;
		}

		return player->uppercut[9 - player->still_frame--];
	}

	else if ((now - player->last_update) > 30 && player->state == slashing) {
		player->hit_available = 0;
		player->rect_type = slash_rect;
		player->last_update = now;
	
		//player->current_frame = (player->current_frame + 1) % 5;
		//SDL_RenderCopy(renderer, player->walking_back[1], NULL, &rect);
		if (11 - player->still_frame - 1 >= 10) {
			player->state = falling;
			return NULL;
		}
		if (player->still_frame == 5) {
			player->hit_available = 1;
		}

		return player->slash[11 - player->still_frame--];
	}
	else if ((now - player->last_update) > 20 && player->state == hurt) {

	player->rect_type = hurt_rect_1;
	player->last_update = now;
	if (5 - player->still_frame >= 4) {
		player->still_frame = 0;
		player->using_move = 0;
		player->state = idle;
		return NULL;
	}
	return player->hurt[4 - player->still_frame--];

	}

	else if ((now - player->last_update) > 20 && player->state == double_kicking) {
	player->hit_available = 0;
	player->rect_type = double_kick_rect;
	player->last_update = now;
	if (25 - player->still_frame >= 24) {
		player->still_frame = 0;
		player->using_move = 0;
		player->state = idle;
		return NULL;
	}
	if (player->still_frame == 16 || player->still_frame == 5) 
		player->hit_available = 1;
	if (player->still_frame == 23 || player->still_frame == 16)
		Mix_PlayChannel(attack_channel_2, kick1_sfx, 0);
	
	if (player->still_frame > 15) {
		if (!(player->x < other_player->x + WIDTH_1 - 50))
			player->x -= 15;
	}
	return player->double_kick[24 - player->still_frame--];

	}
	else if ((now - player->last_update) > 70 && player->state == knocked_down) {
	player->rect_type = double_kick_rect;
	player->last_update = now;
	if (7 - player->still_frame >= 7) {
		player->still_frame = 0;
		player->using_move = 0;
		return NULL;
	}

	return player->knock_down[7 - player->still_frame--];

	}
	
	return NULL;

}

void process_input(struct playable *player, struct playable2 *other_player) {
	key_input = SDL_GetKeyboardState(NULL);
	//key_input[SDL_SCANCODE_M] &&

	if ( key_input[SDL_SCANCODE_J] && player->still_frame <= 0 && player->state != jumping && player->state != dynamic_punch && player->stamina > 180) {
		player->stamina -= 180;
		player->still_frame = 15;
		player->state = dynamic_punch;
		Mix_PlayChannel(attack_channel_1, dynamic_punch_sfx, 0);
		
	}
	else if (key_input[SDL_SCANCODE_D] &&  key_input[SDL_SCANCODE_L] && player->still_frame <= 0 ) {
		if (player->state == jumping) {
			player->still_frame = 6;
			player->state = jump_kicking;
		}

	}
	else if (key_input[SDL_SCANCODE_W] && key_input[SDL_SCANCODE_S] && player->still_frame <= 0 && player->still_frame <= 0 && player->state != jumping && player->state != mach_punch) {
		player->state = back_flipping;
		player->still_frame = 10;

	}
	else if (key_input[SDL_SCANCODE_L] && player->still_frame <= 0 && player->state != jumping)
	{
		Mix_PlayChannel(attack_channel_1, kick2_sfx, 0);
		player->still_frame = 9;
		player->state = kicking;
	}
	else if (key_input[SDL_SCANCODE_D] && player->still_frame <= 0 ) {
		if (player->state != falling && player->state != jumping)
			player->state = walking_forward;
		if(!(player->x + WIDTH_1 - 50  > other_player->x ))
			player->x += 4;
	}
	else if (key_input[SDL_SCANCODE_A] && player->still_frame <= 0) {
		if (player->state != falling && player->state != jumping)
			player->state = walking_backward;
		player->x -= 4;
	}

	else if (key_input[SDL_SCANCODE_W] && player->y == 400 && player->still_frame <= 0 && player->state != hurt) {
		player->state = jumping;
		player->dy = -11;
		player->using_move = 5;
	}
	else if (key_input[SDL_SCANCODE_S] && player->still_frame <= 0 && player->state != jumping && player->stamina >= 40)
		player->state = blocking;

	else if (key_input[SDL_SCANCODE_K] && player->still_frame <= 0 && player->state != jumping && player->state != mach_punch && player->stamina > 80){
		player->stamina -= 80;
		player->still_frame = 23;
		player->state = mach_punch;
		Mix_PlayChannel(attack_channel_1, mach_punch_sfx, 0);
	}


}
void process_input_p2(struct playable2 *player, struct playable *other_player) {
	key_input = SDL_GetKeyboardState(NULL);

	if (key_input[SDL_SCANCODE_RIGHT] && player->still_frame <= 0 && player->state != range_attack && player->state != falling) {
		if (player->state != jumping)
			player->state = walking_backward;
		player->x += 6;
	}
	else if (key_input[SDL_SCANCODE_KP_2] && player->still_frame <= 0 && player->state  == blocking && player->stamina > 50) {
		player->stamina -= 50;
		player->still_frame = 9;
		player->state = uppercutting;
	}
	else if (key_input[SDL_SCANCODE_LEFT] && player->still_frame <= 0 && player->state != range_attack && player->state != falling) {

		if (player->state != jumping)
			player->state = walking_forward;
		if(!(player->x < other_player->x + WIDTH_1 - 50))
			player->x -= 6;
	}
	
	else if (key_input[SDL_SCANCODE_UP] && player->y == 400 && INPUT_CONDITION_2) {
		player->state = jumping;
		player->dy = -12;
		player->using_move = 5;
	}
	
	else if (key_input[SDL_SCANCODE_DOWN] && player->still_frame <= 0 && INPUT_CONDITION_2 && player->stamina > 40){
		player->state = blocking;
		player->still_frame = 7;
	}
	
	else if (key_input[SDL_SCANCODE_KP_1] && player->still_frame <= 0 && INPUT_CONDITION_2&& player->stamina > 180) {
		player->stamina -= 180;
		player->still_frame = 9;
		player->current_frame = 0;
		player->state = range_attack;
		Mix_PlayChannel(attack_channel_2, flame_thrower_sfx, 0);
	}
	else if (key_input[SDL_SCANCODE_KP_2] && player->still_frame <= 0 && INPUT_CONDITION_2) {
		player->still_frame = 11;
		player->state = slashing;
		Mix_PlayChannel(attack_channel_2, slash_sfx, 0);
	}

	else if (key_input[SDL_SCANCODE_KP_3] && player->still_frame <= 0 && INPUT_CONDITION_2 && player->stamina > 20) {
		player->stamina -= 20;
		player->still_frame = 24;
		player->state = double_kicking;
	}

	

}

void toggle_debug(long now) {
	Uint8 *key_input = SDL_GetKeyboardState(NULL);
	if ((now - debug_time) > 200 && key_input[SDL_SCANCODE_P]) {
		if (debug)
			debug = 0;
		else
			debug = 1;
		debug_time = now;
	}
		
}
void init_breloom(struct playable *sprite){
	
	sprite->x = 0;
	sprite->y = 400;
	sprite->dy = 0;
	sprite->current_frame = 0;
	sprite->last_update = 0;
	sprite->state = idle;
	sprite->using_move = 0;
	sprite->still_frame = 0;
	sprite->rect_type = normal;
	sprite->hp = MAX_HP;
	sprite->stamina = 150;
	sprite->hit_available = 0;
	// init standing animation frame
	sprite->standing[0] = loadTexture("breloom\\0344.png");
	sprite->standing[1] = loadTexture("breloom\\0345.png");
	sprite->standing[2] = loadTexture("breloom\\0346.png");
	sprite->standing[3] = loadTexture("breloom\\0347.png");
	sprite->standing[4] = loadTexture("breloom\\0348.png");
	sprite->standing[5] = loadTexture("breloom\\0347.png");
	sprite->standing[6] = loadTexture("breloom\\0346.png");
	sprite->standing[7] = loadTexture("breloom\\0345.png");
	sprite->standing[8] = loadTexture("breloom\\0344.png");

	// init forward walk frame
	sprite->walking[0] = loadTexture("breloom\\0354.png");
	sprite->walking[1] = loadTexture("breloom\\0355.png");
	sprite->walking[2] = loadTexture("breloom\\0356.png");
	sprite->walking[3] = loadTexture("breloom\\0357.png");
	sprite->walking[4] = loadTexture("breloom\\0358.png");
	sprite->walking[5] = loadTexture("breloom\\0359.png");
	sprite->walking[6] = loadTexture("breloom\\0360.png");
	sprite->walking[7] = loadTexture("breloom\\0361.png");
	sprite->walking[8] = loadTexture("breloom\\0362.png");
	sprite->walking[9] = loadTexture("breloom\\0363.png");

	// init backwardward walk frame
	sprite->walking_back[0] = loadTexture("breloom\\0364.png");
	sprite->walking_back[1] = loadTexture("breloom\\0365.png");
	sprite->walking_back[2] = loadTexture("breloom\\0366.png");
	sprite->walking_back[3] = loadTexture("breloom\\0367.png");
	sprite->walking_back[4] = loadTexture("breloom\\0368.png");
	sprite->walking_back[5] = loadTexture("breloom\\0369.png");
	sprite->walking_back[6] = loadTexture("breloom\\0370.png");
	sprite->walking_back[7] = loadTexture("breloom\\0371.png");
	sprite->walking_back[8] = loadTexture("breloom\\0372.png");
	sprite->walking_back[9] = loadTexture("breloom\\0373.png");

	// init jump
	sprite->jump[0] = loadTexture("breloom\\0377.png");
	sprite->jump[1] = loadTexture("breloom\\0378.png");
	sprite->jump[2] = loadTexture("breloom\\0379.png");
	sprite->jump[3] = loadTexture("breloom\\0380.png");
	sprite->jump[4] = loadTexture("breloom\\0381.png");
	sprite->jump[5] = loadTexture("breloom\\0382.png");

	// init mach punch
	sprite->mach_punch[0] = loadTexture("breloom\\0618.png");
	sprite->mach_punch[1] = loadTexture("breloom\\0619.png");
	sprite->mach_punch[2] = loadTexture("breloom\\0620.png");
	sprite->mach_punch[3] = loadTexture("breloom\\0621.png");
	sprite->mach_punch[4] = loadTexture("breloom\\0622.png");
	sprite->mach_punch[5] = loadTexture("breloom\\0623.png");
	sprite->mach_punch[6] = loadTexture("breloom\\0624.png");
	sprite->mach_punch[7] = loadTexture("breloom\\0625.png");
	sprite->mach_punch[8] = loadTexture("breloom\\0626.png");
	sprite->mach_punch[9] = loadTexture("breloom\\0627.png");
	sprite->mach_punch[10] = loadTexture("breloom\\0628.png");
	sprite->mach_punch[11] = loadTexture("breloom\\0629.png");
	sprite->mach_punch[12] = loadTexture("breloom\\0630.png");
	sprite->mach_punch[13] = loadTexture("breloom\\0631.png");
	sprite->mach_punch[14] = loadTexture("breloom\\0632.png");
	sprite->mach_punch[15] = loadTexture("breloom\\0633.png");
	sprite->mach_punch[16] = loadTexture("breloom\\0634.png");
	sprite->mach_punch[17] = loadTexture("breloom\\0635.png");
	sprite->mach_punch[18] = loadTexture("breloom\\0636.png");
	sprite->mach_punch[19] = loadTexture("breloom\\0637.png");
	sprite->mach_punch[20] = loadTexture("breloom\\0638.png");
	sprite->mach_punch[21] = loadTexture("breloom\\0639.png");
	sprite->mach_punch[22] = loadTexture("breloom\\0640.png");


	sprite->guard = loadTexture("breloom\\0439.png");
	//dynamic punch
	sprite->dynamic_punch[0] = loadTexture("breloom\\0697.png");
	sprite->dynamic_punch[1] = loadTexture("breloom\\0698.png");
	sprite->dynamic_punch[2] = loadTexture("breloom\\0699.png");
	sprite->dynamic_punch[3] = loadTexture("breloom\\0700.png");
	sprite->dynamic_punch[4] = loadTexture("breloom\\0701.png");
	sprite->dynamic_punch[5] = loadTexture("breloom\\0702.png");
	sprite->dynamic_punch[6] = loadTexture("breloom\\0703.png");
	sprite->dynamic_punch[7] = loadTexture("breloom\\0704.png");
	sprite->dynamic_punch[8] = loadTexture("breloom\\0705.png");
	sprite->dynamic_punch[9] = loadTexture("breloom\\0706.png");
	sprite->dynamic_punch[10] = loadTexture("breloom\\0707.png");
	sprite->dynamic_punch[11] = loadTexture("breloom\\0708.png");
	sprite->dynamic_punch[12] = loadTexture("breloom\\0709.png");
	sprite->dynamic_punch[13] = loadTexture("breloom\\0710.png");
	sprite->dynamic_punch[14] = loadTexture("breloom\\0711.png");

	//kick
	sprite->kick[0] = loadTexture("breloom\\0587.png");
	sprite->kick[1] = loadTexture("breloom\\0588.png");
	sprite->kick[2] = loadTexture("breloom\\0589.png");
	sprite->kick[3] = loadTexture("breloom\\0590.png");
	sprite->kick[4] = loadTexture("breloom\\0591.png");
	sprite->kick[5] = loadTexture("breloom\\0592.png");
	sprite->kick[6] = loadTexture("breloom\\0593.png");
	sprite->kick[7] = loadTexture("breloom\\0594.png");
	sprite->kick[8] = loadTexture("breloom\\0595.png");


	//jump kick
	sprite->jump_kick[0] = loadTexture("breloom\\0606.png");
	sprite->jump_kick[1] = loadTexture("breloom\\0607.png");
	sprite->jump_kick[2] = loadTexture("breloom\\0608.png");
	sprite->jump_kick[3] = loadTexture("breloom\\0609.png");
	sprite->jump_kick[4] = loadTexture("breloom\\0610.png");
	sprite->jump_kick[5] = loadTexture("breloom\\0611.png");
	sprite->jump_kick[6] = loadTexture("breloom\\0612.png");

	// hurt 
	sprite->hurt[0] = loadTexture("breloom\\0421.png");
	sprite->hurt[1] = loadTexture("breloom\\0422.png");
	sprite->hurt[2] = loadTexture("breloom\\0423.png");
	sprite->hurt[3] = loadTexture("breloom\\0424.png");

	// knock  down
	sprite->knock_down[0] = loadTexture("breloom\\0460.png");
	sprite->knock_down[1] = loadTexture("breloom\\0461.png");
	sprite->knock_down[2] = loadTexture("breloom\\0462.png");
	sprite->knock_down[3] = loadTexture("breloom\\0471.png");
	sprite->knock_down[4] = loadTexture("breloom\\0472.png");
	sprite->knock_down[5] = loadTexture("breloom\\0477.png");
	sprite->knock_down[6] = loadTexture("breloom\\0478.png");

}
void init_combusken(struct playable2 *sprite) {

	sprite->x = 850;
	sprite->y = 400;
	sprite->dy = 0;
	sprite->current_frame = 0;
	sprite->last_update = 0;
	sprite->state = idle;
	sprite->using_move = 0;
	sprite->still_frame = 0;
	sprite->rect_type = normal;
	sprite->hp = MAX_HP;
	sprite->stamina = 150;
	sprite->hit_available = 0;
	// init standing frame
	sprite->standing[0] = loadTexture("combusken\\0344.png");
	sprite->standing[1] = loadTexture("combusken\\0345.png");
	sprite->standing[2] = loadTexture("combusken\\0346.png");
	sprite->standing[3] = loadTexture("combusken\\0347.png");
	sprite->standing[4] = loadTexture("combusken\\0348.png");
	sprite->standing[5] = loadTexture("combusken\\0347.png");
	sprite->standing[6] = loadTexture("combusken\\0346.png");
	sprite->standing[7] = loadTexture("combusken\\0345.png");
	sprite->standing[8] = loadTexture("combusken\\0344.png");

	// init forward walk frame
	sprite->walking[0] = loadTexture("combusken\\0358.png");
	sprite->walking[1] = loadTexture("combusken\\0359.png");
	sprite->walking[2] = loadTexture("combusken\\0360.png");
	sprite->walking[3] = loadTexture("combusken\\0361.png");
	sprite->walking[4] = loadTexture("combusken\\0362.png");
	sprite->walking[5] = loadTexture("combusken\\0363.png");
	sprite->walking[6] = loadTexture("combusken\\0364.png");
	sprite->walking[7] = loadTexture("combusken\\0365.png");
	sprite->walking[8] = loadTexture("combusken\\0366.png");
	sprite->walking[9] = loadTexture("combusken\\0367.png");

	// init backwardward walk frame
	sprite->walking_back[0] = loadTexture("combusken\\0377.png");
	sprite->walking_back[1] = loadTexture("combusken\\0369.png");
	sprite->walking_back[2] = loadTexture("combusken\\0370.png");
	sprite->walking_back[3] = loadTexture("combusken\\0371.png");
	sprite->walking_back[4] = loadTexture("combusken\\0372.png");
	sprite->walking_back[5] = loadTexture("combusken\\0373.png");
	sprite->walking_back[6] = loadTexture("combusken\\0374.png");
	sprite->walking_back[7] = loadTexture("combusken\\0375.png");
	sprite->walking_back[8] = loadTexture("combusken\\0376.png");
	sprite->walking_back[9] = loadTexture("combusken\\0377.png");

	// init jump
	sprite->jump[0] = loadTexture("combusken\\0379.png");
	sprite->jump[1] = loadTexture("combusken\\0380.png");
	sprite->jump[2] = loadTexture("combusken\\0381.png");
	sprite->jump[3] = loadTexture("combusken\\0382.png");
	sprite->jump[4] = loadTexture("combusken\\0383.png");
	sprite->jump[5] = loadTexture("combusken\\0384.png");

	sprite->guard[0] = loadTexture("combusken\\0403.png");
	sprite->guard[1] = loadTexture("combusken\\0404.png");
	sprite->guard[2] = loadTexture("combusken\\0405.png");
	sprite->guard[3] = loadTexture("combusken\\0406.png");
	sprite->guard[4] = loadTexture("combusken\\0407.png");
	sprite->guard[5] = loadTexture("combusken\\0408.png");
	sprite->guard[6] = loadTexture("combusken\\0409.png");
	// flame thrower 
	sprite->range[4] = loadTexture("combusken\\0661.png");
	sprite->range[5] = loadTexture("combusken\\0662.png");

	sprite->range[0] = loadTexture("combusken\\0657.png");
	sprite->range[1] = loadTexture("combusken\\0658.png");
	sprite->range[2] = loadTexture("combusken\\0659.png");
	sprite->range[3] = loadTexture("combusken\\0660.png");

	// init sky uppercut
	sprite->uppercut[0] = loadTexture("combusken\\0509.png");
	sprite->uppercut[1] = loadTexture("combusken\\0510.png");
	sprite->uppercut[2] = loadTexture("combusken\\0511.png");
	sprite->uppercut[3] = loadTexture("combusken\\0512.png");
	sprite->uppercut[4] = loadTexture("combusken\\0513.png");
	sprite->uppercut[5] = loadTexture("combusken\\0514.png");
	sprite->uppercut[6] = loadTexture("combusken\\0515.png");
	sprite->uppercut[7] = loadTexture("combusken\\0516.png");
	sprite->uppercut[8] = loadTexture("combusken\\0517.png");

	//init slash
	sprite->slash[0] = loadTexture("combusken\\0594.png");
	sprite->slash[1] = loadTexture("combusken\\0595.png");
	sprite->slash[2] = loadTexture("combusken\\0596.png");
	sprite->slash[3] = loadTexture("combusken\\0597.png");
	sprite->slash[4] = loadTexture("combusken\\0598.png");
	sprite->slash[5] = loadTexture("combusken\\0599.png");
	sprite->slash[6] = loadTexture("combusken\\0600.png");
	sprite->slash[7] = loadTexture("combusken\\0601.png");
	sprite->slash[8] = loadTexture("combusken\\0602.png");
	sprite->slash[9] = loadTexture("combusken\\0603.png");
	sprite->slash[10] = loadTexture("combusken\\0604.png");
	sprite->slash[11] = loadTexture("combusken\\0605.png");

	// hurt 
	sprite->hurt[0] = loadTexture("combusken\\0436.png");
	sprite->hurt[1] = loadTexture("combusken\\0437.png");
	sprite->hurt[2] = loadTexture("combusken\\0438.png");
	sprite->hurt[3] = loadTexture("combusken\\0439.png");
	//double kick

	sprite->double_kick[0] = loadTexture("combusken\\0605.png");
	sprite->double_kick[1] = loadTexture("combusken\\0606.png");
	sprite->double_kick[2] = loadTexture("combusken\\0607.png");
	sprite->double_kick[3] = loadTexture("combusken\\0608.png");
	sprite->double_kick[4] = loadTexture("combusken\\0609.png");
	sprite->double_kick[5] = loadTexture("combusken\\0610.png");
	sprite->double_kick[6] = loadTexture("combusken\\0611.png");
	sprite->double_kick[7] = loadTexture("combusken\\0612.png");
	sprite->double_kick[8] = loadTexture("combusken\\0613.png");
	sprite->double_kick[9] = loadTexture("combusken\\0614.png");
	sprite->double_kick[10] = loadTexture("combusken\\0615.png");
	sprite->double_kick[11] = loadTexture("combusken\\0616.png");
	sprite->double_kick[12] = loadTexture("combusken\\0617.png");
	sprite->double_kick[13] = loadTexture("combusken\\0618.png");
	sprite->double_kick[14] = loadTexture("combusken\\0619.png");
	sprite->double_kick[15] = loadTexture("combusken\\0620.png");
	sprite->double_kick[16] = loadTexture("combusken\\0621.png");
	sprite->double_kick[17] = loadTexture("combusken\\0622.png");
	sprite->double_kick[18] = loadTexture("combusken\\0623.png");
	sprite->double_kick[19] = loadTexture("combusken\\0624.png");
	sprite->double_kick[20] = loadTexture("combusken\\0625.png");
	sprite->double_kick[21] = loadTexture("combusken\\0626.png");
	sprite->double_kick[22] = loadTexture("combusken\\0627.png");
	sprite->double_kick[23] = loadTexture("combusken\\0628.png");

	// knock  down
	sprite->knock_down[0] = loadTexture("combusken\\0460.png");
	sprite->knock_down[1] = loadTexture("combusken\\0461.png");
	sprite->knock_down[2] = loadTexture("combusken\\0462.png");
	sprite->knock_down[3] = loadTexture("combusken\\0491.png");
	sprite->knock_down[4] = loadTexture("combusken\\0492.png");
	sprite->knock_down[5] = loadTexture("combusken\\0493.png");
	sprite->knock_down[6] = loadTexture("combusken\\0494.png");

}

void get_rect_p1(struct playable *player) {
	switch (player->rect_type)
	{
	case normal:
		rect.x = player->x;
		rect.y = player->y;
		rect.w = WIDTH_1;
		rect.h = HEIGHT_1;
		break;
	case DynamicPunch:
		rect.x = player->x - 5;
		rect.y = player->y - 80;
		rect.w = WIDTH_DP;
		rect.h = HEIGHT_DP;
		break;
	case kick_rect:
		rect.x = player->x;
		rect.y = player->y - 10;
		rect.w = 222;
		rect.h = 180;
		break;
	case hurt_rect_1:
		rect.x = player->x - 50;
		rect.y = player->y - 50;
		rect.w = 300;
		rect.h = 260;
		break;
	case wide:
		rect.x = player->x - 35;
		rect.y = player->y;
		rect.w = WIDTH_MACH;
		rect.h = HEIGHT_MACH;
	default:
		break;
	}
}
void get_rect_p2(struct playable2 *player) {
	switch (player->rect_type)
	{
	case normal:
		rect_p2.x = player->x;
		rect_p2.y = player->y;
		rect_p2.w = WIDTH_1_p2;
		rect_p2.h = HEIGHT_1_p2;
		break;
	case slash_rect:
		rect_p2.x = player->x - 50;
		rect_p2.y = player->y;
		rect_p2.w = WIDTH_3_p2;
		rect_p2.h = HEIGHT_3_p2;
		break;
	case low_rect:
		rect_p2.x = player->x;
		rect_p2.y = player->y + 50;
		rect_p2.w = 130;
		rect_p2.h = 120;
		break;
	case hurt_rect_1:
		rect_p2.x = player->x - 30;
		rect_p2.y = player->y - 10;
		rect_p2.w = WIDTH_2_p2 + 30;
		rect_p2.h = HEIGHT_2_p2 + 30;
		break;
	case double_kick_rect:
		rect_p2.x = player->x - 65;
		rect_p2.y = player->y - 20;
		rect_p2.w = WIDTH_3_p2 + 20;
		rect_p2.h = HEIGHT_3_p2 + 20;
		break;
	default:
		rect_p2.x = player->x - 20;
		rect_p2.y = player->y;
		rect_p2.w = WIDTH_2_p2;
		rect_p2.h = HEIGHT_2_p2;
		break;
	}
}

void update_hurtbox_breloom(struct playable *player) {
	if (player->state == knocked_down) {
		player->hurtbox.x = 0;
		player->hurtbox.y = 0;
		player->hurtbox.w = 0;
		player->hurtbox.h = 0;
	}
	else {
		player->hurtbox.x = player->x + 20;
		player->hurtbox.y = player->y;
		player->hurtbox.w = WIDTH_1 - 30;
		player->hurtbox.h = HEIGHT_1;
	}
}
void update_hurtbox_combusken(struct playable2 *player) {
	if (player->state == knocked_down) {
		player->hurtbox.x = 0;
		player->hurtbox.y = 0;

		player->hurtbox.w = 0;
		player->hurtbox.h = 0;
	}
	else {
		player->hurtbox.x = player->x + 20;
		player->hurtbox.y = player->y;

		player->hurtbox.w = WIDTH_1_p2 - 20;
		player->hurtbox.h = HEIGHT_1_p2;
	}
}
void update_hitbox_breloom(struct playable *player) {
	if (player->state == mach_punch) {
		player->hitbox.x = player->x + 20;
		player->hitbox.y = player->y + 60;

		player->hitbox.w = WIDTH_MACH - 50;
		player->hitbox.h = HEIGHT_MACH - 120;
	}
	else if (player->state == dynamic_punch) {
		player->hitbox.x = player->x + 20;
		player->hitbox.y = player->y - 30;

		player->hitbox.w = WIDTH_DP - 90;
		player->hitbox.h = HEIGHT_DP - 70;


	}
	else if (player->state == kicking) {
		player->hitbox.x = player->x + 120;
		player->hitbox.y = player->y + 40;

		player->hitbox.w = 100;
		player->hitbox.h = 100;
	}
	else if (player->state == jump_kicking) {
		player->hitbox.x = player->x + 80;
		player->hitbox.y = player->y + 70;

		player->hitbox.w = 100;
		player->hitbox.h = 100;
	}
	else {
		player->hitbox.x = player->x;
		player->hitbox.y = player->y;

		player->hitbox.w = 0;
		player->hitbox.h = 0;

	}

}
void update_hitbox_combusken(struct playable2 *player) {
	if (player->state == range_attack) {
		player->hitbox = flame_rect;
	}
	else if (player->state == uppercutting) {
		player->hitbox.x = player->x - 20;
		player->hitbox.y = player->y;
		player->hitbox.w = 100;
		player->hitbox.h = 100;
	}
	else if (player->state == slashing) {
		player->hitbox.x = player->x -40;
		player->hitbox.y = player->y;
		player->hitbox.w = 100;
		player->hitbox.h = 100;
	}
	else if (player->state == double_kicking) {
		player->hitbox.x = player->x - 30;
		player->hitbox.y = player->y;
		player->hitbox.w = 100;
		player->hitbox.h = 100;
	}
	else {
		player->hitbox.x = player->x;
		player->hitbox.y = player->y;
		player->hitbox.w = 0;
		player->hitbox.h = 0;

	}
	

}
void check_hit(struct playable *player, struct playable2 *other_player) {
	if (check_collision(player->hitbox, other_player->hurtbox) && player->hit_available) {
		if (player->state == knocked_down || other_player->state == knocked_down) {
			printf("return");
			return;
		}
		if (other_player->state != blocking) {
			if (player->state == mach_punch) {
				other_player->hp -= 4 * DAMAGE_MULTIPLIER;
				Mix_PlayChannel(hit_channel_1, hit_hard, 0);
				other_player->x += 10;
				other_player->y += -20;
			}

			else if (player->state == dynamic_punch) {
				other_player->hp -= 4 * DAMAGE_MULTIPLIER;
				other_player->x += 2;
				Mix_PlayChannel(hit_channel_1, hit_medium, 0);
			}
			else if (player->state == jump_kicking) {
				other_player->hp -= 6 * DAMAGE_MULTIPLIER;
				Mix_PlayChannel(hit_channel_1, hit_hard, 0);
				other_player->x += 35;
			}
			else if (player->state == kicking) {
				other_player->hp -= 5 * DAMAGE_MULTIPLIER;
				Mix_PlayChannel(hit_channel_1, hit_medium, 0);
				other_player->x += 25;
				other_player->dy =  -5;
			}
			player->hit_available = 0;
			other_player->state = hurt;
			other_player->still_frame = 4;
			if (player->state == mach_punch) {
				other_player->dy = -3;
				other_player->x += 25;
			}
		}
		else {
			other_player->x += 3;
			player->hit_available = 0;
			Mix_PlayChannel(-1, guard_sfx, 0);
			other_player->stamina -= 50;
		
		}

	}
	if (check_collision(other_player->hitbox, player->hurtbox) && other_player->hit_available) {
		if (player->state != blocking) {
			if (other_player->state == range_attack) {
				player->hp -= 12 * DAMAGE_MULTIPLIER;
				Mix_PlayChannel(hit_channel_2, hit_hard, 0);
			}
			else if (other_player->state == slashing) {
				player->hp -= 3 * DAMAGE_MULTIPLIER;
				Mix_PlayChannel(hit_channel_2, hit_medium, 0);
			}
			else if (other_player->state == uppercutting) {
				player->hp -=  6 * DAMAGE_MULTIPLIER;
				Mix_PlayChannel(hit_channel_2, hit_medium, 0);
			}
			else if (other_player->state == kicking)
				player->hp -= 6 * DAMAGE_MULTIPLIER;
			else if (other_player->state == double_kicking) {
				player->hp -= 6 * DAMAGE_MULTIPLIER;
				Mix_PlayChannel(hit_channel_2, hit_hard, 0);
				player->x -= 8;
			}
			other_player->hit_available = 0;
			player->state = hurt;
			player->still_frame = 4;
			
		}
		else {
			if (other_player->state == range_attack) {
				player->hp -= 5 * DAMAGE_MULTIPLIER;
				player->stamina -= 50;
			}

			other_player->hit_available = 0;
			player->x -= 3;
			player->stamina -= 40;
			Mix_PlayChannel(-1, guard_sfx, 0);
		}
		
	}
	hp_bar1.w = player->hp;
	hp_bar2.x = 580 + (MAX_HP - other_player->hp);
	hp_bar2.w = other_player->hp;

	stamina_bar1.w = player->stamina;
	stamina_bar2.x = 580 + (MAX_STAMINA - other_player->stamina);
	stamina_bar2.w = other_player->stamina;

	
}

int main()
{	
	init();
	struct playable player1;
	struct playable2 player2;
	struct resources asset_data;
	SDL_Event event;
	SDL_Texture *victory = loadTexture("combusken\\0494.png");
	unsigned int lastTime = 0, KO_play = 1, victory_play = 1;
	long now;
	int endscreen = 0;
	bool quit = false;

	init_breloom(&player1);
	init_combusken(&player2);
	init_assets(&asset_data);


	Mix_PlayChannel(music_channel, battle_music, 0);
	while (!quit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&event) != 0 && endscreen == 0)
		{
			//User requests quit
			if (event.type == SDL_QUIT)
			{
				quit = true;
			}
		}
		now = SDL_GetTicks();
		if(endscreen == 0) {
			update_render(&player1, &player2, now, &asset_data);
			if (player1.state != knocked_down && player2.state != knocked_down) {
				process_input_p2(&player2, &player1);
				process_input(&player1, &player2);
			}
			toggle_debug(now);
			check_hit(&player1, &player2);
		}
		//process_input(&player1, &player2);
		//process_input_p2(&player2, &player1);
		if (player1.hp <= 0 || player2.hp <= 0) {
		
			if (KO_play) {
				Mix_HaltChannel(music_channel);
				Mix_PlayChannel(-1, KO_sfx, 0);
				KO_play = 0;
				lastTime = now;
			if (player1.hp <= 0) {
				victory = loadTexture("assets\\win_combusken.jpg");
				player1.state = knocked_down;
				}
			else if (player2.hp <= 0) {
				victory = loadTexture("assets\\win_breloom.jpg");
				player2.state = knocked_down;
			}

			}
			if ((now - lastTime) > 4300)
				endscreen = 1;
		}
			
		if (endscreen == 1) {
			if (victory_play) {
				Mix_PlayChannel(-1, victory_sfx, 0);
				victory_play = 0;
				lastTime = now;
			}
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, victory, NULL, NULL);
			SDL_RenderPresent(renderer);
			if ((now - lastTime) > 25000)
				quit = true;
		}
	
	}
	
	return 0;
}
