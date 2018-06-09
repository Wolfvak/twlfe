#include <nds.h>
#include <stdarg.h>
#include <stdio.h>

#include "ui.h"

#include "font.h"

#define TFB_WIDTH	(32)
#define TFB_HEIGHT	(24)

#define KEY_FACE	(KEY_A | KEY_B | KEY_X | KEY_Y)
#define KEY_DPAD	(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)
#define KEY_OTHR	(KEY_SELECT | KEY_START | KEY_L | KEY_R)
#define KEY_STAT	(KEY_TOUCH | KEY_LID)

#define KEY_BUTTON	(KEY_FACE | KEY_DPAD | KEY_OTHR)
#define KEY_ANY		(KEY_BUTTON  | KEY_STAT)

static int ui_bg[2][4];

enum {
	BG_MAIN	= 3,
	BG_PROM	= 2,
	BG_INFO	= 1,
	BG_ERR	= 0
};

enum {
	TOP_SCREEN		= 0,
	BOTTOM_SCREEN	= 1
};

static inline void _ui_drawc(int screen, int bg, int c, size_t x, size_t y) {
	vu16 *m = bgGetMapPtr(ui_bg[screen][bg]);
	m[y * TFB_WIDTH + x] = c - ' ';
}

static void _ui_tilemap_clr(int screen, int bg)
{
	vu16 *map = bgGetMapPtr(ui_bg[screen][bg]);
	for (int i = 0; i < TFB_WIDTH*TFB_HEIGHT; i++)
		map[i] = 0;
}

static int _ui_waitkey(int keymask) {
	int pressed;
	while(1) { /* key debounce */
		scanKeys();
		pressed = keysHeld();
		if ((pressed & keymask) == 0) break;
		swiWaitForVBlank();
	}

	while(1) {
		scanKeys();
		pressed = keysHeld() & keymask;
		if (pressed) return pressed;
		swiWaitForVBlank();
	}
}

static void _ui_fill(int screen, int bg, int c, size_t x,
							size_t y, size_t w, size_t h) {
	vu16 *map = bgGetMapPtr(ui_bg[screen][bg]) + (y * TFB_WIDTH);
	w += x;
	while(h--) {
		for (size_t _x = x; _x < w; _x++) {
			map[_x] = c - ' ';
		}
		map += TFB_WIDTH;
	}
}

static size_t _ui_strdim(const char *str, size_t *width, size_t *height)
{
	size_t slen = strlen(str), mw = 0, w = 0, h = 0;
	const char *send = str + slen;
	while(str < send) {
		if (*str == '\n') {
			if (w > mw) mw = w;
			w = 0;
			h++;
		} else {
			w++;
		}
		str++;
	}
	if (width) *width = (w > mw) ? w : mw;
	if (height) *height = h;
	return slen;
}

static void ui_drawstr(int screen, int bg, const char *str, size_t x, size_t y)
{
	vu16 *map = bgGetMapPtr(ui_bg[screen][bg]);
	size_t i = y * TFB_WIDTH + x;
	while(*str) {
		if (*str == '\n') {
			i -= i % TFB_WIDTH;
			i += TFB_WIDTH + x;
		} else {
			map[i++] = *str - ' ';
		}
		str++;
	}
}

static int ui_drawstr_center(int screen, int bg, const char *str)
{
	size_t w, h, x, y;

	_ui_strdim(str, &w, &h);

	x = (TFB_WIDTH - w) / 2;
	y = (TFB_HEIGHT - h) / 2;
	ui_drawstr(screen, bg, str, x, y);
	return y + h;
}

void ui_msg(const char *fmt, ...)
{
	va_list va;
	char msg[256];
	size_t y;

	sassert(fmt != NULL, "null format");

	va_start(va, fmt);
	vsnprintf(msg, 255, fmt, va);
	va_end(va);

	msg[255] = '\0';
	y = ui_drawstr_center(TOP_SCREEN, BG_PROM, msg) + 2;
	ui_drawstr(TOP_SCREEN, BG_PROM, "Press any button to continue", 3, y);
	_ui_waitkey(KEY_BUTTON);

	_ui_tilemap_clr(TOP_SCREEN, BG_PROM);
}

bool ui_ask(const char *msg)
{
	int key;
	size_t y;

	sassert(msg != NULL, "null msg");

	y = ui_drawstr_center(TOP_SCREEN, BG_PROM, msg);
	ui_drawstr(TOP_SCREEN, BG_PROM, "(A) Yes / (B) No", 8, y + 2);
	key = _ui_waitkey(KEY_A | KEY_B);
	_ui_tilemap_clr(TOP_SCREEN, BG_PROM);
	return (key & KEY_A) != 0;
}

/* shamelessly ""inspired"" by gm9 */
int ui_menu(int nopt, const char **opt, const char *msg)
{
	size_t msg_w, msg_h, msg_x, maxsw;
	int opt_y, opt_x, ret;

	_ui_tilemap_clr(TOP_SCREEN, BG_PROM);
	_ui_strdim(msg, &msg_w, &msg_h);
	msg_x = (TFB_WIDTH - msg_w) / 2;
	opt_y = msg_h + 3;

	ui_drawstr(TOP_SCREEN, BG_PROM, msg, msg_x, 1);

	maxsw = 0;
	for (int i = 0; i < nopt; i++) {
		size_t slen = strlen(opt[i]);
		if (maxsw < slen) maxsw = slen;
	}

	opt_x = ((TFB_WIDTH - maxsw) / 2) - 2;
	for (int i = 0; i < nopt; i++)
		ui_drawstr(TOP_SCREEN, BG_PROM, opt[i], opt_x + 2, opt_y + i);

	ret = 0;
	while(true) {
		int pressed;
		for (int i = 0; i < nopt; i++)
			_ui_drawc(TOP_SCREEN, BG_PROM, (ret==i)?'>':' ', opt_x, opt_y + i);

		pressed = _ui_waitkey(KEY_UP | KEY_DOWN | KEY_A | KEY_B);
		if (pressed & KEY_UP) ret = (ret + nopt - 1) % nopt;
		else if (pressed & KEY_DOWN) ret = (ret + 1) % nopt;
		else if (pressed & KEY_A) break;
		else if (pressed & KEY_B) {
			ret = -1;
			break;
		}
	}
	_ui_tilemap_clr(TOP_SCREEN, BG_PROM);
	return ret;
}

void ui_progress(uint64_t cur, uint64_t tot, const char *un, const char *msg)
{
	char prog[32];
	int per, barc;
	size_t y, w;

	if (cur > tot) {
		_ui_tilemap_clr(TOP_SCREEN, BG_INFO);
		return;
	}

	per = ((cur * 100) / tot);
	barc = (per * TFB_WIDTH) / 100;

	y = ui_drawstr_center(TOP_SCREEN, BG_INFO, msg);

	_ui_fill(TOP_SCREEN, BG_INFO, '-', 0, y + 1, TFB_WIDTH, 1);
	_ui_fill(TOP_SCREEN, BG_INFO, '-', 0, y + 3, TFB_WIDTH, 1);

	_ui_fill(TOP_SCREEN, BG_INFO, '|', 0, y + 2, barc, 1); /* fullblock */
	_ui_fill(TOP_SCREEN, BG_INFO, ' ', barc, y + 2, TFB_WIDTH - barc, 1); /* halfblock */

	if (un == NULL) un = "";

	prog[31] = '\0';
	w = snprintf(prog, 31, "%llu/%llu %s (%d%%)", cur, tot, un, per);
	ui_drawstr(TOP_SCREEN, BG_INFO, prog, (TFB_WIDTH - w) / 2, y + 4);
}

void ui_reset(void)
{
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	/* initialize backgrounds */
	for (int i = 0; i < 4; i++) {
		ui_bg[0][i] = bgInit(i, BgType_Text4bpp, BgSize_T_256x256, i, 4);
		ui_bg[1][i] = bgInitSub(i, BgType_Text4bpp, BgSize_T_256x256, i, 4);
	}

	/* copy palette into ram */
	dmaCopy(fontPal, BG_PALETTE, fontPalLen);
	dmaCopy(fontPal, BG_PALETTE_SUB, fontPalLen);

	/* copy tiles into vram */
	dmaCopy(fontTiles, bgGetGfxPtr(ui_bg[0][0]), fontTilesLen);
	dmaCopy(fontTiles, bgGetGfxPtr(ui_bg[1][0]), fontTilesLen);

	/* clear tilemaps */
	for (int l = 0; l < 4; l++) {
		_ui_tilemap_clr(TOP_SCREEN, l);
		_ui_tilemap_clr(BOTTOM_SCREEN, l);
	}

	/* enable and show all background layers */
	for (int i = 0; i < 4; i++) {		
		videoBgEnable(i);		
		videoBgEnableSub(i);
	}

	for (int i = 0; i < 3; i++) {
		ui_progress(i, 2, "seconds", "Timer");
		for (int j = 0; j < 60; j++) swiWaitForVBlank();
	}
	ui_progress(1, 0, NULL, NULL);

	ui_msg("got option %d\n",
		ui_menu(3, (const char*[]){"Option A", "This is Option B", "OptC"}, "Menu Header"));

	while(1) swiWaitForVBlank();
}
