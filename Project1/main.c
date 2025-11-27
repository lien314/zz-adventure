#include <stdio.h>
#include <stdlib.h>
#include "menu.h"

int main() {
	while (1) {
		char* startmenu[] = { "开始游戏","游戏说明","退出" };
		int choice1 = show_menu(startmenu, 3, "小猪的奇妙冒险！主菜单");
		system("cls");
		switch (choice1) {
		case 0: 
            printf("在遥远的文字王国，生活着一只勇敢的猪猪 。它听说王国的某个角落藏有无尽的财富与宝藏，这激发了它的冒险精神。于是小猪决定不顾一切踏上寻宝之旅，穿越二维世界的迷宫地形，寻找传说中的伟大财富。这一路惊险又刺激，你准备好了吗？\n");
			system("pause");
			char* levels[] = { "第一关","第二关--小心脚下？","自定义","返回主菜单" };
			int choice2 = show_menu(levels, 4, "请选择关卡");
			system("cls");
			
			switch (choice2) {
			case 0:
				printf("第一关开始！祝你好运！\n");
				system("pause");
				break;
			case 1:
				printf("第二关开始！小心脚下的陷阱！\n");
				system("pause");
				break;
			case 2:
				printf("自定义关卡功能尚未开放，敬请期待！\n");
				system("pause");
				break;
			case 3:
				break;
			default:
				printf("无效选择！\n");
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
}