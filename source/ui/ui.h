#ifndef UI_H__
#define UI_H__

#include <nds.h>
#include <stdarg.h>

#include "global.h"

#define KEY_FACE	(KEY_A | KEY_B | KEY_X | KEY_Y)
#define KEY_DPAD	(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)
#define KEY_OTHR	(KEY_SELECT | KEY_START | KEY_L | KEY_R)
#define KEY_STAT	(KEY_TOUCH | KEY_LID)

#define KEY_BUTTON	(KEY_FACE | KEY_DPAD | KEY_OTHR)
#define KEY_ANY		(KEY_BUTTON  | KEY_STAT)

#define TFB_WIDTH	(32)
#define TFB_HEIGHT	(24)

#define CHR_TRANSPARENT	(0x00)
#define CHR_OPAQUE		(0x01)

enum {
	MAINSCR	= 0,
	SUBSCR	= 1
};

enum {
	BG_MAIN	= 3,
	BG_PROM	= 2,
	BG_INFO	= 1,
	BG_ERR	= 0
};

enum {
	COL_WHITE	= 0,
	COL_BLACK	= 1,
	COL_RED		= 2,
	COL_GREEN	= 3,
	COL_BLUE	= 4,
	COL_YELLOW	= 5,
	COL_MAGENTA	= 6,
};

#define UI_WHITE	"\xFF"
#define UI_BLACK	"\xFE"
#define UI_RED		"\xFD"
#define UI_GREEN	"\xFC"
#define UI_BLUE		"\xFB"
#define UI_YELLOW	"\xFA"
#define UI_MAGENTA	"\xF9"

#define CCHR_MAX	(255)
#define CCHR_MIN	(CCHR_MAX - COL_MAGENTA)

typedef struct {
	const char *name;
	const char *desc;
} ui_menu_entry;

/*
 * waits for one of the keys in `keymask` to be pressed
 * includes key debounce in case the key is already held down
 */
int ui_waitkey(int keymask);

/* sets the video modes, initializes tiles and palettes */
void ui_reset(void);

/* get the charmap for the selected screen and background layer */
vu16 *ui_map(int screen, int bg);

static inline int cchr_is_pal(int c)
{
	return (c >= CCHR_MIN && c <= CCHR_MAX);
}

/* transforms a color indicator char into the palette offset bit thing */
static inline u16 cchr_to_pal(int c)
{
	return TILE_PALETTE(CCHR_MAX - c);
}

/* fills the map with `c` */
static inline void ui_tilemap_set(vu16 *map, int c) {
	size_t mapsz = TFB_WIDTH * TFB_HEIGHT;
	if (UNLIKELY(cchr_is_pal(c))) {
		c = cchr_to_pal(c) | CHR_OPAQUE;
	}
	while(mapsz--) *(map++) = c;
}

/*
 * fills the map with transparent chars
 * so the lower priority backgrounds show through
 */
static inline void ui_tilemap_clr(vu16 *map) {
	ui_tilemap_set(map, CHR_TRANSPARENT);
}

/* draw a single character to the map at the specified coordinates */
static inline void ui_drawc(vu16 *map, int c, size_t x, size_t y) {
	if (UNLIKELY(cchr_is_pal(c))) {
		c = cchr_to_pal(c) | CHR_OPAQUE;
	}
	map[y * TFB_WIDTH + x] = c;
}

/* draws a string to the map at the specified coordinates */
void ui_drawstr(vu16 *map, size_t x, size_t y, const char *str);
void ui_drawstrf(vu16 *map, size_t x, size_t y, const char *fmt, ...);

/*
 * draws a string to the map
 * the coordinates are inferred from the string width and height
 * in such a way that the string appears as centered as possible
 */
int ui_drawstr_center(vu16 *map, const char *str);
int ui_drawstr_centerf(vu16 *map, const char *fmt, ...);

/*
 * draws a string to the map
 * the x coordinate is inferred from the string width in such a way
 * that thestring appears as centered as possible in the X axis
 */
int ui_drawstr_xcenter(vu16 *map, size_t y, const char *str);
int ui_drawstr_xcenterf(vu16 *map, size_t y, const char *fmt, ...);

/* same as above, but with the Y axis */
int ui_drawstr_ycenter(vu16 *map, size_t x, const char *str);
int ui_drawstr_ycenterf(vu16 *map, size_t x, const char *fmt, ...);

/*
 * displays a formatted message on screen
 * can exit with any button
 */
void ui_msg(const char *str);
void ui_msgf(const char *fmt, ...);

/*
 * displays `msg` and asks user
 * returns true if answered affirmatively (A)
 * otherwise returns false
 */
bool ui_ask(const char *str);
bool ui_askf(const char *fmt, ...);

/*
 * displays `msg` as header and `nopt` options below
 * returns option index or -1 if menu was cancelled
 */
int ui_menu(int nopt, const ui_menu_entry *opt, const char *msg);
int ui_menuf(int nopt, const ui_menu_entry *opt, const char *fmt, ...);

/*
 * displays `msg` and prompts the user for input:
 * - inputs: input a string, default data must be in `str` already
 * - inputd: input an integer in decimal format
 * - inputh: input an integer in hexadecimal format
 */
/*size_t ui_inputs(size_t slen, char *str, const char *msg);
uint64_t ui_inputd(uint64_t def, const char *msg);
uint64_t ui_inputh(uint64_t def, const char *msg);*/

/*
 * displays `msg` and shows percentage bar according to `cur` and `tot`
 * if `cur` > `tot` the operation has ended and the bar is cleared
 * allows custom units through the `un` string
 */
void ui_progress(uint64_t cur, uint64_t tot, const char *un, const char *msg);
void ui_progressf(uint64_t cur, uint64_t tot, const char *un, const char *fmt, ...);

#endif /* UI_H__ */
