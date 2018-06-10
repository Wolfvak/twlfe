#include <nds.h>
#include <stdio.h>

#include "err.h"
#include "vfs.h"

#include "ui.h"

int mount_ramimg(char drive);
int memfs_init(char drv);

int main(void) {
	defaultExceptionHandler();
	ui_reset();

	ui_msg("Mounting RAMIMG... %d\n", mount_ramimg('A'));
	ui_msg("copy %d\n", copy_file_prog("A:/OwO.jpg", "A:/UwU.jpg"));

	ui_progress(1, 1, NULL, NULL);

	while(1) {
		swiWaitForVBlank();
		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_START) break;
	}

	return 0;
}
