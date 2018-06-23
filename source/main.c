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
	char drv = 'A';
	defaultExceptionHandler();
	ui_reset();

	ui_msgf(UISTR_RED "red " UISTR_WHITE "text, now it's " UISTR_BLUE "blue");

	if (!IS_ERR(dldi_mount(drv))) drv++;
	if (!IS_ERR(memfs_init(drv))) drv++;

	while(1) fe_mount_menu();
}
