#ifndef VFS_H__
#define VFS_H__

#include <nds.h>

/*
 * Some basic VFS rules:
 *
 * File / Directory paths:
 *
 * - all paths MUST be in UTF-8 and the filesystems must recognize ONLY this
 *   encoding. any necessary conversions must be done on the filesystem layer.
 *   inlineable functions for UTF-32 <-> UTF-16 <-> UTF-8 conversion are to
 *   be provided for convenience.
 *
 * - all paths must fit in `MAX_PATH` bytes. 1 byte != 1 rune/char.
 *
 * - all paths MUST start with a single UPPERCASE drive letter, followed by
 *   ONE colon (:) and ONE forward slash (/).
 *
 * - paths passed to the filesystem layer will always be absolute (no cwd)
 *   and start after the first colon, or at the first forward slash.
 *
 *
 * file paths:
 *
 * - all file paths MUST NOT end with a forward slash (/)
 *
 *
 * Directory paths:
 *
 * - all directory paths MUST end with a forward slash (/). this includes
 *   those retrieved by vfs_dirnext.
 *
 *
 * Global paths:
 * - path passed to the VFS, "A:/file.ext", "Z:/directory/folder/"
 *
 * Local paths:
 * - path passed to the FS, "/file.ext", "/directory/folder/"
 *
 */

#define MAX_PATH	(255)
#define MAX_FILES	(128)

#define VFS_FIRSTMOUNT	('A')
#define VFS_LASTMOUNT	('Z')

#define VFS_MOUNTPOINTS	(VFS_LASTMOUNT - VFS_FIRSTMOUNT + 1)

#define VFS_DRVVALID(x)	(((x) >= VFS_FIRSTMOUNT) && ((x) <= VFS_LASTMOUNT))
#define VFS_IDXVALID(x)	(((x) >= 0) && ((x) < VFS_MOUNTPOINTS))

#define SIZE_KIB(x) ((off_t)(x) << 10ULL)
#define SIZE_MIB(x) ((off_t)(x) << 20ULL)
#define SIZE_GIB(x) ((off_t)(x) << 30ULL)

typedef signed long long off_t;
#define VFS_SIZE_MAX ((off_t)0x7FFFFFFFFFFFFFFFULL)

typedef struct {
	const char *label;
	off_t size;
} vfs_info_t;

typedef struct {
	const void *ops;	/**< Filesystem operations */
	vfs_info_t info;	/**< Filesystem information */
	int caps;			/**< Capability bitmask */

	void *priv;
} mount_t;

typedef struct {
	mount_t *mnt;	/**< Mount drive */
	int idx;		/**< Mount drive index (zero-based) */
	int flags;		/**< Entity flags */
	off_t pos;		/**< File position / directory read iterator */

	void *priv;
} vf_t;

typedef struct {
	char path[MAX_PATH + 1];	/**< Directory item path */
	int flags;					/**< Directory item flags */

	void *priv;
} dirinf_t;

typedef struct {
	int (*mount)(mount_t *mnt);
	int (*unmount)(mount_t *mnt);

	int (*open)(mount_t *mnt, vf_t *file, const char *path, int mode);
	int (*close)(mount_t *mnt, vf_t *file);

	int (*unlink)(mount_t *mnt, const char *path);
	int (*rename)(mount_t *mnt, const char *oldp, const char *newp);

	off_t (*read)(mount_t *mnt, vf_t *file, void *buf, off_t size);
	off_t (*write)(mount_t *mnt, vf_t *file, const void *buf, off_t size);
	off_t (*size)(mount_t *mnt, vf_t *file);

	int (*mkdir)(mount_t *mnt, const char *path);
	int (*diropen)(mount_t *mnt, vf_t *dir, const char *path);
	int (*dirclose)(mount_t *mnt, vf_t *dir);
	int (*dirnext)(mount_t *mnt, vf_t *dir, dirinf_t *next);
} vfs_ops_t;

enum {
	VFS_OPEN	= BIT(0),			/**< Entity is open */

	VFS_RO		= BIT(1),			/**< Open with read permissions */
	VFS_WO		= BIT(2),			/**< Open with write permissions */
	VFS_RW		= VFS_RO | VFS_WO,	/**< Open with RW permissions */
	VFS_CREATE	= BIT(3) | VFS_WO,	/**< Create new file */

	VFS_FILE	= BIT(4),			/**< Entity is a file */
	VFS_DIR		= BIT(5),			/**< Entity is a dir */
};

int vfs_mountedcnt(void);
int vfs_state(int drive);

int vfs_mount(int drive, mount_t *mnt);
int vfs_unmount(int drive);

int vfs_open(const char *path, int mode);
int vfs_close(int fd);
int vfs_unlink(const char *path);
int vfs_rename(const char *oldp, const char *newp);

off_t vfs_read(int fd, void *buf, off_t size);
off_t vfs_write(int fd, const void *buf, off_t size);
off_t vfs_seek(int fd, off_t off, int whence);
off_t vfs_size(int fd);

int vfs_mkdir(const char *path);
int vfs_diropen(const char *path);
int vfs_dirclose(int dd);
int vfs_dirnext(int dd, dirinf_t *next);

const vfs_info_t *vfs_info(int drive);

#endif // VFS_H__
