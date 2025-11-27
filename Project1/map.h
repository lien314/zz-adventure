#pragma once

#define MAX_WIDTH  100
#define MAX_HEIGHT 100

void maps(int level, int mode);
void maps_from_buffer(char src[MAX_HEIGHT][MAX_WIDTH], int width, int height, int mode);