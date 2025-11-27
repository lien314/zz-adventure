#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "map.h"

#define INITIAL_CUSTOM_CAPACITY 3

typedef struct {
    int width;
    int height;
    char data[MAX_HEIGHT][MAX_WIDTH];
} CustomMap;

int main() {
    int customCapacity = INITIAL_CUSTOM_CAPACITY;
    int customCount = 0;
    CustomMap* customMaps = (CustomMap*)malloc(sizeof(CustomMap) * customCapacity);
    if (!customMaps) {
        fprintf(stderr, "内存分配失败\n");
        return 1;
    }

    while (1) {
        char* startmenu[] = { "开始游戏","游戏说明","退出" };
        int choice1 = show_menu(startmenu, 3, "小猪的奇妙冒险！主菜单");
        system("cls");
        switch (choice1) {
        case 0:
            printf("在遥远的文字王国，生活着一只勇敢的猪猪 。\n它听说王国的某个角落藏有无尽的财富与宝藏，这激发了它的冒险精神。\n于是小猪决定不顾一切踏上寻宝之旅，穿越二维世界的迷宫地形，寻找传说中的伟大财富。\n这一路惊险又刺激，你准备好了吗？\n");
            system("pause");
            {
                char* levels[] = { "第一关","第二关--小心脚下？","自定义","返回主菜单" };
                int choice2 = show_menu(levels, 4, "请选择关卡");
                system("cls");

                switch (choice2) {
                case 0:
                {
                    char* pattern_menu[] = { "0：实时模式","1：编程模式" };
                    int mode = show_menu(pattern_menu, 2, "请选择控制模式：");
                    system("cls");
                    maps(1, mode);
                }
                break;
                case 1:
                {
                    char* pattern_menu[] = { "0：实时模式","1：编程模式" };
                    int mode = show_menu(pattern_menu, 2, "请选择控制模式：");
                    system("cls");
                    printf("第二关开始！小心脚下的陷阱！\n");
                    system("pause");
                    maps(2, mode);
                }
                break;
                case 2:
                {
                    while (1) {
                        char* custom_menu[] = { "创建新定制关卡","选择已有定制关卡","返回" };
                        int c = show_menu(custom_menu, 3, "定制关卡管理");
                        system("cls");
                        if (c == 0) {
                            int w = 0, h = 0;
                            printf("请输入地图宽度和高度（空格分隔，最大 %d x %d）：\n", MAX_WIDTH, MAX_HEIGHT);
                            if (scanf_s("%d %d", &w, &h) != 2) {
                                int ch;
                                while ((ch = getchar()) != '\n' && ch != EOF) {}
                                printf("输入格式错误，返回定制菜单。\n");
                                system("pause");
                                continue;
                            }
                            int ch;
                            while ((ch = getchar()) != '\n' && ch != EOF) {} // 清行
                            if (w <= 0 || h <= 0 || w > MAX_WIDTH || h > MAX_HEIGHT) {
                                printf("尺寸超出限制，返回定制菜单。\n");
                                system("pause");
                                continue;
                            }

                            char buffer[MAX_WIDTH + 4];
                            char tempMap[MAX_HEIGHT][MAX_WIDTH];
                            int readFailed = 0;
                            for (int y = 0; y < h; y++) {
                                printf("请输入第 %d 行（至少 %d 字符，支持 'W','D','T',' ' 等）：\n", y + 1, w);
                                if (!fgets(buffer, sizeof(buffer), stdin)) {
                                    readFailed = 1;
                                    break;
                                }
                                size_t len = strlen(buffer);
                                if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
                                for (int x = 0; x < w; x++) {
                                    if ((int)strlen(buffer) > x) tempMap[y][x] = buffer[x];
                                    else tempMap[y][x] = ' ';
                                }
                            }
                            if (readFailed) {
                                printf("读取失败，取消创建。\n");
                                system("pause");
                                continue;
                            }

                            system("cls");
                            printf("预览地图：\n");
                            for (int y = 0; y < h; y++) {
                                for (int x = 0; x < w; x++) putchar(tempMap[y][x]);
                                putchar('\n');
                            }
                            printf("\n保存此地图？（Y保存 / 其它取消）\n");
                            char resp = (char)getchar();
                            while ((ch = getchar()) != '\n' && ch != EOF) {}
                            if (resp == 'Y' || resp == 'y') {
                                if (customCount >= customCapacity) {
                                    int newCap = customCapacity * 2;
                                    CustomMap* n = (CustomMap*)realloc(customMaps, sizeof(CustomMap) * newCap);
                                    if (!n) {
                                        printf("无法分配更多内存以保存定制关卡，保存失败。\n");
                                        system("pause");
                                        continue;
                                    }
                                    customMaps = n;
                                    customCapacity = newCap;
                                }
                                customMaps[customCount].width = w;
                                customMaps[customCount].height = h;
                                for (int yy = 0; yy < h; yy++) {
                                    for (int xx = 0; xx < w; xx++) {
                                        customMaps[customCount].data[yy][xx] = tempMap[yy][xx];
                                    }
                                    for (int xx = w; xx < MAX_WIDTH; xx++) customMaps[customCount].data[yy][xx] = ' ';
                                }
                                for (int yy = h; yy < MAX_HEIGHT; yy++) {
                                    for (int xx = 0; xx < MAX_WIDTH; xx++) customMaps[customCount].data[yy][xx] = ' ';
                                }
                                customCount++;
                                printf("已保存为定制关卡 %d。\n", customCount);
                                system("pause");
                            }
                            else {
                                printf("已取消保存。\n");
                                system("pause");
                            }
                        }
                        else if (c == 1) {
                            if (customCount == 0) {
                                printf("当前没有已保存的定制关卡。\n");
                                system("pause");
                                continue;
                            }
                            int listSize = customCount + 1;
                            char** list = (char**)malloc(sizeof(char*) * listSize);
                            if (!list) {
                                printf("内存分配失败。\n");
                                system("pause");
                                continue;
                            }
                            for (int i = 0; i < customCount; i++) {
                                char* p = (char*)malloc(64);
                                if (!p) { p = NULL; }
                                else {
                                    snprintf(p, 64, "定制关卡 %d ( %dx%d )", i + 1, customMaps[i].width, customMaps[i].height);
                                }
                                list[i] = p ? p : "错误项";
                            }
                            list[listSize - 1] = "返回";

                            int sel = show_menu(list, listSize, "选择定制关卡");

                            for (int i = 0; i < customCount; i++) {
                                if (list[i] && list[i] != "错误项") free(list[i]);
                            }
                            free(list);

                            if (sel >= 0 && sel < customCount) {
                                char* pattern_menu[] = { "0：实时模式","1：编程模式" };
                                int mode = show_menu(pattern_menu, 2, "请选择控制模式：");
                                system("cls");
                                maps_from_buffer(customMaps[sel].data, customMaps[sel].width, customMaps[sel].height, mode);
                            }
                            else{
                            }
                        }
                        else {
                            break;
                        }
                    } 
                }
                break;
                case 3:
                    break;
                default:
                    printf("无效选择！\n");
                }
            }
            break;

        case 1:
            printf("游戏说明：用WSAD控制小猪在文字王国里寻找宝藏，小心墙壁和陷阱！\n");
            system("pause");
            break;
        case 2:
            printf("退出游戏，再见！\n");
            free(customMaps);
            exit(0);
        default:
            printf("无效选择！\n");
        }
    }
    return 0;
}