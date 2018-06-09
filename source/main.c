#include <nds.h>
#include <stdio.h>

#include "err.h"
#include "vfs.h"

#include "vfs_blkop.h"

int mount_ramimg(char drive);
int memfs_init(char drv);
void ui_reset(void);

int block_op_cb(off_t blk_len, void *buf, void *priv)
{
	iprintf("Reached callback! Read %llu bytes\n", blk_len);
	return 0;
}

int main(void) {
	char buf[64];
	int res, fd;
	dirinf_t info;

	vfs_blk_op_t blk_op = {
		.cb = block_op_cb,
		.min_blk_len = 4,
		.max_blk_len = 16384,
		.data_len = 1<<20
	};

	ui_reset();
	while(1);

	defaultExceptionHandler();
	consoleDemoInit();

	iprintf("Mounting RAMIMG... %d\n", mount_ramimg('A'));

	fd = vfs_open("A:/text_file.txt", VFS_RO);
	iprintf("vfs_open: %s\n", err_getstr(fd));
	if (!IS_ERR(fd)) {
		memset(buf, 0, 64);
		iprintf("vfs_read: %llu\n", vfs_read(fd, buf, 63));

		iprintf("\"%s\"\n", buf);

		res = vfs_close(fd);
		iprintf("vfs_close: %s\n", err_getstr(res));
	}

	fd = vfs_open("A:/UwU.jpg", VFS_RO);
	iprintf("vfs_open: %s\n", err_getstr(fd));
	if (!IS_ERR(fd)) {
		blk_op.fd = fd;
		iprintf("vfs_blk_read: %s\n", err_getstr(vfs_blk_read(&blk_op)));
		iprintf("vfs_close: %s\n", err_getstr(vfs_close(fd)));
	}

	res = vfs_mkdir("A:/test");
	iprintf("vfs_mkdir: %s\n", err_getstr(res));

	fd = vfs_open("A:/test/test.bin", VFS_RW | VFS_CREATE);
	iprintf("vfs_open w/ create: %s\n", err_getstr(fd));
	if (!IS_ERR(fd)) {
		memset(buf, 0, sizeof(buf));
		memset(buf, 'A', 8);
		iprintf("vfs_write (8): %llu\n", vfs_write(fd, buf, 8));

		iprintf("vfs_size: %llu\n", vfs_size(fd));
		iprintf("vfs_rewind: %llu\n", vfs_seek(fd, 0, SEEK_SET));
		iprintf("vfs_read (8): %llu\n", vfs_read(fd, buf, 8));
		iprintf("buffer: %s\n", buf);

		res = vfs_close(fd);
		iprintf("vfs_close: %s\n", err_getstr(res));
	}

	fd = vfs_diropen("A:/");
	iprintf("vfs_diropen: %s\n", err_getstr(fd));
	if (!IS_ERR(fd)) {
		while(!IS_ERR(vfs_dirnext(fd, &info))) {
			iprintf("\"%s\"\n", info.path);
		}
		res = vfs_dirclose(fd);
		iprintf("vfs_dirclose: %s\n", err_getstr(res));
	}

	while(1) {
		swiWaitForVBlank();
		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_START) break;
	}

	return 0;
}
