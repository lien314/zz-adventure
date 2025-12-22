#pragma once

#define MAX_WIDTH  100
#define MAX_HEIGHT 100

typedef struct GameState GameState;


// 路径相关
// 获取当前可执行文件目录（用于定位 maps、save 等文件）
const char* get_exe_dir(void);
// 获取相对地图文件的完整路径（基于 exe 目录）
const char* get_map_filepath(const char* filename);
// 获取某关卡对应的存档文件名（save_<mapFile>.dat）
const char* save_filename(const char* mapFile);
// 列出 maps 目录下的所有地图文件（用于调试/打印）
void list_map_files(void);

// 存档相关
// has_save: 检查指定地图是否存在存档，若 infoBuf 非空则写入简短信息（时间/模式/进度）
int has_save(const char* mapFile, char* infoBuf, size_t bufSize);
// 删除对应地图的存档文件
int delete_save_file(const char* mapFile);
// 从存档加载并直接运行（恢复地图快照、玩家位置、模式等）
int load_and_run_save(const char* mapFile, const char* playerName);
// 统计地图文件中宝箱（值为 3）的总数，用于在界面显示进度
int get_map_treasure_count(const char* mapFile);
// 将游戏状态写入存档（供内部使用，外部通常使用 save_snapshot）
int game_save(GameState* gs, const char* mapFile);

// 游戏状态相关
// 从地图缓冲区创建游戏状态对象
GameState* game_create_from_buffer(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height);
// 释放 GameState
void game_destroy(GameState* gs);
// 撤销一步（返回 1 成功）
int game_undo(GameState* gs);
// 重做一步（返回 1 成功）
int game_redo(GameState* gs);
// 使用单个字符执行一次移动/操作；返回：0=继续，1=通关，2=退出保存
int game_apply_move(GameState* gs, char mv);

// 地图相关
// 从地图文件加载并进入游戏，mode: 0 实时模式，1 编程模式；playerName 可用于排行榜记录
void maps_from_file(const char* mapFile, int mode, const char* playerName);
// 从内存缓冲区启动地图（内部使用），提供起始坐标和 playerName
void maps_from_buffer(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height, int mode, int startx, int starty, const char* mapFile, const char* playerName);
/* 显示指定关卡的排行榜（按消耗体力升序） */
void show_leaderboard_for_map(const char* mapFile);
// 直接以 mapFile 名称加载地图（兼容旧 API）
void maps(const char* mapFile, int mode);




