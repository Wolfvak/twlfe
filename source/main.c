#include <nds.h>
#include <stdio.h>

#include "err.h"
#include "vfs.h"
#include "ui.h"

#include "vfs_glue.h"

int dldi_mount(char drv);
int memfs_init(char drv);

void fe_mount_menu(void);

int main(void) {
	char drv = 'A';
	defaultExceptionHandler();
	ui_reset();

	if (!IS_ERR(dldi_mount(drv))) drv++;
	if (!IS_ERR(memfs_init(drv))) drv++;

	while(1) fe_mount_menu();
}
