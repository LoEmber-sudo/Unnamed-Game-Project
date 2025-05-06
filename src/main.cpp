#include <iostream>
#include "raylib.h"
#include "raylib-aseprite.h"
#include "imgui.h"
#include "rlImGui.h"
#include "imguiThemes.h"
#include "math.h"
#include "raymath.h"
typedef enum {
	TILE_GRASS,
	TILE_WATER,
	TILE_SAND,
	TILE_FOREST,
	TILE_MOUNTAIN,
	TILE_RIVER
} TileType;

class Player {
public:
	float x, y;
	int speed;

};

bool IsTileBlocked(TileType type) {
	return (type == TILE_WATER || type == TILE_RIVER || type == TILE_MOUNTAIN);
}
float map_scale = 1.0f;
float playerSpriteWidth = 32.0f;
float playerSpriteHeight = 32.0f;
Player player;
Vector2 position{ player.x,player.y };
bool collide_water = false;
int screen_width = 1920;
int screen_height = 1080;
#define TILE_WIDTH 16
#define TILE_HEIGHT 16
#define MAX_TEXTURES 1
bool isAttacking = false;
int attackFrameIndex = 0;
float attackTimer = 0.0f;
const float attackDuration = 0.5f;
float lastAttackTime = 0.0f;
const float attackCooldown = 0.2f;
int last_input = 3;
typedef enum {
	TEXTURE_TILEMAP = 0
} texture_asset;
#define WORLD_W 200//20 * TILE_WIDTH
#define WORLD_H 200//20 * TILE_HEIGHT
typedef struct {
	int x;
	int y;
	TileType type;
} sTile;
sTile world[WORLD_W][WORLD_H];
Texture2D textures[MAX_TEXTURES];
float SimpleHash(int x, int y) {
	int n = x * 374761393 + y * 668265263; // hash
	n = (n ^ (n >> 13)) * 1274126177;
	return ((n ^ (n >> 16)) & 0x7FFFFFFF) / 2147483647.0f;
}
float ValueNoise2D(int x, int y, float scale) { // THIS IS MATH I DO NOT UNDERSTAND DO NOT TOUCH IT
	float fx = x * scale;
	float fy = y * scale;

	int ix = (int)fx;
	int iy = (int)fy;

	float fx_frac = fx - ix;
	float fy_frac = fy - iy;

	// hash noise values
	float v1 = SimpleHash(ix, iy);
	float v2 = SimpleHash(ix + 1, iy);
	float v3 = SimpleHash(ix, iy + 1);
	float v4 = SimpleHash(ix + 1, iy + 1);

	// bilinear interpolation
	float i1 = Lerp(v1, v2, fx_frac);
	float i2 = Lerp(v3, v4, fx_frac);
	return Lerp(i1, i2, fy_frac);
}

void Render() {
	for (int i = 0; i < WORLD_H; i++) {
		for (int j = 0; j < WORLD_W; j++) {
			sTile tile = world[i][j];
			int texture_index_x;
			int texture_index_y;

			switch (tile.type) {
			case TILE_WATER: texture_index_x = 13; texture_index_y = 0; break;
			case TILE_SAND: texture_index_x = 12; texture_index_y = 0; break;
			case TILE_GRASS: texture_index_x = 14; texture_index_y = 0; break;
			}

			Rectangle src = { (float)(texture_index_x * TILE_WIDTH), (float)(texture_index_y * TILE_HEIGHT), TILE_WIDTH, TILE_HEIGHT };
			Rectangle dest = {
				(float)(tile.x * TILE_WIDTH),
				(float)(tile.y * TILE_HEIGHT),
				TILE_WIDTH,
				TILE_HEIGHT
			};
			Vector2 origin = { 0, 8 };
			DrawTexturePro(textures[TEXTURE_TILEMAP], src, dest, origin, 0.0f, WHITE);
		}
	}
}

void GenerateWorld() {
	float scale = 0.05f; // controls smoothness
	for (int y = 0; y < WORLD_H; y++) {
		for (int x = 0; x < WORLD_W; x++) {
			float noise = ValueNoise2D(x, y, scale);
			TileType type;
			if (noise < 0.3f) type = TILE_WATER;
			else if (noise < 0.7f) type = TILE_SAND;
			else if (noise < 0.9f) type = TILE_GRASS;

			world[y][x] = sTile{ x, y, type };
		}
	}
}
sTile* GetTileAtPosition(float x, float y) {
	int tileX = (int)(x / (TILE_WIDTH));
	int tileY = (int)(y / (TILE_HEIGHT));

	if (tileX >= 0 && tileX < WORLD_W && tileY >= 0 && tileY < WORLD_H) {
		return &world[tileY][tileX];
	}
	return nullptr;
}

void AddRivers() {
	for (int y = 0; y < WORLD_H; y++) {
		int riverX = WORLD_W / 2 + (int)(5 * sin(y * 0.1)); // wavy river
		world[y][riverX].type = TILE_RIVER;
		if (riverX + 1 < WORLD_W) world[y][riverX + 1].type = TILE_RIVER;
	}
}
void Begin() {
	Image tilemap = LoadImage("resources/Tiles/Tilemap.png");
	textures[TEXTURE_TILEMAP] = LoadTextureFromImage(tilemap);
	UnloadImage(tilemap);
}

int main(void)
{

	Color transparent = { 0,0,0,0 };
	Color green = { 20, 160, 133, 255 }; //R,G,B,A <-- sorrend
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(screen_width, screen_height, "GAME");
	SetTargetFPS(60);
	player.x = screen_width / 2;
	player.y = screen_height / 2;
	player.speed = 2;
	Aseprite test = LoadAseprite("resources/Player/Player.aseprite");
	AsepriteTag walkS = LoadAsepriteTag(test, "walkS");
	AsepriteTag walkW = LoadAsepriteTag(test, "walkW");
	AsepriteTag walkD = LoadAsepriteTag(test, "walkD");
	AsepriteTag walkA = LoadAsepriteTag(test, "walkA");
	AsepriteTag hitS = LoadAsepriteTag(test, "hitS");
	AsepriteTag hitW = LoadAsepriteTag(test, "hitW");
	AsepriteTag hitD = LoadAsepriteTag(test, "hitD");
	AsepriteTag hitA = LoadAsepriteTag(test, "hitA");
	AsepriteTag idleS = LoadAsepriteTag(test, "idleS");
	AsepriteTag* currentMovement = &walkS;
	SetAsepriteTagFrame(currentMovement, 0);
	currentMovement->paused = true;
	currentMovement->loop = true;
	AsepriteTag* currentAttack = &idleS;
	AsepriteTag* idle = &idleS;
	position.x = 300;
	position.y = 300;
	int a = 0; int b = 0;
	Camera2D camera;
	camera.target = Vector2{ player.x,player.y };
	camera.offset = Vector2{ (float)screen_width / 2, (float)screen_height / 2 };
	camera.rotation = 0.0f;
	camera.zoom = 3.0f;
#pragma region imgui
	rlImGuiSetup(true);

	//you can use whatever imgui theme you like!
	//ImGui::StyleColorsDark();
	//imguiThemes::yellow();
	//imguiThemes::gray();
	//imguiThemes::green();
	//imguiThemes::red();
	//imguiThemes::embraceTheDarkness();


	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.FontGlobalScale = 4;

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.Colors[ImGuiCol_WindowBg].w = 0.5f;
	}

#pragma endregion


	Begin();
	GenerateWorld();
	while (!WindowShouldClose())
	{
		ClearBackground(RAYWHITE);

		BeginDrawing();
		{
			BeginMode2D(camera);
			Render();
			camera.target = Vector2{
	position.x + playerSpriteWidth / 2.0f,
	position.y + playerSpriteHeight / 2.0f
			};
			// Update attack timer
			// Clamp camera to map bounds

			float cam_min_x = screen_width / 2 / camera.zoom;
			float cam_max_x = WORLD_W * TILE_WIDTH * map_scale - screen_width / 2 / camera.zoom;
			float cam_min_y = screen_height / 2 / camera.zoom;
			float cam_max_y = WORLD_H * TILE_HEIGHT * map_scale - screen_height / 2 / camera.zoom;

			camera.target.x = Clamp(camera.target.x, cam_min_x, cam_max_x);
			camera.target.y = Clamp(camera.target.y, cam_min_y, cam_max_y);
			float nextX = position.x;
			float nextY = position.y;
			// 1. Draw the tile map first
			/*
			*/
			// 2. Handle movement input
			bool isMoving = false;
			if (IsKeyDown(KEY_W)) {
				idle->paused = true;
				if (currentMovement != &walkW) SetAsepriteTagFrame(&walkW, 0);
				currentMovement = &walkW;
				currentMovement->paused = false;
				currentMovement->loop = true;
				isMoving = true;
				nextY -= player.speed;
			}
			else if (IsKeyDown(KEY_D)) {
				idle->paused = true;
				if (currentMovement != &walkD) SetAsepriteTagFrame(&walkD, 0);
				currentMovement = &walkD;
				currentMovement->paused = false;
				currentMovement->loop = true;
				isMoving = true;
				nextX += player.speed;
			}
			else if (IsKeyDown(KEY_S)) {
				idle->paused = true;
				if (currentMovement != &walkS) SetAsepriteTagFrame(&walkS, 0);
				currentMovement = &walkS;
				currentMovement->paused = false;
				currentMovement->loop = true;
				isMoving = true;
				nextY += player.speed;
			}
			else if (IsKeyDown(KEY_A)) {
				idle->paused = true;
				if (currentMovement != &walkA) SetAsepriteTagFrame(&walkA, 0);
				currentMovement = &walkA;
				currentMovement->paused = false;
				currentMovement->loop = true;
				isMoving = true;
				nextX -= player.speed;
			}



			bool attackJustStarted = false;

			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !isAttacking) {
				Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), camera);
				Vector2 toMouse = Vector2Subtract(mousePos, Vector2{ position.x + playerSpriteWidth / 2, position.y + playerSpriteHeight / 2 });
				float angle = atan2f(toMouse.y, toMouse.x) * RAD2DEG;

				if (angle >= -45 && angle < 45) {
					currentAttack = &hitD;
				}
				else if (angle >= 45 && angle < 135) {
					currentAttack = &hitS;
				}
				else if (angle >= -135 && angle < -45) {
					currentAttack = &hitW;
				}
				else {
					currentAttack = &hitA;
				}

				currentMovement->paused = true;
				currentAttack->paused = false;
				currentAttack->loop = false;

				SetAsepriteTagFrame(currentAttack, 0); // start from beginning
				isAttacking = true;
			}
			if (isAttacking) {
				UpdateAsepriteTag(currentAttack);
				if (currentAttack->currentFrame >= currentAttack->tag->to_frame) {
					isAttacking = false;
					currentAttack->paused = true;
					if (position.x != nextX && position.y != nextY) {
						isMoving = false;
					}
					if (isMoving) {
						currentMovement->paused = false;
					}
					else if (!isMoving && !isAttacking){
						currentMovement->paused = true;
						idle->paused = false;
						idle->loop = true;
					}
				}
			}
			if (!IsKeyDown(KEY_W) && !IsKeyDown(KEY_A) && !IsKeyDown(KEY_S) && !IsKeyDown(KEY_D)) {
				currentMovement->paused = true;
			}


			sTile* tile = GetTileAtPosition(nextX + playerSpriteWidth / 2, nextY + playerSpriteHeight);
			if (tile && !IsTileBlocked(tile->type)) {
				position.x = nextX;
				position.y = nextY;
			}
			if (currentAttack->currentFrame >= currentAttack->tag->to_frame) {
				isAttacking = false;
				currentAttack->paused = true;
				currentMovement->paused = false;
			}

			float playerSpriteWidth = 32.0f;
			float playerSpriteHeight = 32.0f;
			if (!currentAttack->paused) {
				DrawAsepriteTagEx(*currentAttack, position, 0, 1.0f, WHITE);
			}
			if (!isMoving && !isAttacking) {
				currentMovement->paused = true;
				DrawAsepriteTagEx(*idle, position, 0, 1.0f, WHITE);
				UpdateAsepriteTag(idle);
			}
			if (!currentMovement->paused && currentAttack->paused == true) {
				DrawAsepriteTagEx(*currentMovement, position, 0, 1.0f, WHITE);
				UpdateAsepriteTag(currentMovement);
			}

			// 4. HUD or UI
			const char* text = "Use WASD keys to walk";
			DrawText(text, GetScreenWidth() / 2 - MeasureText(text, 20) / 2, GetScreenHeight() - 80, 20, WHITE);

			// 5. Manage collision boxes for player

			EndMode2D();
		}
#pragma region imgui
		rlImGuiBegin();

		ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {});
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
		ImGui::PopStyleColor(2);
#pragma endregion


#pragma region imgui
		rlImGuiEnd();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
#pragma endregion

		EndDrawing();
	}


#pragma region imgui
	rlImGuiShutdown();
#pragma endregion

	UnloadAseprite(test);
	CloseWindow();

	return 0;
}