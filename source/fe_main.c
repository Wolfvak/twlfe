#include <nds.h>

#include <stdio.h>
#include <string.h>

#include "err.h"
#include "ui.h"
#include "vfs.h"

#include "pstor.h"
#include "vfs_glue.h"

#define FE_PATHBUF	(SIZE_KIB(128))
#define FE_MAXITEM	(16384)

static int scan_dir(pstor_t *ps, const char *dir)
{
	int dd, res, ret = 0;

	dd = vfs_diropen(dir);
	if (IS_ERR(dd)) return dd;

	pstor_clear(ps);
	for (ret = 0; ret < FE_MAXITEM; ret++) {
		dirinf_t inf;

		res = vfs_dirnext(dd, &inf);
		if (IS_ERR(res)) break;

		res = pstor_concat(ps, inf.path);
		if (IS_ERR(res)) break;
	}

	vfs_dirclose(dd);
	if (IS_ERR(res) && (res != -ERR_NOTFOUND)) return res;
	return ret;
}

static inline bool path_is_dir(char *p) {
	return p[strlen(p)] == '/';
}

/*
 * basically the same as ui_menu but uses the
 * pathstore and draws strings in different positions
 */
static int fe_pstormenu(pstor_t *ps)
{
	int res, sel = 0, base = 0, count = pstor_count(ps);
	vu16 *map = ui_map(MAINSCR, BG_MAIN);
	bool redraw_menu = true;

	if (count == 0) return -1;

	while(1) {
		int pressed, end;

		end = base + TFB_HEIGHT;
		if (end > count) {
			end = count;
		}

		if (redraw_menu) {
			char drawpath[MAX_PATH + 1];

			redraw_menu = false;

			swiWaitForVBlank();
			ui_tilemap_clr(map);

			for (int i = base; i < end; i++) {
				res = pstor_getpath(ps, drawpath, MAX_PATH, i);
				if (IS_ERR(res)) {
					ui_msgf("failed to get path:\n%s", err_getstr(res));
					return res;
				}
				ui_drawstr(map, 2, i - base, drawpath);
			}
		}

		for (int i = base; i < end; i++) {
			ui_drawc(map, (i == sel) ? '>' : ' ', 0, i - base);
		}

		pressed = ui_waitkey(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A|KEY_B);

		if (pressed & KEY_UP) {
			sel--;
		} else if (pressed & KEY_DOWN) {
			sel++;
		} else if (pressed & KEY_LEFT) {
			sel -= TFB_HEIGHT;
		} else if (pressed & KEY_RIGHT) {
			sel += TFB_HEIGHT;
		}

		if (pressed & KEY_A) {
			break;
		} else if (pressed & KEY_B) {
			sel = -1;
			break;
		}

		if (sel < 0) sel = 0;
		else if (sel >= count) sel = count-1;

		if (sel >= end) {
			base = end;
			end += TFB_HEIGHT;
			if (end > count) end = count;
			redraw_menu = true;
		}

		if (sel < base) {
			end = base;
			base -= TFB_HEIGHT;
			if (base < 0) base = 0;
			redraw_menu = true;
		}
	}

	return sel;
}

void fe_main(char drv)
{
	int res, sel;
	pstor_t ps;

	res = pstor_init(&ps, FE_PATHBUF, FE_MAXITEM);
	if (IS_ERR(res)) {
		ui_msgf("failed to start path storage:\n%s", err_getstr(res));
		while(1) swiWaitForVBlank();
	}

	char t[3];
	t[2] = '\0';
	for (int i = 'A'; i <= 'Z'; i++) {
		t[0] = i;
		for (int j = 'A'; j <= 'Z'; j++) {
			t[1] = j;
			res = pstor_concat(&ps, t);
			if (IS_ERR(res)) {
				ui_msgf("failed to concat:\n%s", err_getstr(res));
				while(1) swiWaitForVBlank();
			}
		}
	}
	ui_msg("no error");

	while(1) {
		sel = fe_pstormenu(&ps);
		ui_msgf("you picked %d\n", sel);
	}

}
