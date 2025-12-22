#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <windows.h>
//#include <shlwapi.h>
#include "map.h"
#define MAX_WIDTH  100
#define MAX_HEIGHT 100


const char* get_exe_dir() {
    static char exeDir[MAX_PATH];
    GetModuleFileNameA(NULL, exeDir, MAX_PATH);
   // PathRemoveFileSpecA(exeDir); // 去掉 exe 文件名，只保留目录
    char* last = strrchr(exeDir, '\\');
    if (last) {
        *last = '\0';
    }
    return exeDir;
}

const char* get_map_filepath(const char* filename) {
    static char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s\\\\%s", get_exe_dir(), filename);
    return fullpath;
}

const char* save_filename(const char* mapFile) {
    static char fname[MAX_PATH];
    snprintf(fname, sizeof(fname), "%s\\save_%s.dat", get_exe_dir(), mapFile);
    return fname;
}

/* save current snapshot to disk (consistent format) */
static int save_snapshot(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height,
    int mode, int playerX, int playerY, char *pathBuf, size_t pathSize, size_t pathIndex,
    const char* mapFile, int consume_HP, int step, int treasures_found, char underPlayer)
{
    char *sfname = (char*)save_filename(mapFile);
    FILE *sf = NULL;
    if (fopen_s(&sf, sfname, "wb") != 0 || !sf) return 0;
    long long ts = (long long)time(NULL);
    fwrite(&ts, sizeof(ts), 1, sf);
    fwrite(&mode, sizeof(mode), 1, sf);
    fwrite(&consume_HP, sizeof(consume_HP), 1, sf);
    fwrite(&step, sizeof(step), 1, sf);
    fwrite(&treasures_found, sizeof(treasures_found), 1, sf);
    fwrite(&width, sizeof(width), 1, sf);
    fwrite(&height, sizeof(height), 1, sf);
    fwrite(&playerX, sizeof(playerX), 1, sf);
    fwrite(&playerY, sizeof(playerY), 1, sf);
    /* write underPlayer passed in */
    fwrite(&underPlayer, sizeof(underPlayer), 1, sf);
    fwrite(&pathIndex, sizeof(pathIndex), 1, sf);
    /* write fixed-size path buffer */
    if (pathBuf && pathSize > 0) fwrite(pathBuf, 1, pathSize, sf);
    else {
        char z = 0; for (size_t i=0;i<1000;i++) fwrite(&z,1,1,sf);
    }
    /* write map snapshot: write underlying characters (replace P if present with under) */
    size_t msize = (size_t)width * (size_t)height;
    for (int yy=0; yy<height; yy++) {
        for (int xx=0; xx<width; xx++) {
            char ch = src[yy][xx];
            if (yy == playerY && xx == playerX) ch = underPlayer;
            fwrite(&ch, 1, 1, sf);
        }
    }
    fclose(sf);
    return 1;
}
/* 枚举 maps 文件夹下的所有地图文件，展示为菜单项 */
void list_map_files() {
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\maps\\*.map", get_exe_dir());

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(searchPath, &ffd);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("未找到 maps 文件夹或没有地图文件。\n");
        return;
    }

    int index = 1;
    do {
        if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            printf("关卡 %d: %s\n", index++, ffd.cFileName);
        }
    } while (FindNextFileA(hFind, &ffd));

    FindClose(hFind);
}

int has_save(const char* mapFile, char *infoBuf, size_t bufSize) {
    char *fname = save_filename(mapFile);
    FILE *f = NULL;
    if (fopen_s(&f, fname, "rb") != 0 || !f) return 0;
    long long ts = 0;
    int mode = 0, consume_HP = 0, step = 0, treasures_found = 0;
    int width=0,height=0;
    if (fread(&ts, sizeof(ts), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&mode, sizeof(mode), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&consume_HP, sizeof(consume_HP), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&step, sizeof(step), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&treasures_found, sizeof(treasures_found), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&width, sizeof(width), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&height, sizeof(height), 1, f) != 1) { fclose(f); return 0; }
    fclose(f);
    if (infoBuf && bufSize > 0) {
        time_t t = (time_t)ts;
        struct tm tmv;
        localtime_s(&tmv, &t);
        char timestr[64];
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tmv);
        snprintf(infoBuf, bufSize, "%s | 模式:%s 体力:%d 步数:%d 找到:%d 大小:%dx%d",
            timestr, mode==0?"实时":"编程", consume_HP, step, treasures_found, width, height);
    }
    return 1;
}

int delete_save_file(const char* mapFile) {
    char *fname = save_filename(mapFile);
    return (remove(fname)==0)?1:0 ;
}

int load_and_run_save(const char* mapFile, const char* playerName) {
    char *fname = save_filename(mapFile);
    FILE *f = NULL;
    if (fopen_s(&f, fname, "rb") != 0 || !f) return 0;
    long long ts = 0;
    int mode = 0, consume_HP = 0, step = 0, treasures_found = 0;
    int width=0,height=0;
    int playerX=0, playerY=0;
    char underPlayer = ' ';
    size_t pathSize = 1000;
    size_t pathIndex = 0;
    if (fread(&ts, sizeof(ts), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&mode, sizeof(mode), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&consume_HP, sizeof(consume_HP), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&step, sizeof(step), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&treasures_found, sizeof(treasures_found), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&width, sizeof(width), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&height, sizeof(height), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&playerX, sizeof(playerX), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&playerY, sizeof(playerY), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&underPlayer, sizeof(underPlayer), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&pathIndex, sizeof(pathIndex), 1, f) != 1) { fclose(f); return 0; }
    char pathBuf[1000] = {0};
    fread(pathBuf, 1, pathSize, f);
    size_t msize = (size_t)width * (size_t)height;
    char *mapSnap = (char*)malloc(msize ? msize : 1);
    if (!mapSnap) { fclose(f); return 0; }
    if (fread(mapSnap, 1, msize, f) != msize) { free(mapSnap); fclose(f); return 0; }
    fclose(f);
    char src[MAX_HEIGHT][MAX_WIDTH];
    for (int y=0;y<height && y<MAX_HEIGHT;y++) for (int x=0;x<width && x<MAX_WIDTH;x++) src[y][x] = mapSnap[y*width + x];
    free(mapSnap);
    /* no file-provided start coords when loading save snapshot */
    /* pass playerName so restored session can know who (mode is in file) */
    maps_from_buffer(src, width, height, mode, -1, -1, mapFile, playerName);
    return 1;
}

int get_map_treasure_count(const char* mapFile) {
    char filepath[MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s\\maps\\%s", get_exe_dir(), mapFile);
    FILE* f = NULL;
    if (fopen_s(&f, filepath, "r") != 0 || !f) return 0;
    int w,h;
    if (fscanf_s(f, "%d %d", &w, &h) != 2) { fclose(f); return 0; }
    int startx,starty;
    if (fscanf_s(f, "%d %d", &startx, &starty) != 2) { /* ignore */ }
    int treasures = 0;
    char buf[4096];
    for (int y=0;y<h && fgets(buf,sizeof(buf),f); y++) {
        char* ctx = NULL; char* tok = strtok_s(buf, " \t\r\n", &ctx);
        while (tok) { if (atoi(tok)==3) treasures++; tok = strtok_s(NULL, " \t\r\n", &ctx); }
    }
    fclose(f);
    return treasures;
}
void maps_from_file(const char* mapFile, int mode, const char* playerName) {
    char filepath[MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s\\maps\\%s", get_exe_dir(), mapFile);
    FILE* f = NULL;
    if (fopen_s(&f, filepath, "r") != 0 || !f) {
        printf("无法打开地图文件: %s\n", filepath);
        system("pause");
        return;
    }

    int w = 0, h = 0;
    /* read first non-empty line and parse width/height robustly (handles BOM) */
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        /* skip leading UTF-8 BOM if present */
        unsigned char *u = (unsigned char*)line;
        if (u[0] == 0xEF && u[1] == 0xBB && u[2] == 0xBF) {
            memmove(line, line+3, strlen(line)-2);
        }
        /* skip empty lines */
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == '\0') continue;
        if (sscanf_s(p, "%d %d", &w, &h) == 2) break;
    }
    if (w <= 0 || h <= 0) {
        fclose(f);
        printf("地图文件格式错误: 缺少或无效的宽高\n");
        system("pause");
        return;
    }

    int startx = 0, starty = 0;
    /* read next non-empty line as optional start coordinates */
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == '\0') continue;
        if (sscanf_s(p, "%d %d", &startx, &starty) == 2) break;
        /* if parsing failed, treat this line as the first map data line by rewinding file pointer */
        fseek(f, -((long)strlen(line)), SEEK_CUR);
        startx = 0; starty = 0;
        break;
    }

    char src[MAX_HEIGHT][MAX_WIDTH];
    for (int y = 0; y < h && y < MAX_HEIGHT; y++) {
        for (int x = 0; x < w && x < MAX_WIDTH; x++) src[y][x] = ' ';
    }

    char buf[4096];
    for (int y = 0; y < h && fgets(buf, sizeof(buf), f); y++) {
        int col = 0;
        char* ctx = NULL;
        char* tok = strtok_s(buf, " \t\r\n", &ctx);
        while (tok && col < w) {
            int v = atoi(tok);
            char ch = ' ';
            switch (v) {
            case 1: ch = 'W'; break;
            case 2: ch = 'D'; break;
            case 3: ch = 'T'; break;
            default: ch = ' '; break;
            }
            src[y][col++] = ch;
            tok = strtok_s(NULL, " \t\r\n", &ctx);
        }
        while (col < w) src[y][col++] = ' ';
    }

    /* close file and call maps_from_buffer passing start coords (do not modify src) */
    fclose(f);
    maps_from_buffer(src, w, h, mode, startx, starty, mapFile, playerName);
}

static int load_map_from_file(char* filename, char map[MAX_HEIGHT][MAX_WIDTH], int* width, int* height, int* treasures) {
    FILE* file = NULL;
    if (fopen_s(&file, filename, "r") != 0 || !file) {
        return 1;
    }

    for (int y = 0; y < MAX_HEIGHT; y++) {
        for (int x = 0; x < MAX_WIDTH; x++) {
            map[y][x] = ' ';
        }
    }

    *width = 0;
    *height = 0;
    *treasures = 0;

    char line[MAX_WIDTH + 2];
    while (fgets(line, sizeof(line), file) && *height < MAX_HEIGHT) {
        int len = 0;
        while (line[len] != '\n' && line[len] != '\0' && len < MAX_WIDTH) {
            map[*height][len] = line[len];
            if (line[len] == 'T') (*treasures)++;
            len++;
        }
        if (len > *width) {
            *width = len;
        }
        (*height)++;
    }
    fclose(file);
    return 0;
}

static void print_map(char map[MAX_HEIGHT][MAX_WIDTH], int width, int height) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    WORD defaultAttr = 0;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        defaultAttr = csbi.wAttributes;
    } else {
        defaultAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            char c = map[y][x];
            WORD attr = defaultAttr;
            switch (c) {
            case 'P': attr = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case 'T': attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case 'D': attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case 'W': attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            default: attr = defaultAttr; break;
            }
            SetConsoleTextAttribute(hConsole, attr);
            putchar(c);
            putchar(' ');
            SetConsoleTextAttribute(hConsole, defaultAttr);
        }
        putchar('\n');
    }
    SetConsoleTextAttribute(hConsole, defaultAttr);
}


typedef struct StateNode {
    char *mapSnapshot; /* width*height bytes */
    int playerX;
    int playerY;
    char underPlayer;
    int consume_HP;
    int step;
    int treasures_found;
    size_t pathIndex;
    char *pathBuf;
    struct StateNode *prev;
    struct StateNode *next;
} StateNode;

static StateNode* create_node_from_state(char map[MAX_HEIGHT][MAX_WIDTH], int width, int height,
    int playerX, int playerY, char underPlayer, int consume_HP, int step, int treasures_found,
    size_t pathIndex, char *path, size_t pathSize)
{
    StateNode *n = (StateNode*)malloc(sizeof(StateNode));
    if (!n) return NULL;
    size_t msize = (size_t)width * (size_t)height;
    if (msize == 0) msize = 1; /* ensure non-zero allocation to avoid malloc(0) */
    n->mapSnapshot = (char*)malloc(msize);
    n->pathBuf = (char*)malloc(pathSize);
    if (!n->mapSnapshot || !n->pathBuf) {
        free(n->mapSnapshot);
        free(n->pathBuf);
        free(n);
        return NULL;
    }
    for (int yy = 0; yy < height; yy++) {
        for (int xx = 0; xx < width; xx++) {
            size_t pos = (size_t)yy * (size_t)width + (size_t)xx;
            if (pos < msize) {
                if (yy == playerY && xx == playerX) n->mapSnapshot[pos] = underPlayer;
                else n->mapSnapshot[pos] = map[yy][xx];
            }
        }
    }
    n->playerX = playerX;
    n->playerY = playerY;
    n->underPlayer = underPlayer;
    n->consume_HP = consume_HP;
    n->step = step;
    n->treasures_found = treasures_found;
    n->pathIndex = pathIndex;
    memcpy(n->pathBuf, path, pathSize);
    n->prev = n->next = NULL;
    return n;
}

static void free_forward(StateNode *n) {
    StateNode *cur = n ? n->next : NULL;
    while (cur) {
        StateNode *tmp = cur->next;
        free(cur->mapSnapshot);
        free(cur->pathBuf);
        free(cur);
        cur = tmp;
    }
    if (n) n->next = NULL;
}

static void free_all(StateNode *n) {
    if (!n) return;
    while (n->prev) n = n->prev;
    StateNode *cur = n;
    while (cur) {
        StateNode *tmp = cur->next;
        free(cur->mapSnapshot);
        free(cur->pathBuf);
        free(cur);
        cur = tmp;
    }
}

/* --- Game logic abstraction (no I/O) --- */
typedef struct GameState {
    char map[MAX_HEIGHT][MAX_WIDTH];
    int width;
    int height;
    int playerX;
    int playerY;
    char underPlayer;
    int consume_HP;
    int step;
    int treasures_found;
    int treasures_total;
    char path[1000];
    size_t pathIndex;
    size_t pathSize;
    StateNode *current; /* undo/redo pointer */
} GameState;

static GameState* game_create_from_buffer(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height) {
    GameState *gs = (GameState*)malloc(sizeof(GameState));
    if (!gs) return NULL;
    gs->width = width;
    gs->height = height;
    /* ensure positive dimensions to avoid analyzer/UB */
    if (gs->width <= 0) gs->width = 1;
    if (gs->height <= 0) gs->height = 1;
    gs->consume_HP = 0;
    gs->step = 0;
    gs->treasures_found = 0;
    gs->treasures_total = 0;
    gs->pathSize = sizeof(gs->path);
    gs->pathIndex = 0;
    gs->path[0] = '\0';
    /* init map */
    for (int y=0;y<MAX_HEIGHT;y++) for (int x=0;x<MAX_WIDTH;x++) gs->map[y][x] = ' ';
    for (int y=0;y<height && y<MAX_HEIGHT;y++) {
        for (int x=0;x<width && x<MAX_WIDTH;x++) {
            gs->map[y][x] = src[y][x];
            if (gs->map[y][x] == 'T') gs->treasures_total++;
        }
    }
    gs->playerX = 1; gs->playerY = 1;
    if (gs->playerY < 0) gs->playerY = 0;
    if (gs->playerY >= gs->height) gs->playerY = gs->height - 1;
    if (gs->playerX < 0) gs->playerX = 0;
    if (gs->playerX >= gs->width) gs->playerX = gs->width - 1;
    gs->underPlayer = gs->map[gs->playerY][gs->playerX];
    gs->map[gs->playerY][gs->playerX] = 'P';
    gs->current = create_node_from_state(gs->map, gs->width, gs->height, gs->playerX, gs->playerY,
        gs->underPlayer, gs->consume_HP, gs->step, gs->treasures_found, gs->pathIndex, gs->path, gs->pathSize);
    return gs;
}

static void game_destroy(GameState *gs) {
    if (!gs) return;
    free_all(gs->current);
    free(gs);
}

static int game_undo(GameState *gs) {
    if (!gs || !gs->current) return 0;
    if (gs->current->prev) {
        gs->current = gs->current->prev;
        for (int yy=0; yy<gs->height; yy++) for (int xx=0; xx<gs->width; xx++) gs->map[yy][xx] = gs->current->mapSnapshot[yy * gs->width + xx];
        gs->playerX = gs->current->playerX; gs->playerY = gs->current->playerY; gs->underPlayer = gs->current->underPlayer;
        gs->consume_HP = gs->current->consume_HP; gs->step = gs->current->step; gs->treasures_found = gs->current->treasures_found;
        gs->pathIndex = gs->current->pathIndex; memcpy(gs->path, gs->current->pathBuf, gs->pathSize);
        gs->map[gs->playerY][gs->playerX] = 'P';
        return 1;
    }
    return 0;
}

static int game_redo(GameState *gs) {
    if (!gs || !gs->current) return 0;
    if (gs->current->next) {
        gs->current = gs->current->next;
        for (int yy=0; yy<gs->height; yy++) for (int xx=0; xx<gs->width; xx++) gs->map[yy][xx] = gs->current->mapSnapshot[yy * gs->width + xx];
        gs->playerX = gs->current->playerX; gs->playerY = gs->current->playerY; gs->underPlayer = gs->current->underPlayer;
        gs->consume_HP = gs->current->consume_HP; gs->step = gs->current->step; gs->treasures_found = gs->current->treasures_found;
        gs->pathIndex = gs->current->pathIndex; memcpy(gs->path, gs->current->pathBuf, gs->pathSize);
        gs->map[gs->playerY][gs->playerX] = 'P';
        return 1;
    }
    return 0;
}

/* apply a single move; return codes: 0 = continue, 1 = won, 2 = quit */
static int game_apply_move(GameState* gs, char mv) {
    if (!gs) return 0;
    if (mv == 'q') return 2;
    if (mv == 'y') { game_undo(gs); return 0; }
    if (mv == 'z') { game_redo(gs); return 0; }
    if (mv != 'w' && mv != 'a' && mv != 's' && mv != 'd' && mv != 'i') return 0;

    /* use clamped current position for safe indexing */
    int curX = gs->playerX, curY = gs->playerY;
    clamp_to_bounds(gs, &curX, &curY);
    int nx = curX, ny = curY;
    if (mv == 'w') ny--;
    else if (mv == 's') ny++;
    else if (mv == 'a') nx--;
    else if (mv == 'd') nx++;

    if (mv == 'i') {
        if (gs->current) free_forward(gs->current);
        if (gs->pathIndex + 1 < gs->pathSize) { gs->path[gs->pathIndex++] = mv; gs->path[gs->pathIndex] = '\0'; }
        gs->consume_HP++; gs->step++;
        StateNode *n = create_node_from_state(gs->map, gs->width, gs->height, curX, curY, gs->underPlayer,
            gs->consume_HP, gs->step, gs->treasures_found, gs->pathIndex, gs->path, gs->pathSize);
        if (n) { n->prev = gs->current; if (gs->current) gs->current->next = n; gs->current = n; }
        return 0;
    }

    if (nx < 0 || nx >= gs->width || ny < 0 || ny >= gs->height) return 0;
    /* safe read using validated indices */
    int safe_nx = nx, safe_ny = ny;
    clamp_to_bounds(gs, &safe_nx, &safe_ny);
    char target = gs->map[safe_ny][safe_nx];
    if (target == 'W') return 0;

    if (gs->current) free_forward(gs->current);
    if (gs->pathIndex + 1 < gs->pathSize) { gs->path[gs->pathIndex++] = mv; gs->path[gs->pathIndex] = '\0'; }
    gs->consume_HP++; gs->step++;
    if (gs->underPlayer == 'D') gs->consume_HP++;

    /* write back using clamped previous pos and validated new pos */
    gs->map[curY][curX] = gs->underPlayer;
    gs->underPlayer = target; gs->playerX = nx; gs->playerY = ny;
    int newX = gs->playerX, newY = gs->playerY; clamp_to_bounds(gs, &newX, &newY);
    gs->map[newY][newX] = 'P';
    if (gs->underPlayer == 'T') { gs->treasures_found++; gs->underPlayer = ' '; }
    int dx[4] = {0,0,-1,1}, dy[4] = {-1,1,0,0};
    for (int k=0;k<4;k++){
        int tx = gs->playerX + dx[k], ty = gs->playerY + dy[k];
        if (tx>=0 && tx<gs->width && ty>=0 && ty<gs->height && gs->map[ty][tx]=='T') { gs->treasures_found++; gs->map[ty][tx] = ' '; }
    }

    StateNode *n = create_node_from_state(gs->map, gs->width, gs->height, gs->playerX, gs->playerY, gs->underPlayer,
        gs->consume_HP, gs->step, gs->treasures_found, gs->pathIndex, gs->path, gs->pathSize);
    if (n) { n->prev = gs->current; if (gs->current) gs->current->next = n; gs->current = n; }

    if (gs->treasures_found == gs->treasures_total) return 1;
    return 0;
}

static int game_save(GameState *gs, const char* mapFile) {
    if (!gs) return 0;
    char *sfname = save_filename(mapFile);
    FILE *sf = NULL;
    if (fopen_s(&sf, sfname, "wb") != 0 || !sf) return 0;
    long long ts = (long long)time(NULL);
    fwrite(&ts, sizeof(ts), 1, sf);
    int mode = 0; /* mode unknown here - UI should pass if needed; store 0 */
    fwrite(&mode, sizeof(mode), 1, sf);
    fwrite(&gs->consume_HP, sizeof(gs->consume_HP), 1, sf);
    fwrite(&gs->step, sizeof(gs->step), 1, sf);
    fwrite(&gs->treasures_found, sizeof(gs->treasures_found), 1, sf);
    fwrite(&gs->width, sizeof(gs->width), 1, sf);
    fwrite(&gs->height, sizeof(gs->height), 1, sf);
    fwrite(&gs->playerX, sizeof(gs->playerX), 1, sf);
    fwrite(&gs->playerY, sizeof(gs->playerY), 1, sf);
    fwrite(&gs->underPlayer, sizeof(gs->underPlayer), 1, sf);
    fwrite(&gs->pathIndex, sizeof(gs->pathIndex), 1, sf);
    fwrite(gs->path, 1, gs->pathSize, sf);
    for (int yy=0; yy<gs->height; yy++) for (int xx=0; xx<gs->width; xx++) {
        char ch = (yy==gs->playerY && xx==gs->playerX) ? gs->underPlayer : gs->map[yy][xx];
        fwrite(&ch,1,1,sf);
    }
    fclose(sf);
    return 1;
}

static void append_leaderboard_entry(const char* mapFile, const char* name, int consume_HP) {
    if (!mapFile || !name) return;
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\leaderboard_%s.txt", get_exe_dir(), mapFile);
    FILE *f = NULL;
    if (fopen_s(&f, path, "a") != 0 || !f) return;
    long long ts = (long long)time(NULL);
    fprintf(f, "%lld\t%s\t%d\n", ts, name, consume_HP);
    fclose(f);
}

void show_leaderboard_for_map(const char* mapFile) {
    if (!mapFile) return;
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\leaderboard_%s.txt", get_exe_dir(), mapFile);
    FILE *f = NULL;
    if (fopen_s(&f, path, "r") != 0 || !f) {
        printf("该关卡暂无排行榜记录。\n按回车返回。");
        getchar();
        return;
    }
    typedef struct { long long ts; char name[128]; int hp; } Entry;
    Entry entries[256]; int ec = 0;
    char line[512];
    while (fgets(line, sizeof(line), f) && ec < 256) {
        char *p = line;
        char *saveptr = NULL;
        char *tok = strtok_s(p, "\t\n\r", &saveptr);
        if (!tok) continue;
        entries[ec].ts = _atoi64(tok);
        tok = strtok_s(NULL, "\t\n\r", &saveptr);
        if (!tok) continue;
        strncpy_s(entries[ec].name, sizeof(entries[ec].name), tok, _TRUNCATE);
        tok = strtok_s(NULL, "\t\n\r", &saveptr);
        if (!tok) continue;
        entries[ec].hp = atoi(tok);
        ec++;
    }
    fclose(f);
    if (ec == 0) {
        printf("该关卡暂无排行榜记录。\n按回车返回。");
        getchar();
        return;
    }
    /* sort by hp ascending */
    for (int i = 0; i < ec; ++i) for (int j = i+1; j < ec; ++j) {
        if (entries[j].hp < entries[i].hp) {
            Entry t = entries[i]; entries[i] = entries[j]; entries[j] = t;
        }
    }
    printf("排行榜 - %s\n", mapFile);
    printf("排名\t挑战者\t消耗体力\n");
    int limit = ec < 50 ? ec : 50;
    for (int i = 0; i < limit; ++i) {
        printf("%d\t%s\t%d\n", i+1, entries[i].name, entries[i].hp);
    }
    printf("按回车返回。");
    getchar();
}

/* (previously had a forward declaration for an extracted run loop) */

void maps_from_buffer(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height, int mode, int startx, int starty, const char* mapFile, const char* playerName) {
    int consume_HP = 0;
    int step = 0;
    int treasures_found = 0;
    char path[1000] = {0};
    size_t pathSize = sizeof(path);
    size_t pathIndex = 0;

    char map[MAX_HEIGHT][MAX_WIDTH];
    int treasures = 0;
    int playerX = 1, playerY = 1;
    /* if file provided start coordinates are valid, use them */
    if (startx >= 0 && startx < width && starty >= 0 && starty < height) {
        playerX = startx;
        playerY = starty;
    }

    if (width <= 0 || height <= 0) {
        return;
    }

    for (int y = 0; y < MAX_HEIGHT; y++) {
        for (int x = 0; x < MAX_WIDTH; x++) {
            map[y][x] = ' ';
        }
    }
    for (int y = 0; y < height && y < MAX_HEIGHT; y++) {
        for (int x = 0; x < width && x < MAX_WIDTH; x++) {
            map[y][x] = src[y][x];
            if (map[y][x] == 'T') treasures++;
        }
    }

    char underPlayer = ' ';

    if (playerY < 0) playerY = 0;
    if (playerY >= height) playerY = height - 1;
    if (playerX < 0) playerX = 0;
    if (playerX >= width) playerX = width - 1;
    underPlayer = map[playerY][playerX];
    map[playerY][playerX] = 'P';

    pathIndex = 0;
    path[0] = '\0';

    /* use top-level StateNode and helper functions */
    StateNode *current = NULL;
    /* push initial state */
    current = create_node_from_state(map, width, height, playerX, playerY, underPlayer,
        consume_HP, step, treasures_found, pathIndex, path, pathSize);

    while (1) {
        system("cls");
        print_map(map, width, height);
        printf("体力消耗:%d\n", consume_HP);
        printf("控制方法：W 上, S 下, A 左, D 右, I 原地不动, Q 返回主菜单\n");

        if (mode == 0) {
            char ch = _getch();
            char mv = (char)tolower((unsigned char)ch);

            if (mv == 'q') {
                /* save progress to file */
                save_snapshot(map, width, height, mode, playerX, playerY, path, pathSize, pathIndex, mapFile, consume_HP, step, treasures_found, underPlayer);
                map[playerY][playerX] = underPlayer;
                free_all(current);
                return;
            }

            /* Undo (Y) */
            if (mv == 'y') {
                if (current && current->prev) {
                    current = current->prev;
                    /* restore state */
                    for (int yy = 0; yy < height; yy++) for (int xx = 0; xx < width; xx++) map[yy][xx] = current->mapSnapshot[yy * width + xx];
                    playerX = current->playerX;
                    playerY = current->playerY;
                    underPlayer = current->underPlayer;
                    consume_HP = current->consume_HP;
                    step = current->step;
                    treasures_found = current->treasures_found;
                    pathIndex = current->pathIndex;
                    memcpy(path, current->pathBuf, pathSize);
                    map[playerY][playerX] = 'P';
                }
                continue;
            }

            /* Redo (Z) */
            if (mv == 'z') {
                if (current && current->next) {
                    current = current->next;
                    for (int yy = 0; yy < height; yy++) for (int xx = 0; xx < width; xx++) map[yy][xx] = current->mapSnapshot[yy * width + xx];
                    playerX = current->playerX;
                    playerY = current->playerY;
                    underPlayer = current->underPlayer;
                    consume_HP = current->consume_HP;
                    step = current->step;
                    treasures_found = current->treasures_found;
                    pathIndex = current->pathIndex;
                    memcpy(path, current->pathBuf, pathSize);
                    map[playerY][playerX] = 'P';
                }
                continue;
            }

            if (mv != 'w' && mv != 'a' && mv != 's' && mv != 'd' && mv != 'i') {
                printf("输入错误，请使用 W/A/S/D/I 或 Q 返回。\n");
                system("pause");
                continue;
            }

            /* compute target for movement first */
            int nx = playerX, ny = playerY;
            if (mv == 'w') ny--;
            else if (mv == 's') ny++;
            else if (mv == 'a') nx--;
            else if (mv == 'd') nx++;

            /* For 'i' (stay), always consume and record */
            if (mv == 'i') {
                /* clear redo history when making a new action */
                if (current) free_forward(current);
                if (pathIndex + 1 < pathSize) {
                    path[pathIndex++] = mv;
                    path[pathIndex] = '\0';
                }
                consume_HP++;
                step++;
                /* push new state */
                StateNode *n = create_node_from_state(map, width, height, playerX, playerY, underPlayer,
                    consume_HP, step, treasures_found, pathIndex, path, pathSize);
                if (n) {
                    /* attach */
                    n->prev = current;
                    if (current) current->next = n;
                    current = n;
                }
                continue;
            }

            /* movement: check bounds */
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
                printf("撞墙了，很痛！（越界）\n");
                system("pause");
                continue;
            }

            char target = map[ny][nx];
            if (target == 'W') {
                printf("撞墙了，很痛！\n");
                system("pause");
                continue;
            }

            /* valid move: clear redo history */
            if (current) free_forward(current);

            /* record path and cost */
            if (pathIndex + 1 < pathSize) {
                path[pathIndex++] = mv;
                path[pathIndex] = '\0';
            }
            consume_HP++;
            step++;

            if (underPlayer == 'D') {
                consume_HP++;
            }

            map[playerY][playerX] = underPlayer;

            underPlayer = target;
            playerX = nx;
            playerY = ny;
            map[playerY][playerX] = 'P';

            if (underPlayer == 'T') {
                treasures_found++;
                underPlayer = ' ';
            }

            int dx[4] = { 0, 0, -1, 1 };
            int dy[4] = { -1, 1, 0, 0 };
            for (int k = 0; k < 4; k++) {
                int tx = playerX + dx[k];
                int ty = playerY + dy[k];
                if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
                    if (map[ty][tx] == 'T') {
                        treasures_found++;
                        map[ty][tx] = ' ';
                        printf("你发现了一个宝藏!\n");
                    }
                }
            }

            StateNode *n = create_node_from_state(map, width, height, playerX, playerY, underPlayer,
                consume_HP, step, treasures_found, pathIndex, path, pathSize);
            if (n) { n->prev = current; if (current) current->next = n; current = n; }

            if (treasures_found == treasures) {
                system("cls");
                print_map(map, width, height);

                char displayPath[1000];
                size_t di = 0;
                for (size_t i = 0; i < pathIndex && di + 1 < sizeof(displayPath); i++) {
                    char c = path[i];
                    char out = c;
                    if (c == 'w') out = 'U';
                    else if (c == 's') out = 'D';
                    else if (c == 'a') out = 'L';
                    else if (c == 'd') out = 'R';
                    else if (c == 'i') out = 'I';
                    else out = toupper((unsigned char)c);
                    displayPath[di++] = out;
                }
                displayPath[di] = '\0';

                printf("恭喜你，小猪找到了所有宝藏！\n行动路径：%s\n消耗的体力：%d\n找到的宝箱的数量：%d\n",
                    displayPath, consume_HP, treasures_found);
                if (playerName && playerName[0]) append_leaderboard_entry(mapFile, playerName, consume_HP);
                else append_leaderboard_entry(mapFile, "匿名", consume_HP);
                /* clear any existing save since challenge completed */
                delete_save_file(mapFile);
                system("pause");
                return;
            }
        }
        else if (mode == 1) {
            printf("请输入预设的移动路径（仅包含WASDI或Q字符）：");
            if (scanf_s("%999s", path, (unsigned)pathSize) != 1) {
                int c;
                while ((c = getchar()) != '\n' && c != EOF) {}
                printf("读取路径失败或为空。\n");
                system("pause");
                return;
            }

            pathIndex = strlen(path);
            if (pathIndex >= pathSize) pathIndex = pathSize - 1;
            path[pathIndex] = '\0';

            for (size_t idx = 0; idx < pathIndex; idx++) {
                char mv = (char)tolower((unsigned char)path[idx]);

                if (mv == 'q') {
                    /* save progress to file when player explicitly quits */
                    save_snapshot(map, width, height, mode, playerX, playerY, path, pathSize, pathIndex, mapFile, consume_HP, step, treasures_found, underPlayer);
                    map[playerY][playerX] = underPlayer;
                    free_all(current);
                    return;
                }

                if (mv != 'w' && mv != 'a' && mv != 's' && mv != 'd' && mv != 'i') {
                    printf("预设路径包含非法字符 '%c'，返回主菜单。\n", path[idx]);
                    system("pause");
                    return;
                }

                consume_HP++;
                step++;

                if (mv == 'i') {
                    continue;
                }

                int nx = playerX, ny = playerY;
                if (mv == 'w') ny--;
                else if (mv == 's') ny++;
                else if (mv == 'a') nx--;
                else if (mv == 'd') nx++;

                if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
                    continue;
                }

                char target = map[ny][nx];
                if (target == 'W') {
                    continue;
                }
                else {
                    if (underPlayer == 'D') {
                        consume_HP++;
                    }

                    map[playerY][playerX] = underPlayer;
                    underPlayer = target;
                    playerX = nx;
                    playerY = ny;
                    map[playerY][playerX] = 'P';

                    if (underPlayer == 'T') {
                        treasures_found++;
                        underPlayer = ' ';
                    }

                    int dx[4] = { 0, 0, -1, 1 };
                    int dy[4] = { -1, 1, 0, 0 };
                    for (int k = 0; k < 4; k++) {
                        int tx = playerX + dx[k];
                        int ty = playerY + dy[k];
                        if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
                            if (map[ty][tx] == 'T') {
                                treasures_found++;
                                map[ty][tx] = ' ';
                                printf("你发现了一个宝藏!\n");
                            }
                        }
                    }
                }
            }
        }
        else {
            return;
        }
    }
}

/* removed duplicate extracted run loop - maps_from_buffer contains the game loop inline */

void maps(const char* mapFile, int mode) {
    char map[MAX_HEIGHT][MAX_WIDTH];
    int width = 0, height = 0, treasures = 0;
    char* filename = get_map_filepath(mapFile);
    if (load_map_from_file(filename, map, &width, &height, &treasures) != 0)
        return;
    /* no file-provided start coordinates here -> pass -1 to indicate none */
    maps_from_buffer(map, width, height, mode, -1, -1, mapFile, NULL);
}