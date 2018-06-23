#include <nds.h>

#include <stdio.h>
#include <string.h>

#include "err.h"
#include "ui.h"
#include "vfs.h"

#include "bp.h"
#include "pstor.h"
#include "vfs_glue.h"

#define FE_SCR		(MAINSCR)
#define FE_LAYER	(BG_MAIN)

typedef struct {
	char drv;
	const char *label;
	const char *icon;
	off_t size;
} fe_mount_info;

void fe_main(char drv, pstor_t *paths, bp_t *clipboard, vu16 *map);

static inline off_t get_mount_size(char drv) {
	vfs_ioctl_t io;
	if (!IS_ERR(vfs_ioctl(drv, VFS_IOCTL_SIZE, &io)))
		return io.intval;
	return 0;
}

static inline const char *get_mount_label(char drv) {
	vfs_ioctl_t io;
	if (!IS_ERR(vfs_ioctl(drv, VFS_IOCTL_LABEL, &io)) &&
		io.strval == NULL && *io.strval != '\0')
		return io.strval;
	return "(No label)";
}

static inline const char *get_mount_icon(char drv) {
	vfs_ioctl_t io;
	if (!IS_ERR(vfs_ioctl(drv, VFS_IOCTL_ASCII_ICON, &io)))
		return io.strval;
	return NULL;
}

static size_t get_mount_info(fe_mount_info *info, size_t max) {
	size_t ret = 0;
	for (size_t i = VFS_FIRSTMOUNT; i <= VFS_LASTMOUNT && ret < max; i++) {
		if (vfs_state(i) >= 0) {
			info[ret].drv = i;
			info[ret].label = get_mount_label(i);
			info[ret].size = get_mount_size(i);
			info[ret].icon = get_mount_icon(i);
			ret++;
		}
	}
	return ret;
}

static inline void fe_draw_icon(vu16 *map, const char *icon, size_t x, size_t y)
{
	for (size_t _y = 0; _y < 8; _y++) {
		for (size_t _x = 0; _x < 8; _x++) {
			ui_drawc(map, *icon, x + _x, y + _y);
			icon++;
		}
	}
}

#define FE_PATHBUF	(SIZE_KIB(2048))
#define FE_MAXITEM	(16384)

void fe_mount_menu(void)
{
	vu16 *map = ui_map(MAINSCR, BG_MAIN);
	fe_mount_info minf[VFS_MOUNTPOINTS];
	int mcnt = get_mount_info(minf, VFS_MOUNTPOINTS), idx = 0, res;

	pstor_t ps;
	bp_t clipboard;

	res = pstor_init(&ps, FE_PATHBUF, FE_MAXITEM);
	if (IS_ERR(res)) {
		ui_msgf("Failed pathstore init:\n%s", err_getstr(res));
		return;
	}

	res = bp_init(&clipboard, FE_MAXITEM);
	if (IS_ERR(res)) {
		ui_msgf("Failed clipboard init:\n%s", err_getstr(res));
		return;
	}

	while(1) {
		int keypress;
		char szstr[16];
		int next_idx, prev_idx;

		swiWaitForVBlank();
		ui_tilemap_clr(map);

		fe_draw_icon(map, minf[idx].icon, 12, 4);
		ui_drawstr(map, 12, 13, "^^^^^^^^");

		if (mcnt > 1) {
			next_idx = (idx + 1) % mcnt;
			prev_idx = (idx + mcnt - 1) % mcnt;
			fe_draw_icon(map, minf[next_idx].icon, 23, 4);
			fe_draw_icon(map, minf[prev_idx].icon, 1, 4);

			ui_drawstr(map, 21, 7, "\\\n/");
			ui_drawstr(map, 10, 7, "/\n\\");
		} else {
			next_idx = idx;
			prev_idx = idx;
		}

		size_format(szstr, minf[idx].size);
		ui_drawstr_xcenterf(map, 15, "%c:\n%s", minf[idx].drv, minf[idx].label);
		ui_drawstr_xcenterf(map, 17, "Size: %s", szstr);

		keypress = ui_waitkey(KEY_LEFT | KEY_RIGHT | KEY_A);

		if (keypress & KEY_LEFT) idx--;
		else if (keypress & KEY_RIGHT) idx++;
		else if (keypress & KEY_A) {
			fe_main(minf[idx].drv, &ps, &clipboard, map);
		}

		if (idx < 0) idx = mcnt-1;
		if (idx > mcnt-1) idx = 0;
	}
}
