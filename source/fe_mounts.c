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

void fe_main(char drv, pstor_t *paths, pstor_t *clippaths, vu16 *map);

#define FE_PATHBUF	(SIZE_KIB(128))
#define FE_MAXITEM	(16384)

/*int fe_mount_state_get(fe_mount_state *mounts, int max)
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
}*/

void fe_mount_menu(void)
{
	vu16 *map = ui_map(MAINSCR, BG_MAIN);
	ui_menu_entry mount_menu[VFS_MOUNTPOINTS];
	int mcnt, omcnt, res;
	pstor_t ps, clip;

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

	omcnt = -1;
	do {
		int sel;

		/*
		 * recount and reindex all filesystems
		 * when one is mounted or unmounted
		 */

		mcnt = vfs_mountedcnt();
		if (mcnt == 0) break;

		if (omcnt != mcnt) {
			int ctr = 0;
			ui_menu_entry *mmenu_entry = &mount_menu[0];

			for (int i = VFS_FIRSTMOUNT; i <= VFS_LASTMOUNT; i++) {
				if (ctr >= mcnt) break;
				if (vfs_state(i) >= 0) {
					const vfs_info_t *info = vfs_info(i);
					mmenu_entry->name = info->label;
					mmenu_entry->desc = "this should be a description";
					mmenu_entry++;
					ctr++;
				}
			}

			omcnt = mcnt;
		}

		sel = ui_menu(mcnt, mount_menu, "Main menu");
		if (sel >= 0) fe_main(sel + 'A', &ps, &clip, map);
	} while(1);
}
