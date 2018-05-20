#ifndef VFS_H__
#define VFS_H__

#define MAX_PATH	(255)

#define VFS_FIRSTMOUNT	('A')
#define VFS_LASTMOUNT	('Z')

#define VFS_MOUNTPOINTS	(VFS_LASTMOUNT - VFS_FIRSTMOUNT)

typedef signed long long off_t;

enum {
	VFS_RO	= (1 << 0),			/**< Open file with read permissions */
	VFS_WO	= (1 << 1),			/**< Open file with write permissions */
	VFS_RW	= VFS_RO | VFS_WO,	/**< Open file with read & write permissions */

	VFS_CREATE = (1 << 2),		/**< Create file if it doesn't exist */
};

typedef struct {
	off_t size;			/**< Filesystem size in bytes */
	const char *label;	/**< Filesystem label */
	const char *serial;	/**< Filesystem UUID */
} vfs_info_t;

typedef struct {
	const void *ops;	/**< Filesystem operations */
	void *priv;
	vfs_info_t info;	/**< Filesystem information */
} mount_t;

typedef struct {
	int idx;	/**< Mount drive index */
	int mode;	/**< Mode the file was opened with */
	void *priv;
} file_t;

typedef struct {
	int idx;	/**< Mount drive index */
	void *priv;
} dir_t;

typedef struct {
	const char *path; /**< Directory entity path */
	void *priv;
} dirent_t;

typedef struct {
	int (*mount)(mount_t *mnt);
	int (*unmount)(mount_t *mnt);

	int (*open)(mount_t *mnt, file_t *file, const char *path, int mode);
	int (*close)(mount_t *mnt, file_t *file);
	int (*unlink)(mount_t *mnt, const char *path);

	off_t (*read)(mount_t *mnt, file_t *file, void *buf, off_t size);
	off_t (*write)(mount_t *mnt, file_t *file, const void *buf, off_t size);
	off_t (*seek)(mount_t *mnt, file_t *file, off_t off);

	int (*mkdir)(mount_t *mnt, const char *path);
	int (*diropen)(mount_t *mnt, dir_t *dir, const char *path);
	int (*dirclose)(mount_t *mnt, dir_t *dir);
	int (*dirnext_clr)(mount_t *mnt, dir_t *dir, dirent_t *next);
	int (*dirnext)(mount_t *mnt, dir_t *dir, dirent_t *next);
} vfs_ops_t;

int vfs_mount(char drive, const mount_t *mnt);
int vfs_unmount(char drive);

off_t vfs_size(char drive);
const char *vfs_serial(char drive);
const char *vfs_label(char drive);

int vfs_open(file_t *f, const char *path, int mode);
int vfs_close(file_t *f);
int vfs_unlink(const char *path);

off_t vfs_read(file_t *f, void *buf, off_t size);
off_t vfs_write(file_t *f, const void *buf, off_t size);
off_t vfs_seek(file_t *f, off_t off);

int vfs_mkdir(const char *path);
int vfs_diropen(dir_t *dir, const char *path);
int vfs_dirclose(dir_t *dir);
int vfs_dirnext_clr(dir_t *dir, dirent_t *next);
int vfs_dirnext(dir_t *dir, dirent_t *next);

#endif // VFS_H__
