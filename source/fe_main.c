#include <nds.h>

#include <stdio.h>
#include <string.h>

#include "err.h"
#include "ui.h"
#include "vfs.h"

#include "global.h"

#include "bp.h"
#include "pstor.h"
#include "vfs_glue.h"

#define FE_PATHBUF	(SIZE_KIB(2048))
#define FE_MAXITEM	(16384)

static int scan_dir(pstor_t *ps, const char *dir)
{
	int dd, res = 0;
	size_t max;

	if (ps == NULL) return -ERR_MEM;
	max = pstor_max(ps) - 1;

	dd = vfs_diropen(dir);
	if (IS_ERR(dd)) return dd;

	pstor_reset(ps);

	res = pstor_add(ps, "../");
	if (IS_ERR(res)) return res;

	for (int i = 0; i < max; i++) {
		dirinf_t inf;

		res = vfs_dirnext(dd, &inf);
		if (IS_ERR(res)) break;

		res = pstor_add(ps, inf.path);
		if (IS_ERR(res)) break;
	}

	vfs_dirclose(dd);
	if (IS_ERR(res) && (res != -ERR_NOTFOUND)) return res;

	return 0;
}

static inline bool path_is_dir(char *p, size_t l) {
	return p[l - 1] == '/';
}

/*
 * basically the same as ui_menu but uses the
 * pathstore and draws strings in different positions
 */

#define FE_PSTORM_X		(2)
#define FE_PSTORM_Y		(2)
#define FE_PSTORM_YSZ	(TFB_HEIGHT - 2)

static int fe_filemenu(vu16 *map, int *keys, pstor_t *ps, bp_t *cb)
{
	int res, sel = 0, base = 0, count = pstor_count(ps);
	bool redraw_menu = true;

	if (count == 0) {
		*keys = 0;
		return -1;
	}

	bp_clearall(cb);

	while(1) {
		int end = CLAMP(base + FE_PSTORM_YSZ, 0, count);

		if (UNLIKELY(redraw_menu)) {
			redraw_menu = false;

			swiWaitForVBlank();
			ui_tilemap_clr(map);

			for (int i = base; i < end; i++) {
				char drawpath[MAX_PATH + 1];
				res = pstor_get(ps, drawpath, MAX_PATH, i);
				if (IS_ERR(res)) return res;

				ui_drawstr(map, FE_PSTORM_X, FE_PSTORM_Y + i - base, drawpath);
			}
		}

		for (int i = base; i < end; i++) {
			int yc = FE_PSTORM_Y + i - base;
			ui_drawc(map, (i == sel) ? '>' : ' ', 0, yc);
			ui_drawc(map, bp_tst(cb, i) ? '^' : ' ', 1, yc);
		}

		*keys = ui_waitkey(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A|KEY_B|KEY_Y|KEY_R);
		switch(*keys) {
			case KEY_Y:
			case KEY_A:
				return sel;

			case KEY_B:
				return 0;

			case KEY_R:
				bp_xor(cb, sel);
				break;

			case KEY_UP:
				sel--;
				break;

			case KEY_DOWN:
				sel++;
				break;

			case KEY_LEFT:
				sel -= FE_PSTORM_YSZ;
				break;

			case KEY_RIGHT:
				sel += FE_PSTORM_YSZ;
				break;

			default:
				break;
		}

		sel = CLAMP(sel, 0, count - 1);

		if (sel >= end) {
			base = end;
			end += FE_PSTORM_YSZ;
			end = CLAMP(end, 0, count);
			redraw_menu = true;
		}

		if (sel < base) {
			end = base;
			base -= FE_PSTORM_YSZ;
			base = CLAMP(base, 0, count);
			redraw_menu = true;
		}
	}

	return sel;
}

void fe_main(char drv, pstor_t *paths, pstor_t *clippaths, vu16 *map)
{
	char cwd[MAX_PATH + 1];
	int res, sel, rectr;
	bp_t cb;

	if (pstor_max(paths) != pstor_max(clippaths)) {
		ui_msg("someone seriously needs to\npunch Wolfvak right now");
		return;
	}

	if (bp_init(&cb, pstor_max(paths)) < 0) {
		ui_msg("failed to allocate clipboard? what?");
		return;
	}

	sprintf(cwd, "%c:/", drv);

	rectr = 0;
	while(1) {
		int keys;

		bp_clearall(&cb);

		res = scan_dir(paths, cwd);
		if (IS_ERR(res)) {
			ui_msgf("Failed to scan dir\n\"%s\"\n%s", cwd, err_getstr(res));
			break;
		}

		sel = fe_filemenu(map, &keys, paths, &cb);
		if (sel < 0) {
			break;
		} else if (sel == 0) {
			if (rectr == 0) break;
			else {
				rectr--;
				path_basedir(cwd);
			}
		} else {
			char fpath[MAX_PATH + 1];
			int cwdlen = strlen(cwd), fpath_rem = MAX_PATH - cwdlen;

			memset(fpath, 0, MAX_PATH + 1);
			strcpy(fpath, cwd);

			if (keys & KEY_Y) {
				pstor_reset(clippaths);

				while(bp_setcnt(&cb)) {
					char fpath[MAX_PATH + 1];
					int idx = bp_find_set(&cb);

					bp_clr(&cb, idx);
					pstor_get(paths, &fpath[cwdlen], fpath_rem, idx);
					pstor_add(clippaths, &fpath[cwdlen]);
				}
				bp_clearall(&cb);
			} else if (keys & KEY_A) {
				int plen = pstor_get(paths, &fpath[cwdlen], fpath_rem, sel);
				if (path_is_dir(fpath, cwdlen + plen)) {
					rectr++;
					strcpy(&cwd[cwdlen], &fpath[cwdlen]);
				} else {
					//fe_file_menu(fpath);
					ui_msgf("you picked\n\"%s\"", fpath);
				}
			}
		}
	}

	bp_free(&cb);
}
