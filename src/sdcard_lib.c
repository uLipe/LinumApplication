#include "sdcard_lib.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

LOG_MODULE_REGISTER(sdcard_demo, LOG_LEVEL_DBG);

#define DISK_DRIVE_NAME "SD"
#define DISK_MOUNT_PT "/"DISK_DRIVE_NAME":"
#define FILE_PATH "/SD:/hello.txt"

static const char *disk_mount_pt = DISK_MOUNT_PT;
static const char hello_str[] = "Hello from Zephyr SD Card!";

static struct fs_file_t file;
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .mnt_point = DISK_MOUNT_PT,
};

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#define FS_RET_OK FR_OK
#else
#define FS_RET_OK 0
#endif


#define MAX_PATH 128
#define SOME_FILE_NAME "some.dat"
#define SOME_DIR_NAME "some"
#define SOME_REQUIRED_LEN MAX(sizeof(SOME_FILE_NAME), sizeof(SOME_DIR_NAME))


int sdcard_init(void)
{
    // int ret;

    // LOG_INF("Initializing SD Card");

    // // Initialize disk access
    // ret = disk_access_init(DISK_DRIVE_NAME);
    // if (ret != 0) {
    //     LOG_ERR("Disk access init failed (%d)", ret);
    //     return ret;
    // }

    // // Mount filesystem
    // ret = fs_mount(&mp);
    // if (ret != 0) {
    //     LOG_ERR("Failed to mount filesystem (%d)", ret);
    //     return ret;
    // }

    // LOG_INF("Filesystem mounted successfully");
    // return 0;

    /* raw disk i/o */
	do {
		static const char *disk_pdrv = DISK_DRIVE_NAME;
		uint64_t memory_size_mb;
		uint32_t block_count;
		uint32_t block_size;

		if (disk_access_ioctl(disk_pdrv,
				DISK_IOCTL_CTRL_INIT, NULL) != 0) {
			LOG_ERR("Storage init ERROR!");
			break;
		}

		if (disk_access_ioctl(disk_pdrv,
				DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
			LOG_ERR("Unable to get sector count");
			break;
		}
		LOG_INF("Block count %u", block_count);

		if (disk_access_ioctl(disk_pdrv,
				DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
			LOG_ERR("Unable to get sector size");
			break;
		}
		printk("Sector size %u\n", block_size);

		memory_size_mb = (uint64_t)block_count * block_size;
		printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

		if (disk_access_ioctl(disk_pdrv,
				DISK_IOCTL_CTRL_DEINIT, NULL) != 0) {
			LOG_ERR("Storage deinit ERROR!");
			break;
		}
	} while (0);

	mp.mnt_point = disk_mount_pt;

	int res = fs_mount(&mp);

	if (res == FS_RET_OK) {
		printk("Disk mounted.\n");
		/* Try to unmount and remount the disk */
		res = fs_unmount(&mp);
		if (res != FS_RET_OK) {
			printk("Error unmounting disk\n");
			return res;
		}
		res = fs_mount(&mp);
		if (res != FS_RET_OK) {
			printk("Error remounting disk\n");
			return res;
		}

		// if (lsdir(disk_mount_pt) == 0) {
#ifdef CONFIG_FS_SAMPLE_CREATE_SOME_ENTRIES
			if (create_some_entries(disk_mount_pt)) {
				lsdir(disk_mount_pt);
			}
#endif
		// }
	} else {
		printk("Error mounting disk.\n");
	}

	fs_unmount(&mp);

	// while (1) {
	// 	k_sleep(K_MSEC(1000));
	// }
	return 0;
}

int sdcard_test(void)
{
    int ret;
    char read_buf[40] = {0};

    LOG_INF("Starting SD Card test");
    memset(read_buf, 0, sizeof(read_buf));

    // Write file
    fs_file_t_init(&file);
    
    ret = fs_open(&file, FILE_PATH, FS_O_CREATE | FS_O_WRITE);
    if (ret != 0) {
        LOG_ERR("Failed to open file for write (%d)", ret);
        return ret;
    }

    ret = fs_write(&file, hello_str, strlen(hello_str));
    if (ret < 0) {
        LOG_ERR("Failed to write to file (%d)", ret);
        fs_close(&file);
        return ret;
    }

    ret = fs_close(&file);
    if (ret != 0) {
        LOG_ERR("Failed to close file (%d)", ret);
        return ret;
    }

    LOG_INF("File written successfully");

    // Read file
    fs_file_t_init(&file);
    ret = fs_open(&file, FILE_PATH, FS_O_READ);
    if (ret != 0) {
        LOG_ERR("Failed to open file for read (%d)", ret);
        return ret;
    }

    ret = fs_read(&file, read_buf, sizeof(read_buf));
    if (ret < 0) {
        LOG_ERR("Failed to read file (%d)", ret);
        fs_close(&file);
        return ret;
    }

    ret = fs_close(&file);
    if (ret != 0) {
        LOG_ERR("Failed to close file (%d)", ret);
        return ret;
    }

    LOG_INF("File content: %s", read_buf);

    // Verify content
    // if (strcmp(hello_str, read_buf) {
    //     LOG_ERR("Content verification failed!");
    //     return -EIO;
    // }

    LOG_INF("SD Card test completed successfully");
    return 0;
}

int sdcard_deinit(void)
{
    int ret = fs_unmount(&mp);
    if (ret != 0) {
        LOG_ERR("Failed to unmount filesystem (%d)", ret);
        return ret;
    }
    LOG_INF("Filesystem unmounted successfully");
    return 0;
}