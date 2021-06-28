#include "SDL.h"
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>

#define MAX_ROW_NUM 7
#define MAX_COL_NUM 7
#define MAX_BLK_NUM 8
#define MAX_BLKROW_NUM 4
#define MAX_BLKCOL_NUM 4

static const uint8_t gGridMarks[MAX_ROW_NUM * MAX_COL_NUM] = {
	0,   0,   0,   0,   0,   0,   255,
	0,   0,   0,   0,   0,   0,   255,
	0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   255, 255, 255, 255
};
static const uint8_t gGridValues[MAX_ROW_NUM * MAX_COL_NUM] = {
	101, 102, 103, 104, 105, 106, 0,
	107, 108, 109, 110, 111, 112, 0,
	1,   2,   3,   4,   5,   6,   7,
	8,   9,   10,  11,  12,  13,  14,
	15,  16,  17,  18,  19,  20,  21,
	22,  23,  24,  25,  26,  27,  28,
	29,  30,  31,  0,   0,   0,   0
};
struct BlockInfo {
	int rows;
	int cols;
	uint8_t data[MAX_BLKROW_NUM * MAX_BLKCOL_NUM];
};
static const BlockInfo gBlockInfos[MAX_BLK_NUM] = {
	{
		2, 3,
		{
			1, 1, 1,
			1, 1, 1
		}
	},
	{
		3, 2,
		{
			1, 1,
			0, 1,
			1, 1
		}
	},
	{
		3, 3,
		{
			0, 0, 1,
			0, 0, 1,
			1, 1, 1
		}
	},
	{
		4, 2,
		{
			1, 1,
			1, 0,
			1, 0,
			1, 0
		}
	},
	{
		3, 2,
		{
			1, 0,
			1, 1,
			1, 1
		}
	},
	{
		3, 3,
		{
			0, 0, 1,
			1, 1, 1,
			1, 0, 0
		}
	},
	{
		2, 4,
		{
			1, 1, 1, 1,
			0, 1, 0, 0
		}
	},
	{
		2, 4,
		{
			1, 1, 1, 0,
			0, 0, 1, 1
		}
	}
};

/**
*       1
*     -----
*    4|   |5
*     | 2 | 
*     -----
*    6|   |7
*     | 3 |
*     -----
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
	BlockInfo info;
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

#define MAX_RESULT_NUM 256
struct Result {
	BlockInfo blkData[MAX_BLK_NUM];
	uint8_t gridData[MAX_COL_NUM * MAX_ROW_NUM];
};
struct Branch {
	int idx;
	int num;
	int firstCols[8];
	BlockInfo data[8];
};
struct Solve {
	bool enabled;
	int month;
	int day;
	SDL_Rect checkRect;
	SDL_Rect preRect;
	SDL_Rect nextRect;
	uint8_t gridData[MAX_COL_NUM * MAX_ROW_NUM];
	uint32_t blkMask;
	Branch blkData[MAX_BLK_NUM];
	int resultIdx;
	int resultNum;
	Result results[MAX_RESULT_NUM];
};
static Solve gSolve = { 0 };

void drawNumber(SDL_Renderer* render, int x, int y, int size, uint8_t val)
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
			SDL_RenderDrawLine(render, x, y, x + a, y);
		}
		if(m & (1u << 1u))
		{
			SDL_RenderDrawLine(render, x, y + a, x + a, y + a);
		}
		if(m & (1u << 2u))
		{
			SDL_RenderDrawLine(render, x, y + a + a, x + a, y + a + a);
		}
		if(m & (1u << 3u))
		{
			SDL_RenderDrawLine(render, x, y, x, y + a);
		}
		if(m & (1u << 4u))
		{
			SDL_RenderDrawLine(render, x + a, y, x + a, y + a);
		}
		if(m & (1u << 5u))
		{
			SDL_RenderDrawLine(render, x, y + a, x, y + a + a);
		}
		if(m & (1u << 6u))
		{
			SDL_RenderDrawLine(render, x + a, y + a, x + a, y + a + a);
		}
	}
}
int drawUint(SDL_Renderer* render, int x, int y, int size, uint32_t val)
{
	const int a = (size - 2) / 2;
	x += a / 2;
	y += 1;
	int n = 0;
	if(val < 10)
	{
		drawNumber(render, x, y, a * 2, val);
		n = a * 2;
	}
	else if(val < 100)
	{
		drawNumber(render, x, y, a * 2, val / 10);
		drawNumber(render, x + a + a / 2, y, a * 2, val % 10);
		n = a * 4;
	}
	else if(val < 1000)
	{
		drawNumber(render, x, y, a * 2, val / 100);
		drawNumber(render, x + 3 * a / 2, y, a * 2, (val / 10) % 10);
		drawNumber(render, x + 3 * a, y, a * 2, val % 10);
		n = a * 6;
	}
	return n;
}
void drawGrid(SDL_Renderer* render, const SDL_Rect* rect)
{
	const int a = gCellSize;
	int x0 = rect->x + (rect->w - a * MAX_COL_NUM) / 2;
	int y0 = rect->y + (rect->h - a * MAX_ROW_NUM) / 2;

	{
		SDL_Rect rc = { x0, y0, a * MAX_COL_NUM + 1, a * MAX_ROW_NUM + 1 };
		SDL_SetRenderDrawColor(render, 100, 100, 100, 255);
		SDL_RenderDrawRect(render, &rc);
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
			m = m >= 200 ? 0 : m;
			if(0 != m)
			{
				const Color& clr = gBlockClrs[(m - 1) % MAX_BLK_NUM];
				SDL_SetRenderDrawColor(render, clr.r, clr.g, clr.b, clr.a);
			}
			else
			{
				SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
			}
			SDL_RenderFillRect(render, &rc);

			SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
			rc.w++;
			rc.h++;
			SDL_RenderDrawRect(render, &rc);
			uint8_t v = gGridValues[r * MAX_COL_NUM + c];
			if(0 == m)
			{
				if(v >= 100)
				{
					SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
					v %= 100;
				}
				drawUint(render, v > 9 ? rc.x + rc.h / 8 : rc.x + rc.h / 4, rc.y + rc.h / 4, rc.h / 2, v);
			}
		}
	}
}
void drawBlock(SDL_Renderer* render, const SDL_Rect* rect, int rows, int cols, const uint8_t* data)
{
	const int a = gCellSize;
	int x0 = rect->x + (rect->w - a * 4) / 2;
	int y0 = rect->y + (rect->h - a * 4) / 2;

	for(int c = 0, x = x0; c < cols; ++c, x += a)
	{
		for(int r = 0, y = y0; r < rows; ++r, y += a)
		{
			if(0 == data[r * cols + c])
			{
				continue;
			}
			SDL_Rect rc = { x, y, a, a };
			SDL_RenderFillRect(render, &rc);
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
		drawBlock(gRender, &blk.rect, blk.info.rows, blk.info.cols, blk.info.data);
	}
	gGrid.rect.x = rect.x + gCellSize * gGrid.xpos;
	gGrid.rect.y = rect.y + gCellSize * gGrid.ypos;
	gGrid.rect.w = gCellSize * MAX_COL_NUM;
	gGrid.rect.h = gCellSize * MAX_ROW_NUM;

	drawGrid(gRender, &gGrid.rect);

	if(gDropIndex >= 0)
	{
		SDL_SetRenderDrawColor(gRender, 70, 70, 70, 128);
		Block& blk = gBlocks[gDropIndex];
		SDL_Rect rc = blk.rect;
		rc.x = gDropX + gDropCX;
		rc.y = gDropY + gDropCY;
		drawBlock(gRender, &rc, blk.info.rows, blk.info.cols, blk.info.data);
	}

	SDL_SetRenderDrawColor(gRender, 0, 0, 0, 255);
	gSolve.checkRect.x = gGrid.rect.x;
	gSolve.checkRect.y = gGrid.rect.y + gGrid.rect.h + gCellSize + gCellSize / 2;
	gSolve.checkRect.w = gCellSize / 2;
	gSolve.checkRect.h = gCellSize / 2;
	SDL_RenderDrawRect(gRender, &gSolve.checkRect);
	if(gSolve.enabled)
	{
		int x0 = gSolve.checkRect.x + gSolve.checkRect.w / 5;
		int y0 = gSolve.checkRect.y + gSolve.checkRect.h / 3;
		int x1 = gSolve.checkRect.x + gSolve.checkRect.w / 3;
		int y1 = gSolve.checkRect.y + gSolve.checkRect.h * 4 / 5;
		SDL_RenderDrawLine(gRender, x0, y0, x1, y1);
		x0 = gSolve.checkRect.x + gSolve.checkRect.w * 4 / 5;
		y0 = gSolve.checkRect.y + gSolve.checkRect.h / 5;
		SDL_RenderDrawLine(gRender, x1, y1, x0, y0);

		if(gSolve.resultNum > 0)
		{
			x0 = gSolve.checkRect.x + gSolve.checkRect.w + gCellSize / 4;
			y0 = gSolve.checkRect.y;
			gSolve.preRect.x = x0;
			gSolve.preRect.y = y0;
			gSolve.preRect.w = gSolve.checkRect.w;
			gSolve.preRect.h = gSolve.checkRect.h;
			x1 = gSolve.preRect.x + gSolve.preRect.w * 2 / 3;
			y1 = gSolve.preRect.y + gSolve.preRect.h / 2;
			SDL_RenderDrawLine(gRender, x1, y0, x0, y1);
			SDL_RenderDrawLine(gRender, x0, y1, x1, gSolve.preRect.y + gSolve.preRect.h);

			x0 = gSolve.preRect.x + gSolve.preRect.w;
			x0 += drawUint(gRender, x0, y0, gCellSize / 2, gSolve.resultIdx + 1);
			SDL_RenderDrawLine(gRender, x0, y0 + gCellSize / 2, x0 + gCellSize / 4, y0);
			x0 += gCellSize / 4;
			x0 += drawUint(gRender, x0, y0, gCellSize / 2, gSolve.resultNum);

			x0 += gCellSize / 4;
			gSolve.nextRect.x = x0;
			gSolve.nextRect.y = y0;
			gSolve.nextRect.w = gSolve.checkRect.w;
			gSolve.nextRect.h = gSolve.checkRect.h;

			x1 = gSolve.nextRect.x + gSolve.nextRect.w * 2 / 3;
			y1 = gSolve.nextRect.y + gSolve.nextRect.h / 2;
			SDL_RenderDrawLine(gRender, x0, y0, x1, y1);
			SDL_RenderDrawLine(gRender, x1, y1, x0, gSolve.nextRect.y + gSolve.nextRect.h);
		}
	}

	SDL_RenderPresent(gRender);
}
bool isInRect(int x, int y, const SDL_Rect* rect)
{
	return x >= rect->x && x <= rect->x + rect->w && y >= rect->y && y <= rect->y + rect->h;
}
int testGridIndex(int x, int y)
{
	if(!isInRect(x, y, &gGrid.rect))
	{
		return -1;
	}
	int c = (x - gGrid.rect.x) / gCellSize;
	int r = (y - gGrid.rect.y) / gCellSize;
	if(c >= MAX_COL_NUM || r >= MAX_ROW_NUM)
	{
		return -1;
	}
	if(255 == gGridMarks[r * MAX_COL_NUM + c])
	{
		return -1;
	}
	return r * MAX_COL_NUM + c;
}
int testBlockIndex(int x, int y)
{
	for(int i = 0; i < MAX_BLK_NUM; ++i)
	{
		Block& blk = gBlocks[i];
		if(isInRect(x, y, &blk.rect))
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
	for(int r = 0; r < blk.info.rows; ++r)
	{
		for(int c = 0; c < blk.info.cols; ++c)
		{
			int pos = r * blk.info.cols + c;
			if(0 == blk.info.data[pos])
			{
				continue;
			}
			int idx = testGridIndex(x + c * gCellSize + gCellSize / 4, y + r * gCellSize + gCellSize / 4);
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
void rotateBlock(BlockInfo* blk)
{
	uint8_t buf[MAX_BLKCOL_NUM * MAX_BLKROW_NUM];
	memcpy(buf, blk->data, sizeof(buf));
	int rows = blk->rows;
	blk->rows = blk->cols;
	blk->cols = rows;
	for(int r = 0; r < blk->rows; ++r)
	{
		for(int c = 0; c < blk->cols; ++c)
		{
			blk->data[r * blk->cols + c] = buf[(blk->cols - 1 - c) * blk->rows + r];
		}
	}
}
void mirrorBlock(BlockInfo* blk)
{
	uint8_t buf[MAX_BLKCOL_NUM * MAX_BLKROW_NUM];
	memcpy(buf, blk->data, sizeof(buf));
	for(int r = 0; r < blk->rows; ++r)
	{
		for(int c = 0; c < blk->cols; ++c)
		{
			blk->data[r * blk->cols + c] = buf[r * blk->cols + blk->cols - c - 1];
		}
	}
}
bool solvePlace(Solve* s, int row, int col, int index, int branch)
{
	BlockInfo* blk = &s->blkData[index].data[branch];
	if(row + blk->rows > MAX_ROW_NUM || col + blk->cols > MAX_COL_NUM)
	{
		return false;
	}
	const int num = blk->cols * blk->rows;
	for(int i = 0; i < num; ++i)
	{
		if(0 == blk->data[i])
		{
			continue;
		}
		int r = i / blk->cols;
		int c = i % blk->cols;
		if(0 != s->gridData[(row + r) * MAX_COL_NUM + col + c])
		{
			return false;
		}
	}
	for(int i = 0; i < num; ++i)
	{
		if(0 == blk->data[i])
		{
			continue;
		}
		int r = i / blk->cols;
		int c = i % blk->cols;
		s->gridData[(row + r) * MAX_COL_NUM + col + c] = index + 1;
	}
	return true;
}
void solveUnplace(Solve* s, int row, int col, int index, int branch)
{
	BlockInfo* blk = &s->blkData[index].data[branch];
	const int num = blk->cols * blk->rows;
	for(int i = 0; i < num; ++i)
	{
		if(0 == blk->data[i])
		{
			continue;
		}
		int r = i / blk->cols;
		int c = i % blk->cols;
		s->gridData[(row + r) * MAX_COL_NUM + col + c] = 0;
	}
}
void solveGrid(Solve* s, int index)
{
	if(index >= MAX_COL_NUM * MAX_ROW_NUM)
	{
		if(s->resultNum < MAX_RESULT_NUM)
		{
			Result* res = &s->results[s->resultNum];
			for(int k = 0; k < MAX_BLK_NUM; ++k)
			{
				memcpy(&res->blkData[k], &s->blkData[k].data[s->blkData[k].idx], sizeof(BlockInfo));
			}
			memcpy(res->gridData, s->gridData, sizeof(s->gridData));
			s->resultNum++;
		}
		return;
	}
	if(0 != s->gridData[index])
	{
		solveGrid(s, index + 1);
		return;
	}
	int r = index / MAX_COL_NUM;
	int c = index % MAX_COL_NUM;
	for(int i = 0; i < MAX_BLK_NUM; ++i)
	{
		const uint32_t msk = (1u << i);
		if(0 != (s->blkMask & msk))
		{
			continue;
		}
		Branch* blk = &s->blkData[i];
		for(int b = 0; b < blk->num; ++b)
		{
			int x = c - blk->firstCols[b];
			if(x >= 0 && solvePlace(s, r, x, i, b))
			{
				if(0 != s->gridData[index])
				{
					blk->idx = b;
					s->blkMask |= msk;
					solveGrid(s, index + 1);
					s->blkMask &= ~msk;
				}
				solveUnplace(s, r, x, i, b);
			}
		}
	}
}

void solve(int mon, int day)
{
	memcpy(gSolve.gridData, gGridMarks, sizeof(gGridMarks));
	for(int r = 0; r < 2; ++r)
	{
		for(int c = 0; c < MAX_COL_NUM; ++c)
		{
			if(100 + mon == gGridValues[r * MAX_COL_NUM + c])
			{
				gSolve.gridData[r * MAX_COL_NUM + c] = 200;
				break;
			}
		}
	}
	for(int r = 2; r < MAX_ROW_NUM; ++r)
	{
		for(int c = 0; c < MAX_COL_NUM; ++c)
		{
			if(day == gGridValues[r * MAX_COL_NUM + c])
			{
				gSolve.gridData[r * MAX_COL_NUM + c] = 200;
				break;
			}
		}
	}

	gSolve.blkMask = 0;
	gSolve.resultIdx = 0;
	gSolve.resultNum = 0;
	solveGrid(&gSolve, 0);
}
bool containsBlock(Branch* blk, const BlockInfo* dat)
{
	const int len = dat->cols * dat->rows;
	for(int i = 0; i < blk->num; ++i)
	{
		BlockInfo* inf = &blk->data[i];
		if(inf->cols == dat->cols && inf->rows == dat->rows && 0 == memcmp(blk->data[i].data, dat->data, len))
		{
			return true;
		}
	}
	return false;
}
int firstBlockCell(const BlockInfo* dat)
{
	for(int i = 0; i < dat->cols; ++i)
	{
		if(0 != dat->data[i])
		{
			return i;
		}
	}
	return 0;
}
void initSolve()
{
	gSolve.enabled = false;
	gSolve.month = 1;
	gSolve.day = 1;
	gSolve.resultNum = 0;
	for(int i = 0; i < MAX_BLK_NUM; ++i)
	{
		const BlockInfo* tpl = &gBlockInfos[i];
		int len = tpl->rows * tpl->cols;
		Branch* blk = &gSolve.blkData[i];
		BlockInfo tmp;
		blk->idx = 0;
		blk->num = 0;
		memcpy(&tmp, tpl, sizeof(BlockInfo));
		for(int k = 0; k < 4; ++k)
		{
			if(!containsBlock(blk, &tmp))
			{
				memcpy(&blk->data[blk->num], &tmp, sizeof(BlockInfo));
				blk->firstCols[blk->num] = firstBlockCell(&tmp);
				blk->num++;
			}
			rotateBlock(&tmp);
		}
		memcpy(&tmp, tpl, sizeof(BlockInfo));
		mirrorBlock(&tmp);
		for(int k = 4; k < 8; ++k)
		{
			if(!containsBlock(blk, &tmp))
			{
				memcpy(&blk->data[blk->num], &tmp, sizeof(BlockInfo));
				blk->firstCols[blk->num] = firstBlockCell(&tmp);
				blk->num++;
			}
			rotateBlock(&tmp);
		}
	}
}
void commitResult()
{
	if(gSolve.resultIdx < 0 || gSolve.resultIdx >= gSolve.resultNum)
	{
		memcpy(gGrid.data, gGridMarks, sizeof(gGrid.data));
		for(int i = 0; i < MAX_BLK_NUM; ++i)
		{
			Block* blk = &gBlocks[i];
			blk->state = 0;
		}
	}
	else
	{
		Result* res = &gSolve.results[gSolve.resultIdx];
		for(int i = 0; i < MAX_COL_NUM * MAX_ROW_NUM; ++i)
		{
			uint8_t val = res->gridData[i];
			gGrid.data[i] = 200 == val ? 0 : val;
		}
		for(int i = 0; i < MAX_BLK_NUM; ++i)
		{
			Block* blk = &gBlocks[i];
			memcpy(&blk->info, &res->blkData[i], sizeof(blk->info));
			blk->state = 1;
		}
	}
}
void onMouseDown(int key, int x, int y)
{
	if(1 == key)
	{
		int idx = testBlockIndex(x, y);
		if(idx >= 0 && 0 == gBlocks[idx].state)
		{
			gDropX = x;
			gDropY = y;
			gDropCX = gBlocks[idx].rect.x - x;
			gDropCY = gBlocks[idx].rect.y - y;
			gDropIndex = idx;
			updateWindow();
		}
		else if((idx = testGridIndex(x, y)) >= 0 && 0 != gGrid.data[idx])
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
			mirrorBlock(&gBlocks[gDropIndex].info);
			updateWindow();
		}
		else
		{
			int idx = testBlockIndex(x, y);
			if(idx >= 0 && 0 == gBlocks[idx].state)
			{
				mirrorBlock(&gBlocks[idx].info);
				updateWindow();
			}
		}
	}
	else if(3 == key)
	{
		if(gDropIndex >= 0)
		{
			rotateBlock(&gBlocks[gDropIndex].info);
			updateWindow();
		}
		else
		{
			int idx = testBlockIndex(x, y);
			if(idx >= 0 && 0 == gBlocks[idx].state)
			{
				rotateBlock(&gBlocks[idx].info);
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
		else if(isInRect(x, y, &gSolve.checkRect))
		{
			gSolve.enabled = !gSolve.enabled;
			if(gSolve.enabled)
			{
				solve(gSolve.month, gSolve.day);
				commitResult();
			}
			updateWindow();
		}
		else if(gSolve.enabled)
		{
			if(gSolve.resultIdx > 0 && isInRect(x, y, &gSolve.preRect))
			{
				gSolve.resultIdx--;
				commitResult();
				updateWindow();
			}
			else if(gSolve.resultIdx + 1 < gSolve.resultNum && isInRect(x, y, &gSolve.nextRect))
			{
				gSolve.resultIdx++;
				commitResult();
				updateWindow();
			}
		}
	}
	else if(3 == key)
	{
		if(gDropIndex < 0)
		{
			int idx = testGridIndex(x, y);
			if(idx >= 0)
			{
				int day = gSolve.day;
				int mon = gSolve.month;
				int val = gGridValues[idx];
				if(val < 100)
				{
					gSolve.day = val;
				}
				else if(val < 200)
				{
					gSolve.month = val - 100;
				}
				if(gSolve.enabled && (day != gSolve.day || mon != gSolve.month))
				{
					solve(gSolve.month, gSolve.day);
					commitResult();
					updateWindow();
				}
			}
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
		memcpy(&blk.info, &gBlockInfos[i], sizeof(blk.info));
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
	initSolve();

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
