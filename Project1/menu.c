# include<stdio.h>
# include<stdlib.h>
#include<conio.h>
#include <ctype.h>
# include"menu.h"

int show_menu(const char* menu[], int size, const char* title) {
	char ch;
	int index = 0;
	while (1) {
		system("cls");
		printf("%s\n请选择\n", title);
		for (int i = 0; i < size; i++) {
			if (i == index) {
				printf("--> %s\n", menu[i]);
			}
			else {
				printf("    %s\n", menu[i]);
			}
		}
		printf("\n使用W/S键选择，ENTER确认\n");
		ch = _getch();
		ch = tolower(ch);
		if (ch == 'w') {
			index--;
			if (index < 0) index = size - 1;
		}
		else if (ch == 's') {
			index++;
			if (index >= size) index = 0;
		}
		else if (ch == '\r') {
			return index;
		}
	}
}