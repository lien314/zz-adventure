#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "menu.h"
#include "map.h"

typedef struct {
    int width;
    int height;
    char data[MAX_HEIGHT][MAX_WIDTH];
} CustomMap;

int main() {
    /* 程序主循环：显示主菜单并响应用户选择 */
    while (1) {
        char* startmenu[] = { "开始游戏","游戏说明","退出" };
        int choice1 = show_menu(startmenu, 3, "小猪的奇妙冒险！主菜单");
        system("cls");
        switch (choice1) {
        case 0:
            printf("在遥远的文字王国，生活着一只勇敢的猪猪 。\n它听说王国的某个角落藏有无尽的财富与宝藏，这激发了它的冒险精神。\n于是小猪决定不顾一切踏上寻宝之旅，穿越二维世界的迷宫地形，寻找传说中的伟大财富。\n这一路惊险又刺激，你准备好了吗？\n");
            system("pause");
            {
                /* 在可执行目录下搜索 maps 目录中的 .map 文件 */
                char pattern[MAX_PATH];
                snprintf(pattern, sizeof(pattern), "%s\\maps\\*.map", get_exe_dir());
                WIN32_FIND_DATAA fd;
                HANDLE h = FindFirstFileA(pattern, &fd);
                /* filenames 保存实际文件名，labels 保存显示在菜单上的标签（可能包含存档信息） */
                char* filenames[64] = { 0 };
                char* labels[64] = { 0 };
                int count = 0;
                if (h != INVALID_HANDLE_VALUE) {
                    do {
                        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            size_t n = strlen(fd.cFileName);
                            filenames[count] = (char*)malloc(n + 1);
                            if (filenames[count]) strcpy_s(filenames[count], n + 1, fd.cFileName);
                            /* 检查该关卡是否有存档，供菜单显示和恢复提示 */
                            char saveInfo[256] = { 0 };
                            int has = has_save(fd.cFileName, saveInfo, sizeof(saveInfo));
                            size_t lablen = n + 64 + (has ? strlen(saveInfo) : 0);
                            labels[count] = (char*)malloc(lablen);
                            if (labels[count]) {
                                if (has) snprintf(labels[count], lablen, "开始<%s>（上次）", fd.cFileName);
                                else snprintf(labels[count], lablen, "开始<%s>", fd.cFileName);
                            }

                            count++;
                            /* 限制最多展示的关卡数量以避免内存问题 */
                            if (count >= 62) break;
                        }
                    } while (FindNextFileA(h, &fd));
                    FindClose(h);
                }

                /* 构造关卡菜单：每个关卡条目 + 排行榜 + 返回 */
                int menuCount = count + 2;
                char** menu = malloc(sizeof(char*) * menuCount);
                for (int i = 0; i < count; ++i) menu[i] = labels[i] ? labels[i] : filenames[i];
                /* 最后两个选项：排行榜 和 返回主菜单 */
                if (menu != NULL) {
                    menu[count] = "排行榜";
                    menu[count + 1] = "返回主菜单";
                }

                int sel = show_menu(menu, menuCount, "请选择关卡");
                system("cls");
                if (sel >= 0 && sel < count) {
                    const char* sf = save_filename(filenames[sel]);
                    FILE* sfh = NULL;
                    int hasSave = 0;
                    long long ts = 0; int smode = 0, sconsume = 0, sstep = 0, streasures_found = 0, sw = 0, sh = 0;
                    if (fopen_s(&sfh, sf, "rb") == 0 && sfh) {
                        if (fread(&ts, sizeof(ts), 1, sfh) == 1 && fread(&smode, sizeof(smode), 1, sfh) == 1) {
                            fread(&sconsume, sizeof(sconsume), 1, sfh);
                            fread(&sstep, sizeof(sstep), 1, sfh);
                            fread(&streasures_found, sizeof(streasures_found), 1, sfh);
                            fread(&sw, sizeof(sw), 1, sfh);
                            fread(&sh, sizeof(sh), 1, sfh);
                            hasSave = 1;
                        }
                        fclose(sfh);
                    }
                    if (hasSave) {
                        time_t t = (time_t)ts; struct tm tmv; localtime_s(&tmv, &t); char timestr[64];
                        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tmv);
                        int totalTreasures = get_map_treasure_count(filenames[sel]);
                        char titlebuf[256];
                        snprintf(titlebuf, sizeof(titlebuf), "是否加载上次的进度？\n%s\n退出时间：%s\n进度：已找到 %d / %d 个宝箱\n请选择：",
                            filenames[sel], timestr, streasures_found, totalTreasures);
                        const char* resumeOptions[] = { "是", "否" };
                        int r = show_menu((char**)resumeOptions, 2, titlebuf);
                        system("cls");
                        if (r == 0) {
                            load_and_run_save(filenames[sel], NULL);
                            /* 释放资源并回到主菜单 */
                            for (int i = 0; i < count; ++i) { free(filenames[i]); free(labels[i]); }
                            free(menu);
                            continue;
                        }
                        else {
                            /* 用户选择不加载：删除已有存档，开始新游戏 */
                            delete_save_file(filenames[sel]);
                        }
                    }


                    char namebuf[64] = { 0 };
                    printf("请输入你的名字（用于排行榜，回车确认）：");
                    fflush(stdout);
                    if (fgets(namebuf, sizeof(namebuf), stdin)) {
                        size_t ln = strlen(namebuf); if (ln > 0 && namebuf[ln - 1] == '\n') namebuf[ln - 1] = 0;
                    }
                    const char* modeMenu[] = { "0：实时模式", "1：编程模式" };
                    int mode = show_menu(modeMenu, 2, "请选择控制模式：");
                    system("cls");
                    maps_from_file(filenames[sel], mode, namebuf[0] ? namebuf : NULL);
                }
                else if (sel == count) {
                    if (count == 0) {
                        printf("没有可用的关卡。\n");
                        system("pause");
                    }
                    else {
                        char* lbmenu[64];
                        for (int i = 0; i < count; ++i) lbmenu[i] = filenames[i];
                        int lsel = show_menu(lbmenu, count, "请选择要查看排行榜的关卡");
                        system("cls");
                        if (lsel >= 0 && lsel < count) {
                            show_leaderboard_for_map(filenames[lsel]);
                        }
                    }
                }

                /* 释放为菜单分配的字符串 */
                for (int i = 0; i < count; ++i) { free(filenames[i]); free(labels[i]); }
                free(menu);
            }
            break;

        case 1:
            printf("游戏说明：用WSAD控制小猪在文字王国里寻找宝藏，小心墙壁和陷阱！\n");
            system("pause");
            break;
        case 2:
            printf("退出游戏，再见！\n");
            exit(0);
        default:
            printf("无效选择！\n");
        }
    }
    return 0;
}