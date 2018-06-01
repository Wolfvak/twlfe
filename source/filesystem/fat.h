#ifndef FAT_H__
#define FAT_H__

#define FAT_SECT_SIZE ((off_t)512ULL)

enum {
	FF_DISK_DLDI = 0,

	FF_DISK_TWLSD = 1,
	FF_DISK_TWLNAND = 2,
	FF_DISK_TWLPHOTO = 3,

	FF_MAX_DISK = 10
};

#define FAT_LABEL_LENGTH	(16)
#define FAT_SERIAL_LENGTH	(16)

#endif /* FAT_H__ */
