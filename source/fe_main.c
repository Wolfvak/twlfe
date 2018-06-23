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
	int dd, res = 0;
	size_t max;

	if (ps == NULL) return -ERR_MEM;
	max = pstor_max(ps) - 1;

	dd = vfs_diropen(dir);
	if (IS_ERR(dd)) return dd;

	pstor_clear(ps);

	res = pstor_cat(ps, "../");
	if (IS_ERR(res)) return res;

	res = pstor_finish(ps);
	if (IS_ERR(res)) return res;

	for (int i = 0; i < max; i++) {
		dirinf_t inf;

		res = vfs_dirnext(dd, &inf);
		if (IS_ERR(res)) break;

		res = pstor_cat(ps, inf.path);
		if (IS_ERR(res)) break;

		if (inf.flags & VFS_DIR) {
			res = pstor_cat(ps, "/");
			if (IS_ERR(res)) break;
		}

		res = pstor_finish(ps);
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

#define FE_PSTORM_Y		(2)
#define FE_PSTORM_YSZ	(TFB_HEIGHT - 2)
static int fe_pstormenu(vu16 *map, pstor_t *ps)
{
	int res, sel = 0, base = 0, count = pstor_count(ps);
	bool redraw_menu = true;

	if (count == 0) {
		return -1;
	}

	while(1) {
		int pressed, end;

		end = base + FE_PSTORM_YSZ;
		if (end > count) end = count;

		if (redraw_menu) {
			char drawpath[MAX_PATH + 1];

			redraw_menu = false;

			swiWaitForVBlank();
			ui_tilemap_clr(map);

			for (int i = base; i < end; i++) {
				res = pstor_getpath(ps, drawpath, MAX_PATH, i);
				if (IS_ERR(res)) {
					ui_msgf("Failed to get path:\n%s", err_getstr(res));
					return res;
				}
				ui_drawstr(map, 2, i - base + FE_PSTORM_Y, drawpath);
			}
		}

		for (int i = base; i < end; i++)
			ui_drawc(map, (i == sel) ? '>' : ' ', 0, i - base + FE_PSTORM_Y);

		pressed = ui_waitkey(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A|KEY_B);

		if (pressed & KEY_UP) {
			sel--;
		} else if (pressed & KEY_DOWN) {
			sel++;
		} else if (pressed & KEY_LEFT) {
			sel -= FE_PSTORM_YSZ;
		} else if (pressed & KEY_RIGHT) {
			sel += FE_PSTORM_YSZ;
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
			end += FE_PSTORM_YSZ;
			if (end > count) end = count;
			redraw_menu = true;
		}

		if (sel < base) {
			end = base;
			base -= FE_PSTORM_YSZ;
			if (base < 0) base = 0;
			redraw_menu = true;
		}
	}

	return sel;
}

void fe_main(char drv, vu16 *map)
{	
	char cwd[MAX_PATH + 1];
	int res, sel;
	pstor_t ps;

	sprintf(cwd, "%c:/", drv);

	res = pstor_init(&ps, FE_PATHBUF, FE_MAXITEM);
	if (IS_ERR(res)) {
		ui_msgf("Failed to allocate path store:\n%s", err_getstr(res));
		return;
	}

	while(1) {
		res = scan_dir(&ps, cwd);
		if (IS_ERR(res)) {
			ui_msgf("Failed to scan dir\n\"%s\"\n%s", cwd, err_getstr(res));
			break;
		}

		sel = fe_pstormenu(map, &ps);
		if (sel <= 0) {
			if (path_is_topdir(cwd)) return;
			path_basedir(cwd);
		} else {
			char fpath[MAX_PATH + 1];
			int len = pstor_getpath(&ps, fpath, MAX_PATH, sel);
			if (path_is_dir(fpath, len)) {
				strcat(cwd, fpath);
			} else {
				ui_msgf("is a file");
			}
		}
	}
}
