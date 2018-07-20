#include <stdio.h>
#include <nds.h>

#include "global.h"
#include "err.h"

#include "ui.h"
#include "vfs.h"

#include "bp.h"
#include "pstor.h"
#include "vfs_glue.h"

#define FE_SCR		(MAINSCR)
#define FE_LAYER	(BG_MAIN)

typedef struct {
	int drv;
	const char *label;
	off_t size;
} fe_mount_state;

void fe_main(char drv, pstor_t *paths, pstor_t *clippaths, vu16 *map);

#define FE_PATHBUF	(SIZE_KIB(128))
#define FE_MAXITEM	(16384)

int fe_mount_state_get(fe_mount_state *mounts, int max)
{
	int j = 0;
	for (int i = VFS_FIRSTMOUNT; i <= VFS_LASTMOUNT; i++) {
		if (j >= max) break;
		if (vfs_state(i) >= 0) {
			mounts[j].drv = i;
			mounts[j].label = vfs_ioctl_label(i);
			mounts[j].size = vfs_ioctl_size(i);
			j++;
		}
	}
	return j;
}

void fe_mount_menu(void)
{
	vu16 *map = ui_map(MAINSCR, BG_MAIN);
	int mcnt = vfs_count(), res;
	ui_menu_entry *mount_menu;
	fe_mount_state *mounts;
	pstor_t ps, clip;

	if (mcnt == 0) return;

	res = pstor_init(&ps, FE_PATHBUF, FE_MAXITEM);
	if (IS_ERR(res)) {
		ui_msgf("Failed pathstore init:\n%s", err_getstr(res));
		return;
	}

	res = pstor_init(&clip, FE_PATHBUF, FE_MAXITEM);
	if (IS_ERR(res)) {
		ui_msgf("Failed clipstore init:\n%s", err_getstr(res));
		return;
	}

	mounts = malloc(sizeof(*mounts) * mcnt);
	fe_mount_state_get(mounts, mcnt);

	mount_menu = malloc(sizeof(*mount_menu) * mcnt);
	for (int i = 0; i < mcnt; i++) {
		mount_menu[i].name = malloc(32);
		sprintf(mount_menu[i].name, "%c: (%s)", mounts[i].drv, mounts[i].label);

		mount_menu[i].desc = NULL;
	}

	while(1) {
		int sel = ui_menu(mcnt, mount_menu, "Mount menu");
		if (sel >= 0) {
			fe_main(mounts[sel].drv, &ps, &clip, map);
		}
	}
}
