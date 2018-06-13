#ifndef UI_H__
#define UI_H__

#include <nds.h>
#include <stdarg.h>

#define KEY_FACE	(KEY_A | KEY_B | KEY_X | KEY_Y)
#define KEY_DPAD	(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)
#define KEY_OTHR	(KEY_SELECT | KEY_START | KEY_L | KEY_R)
#define KEY_STAT	(KEY_TOUCH | KEY_LID)

#define KEY_BUTTON	(KEY_FACE | KEY_DPAD | KEY_OTHR)
#define KEY_ANY		(KEY_BUTTON  | KEY_STAT)

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


/*
 * sets the video modes, initializes tiles and palettes
 */
void ui_reset(void);

/*
 * waits for one of the keys in `keymask` to be pressed
 * includes key debounce in case the key is already held down
 */
int ui_waitkey(int keymask);

void ui_tilemap_clr(int screen, int bg);
void ui_drawc(int screen, int bg, int c, size_t x, size_t y);

void ui_drawstr(int screen, int bg, const char *str, size_t x, size_t y);
int ui_drawstr_center(int screen, int bg, const char *str);
void ui_drawstr_xcenter(int screen, int bg, const char *str, size_t y);

/*
 * displays a formatted message on screen
 * can exit with any button
 */
void ui_msg(const char *fmt, ...);

/*
 * displays `msg` and asks user
 * returns true if answered affirmatively (A)
 * otherwise returns false
 */
bool ui_ask(const char *msg);

/*
 * displays `msg` as header and `nopt` options below
 * returns option index or -1 if menu was cancelled
 */
int ui_menu(int nopt, const char **opt, const char *msg);

/*
 * displays `msg` and prompts the user for input:
 * - inputs: input a string, default data must be in `str` already
 * - inputd: input an integer in decimal format
 * - inputh: input an integer in hexadecimal format
 */
size_t ui_inputs(size_t slen, char *str, const char *msg);
uint64_t ui_inputd(uint64_t def, const char *msg);
uint64_t ui_inputh(uint64_t def, const char *msg);

/*
 * displays `msg` and shows percentage bar according to `cur` and `tot`
 * if `cur` > `tot` the operation has ended and the bar is cleared
 * allows custom units through the `un` string
 */
void ui_progress(uint64_t cur, uint64_t tot, const char *un, const char *msg);

#endif /* UI_H__ */
