#include <nds.h>
#include <stdio.h>

#include "err.h"
#include "vfs.h"
#include "ui.h"

#include "vfs_glue.h"

int dldi_mount(char drive);
int memfs_init(char drv);

void fe_mount_menu(void);

int main(void) {
	defaultExceptionHandler();
	ui_reset();

	dldi_mount('A');
	memfs_init('B');

	while(1) fe_mount_menu();
}
