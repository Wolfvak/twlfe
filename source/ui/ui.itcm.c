#include <stdarg.h>
#include <stdio.h>
#include <nds.h>

#include "global.h"

#include "ui.h"

#include "font.h"

static int ui_bg[2][4];

#define UI_MSGSCR	(SUBSCR)
#define UI_ASKSCR	(SUBSCR)
#define UI_MENUSCR	(MAINSCR)
#define UI_PROGSCR	(MAINSCR)

#define STRBUF_LEN	(64)

#define UI_FORMAT_HELPER(f, s) \
	va_list va; \
	char s[STRBUF_LEN]; \
	va_start(va, (f)); \
	vsnprintf(s, STRBUF_LEN-1, (f), va);\
	va_end(va); \

/*
 * index zero is always transparent
 * indexes 1-14 are black
 * index 15 is white
 */
static void _ui_init_palette(vu16 *pal)
{
	/* 16 palettes, each with 16 RGB555 colors */
	for (int i = 0; i < 16*16/2; i++) {
		((vu32*)pal)[i] = 0;
	}

	pal[COL_WHITE*16 + 15]		= RGB15(0x1F, 0x1F, 0x1F);
	pal[COL_BLACK*16 + 15]		= RGB15(0x00, 0x00, 0x00);
	pal[COL_RED*16 + 15]		= RGB15(0x1F, 0x00, 0x00);
	pal[COL_GREEN*16 + 15]		= RGB15(0x00, 0x1F, 0x00);
	pal[COL_BLUE*16 + 15]		= RGB15(0x00, 0x00, 0x1F);
	pal[COL_YELLOW*16 + 15]		= RGB15(0x1F, 0x1F, 0x00);
	pal[COL_MAGENTA*16 + 15]	= RGB15(0x1F, 0x00, 0x1F);
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

static void _ui_rect(vu16 *map, int c, size_t x,
					size_t y, size_t w, size_t h)
{
	map += y * TFB_WIDTH;
	w += x;

	if (UNLIKELY(cchr_is_pal(c))) {
		c = cchr_to_pal(c) | CHR_OPAQUE;
	}

	while(h--) {
		for (size_t _x = x; _x < w; _x++)
			map[_x] = c;
		map += TFB_WIDTH;
	}
}

static size_t _ui_strdim(const char *str, u8 *width, size_t *height)
{
	const char *str_s = str;
	size_t w = 0, line = 0;

	while(*str) {
		char c = *str;
		if (UNLIKELY(c == '\n')) {
			if (width) width[line++] = w;
			w = 0;
		} else if (LIKELY(!cchr_is_pal(c))) {
			w++;
		}
		str++;
	}

	if (width) width[line] = w;
	if (height) *height = line;

	return (str - str_s);
}

static void ui_drawmwidthstr(vu16 *map, u8 *x, size_t y, const char *str)
{
	size_t line = 0, i = y * TFB_WIDTH + x[0];
	u16 color = 0;

	while(*str) {
		char c = *str;
		if (UNLIKELY(c == '\n')) {
			i -= i % TFB_WIDTH;
			i += TFB_WIDTH + x[++line];
		} else if (UNLIKELY(cchr_is_pal(c))) {
			color = cchr_to_pal(c);
		} else {
			map[i++] = (u16)c | color;
		}

		str++;
	}
}

int ui_waitkey(int keymask) {
	int pressed;
	while(1) { /* key debounce */
		scanKeys();
		pressed = keysHeld() & keymask;
		if (!pressed) break;
		swiWaitForVBlank();
	}

	while(1) {
		scanKeys();
		pressed = keysHeld() & keymask;
		if (pressed) return pressed;
		swiWaitForVBlank();
	}
}

void ui_reset(void)
{
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
	_ui_init_font(font, 256, bgGetGfxPtr(ui_bg[MAINSCR][0]));
	_ui_init_font(font, 256, bgGetGfxPtr(ui_bg[SUBSCR][0]));

	/* clear tilemaps, enable bg layer */
	for (int l = 0; l < 4; l++) {
		ui_tilemap_clr(ui_map(MAINSCR, l));
		ui_tilemap_clr(ui_map(SUBSCR, l));
		videoBgEnable(l);
		videoBgEnableSub(l);
	}
}

vu16 *ui_map(int screen, int bg)
{
	return bgGetMapPtr(ui_bg[screen][bg]);
}

void ui_drawstr(vu16 *map, size_t x, size_t y, const char *str)
{
	size_t i = y * TFB_WIDTH + x;
	u16 color = 0;

	while(*str) {
		char c = *str;
		if (UNLIKELY(c == '\n')) {
			i -= i % TFB_WIDTH;
			i += TFB_WIDTH + x;
		} else if (UNLIKELY(cchr_is_pal(c))) {
			color = cchr_to_pal(c);
		} else {
			map[i++] = (u16)c | color;
		}

		str++;
	}
}

void ui_drawstrf(vu16 *map, size_t x, size_t y, const char *fmt, ...)
{
	UI_FORMAT_HELPER(fmt, str);
	ui_drawstr(map, x, y, str);
}

int ui_drawstr_center(vu16 *map, const char *str)
{
	size_t h, y;
	u8 w[TFB_HEIGHT];

	_ui_strdim(str, w, &h);
	y = (TFB_HEIGHT - h) / 2;

	for (int i = 0; i <= h; i++)
		w[i] = (TFB_WIDTH - w[i]) / 2;

	ui_drawmwidthstr(map, w, y, str);
	return y + h;
}

int ui_drawstr_centerf(vu16 *map, const char *fmt, ...)
{
	UI_FORMAT_HELPER(fmt, str);
	return ui_drawstr_center(map, str);
}

int ui_drawstr_xcenter(vu16 *map, size_t y, const char *str)
{
	size_t h;
	u8 w[TFB_HEIGHT];
	_ui_strdim(str, w, &h);

	for (int i = 0; i <= h; i++)
		w[i] = (TFB_WIDTH - w[i]) / 2;

	ui_drawmwidthstr(map, w, y, str);
	return y + h;
}

int ui_drawstr_xcenterf(vu16 *map, size_t y, const char *fmt, ...)
{
	UI_FORMAT_HELPER(fmt, str);
	return ui_drawstr_xcenter(map, y, str);
}

int ui_drawstr_ycenter(vu16 *map, size_t x, const char *str)
{
	size_t h, y;
	u8 xpos[TFB_HEIGHT];

	_ui_strdim(str, NULL, &h);
	for (int i = 0; i <= h; i++)
		xpos[i] = x;

	y = (TFB_HEIGHT - h) / 2;
	ui_drawmwidthstr(map, xpos, y, str);
	return y + h;
}

int ui_drawstr_ycenterf(vu16 *map, size_t x, const char *fmt, ...)
{
	UI_FORMAT_HELPER(fmt, str);
	return ui_drawstr_ycenter(map, x, str);
}

void ui_msg(const char *str)
{
	vu16 *map = ui_map(UI_MSGSCR, BG_PROM);
	size_t y = ui_drawstr_center(map, str);

	ui_drawstr_xcenter(map, y + 2, "Press any button to continue");
	ui_waitkey(KEY_BUTTON);
	ui_tilemap_clr(map);
}

void ui_msgf(const char *fmt, ...)
{
	UI_FORMAT_HELPER(fmt, str);
	ui_msg(str);
}

bool ui_ask(const char *str)
{
	vu16 *map = ui_map(UI_ASKSCR, BG_PROM);
	size_t y;
	int key;

	ui_tilemap_set(map, CHR_OPAQUE);
	y = ui_drawstr_center(map, str);
	ui_drawstr_xcenter(map, y + 2, "(A) Yes / (B) No");
	key = ui_waitkey(KEY_A | KEY_B);
	ui_tilemap_clr(map);
	return (key & KEY_A) != 0;
}

bool ui_askf(const char *fmt, ...)
{
	UI_FORMAT_HELPER(fmt, str);
	return ui_ask(str);
}

/* shamelessly ""inspired"" by gm9 */
int ui_menu(int nopt, const ui_menu_entry *opt, const char *str)
{
	int opt_y, opt_xl, opt_xr, winsz, wins, wine, sel;
	bool redraw, run_menu;
	vu16 *map, *descmap;
	size_t maxsw;

	sassert(nopt > 0, "no options provided");

	map = ui_map(UI_MENUSCR, BG_PROM);
	descmap = ui_map(!UI_MENUSCR, BG_PROM);

	ui_tilemap_set(map, CHR_OPAQUE);
	ui_tilemap_set(descmap, CHR_OPAQUE);
	opt_y = ui_drawstr_xcenter(map, 1, str) + 3;

	maxsw = 0;
	for (int i = 0; i < nopt; i++) {
		size_t slen = 0;
		while(opt[i].name[slen] != '\0') slen++;
		if (maxsw < slen) maxsw = slen;
	}

	opt_xl = ((TFB_WIDTH - maxsw) / 2) - 1;
	if (opt_xl & 1) opt_xl--;
	if (opt_xl < 0) opt_xl = 0;

	opt_xr = ((TFB_WIDTH + maxsw) / 2) + 1;
	if (opt_xr & 1) opt_xr++;
	if (opt_xr >= TFB_WIDTH) opt_xr = TFB_WIDTH-1;

	sel = 0;
	winsz = (nopt > 20) ? 20 : nopt;
	wins = 0;
	wine = winsz;
	redraw = true;
	run_menu = true;
	while(run_menu) {
		int pressed;
		const char *description;

		if (UNLIKELY(redraw)) {
			redraw = false;
			swiWaitForVBlank();
			_ui_rect(map, CHR_OPAQUE, opt_xl + 2, opt_y, maxsw, winsz);
			for (int i = 0; i < winsz; i++) {
				int opti = wins + i;
				if (opti >= nopt) break;
				ui_drawstr(map, opt_xl + 2, opt_y + i, opt[opti].name);
			}
		}

		for (int i = 0; i < winsz; i++) {
			int opti = wins + i;
			ui_drawc(map, (sel==opti) ? '[' : ' ', opt_xl, opt_y + i);
			ui_drawc(map, (sel==opti) ? ']' : ' ', opt_xr, opt_y + i);
		}

		description = opt[sel].desc;
		if (description == NULL) description = "No description available";

		/* TODO: clear map + draw description here */

		pressed = ui_waitkey(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B);
		PROCESS_KEYS(pressed) {
			case KEY_UP:
				sel--;
				break;

			case KEY_DOWN:
				sel++;
				break;

			case KEY_LEFT:
				if (sel != wins) sel = wins;
				else sel--;
				break;

			case KEY_RIGHT:
				if (sel != wine-1) sel = wine-1;
				else sel++;
				break;

			case KEY_A:
				PROCESS_KEYS_STOP;
				run_menu = false;
				break;

			case KEY_B:
				PROCESS_KEYS_STOP;
				run_menu = false;
				sel = -1;
				break;
		}

		if (run_menu) {
			sel = CLAMP(sel, 0, nopt-1);
		}

		if (sel >= wine) {
			wins = wine;
			wine += winsz;
			wine = CLAMP(wine, 0, nopt);
			redraw = true;
		}

		if (sel < wins) {
			wine = wins;
			wins -= winsz;
			wins = CLAMP(wins, 0, nopt);
			redraw = true;
		}
	}

	ui_tilemap_clr(map);
	ui_tilemap_clr(descmap);
	return sel;
}

int ui_menuf(int nopt, const ui_menu_entry *opt, const char *fmt, ...)
{
	UI_FORMAT_HELPER(fmt, str);
	return ui_menu(nopt, opt, str);
}

void ui_progress(uint64_t cur, uint64_t tot, const char *un, const char *str)
{
	vu16 *map;
	int per, barc, y;

	map = ui_map(UI_PROGSCR, BG_PROM);

	if (cur > tot) {
		ui_tilemap_clr(map);
		return;
	}

	if (un == NULL) un = "";

	per = ((cur * 100) / tot);
	barc = (per * TFB_WIDTH) / 100;

	swiWaitForVBlank();
	ui_tilemap_set(map, CHR_OPAQUE);

	y = (TFB_WIDTH / 2) - 3;
	ui_drawstr_xcenter(map, y, str);

	_ui_rect(map, CHR_SEPTOP, 0, y + 1, TFB_WIDTH, 1);
	_ui_rect(map, CHR_SEPBOT, 0, y + 3, TFB_WIDTH, 1);

	static const char ui_prog_char[] = {CHR_PBARS, CHR_PBARM, CHR_PBARE};
	for (size_t i = 0; i < barc; i++) {
		size_t idx = (i == 0) ? 0 : ((i == 32) ? 2 : 1);
		ui_drawc(map, ui_prog_char[idx], i, y + 2);
	}

	_ui_rect(map, CHR_OPAQUE, barc, y + 2, TFB_WIDTH - barc, 1);
	ui_drawstr_xcenterf(map, y + 3, "%llu/%llu %s (%d%%)", cur, tot, un, per);
}

void ui_progressf(uint64_t cur, uint64_t tot, const char *un, const char *fmt, ...)
{
	UI_FORMAT_HELPER(fmt, str);
	ui_progress(cur, tot, un, str);
}
