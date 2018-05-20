#include <nds.h>
#include <stdio.h>

#include <nds/arm9/dldi.h>
#include <nds/disc_io.h>

#include "err.h"
#include "vfs.h"

void memfs_init(void);

int main(void) {
	char bios_buf[1024];
	file_t bios_file;
	dir_t memfs_root;
	dirent_t memfs_dirfile;

	consoleDemoInit();
	memfs_init();

	iprintf("vfs_diropen = %d\n", vfs_diropen(&memfs_root, "S:/"));
	vfs_dirnext_clr(&memfs_root, &memfs_dirfile);

	while(!IS_ERR(vfs_dirnext(&memfs_root, &memfs_dirfile)))
		iprintf("\"%s\"\n", memfs_dirfile.path);

	iprintf("vfs_dirclose = %d\n", vfs_dirclose(&memfs_root));

	iprintf("vfs_open = %d\n", vfs_open(&bios_file, "S:/bios", VFS_RO));
	iprintf("vfs_seek max = %llu\n", vfs_seek(&bios_file, 1000000));
	iprintf("vfs_seek min = %llu\n", vfs_seek(&bios_file, -1000000));

	iprintf("vfs_read = %llu\n", vfs_read(&bios_file, bios_buf, 1024));
	iprintf("vfs_unmount = %d\n", vfs_unmount('S'));
	iprintf("vfs_close = %d\n", vfs_close(&bios_file));
	iprintf("vfs_unmount = %d\n", vfs_unmount('S'));

	while(1) {
		swiWaitForVBlank();
		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_START) break;
	}

	return 0;
}
