#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>
#include <windows.h>
#include "map.h"

#define MAX_WIDTH  100
#define MAX_HEIGHT 100

static char* get_map_filename(int level) {
    static char filename[20];
    snprintf(filename, sizeof(filename), "map_level_%d.txt", level);
    return filename;
}

static int load_map_from_file(char* filename, char map[MAX_HEIGHT][MAX_WIDTH], int* width, int* height, int* treasures) {
    FILE* file = fopen(filename, "r");
    if (!file) {
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
    }
    else {
        defaultAttr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            char c = map[y][x];
            WORD attr = defaultAttr;

            switch (c) {
            case 'P':
                attr = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                break;
            case 'T':
                attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case 'D':
                attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
                break;
            case 'W':
                attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                break;
            default:
                attr = defaultAttr;
                break;
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

void maps_from_buffer(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height, int mode) {
    int consume_HP = 0;
    int step = 0;
    int treasures_found = 0;
    char path[1000] = {0};
    size_t pathSize = sizeof(path);
    size_t pathIndex = 0;

    char map[MAX_HEIGHT][MAX_WIDTH];
    int treasures = 0;
    int playerX = 1, playerY = 1;

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

    while (1) {
        system("cls");
        print_map(map, width, height);
        printf("体力消耗:%d\n", consume_HP);
        printf("控制方法：W 上, S 下, A 左, D 右, I 原地不动, Q 返回主菜单\n");

        if (mode == 0) {
            char ch = _getch();
            char mv = tolower(ch);

            if (mv == 'q') {
                map[playerY][playerX] = underPlayer;
                return;
            }

            if (mv != 'w' && mv != 'a' && mv != 's' && mv != 'd' && mv != 'i') {
                printf("输入错误，请使用 W/A/S/D/I 或 Q 返回。\n");
                system("pause");
                continue;
            }

            if (pathIndex + 1 < pathSize) {
                path[pathIndex++] = mv;
                path[pathIndex] = '\0';
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
                printf("撞墙了，很痛！（越界）\n");
                system("pause");
                continue;
            }

            char target = map[ny][nx];
            if (target == 'W') {
                printf("撞墙了，很痛！\n");
                system("pause");
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

            for (size_t idx = 0; path[idx] != '\0'; idx++) {
                char mv = (char)tolower((unsigned char)path[idx]);

                if (mv == 'q') {
                    map[playerY][playerX] = underPlayer;
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
            system("pause");
            return;
        }
    }
}

void maps(int level, int mode) {
    char map[MAX_HEIGHT][MAX_WIDTH];
    int width = 0, height = 0, treasures = 0;
    char* filename = get_map_filename(level);
    if (load_map_from_file(filename, map, &width, &height, &treasures) != 0)
        return;
    maps_from_buffer(map, width, height, mode);
}