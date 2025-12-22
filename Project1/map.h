#pragma once

#define MAX_WIDTH  100
#define MAX_HEIGHT 100

typedef struct GameState GameState;


// 路径相关
const char* get_exe_dir(void);
const char* get_map_filepath(const char* filename);
const char* save_filename(const char* mapFile);
void list_map_files(void);

// 存档相关
int has_save(const char* mapFile, char* infoBuf, size_t bufSize);
int delete_save_file(const char* mapFile);
int load_and_run_save(const char* mapFile, const char* playerName);
int get_map_treasure_count(const char* mapFile);
int game_save(GameState* gs, const char* mapFile);

// 游戏状态相关
GameState* game_create_from_buffer(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height);
void game_destroy(GameState* gs);
int game_undo(GameState* gs);
int game_redo(GameState* gs);
int game_apply_move(GameState* gs, char mv);

// 地图相关
void maps_from_file(const char* mapFile, int mode, const char* playerName);
void maps_from_buffer(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height, int mode, int startx, int starty, const char* mapFile, const char* playerName);
/* show leaderboard for a given map file (prints sorted by consume_HP asc) */
void show_leaderboard_for_map(const char* mapFile);
void maps(const char* mapFile, int mode);




