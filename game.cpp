#include "SDL.h"
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include "SApplication.h"
#include "SWindow.h"

#define MAX_ROW_NUM 7
#define MAX_COL_NUM 7
#define MAX_BLK_NUM 8
#define MAX_BLKROW_NUM 4
#define MAX_BLKCOL_NUM 4

static const uint8_t gGridMarks[MAX_ROW_NUM][MAX_COL_NUM] = {
	{ 0,   0,   0,   0,   0,   0,   255 },
	{ 0,   0,   0,   0,   0,   0,   255 },
	{ 0,   0,   0,   0,   0,   0,   0   },
	{ 0,   0,   0,   0,   0,   0,   0   },
	{ 0,   0,   0,   0,   0,   0,   0   },
	{ 0,   0,   0,   0,   0,   0,   0   },
	{ 0,   0,   0,   255, 255, 255, 255 }
};
static const uint8_t gGridValues[MAX_ROW_NUM][MAX_COL_NUM] = {
	{ 101, 102, 103, 104, 105, 106, 0   },
	{ 107, 108, 109, 110, 111, 112, 0   },
	{ 1,   2,   3,   4,   5,   6,   7   },
	{ 8,   9,   10,  11,  12,  13,  14  },
	{ 15,  16,  17,  18,  19,  20,  21  },
	{ 22,  23,  24,  25,  26,  27,  28  },
	{ 29,  30,  31,  0,   0,   0,   0   }
};
static const uint8_t gBlockData[MAX_BLK_NUM][MAX_BLKROW_NUM * MAX_BLKCOL_NUM] = {
	{
		1, 1, 0, 0,
		0, 1, 0, 0,
		1, 1, 0, 0
	},
	{
		1, 1, 1, 0,
		1, 1, 1, 0,
		0, 0, 0, 0
	},
	{
		1, 0, 0, 0,
		1, 1, 0, 0,
		1, 1, 0, 0,
		0, 0, 0, 0
	},
	{
		0, 0, 1, 0,
		1, 1, 1, 0,
		1, 0, 0, 0,
		0, 0, 0, 0
	},
	{
		0, 0, 1, 0,
		0, 0, 1, 0,
		1, 1, 1, 0,
		0, 0, 0, 0
	},
	{
		1, 1, 0, 0,
		1, 0, 0, 0,
		1, 0, 0, 0,
		1, 0, 0, 0
	},
	{
		1, 1, 1, 1,
		0, 1, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	},
	{
		1, 1, 1, 0,
		0, 0, 1, 1,
		0, 0, 0, 0,
		0, 0, 0, 0
	}
};

/**
		1
	  -----
	 4|   |5
	  | 2 | 
	  -----
	 6|   |7
	  | 3 |
	  -----
 */
static const uint8_t gNumberPiex[10] = {
	/* 7654321 */
	0b01111101, /* 0 */
	0b01010000, /* 1 */
	0b00110111, /* 2 */
	0b01010111, /* 3 */
	0b01011010, /* 4 */
	0b01001111, /* 5 */
	0b01101111, /* 6 */
	0b01010001, /* 7 */
	0b01111111, /* 8 */
	0b01011111  /* 9 */
};

struct Color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};
struct Grid {
	int xpos;
	int ypos;
	SDL_Rect rect;
	uint8_t data[MAX_ROW_NUM * MAX_COL_NUM];
	uint8_t cache[MAX_ROW_NUM * MAX_COL_NUM];
};
struct Block {
	int state;
	int xpos;
	int ypos;
	Color clr;
	SDL_Rect rect;
	uint8_t data[MAX_BLKROW_NUM * MAX_BLKCOL_NUM];
};

static SDL_Window* gWindow = nullptr;
static SDL_Renderer* gRender = nullptr;

static int gCellSize = 50;
static int gDropIndex = -1;
static int gDropX = 0;
static int gDropY = 0;
static int gDropCX = 0;
static int gDropCY = 0;
static Grid gGrid = { 0 };
static Block gBlocks[MAX_BLK_NUM] = { 0 };
static const Color gBlockClrs[MAX_BLK_NUM] = {
	{ 0x70, 0xf3, 0xff, 0xff },
	{ 0x44, 0xce, 0xf6, 0xff },
	{ 0x16, 0x85, 0xa9, 0xff },
	{ 0x46, 0x5c, 0xc4, 0xff },
	{ 0xed, 0xd1, 0xd8, 0xff },
	{ 0xb0, 0xa4, 0xe3, 0xff },
	{ 0xa4, 0xe2, 0xc6, 0xff },
	{ 0xa9, 0x81, 0x75, 0xff }
};

void drawNumber(SDL_Renderer& render, int x, int y, int size, uint8_t val)
{
	if(val < 10)
	{
		x++;
		y++;
		size -= 2;
		const int a = size / 2;
		uint8_t m = gNumberPiex[val];
		if(m & (1u << 0u))
		{
			SDL_RenderDrawLine(&render, x, y, x + a, y);
		}
		if(m & (1u << 1u))
		{
			SDL_RenderDrawLine(&render, x, y + a, x + a, y + a);
		}
		if(m & (1u << 2u))
		{
			SDL_RenderDrawLine(&render, x, y + a + a, x + a, y + a + a);
		}
		if(m & (1u << 3u))
		{
			SDL_RenderDrawLine(&render, x, y, x, y + a);
		}
		if(m & (1u << 4u))
		{
			SDL_RenderDrawLine(&render, x + a, y, x + a, y + a);
		}
		if(m & (1u << 5u))
		{
			SDL_RenderDrawLine(&render, x, y + a, x, y + a + a);
		}
		if(m & (1u << 6u))
		{
			SDL_RenderDrawLine(&render, x + a, y + a, x + a, y + a + a);
		}
	}
}
void drawText(SDL_Renderer& render, const SDL_Rect& rect, uint8_t val)
{
	const int a = (rect.w < rect.h ? rect.w : rect.h) / 5;
	if(val < 10)
	{
		int x = rect.x + (rect.w - a) / 2;
		int y = rect.y + (rect.h - a * 2) / 2;
		drawNumber(render, x, y, a * 2, val);
	}
	else if(val < 100)
	{
		int x = rect.x + (rect.w - a * 3) / 2;
		int y = rect.y + (rect.h - a * 2) / 2;
		drawNumber(render, x, y, a * 2, val / 10);
		drawNumber(render, x + a + a / 2, y, a * 2, val % 10);
	}
}
void drawGrid(SDL_Renderer& render, const SDL_Rect& rect)
{
	const int a = gCellSize;
	int x0 = rect.x + (rect.w - a * MAX_COL_NUM) / 2;
	int y0 = rect.y + (rect.h - a * MAX_ROW_NUM) / 2;

	{
		SDL_Rect rc = { x0, y0, a * MAX_COL_NUM + 1, a * MAX_ROW_NUM + 1 };
		SDL_SetRenderDrawColor(&render, 100, 100, 100, 255);
		SDL_RenderDrawRect(&render, &rc);
	}

	for(int c = 0, x = x0; c < MAX_COL_NUM; ++c, x += a)
	{
		for(int r = 0, y = y0; r < MAX_ROW_NUM; ++r, y += a)
		{
			uint8_t m = gGrid.data[r * MAX_COL_NUM + c];
			if(255 == m)
			{
				continue;
			}
			SDL_Rect rc = { x, y, a, a };
			if(0 != m)
			{
				const Color& clr = gBlockClrs[(m - 1) % MAX_BLK_NUM];
				SDL_SetRenderDrawColor(&render, clr.r, clr.g, clr.b, clr.a);
			}
			else
			{
				SDL_SetRenderDrawColor(&render, 255, 255, 255, 255);
			}
			SDL_RenderFillRect(&render, &rc);

			SDL_SetRenderDrawColor(&render, 0, 0, 0, 255);
			rc.w++;
			rc.h++;
			SDL_RenderDrawRect(&render, &rc);
			uint8_t v = gGridValues[r][c];
			if(0 == m)
			{
				if(v >= 100)
				{
					SDL_SetRenderDrawColor(&render, 255, 0, 0, 255);
					v %= 100;
				}
				drawText(render, rc, v);
			}
		}
	}
}
void drawBlock(SDL_Renderer& render, const SDL_Rect& rect, const uint8_t* data)
{
	const int a = gCellSize;
	int x0 = rect.x + (rect.w - a * 4) / 2;
	int y0 = rect.y + (rect.h - a * 4) / 2;

	for(int c = 0, x = x0; c < MAX_BLKCOL_NUM; ++c, x += a)
	{
		for(int r = 0, y = y0; r < MAX_BLKROW_NUM; ++r, y += a)
		{
			if(0 == data[r * MAX_BLKCOL_NUM + c])
			{
				continue;
			}
			SDL_Rect rc = { x, y, a, a };
			SDL_RenderFillRect(&render, &rc);
		}
	}
}

void updateWindow()
{
	SDL_Rect rect = { 0 };
	SDL_GetRendererOutputSize(gRender, &rect.w, &rect.h);
	{
		int x0 = rect.w / (1 + (MAX_BLKCOL_NUM + 1) * 4);
		int y0 = rect.h / (1 + (MAX_BLKROW_NUM + 1) * 3);
		gCellSize = x0 < y0 ? x0 : y0;
	}

	SDL_RenderClear(gRender);

	SDL_SetRenderDrawColor(gRender, 255, 255, 255, 255);
	SDL_RenderFillRect(gRender, &rect);

	for(int i = 0; i < MAX_BLK_NUM; ++i)
	{
		Block& blk = gBlocks[i];
		blk.rect.x = rect.x + blk.xpos * gCellSize;
		blk.rect.y = rect.y + blk.ypos * gCellSize;
		blk.rect.w = gCellSize * MAX_BLKCOL_NUM;
		blk.rect.h = gCellSize * MAX_BLKROW_NUM;
		if(0 != blk.state)
		{
			SDL_SetRenderDrawColor(gRender, blk.clr.r, blk.clr.g, blk.clr.b, blk.clr.a);
		}
		else
		{
			SDL_SetRenderDrawColor(gRender, 70, 70, 70, 255);
		}
		drawBlock(*gRender, blk.rect, blk.data);
	}
	gGrid.rect.x = rect.x + gCellSize * gGrid.xpos;
	gGrid.rect.y = rect.y + gCellSize * gGrid.ypos;
	gGrid.rect.w = gCellSize * MAX_COL_NUM;
	gGrid.rect.h = gCellSize * MAX_ROW_NUM;

	drawGrid(*gRender, gGrid.rect);

	if(gDropIndex >= 0)
	{
		SDL_SetRenderDrawColor(gRender, 70, 70, 70, 128);
		Block& blk = gBlocks[gDropIndex];
		SDL_Rect rc = blk.rect;
		rc.x = gDropX + gDropCX;
		rc.y = gDropY + gDropCY;
		drawBlock(*gRender, rc, blk.data);
	}

	SDL_RenderPresent(gRender);
}
int findGridIndex(int x, int y)
{
	if(x < gGrid.rect.x || x > gGrid.rect.x + gGrid.rect.w ||
		y < gGrid.rect.y || y > gGrid.rect.y + gGrid.rect.h)
	{
		return -1;
	}
	int c = (x - gGrid.rect.x) / gCellSize;
	int r = (y - gGrid.rect.y) / gCellSize;
	if(c >= MAX_COL_NUM || r >= MAX_ROW_NUM)
	{
		return -1;
	}
	if(255 == gGridMarks[r][c])
	{
		return -1;
	}
	return r * MAX_COL_NUM + c;
}
int findBlockIndex(int x, int y)
{
	for(int i = 0; i < MAX_BLK_NUM; ++i)
	{
		Block& blk = gBlocks[i];
		if(x > blk.rect.x && x < blk.rect.x + blk.rect.w &&
			y > blk.rect.y && y < blk.rect.y + blk.rect.h)
		{
			return i;
		}
	}
	return -1;
}
void removeBlock(int index)
{
	for(int r = 0; r < MAX_ROW_NUM; ++r)
	{
		for(int c = 0; c < MAX_COL_NUM; ++c)
		{
			int pos = r * MAX_COL_NUM + c;
			if(index + 1 == gGrid.data[pos])
			{
				gGrid.data[pos] = 0;
			}
		}
	}
	gBlocks[index].state = 0;
}
bool placeBlock(int index, int x, int y)
{
	Block& blk = gBlocks[index];
	memcpy(gGrid.cache, gGrid.data, sizeof(gGrid.data));
	for(int r = 0; r < MAX_BLKROW_NUM; ++r)
	{
		for(int c = 0; c < MAX_BLKCOL_NUM; ++c)
		{
			int pos = r * MAX_BLKCOL_NUM + c;
			if(0 == blk.data[pos])
			{
				continue;
			}
			int idx = findGridIndex(x + c * gCellSize + gCellSize / 4, y + r * gCellSize + gCellSize / 4);
			if(idx < 0 || 0 != gGrid.data[idx])
			{
				return false;
			}
			gGrid.cache[idx] = index + 1;
		}
	}
	memcpy(gGrid.data, gGrid.cache, sizeof(gGrid.data));
	blk.state = 1;
	return true;
}
int firstDataRow(const uint8_t* data, int rows, int cols, uint8_t val)
{
	for(int r = 0; r < rows; ++r)
	{
		for(int c = 0; c < cols; ++c)
		{
			if(val == data[r * cols + c])
			{
				return r;
			}
		}
	}
	return 0;
}
int firstDataCol(const uint8_t* data, int rows, int cols, uint8_t val)
{
	for(int c = 0; c < cols; ++c)
	{
		for(int r = 0; r < rows; ++r)
		{
			if(val == data[r * cols + c])
			{
				return c;
			}
		}
	}
	return 0;
}
void formatBlock(int index)
{
	Block& blk = gBlocks[index];
	int r0 = firstDataRow(blk.data, MAX_BLKROW_NUM, MAX_BLKCOL_NUM, 1);
	int c0 = firstDataCol(blk.data, MAX_BLKROW_NUM, MAX_BLKCOL_NUM, 1);
	if(r0 > 0 || c0 > 0)
	{
		uint8_t buf[MAX_BLKCOL_NUM * MAX_BLKROW_NUM];
		memset(buf, 0, sizeof(buf));
		for(int r = r0; r < MAX_BLKROW_NUM; ++r)
		{
			for(int c = c0; c < MAX_BLKCOL_NUM; ++c)
			{
				buf[(r - r0) * MAX_BLKCOL_NUM + (c - c0)] = blk.data[r * MAX_BLKCOL_NUM + c];
			}
		}
		memcpy(blk.data, buf, sizeof(blk.data));
	}
}
void rotateBlock(int index)
{
	Block& blk = gBlocks[index];
	static const uint8_t maps[] = {
		12, 8, 4, 0,
		13, 9, 5, 1,
		14, 10, 6, 2,
		15, 11, 7, 3
	};
	uint8_t buf[MAX_BLKCOL_NUM * MAX_BLKROW_NUM];
	memcpy(buf, blk.data, sizeof(buf));
	for(int r = 0; r < MAX_BLKROW_NUM; ++r)
	{
		for(int c = 0; c < MAX_BLKCOL_NUM; ++c)
		{
			int idx = r * MAX_BLKCOL_NUM + c;
			blk.data[idx] = buf[maps[idx]];
		}
	}
	formatBlock(index);
}
void mirrorBlock(int index)
{
	Block& blk = gBlocks[index];
	uint8_t buf[MAX_BLKCOL_NUM * MAX_BLKROW_NUM];
	memcpy(buf, blk.data, sizeof(buf));
	for(int r = 0; r < MAX_BLKROW_NUM; ++r)
	{
		for(int c = 0; c < MAX_BLKCOL_NUM; ++c)
		{
			blk.data[r * MAX_BLKCOL_NUM + c] = buf[r * MAX_BLKCOL_NUM + MAX_BLKCOL_NUM - c - 1];
		}
	}
	formatBlock(index);
}
void onMouseDown(int key, int x, int y)
{
	if(1 == key)
	{
		int idx = findBlockIndex(x, y);
		if(idx >= 0 && 0 == gBlocks[idx].state)
		{
			gDropX = x;
			gDropY = y;
			gDropCX = gBlocks[idx].rect.x - x;
			gDropCY = gBlocks[idx].rect.y - y;
			gDropIndex = idx;
			updateWindow();
		}
		else if((idx = findGridIndex(x, y)) >= 0 && 0 != gGrid.data[idx])
		{
			int r0 = firstDataRow(gGrid.data, MAX_ROW_NUM, MAX_COL_NUM, gGrid.data[idx]);
			int c0 = firstDataCol(gGrid.data, MAX_ROW_NUM, MAX_COL_NUM, gGrid.data[idx]);
			gDropX = x;
			gDropY = y;
			gDropCX = gGrid.rect.x - x + c0 * gCellSize;
			gDropCY = gGrid.rect.y - y + r0 * gCellSize;
			gDropIndex = gGrid.data[idx] - 1;
			removeBlock(gDropIndex);
			updateWindow();
		}
	}
	else if(2 == key)
	{
		if(gDropIndex >= 0)
		{
			mirrorBlock(gDropIndex);
			updateWindow();
		}
		else
		{
			int idx = findBlockIndex(x, y);
			if(idx >= 0 && 0 == gBlocks[idx].state)
			{
				mirrorBlock(idx);
				updateWindow();
			}
		}
	}
	else if(3 == key)
	{
		if(gDropIndex >= 0)
		{
			rotateBlock(gDropIndex);
			updateWindow();
		}
		else
		{
			int idx = findBlockIndex(x, y);
			if(idx >= 0 && 0 == gBlocks[idx].state)
			{
				rotateBlock(idx);
				updateWindow();
			}
		}
	}
}
void onMouseUp(int key, int x, int y)
{
	if(1 == key)
	{
		if(gDropIndex >= 0)
		{
			placeBlock(gDropIndex, x + gDropCX, y + gDropCY);
			gDropIndex = -1;
			updateWindow();
		}
	}
}
void onMouseMove(int x, int y)
{
	if(gDropIndex >= 0)
	{
		gDropX = x;
		gDropY = y;
		updateWindow();
	}
}

bool handleEvent(const SDL_Event& evt)
{
	bool ret = true;
	switch(evt.type)
	{
	case SDL_QUIT:
		ret = false;
		break;
	case SDL_MOUSEBUTTONDOWN:
		onMouseDown(evt.button.button, evt.button.x, evt.button.y);
		break;
	case SDL_MOUSEBUTTONUP:
		onMouseUp(evt.button.button, evt.button.x, evt.button.y);
		break;
	case SDL_MOUSEMOTION:
	{
		SDL_Event next;
		bool flag = false;
		int x = evt.motion.x;
		int y = evt.motion.y;
		while(SDL_PollEvent(&next))
		{
			if(SDL_MOUSEMOTION != next.type)
			{
				flag = true;
				break;
			}
			x = evt.motion.x;
			y = evt.motion.y;
		}
		onMouseMove(x, y);
		if(flag)
		{
			ret = handleEvent(next);
		}
		break;
	}
	case SDL_WINDOWEVENT:
		if(SDL_WINDOWEVENT_SIZE_CHANGED == evt.window.event)
		{
			updateWindow();
		}
		break;
	default:
		break;
	}
	return ret;
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	gWindow = SDL_CreateWindow("MoyuDay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if(nullptr == gWindow)
	{
		printf("Could not create window: %s\n", SDL_GetError());
		return -1;
	}

	gRender = SDL_CreateRenderer(gWindow, -1, 0);
	if(nullptr == gRender)
	{
		printf("Could not create render: %s\n", SDL_GetError());
		return -1;
	}

	memcpy(gGrid.data, gGridMarks, sizeof(gGrid.data));
	for(int i = 0; i < MAX_BLK_NUM; ++i)
	{
		Block& blk = gBlocks[i];
		blk.state = 0;
		blk.clr = gBlockClrs[i];
		memcpy(blk.data, gBlockData[i], sizeof(blk.data));
	}
	gBlocks[0].xpos = 1;
	gBlocks[0].ypos = 1;
	gBlocks[1].xpos = 1 + MAX_BLKCOL_NUM + 1;
	gBlocks[1].ypos = 1;
	gBlocks[2].xpos = 1 + (MAX_BLKCOL_NUM + 1) * 2;
	gBlocks[2].ypos = 1;
	gBlocks[3].xpos = 1 + (MAX_BLKCOL_NUM + 1) * 3;
	gBlocks[3].ypos = 1;
	gBlocks[4].xpos = 1;
	gBlocks[4].ypos = 1 + MAX_BLKROW_NUM + 1;
	gBlocks[5].xpos = 1;
	gBlocks[5].ypos = 1 + (MAX_BLKROW_NUM + 1) * 2;
	gBlocks[6].xpos = 1 + (MAX_BLKCOL_NUM + 1) * 3;
	gBlocks[6].ypos = 1 + MAX_BLKROW_NUM + 1;
	gBlocks[7].xpos = 1 + (MAX_BLKCOL_NUM + 1) * 3;
	gBlocks[7].ypos = 1 + (MAX_BLKROW_NUM + 1) * 2;
	gGrid.xpos = 1 + MAX_BLKCOL_NUM + 1;
	gGrid.ypos = 1 + MAX_BLKROW_NUM + 1;

	updateWindow();

	bool running = true;
	while(running)
	{
		SDL_Event evt;
		if(SDL_WaitEvent(&evt))
		{
			running = handleEvent(evt);
		}
	}

	SDL_DestroyRenderer(gRender);
	SDL_DestroyWindow(gWindow);
	SDL_Quit();
	return 0;
}
