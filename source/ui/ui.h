#ifndef UI_H__
#define UI_H__

#include <nds.h>
#include <stdarg.h>

#define CHR_TRANSPARENT	(0x00)
#define CHR_OPAQUE		(0x01)

/*
 * sets the video modes, initializes tiles and palettes
 */
void ui_reset(void);

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
