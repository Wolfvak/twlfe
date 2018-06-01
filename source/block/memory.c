#include <nds.h>
#include <stdio.h>

#include "devfs.h"
#include "err.h"
#include "vfs.h"

off_t memfs_read(devfs_entry_t *entry, void *buf, off_t size)
{
    uintptr_t src = (uintptr_t)entry->priv + entry->pos;
    memcpy(buf, (void*)src, size);
    return size;
}

off_t memfs_write(devfs_entry_t *entry, const void *buf, off_t size)
{
    uintptr_t dst = (uintptr_t)entry->priv + entry->pos;
    memcpy((void*)dst, buf, size);
    return size;
}

static devfs_entry_t memfs_entries[] = {
    {.name = "bios", .priv = (void*)0xFFFF0000, .size = 1<<15, .mode = VFS_RO},
    {.name = "itcm", .priv = (void*)0x00000000, .size = 1<<15, .mode = VFS_RO},
    {.name = "mram", .priv = (void*)0x02000000, .size = 1<<22, .mode = VFS_RO},
};
static const devfs_t memfs = {
    .dev_entry = memfs_entries,
    .n_entries = sizeof(memfs_entries)/sizeof(*memfs_entries),
    .label = "Memory",
    .dev_read = memfs_read,
    .dev_write = memfs_write,
};

void memfs_init(void)
{
    iprintf("memfs: vfs_mount = %d\n", devfs_mount('S', &memfs));
}
