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
	MAINSCR	= 0,
	SUBSCR	= 1
};

#define UI_MSGSCR	(SUBSCR)
#define UI_ASKSCR	(SUBSCR)
#define UI_MENUSCR	(SUBSCR)
#define UI_PROGSCR	(SUBSCR)

static void _ui_init_palette(vu16 *pal)
{
	for (int i = 0; i < 15; i++)
		pal[i] = 0;
	pal[15] = RGB15(0x1F, 0x1F, 0x1F);
}

/*
 * char zero is always a transparent tile
 * char one is always a full opaque block
 * everything else is transcoded from the font
 */

static void _ui_init_font(const unsigned char *font, size_t chars, vu16 *tile)
{
	vu32 *tile32 = (vu32*)tile;

	for (int i = 0; i < 8; i++)
		tile32[i] = 0;

	for (int i = 8; i < 16; i++)
		tile32[i] = 0x11111111;

	for (size_t i = 2; i < chars; i++) {
		const unsigned char *current_char = &font[i * 8];
		vu32 *current_tile = &tile32[i * 8];
		for (int j = 0; j < 8; j++) {
			unsigned char c = current_char[j];
			u32 tile_row = 0x11111111;
			while(c) {
				int ctz = (31 - __builtin_clz(c));
				tile_row |= (0xF << ((7 - ctz) << 2));
				c &= ~(1 << ctz);
			}
			current_tile[j] = tile_row;
		}
	}
}

static inline void _ui_drawc(int screen, int bg, int c, size_t x, size_t y) {
	vu16 *m = bgGetMapPtr(ui_bg[screen][bg]);
	m[y * TFB_WIDTH + x] = c;
}

static void _ui_tilemap_set(int screen, int bg, int c)
{
	vu16 *map = bgGetMapPtr(ui_bg[screen][bg]);
	for (int i = 0; i < TFB_WIDTH*TFB_HEIGHT; i++)
		map[i] = c;
}

static void _ui_tilemap_clr(int screen, int bg)
{
	_ui_tilemap_set(screen, bg, CHR_TRANSPARENT);
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
		for (size_t _x = x; _x < w; _x++)
			map[_x] = c;
		map += TFB_WIDTH;
	}
}

static size_t _ui_strdim(const char *str, size_t *width, size_t *height)
{
	const char *strs = str;
	size_t mw = 0, w = 0, h = 0;
	while(*str) {
		switch(*str) {
			case '\n':
				mw = (w > mw) ? w : mw;
				w = 0;
				h++;
				break;

			default:
				w++;
				break;
		}
		str++;
	}
	if (width) *width = (w > mw) ? w : mw;
	if (height) *height = h;
	return (str - strs);
}

static void ui_drawstr(int screen, int bg, const char *str, size_t x, size_t y)
{
	vu16 *map = bgGetMapPtr(ui_bg[screen][bg]);
	size_t i = y * TFB_WIDTH + x;
	while(*str) {
		switch(*str) {
			case '\n':
				i -= i % TFB_WIDTH;
				i += TFB_WIDTH + x;
				break;

			default:
				map[i++] = *str;
				break;
		}
		str++;
	}
}

static int ui_drawstr_center(int screen, int bg, const char *str)
{
	size_t w, h, y;

	_ui_strdim(str, &w, &h);
	y = (TFB_HEIGHT - h) / 2;
	ui_drawstr(screen, bg, str, (TFB_WIDTH - w) / 2, y);
	return y + h;
}

void ui_msg(const char *fmt, ...)
{
	va_list va;
	char msg[256];
	size_t y;

	va_start(va, fmt);
	vsnprintf(msg, 255, fmt, va);
	va_end(va);

	msg[255] = '\0';
	y = ui_drawstr_center(UI_MSGSCR, BG_PROM, msg) + 2;
	ui_drawstr(UI_MSGSCR, BG_PROM, "Press any button to continue", 3, y);
	_ui_waitkey(KEY_BUTTON);

	_ui_tilemap_clr(UI_MSGSCR, BG_PROM);
}

bool ui_ask(const char *msg)
{
	int key;
	size_t y;

	_ui_tilemap_set(UI_ASKSCR, BG_PROM, CHR_OPAQUE);
	y = ui_drawstr_center(UI_ASKSCR, BG_PROM, msg);
	ui_drawstr(UI_ASKSCR, BG_PROM, "(A) Yes / (B) No", 8, y + 2);
	key = _ui_waitkey(KEY_A | KEY_B);
	_ui_tilemap_clr(UI_ASKSCR, BG_PROM);
	return (key & KEY_A) != 0;
}

/* shamelessly ""inspired"" by gm9 */
int ui_menu(int nopt, const char **opt, const char *msg)
{
	size_t msg_w, msg_h, msg_x, maxsw;
	int opt_y, opt_x, ret;

	int winsz, wins, wine;
	bool redraw;

	_ui_fill(UI_MENUSCR, BG_PROM, CHR_OPAQUE, 0, 0, TFB_WIDTH, TFB_HEIGHT);
	_ui_strdim(msg, &msg_w, &msg_h);
	msg_x = (TFB_WIDTH - msg_w) / 2;
	opt_y = msg_h + 3;

	ui_drawstr(UI_MENUSCR, BG_PROM, msg, msg_x, 1);

	maxsw = 0;
	for (int i = 0; i < nopt; i++) {
		size_t slen = strlen(opt[i]);
		if (maxsw < slen) maxsw = slen;
	}

	opt_x = ((TFB_WIDTH - maxsw) / 2) - 2;

	ret = 0;
	winsz = (nopt > 20) ? 20 : nopt;
	wins = 0;
	wine = winsz;
	redraw = true;
	while(true) {
		int pressed;

		if (redraw) {
			swiWaitForVBlank();
			_ui_fill(UI_MENUSCR, BG_PROM, CHR_OPAQUE, opt_x + 2, opt_y, maxsw, winsz);
			for (int i = 0; i < winsz; i++) {
				int opti = wins + i;
				if (opti >= nopt) break;
				ui_drawstr(UI_MENUSCR, BG_PROM, opt[opti], opt_x + 2, opt_y + i);
			}
			redraw = false;
		}

		for (int i = 0; i < winsz; i++) {
			int opti = wins + i;
			_ui_drawc(UI_MENUSCR, BG_PROM, (ret==opti)?'>':' ', opt_x, opt_y + i);
		}

		pressed = _ui_waitkey(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B);
		if (pressed & KEY_UP) ret--;
		else if (pressed & KEY_DOWN) ret++;

		if (pressed & KEY_LEFT) {
			if (ret != wins) ret = wins;
			else ret--;
		} else if (pressed & KEY_RIGHT) {
			if (ret != wine-1) ret = wine-1;
			else ret++;
		}

		if (pressed & KEY_A) break;
		if (pressed & KEY_B) {
			ret = -1;
			break;
		}

		if (ret < 0) ret = nopt-1;
		if (ret >= nopt) ret = 0;

		if (ret >= wine) {
			wins = wine;
			wine += winsz;
			if (wine > nopt) wine = nopt;
			redraw = true;
		}

		if (ret < wins) {
			wine = wins;
			wins -= winsz;
			if (wins < 0) wins = 0;
			redraw = true;
		}
	}
	_ui_tilemap_clr(UI_MENUSCR, BG_PROM);
	return ret;
}

void ui_progress(uint64_t cur, uint64_t tot, const char *un, const char *msg)
{
	char prog[32];
	int per, barc;
	size_t w, h, x, y;

	if (cur > tot) {
		_ui_tilemap_clr(UI_PROGSCR, BG_INFO);
		return;
	}

	per = ((cur * 100) / tot);
	barc = (per * TFB_WIDTH) / 100;

	_ui_strdim(msg, &w, &h);

	x = (TFB_WIDTH - w) / 2;
	y = ((TFB_HEIGHT - h) / 2) - 3;

	swiWaitForVBlank();
	_ui_fill(UI_PROGSCR, BG_INFO, CHR_OPAQUE, 0, y, TFB_WIDTH, 3);

	ui_drawstr(UI_PROGSCR, BG_INFO, msg, x, y);

	_ui_fill(UI_PROGSCR, BG_INFO, CHR_SEPTOP, 0, y + 1, TFB_WIDTH, 1);
	_ui_fill(UI_PROGSCR, BG_INFO, CHR_SEPBOT, 0, y + 3, TFB_WIDTH, 1);

	if (barc >= 0) {
		_ui_drawc(UI_PROGSCR, BG_INFO, CHR_PBARS, 0, y + 2);
		_ui_fill(UI_PROGSCR, BG_INFO, CHR_PBARM, 1, y + 2, barc - 1, 1);
		if (barc == TFB_WIDTH)
			_ui_drawc(UI_PROGSCR, BG_INFO, CHR_PBARE, TFB_WIDTH - 1, y + 2);
	}
	_ui_fill(UI_PROGSCR, BG_INFO, CHR_OPAQUE, barc, y + 2, TFB_WIDTH - barc, 1);

	if (un == NULL) un = "";

	prog[31] = '\0';
	w = snprintf(prog, 31, "%llu/%llu %s (%d%%)", cur, tot, un, per);
	ui_drawstr(UI_PROGSCR, BG_INFO, prog, (TFB_WIDTH - w) / 2, y + 3);
}

void ui_reset(void)
{
	u16 *font_tiles;

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	/* initialize backgrounds */
	for (int i = 0; i < 4; i++) {
		ui_bg[MAINSCR][i] = bgInit(i, BgType_Text4bpp, BgSize_T_256x256, i, 4);
		ui_bg[SUBSCR][i] = bgInitSub(i, BgType_Text4bpp, BgSize_T_256x256, i, 4);
	}

	/* initialize palettes */
	_ui_init_palette(BG_PALETTE);
	_ui_init_palette(BG_PALETTE_SUB);

	/* intitialize font tiles */
	font_tiles = malloc(256 * 32);
	sassert(font_tiles, "this is fine");

	_ui_init_font(font, 256, font_tiles);
	dmaCopy(font_tiles, bgGetGfxPtr(ui_bg[MAINSCR][0]), 256 * 32);
	dmaCopy(font_tiles, bgGetGfxPtr(ui_bg[SUBSCR][0]), 256 * 32);

	free(font_tiles);

	/* clear tilemaps */
	for (int l = 0; l < 4; l++) {
		_ui_tilemap_clr(MAINSCR, l);
		_ui_tilemap_clr(SUBSCR, l);
	}

	/* enable and show all background layers */
	for (int i = 0; i < 4; i++) {		
		videoBgEnable(i);		
		videoBgEnableSub(i);
	}
	return;
}
