#ifndef VFS_H__
#define VFS_H__

#include <nds.h>

#define MAX_PATH	(255)
#define MAX_FILES	(64)

#define VFS_FIRSTMOUNT	('A')
#define VFS_LASTMOUNT	('Z')

#define VFS_MOUNTPOINTS	(VFS_LASTMOUNT - VFS_FIRSTMOUNT)

typedef signed long long off_t;

typedef struct {
	off_t size;		/**< Filesystem size in bytes */
	char *label;	/**< Filesystem label */
	char *serial;	/**< Filesystem UUID */
} vfs_info_t;

typedef struct {
	const void *ops;	/**< Filesystem operations */
	int caps;			/**< Permitted operations bitmask */
	int acts;			/**< Active file handles */
	vfs_info_t info;	/**< Filesystem information */

	void *priv;
} mount_t;

typedef struct {
	mount_t *mnt;	/**< Mount drive */
	int flags;		/**< Entity flags */

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
	off_t (*seek)(mount_t *mnt, vf_t *file, off_t off);
	off_t (*size)(mount_t *mnt, vf_t *file);

	int (*mkdir)(mount_t *mnt, const char *path);
	int (*diropen)(mount_t *mnt, vf_t *dir, const char *path);
	int (*dirclose)(mount_t *mnt, vf_t *dir);
	int (*dirnext)(mount_t *mnt, vf_t *dir, dirinf_t *next);
} vfs_ops_t;

enum {
	VFS_OPEN	= BIT(0),				/**< Entity is open */

	VFS_RO		= BIT(1),				/**< Open with read permissions */
	VFS_WO		= BIT(2),				/**< Open with write permissions */
	VFS_RW		= VFS_RO | VFS_WO,		/**< Open with RW permissions */
	VFS_CREATE	= BIT(3) | VFS_WO,	/**< Create new file */

	VFS_FILE	= BIT(4),				/**< Entity is a file */
	VFS_DIR		= BIT(5),				/**< Entity is a dir */
};

int vfs_state(char drive);

int vfs_mount(char drive, const mount_t *mnt);
int vfs_unmount(char drive);

int vfs_open(const char *path, int mode);
int vfs_close(int fd);
int vfs_unlink(const char *path);
int vfs_rename(const char *oldp, const char *newp);

off_t vfs_read(int fd, void *buf, off_t size);
off_t vfs_write(int fd, const void *buf, off_t size);
off_t vfs_seek(int fd, off_t off);
off_t vfs_size(int fd);

int vfs_mkdir(const char *path);
int vfs_diropen(const char *path);
int vfs_dirclose(int dd);
int vfs_dirnext(int dd, dirinf_t *next);

#endif // VFS_H__
