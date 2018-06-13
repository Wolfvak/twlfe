#include <nds.h>
#include <stdio.h>

#include "err.h"
#include "vfs.h"
#include "ui.h"

#include "vfs_glue.h"

int dldi_mount(char drive);
int memfs_init(char drv);

char *fe_mount_menu(char*);

int main(void) {
	defaultExceptionHandler();
	ui_reset();

	memfs_init('A');
	dldi_mount('B');

	while(1) fe_mount_menu(NULL);
}
