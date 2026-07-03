/**
 * @file tkl_fs.c
 * @brief ESP32 TKL file system implementation.
 *
 * POSIX-compatible file system operations for ESP32 platform.
 * SD card is mounted via esp_vfs_fat_sdspi_mount() when tkl_fs_mount() is called.
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc.
 */

#include "tkl_fs.h"
#include "tuya_cloud_types.h"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"

/* Internal helpers from tkl_spi.c (read pinmux-configured pins + port→host map) */
extern int  __tkl_spi_port_to_host(TUYA_SPI_NUM_E port);
extern void __tkl_spi_get_pins(TUYA_SPI_NUM_E port, int *mosi, int *miso, int *sclk, int *cs);

/***********************************************************
************************macro define************************
***********************************************************/
/* SD card uses the second SPI port by default (SPI3_HOST on most chips).
 * Boards that use a different port can override this via Kconfig or
 * a board-level #define before including this file. */
#ifndef TKL_SD_SPI_PORT
  #define TKL_SD_SPI_PORT   TUYA_SPI_NUM_1
#endif
#ifndef TKL_SD_MAX_FREQ_KHZ
  #define TKL_SD_MAX_FREQ_KHZ 4000
#endif

/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define***********************
***********************************************************/

int tkl_fs_mount(const char *path, FS_DEV_TYPE_T dev_type)
{
    if (path == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (dev_type == DEV_SDCARD) {
        /* Read SPI pins from the pinmux-configured context */
        int mosi, miso, sclk, cs;
        __tkl_spi_get_pins(TKL_SD_SPI_PORT, &mosi, &miso, &sclk, &cs);
        if (mosi <= 0 || sclk <= 0) {
            ESP_LOGE("tkl_fs", "SPI pins not configured for SD (m=%d c=%d)",
                     mosi, sclk);
            return OPRT_RESOURCE_NOT_READY;
        }

        /* Map TKL port to ESP-IDF host */
        int host_id = __tkl_spi_port_to_host(TKL_SD_SPI_PORT);
        if (host_id < 0) {
            ESP_LOGE("tkl_fs", "unsupported SPI host");
            return OPRT_NOT_SUPPORTED;
        }

        /* Pre-init the SPI bus — sdspi's later spi_bus_initialize(-1,-1,-1)
         * will get ESP_ERR_INVALID_STATE and reuse our routed bus. */
        spi_bus_config_t buscfg = {
            .mosi_io_num = mosi,
            .miso_io_num = (miso > 0) ? miso : -1,
            .sclk_io_num = sclk,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4092,
        };
        esp_err_t esp_rt = spi_bus_initialize((spi_host_device_t)host_id, &buscfg, SPI_DMA_CH_AUTO);
        if (esp_rt != ESP_OK && esp_rt != ESP_ERR_INVALID_STATE) {
            ESP_LOGE("tkl_fs", "spi_bus_initialize failed: 0x%x", esp_rt);
            return OPRT_COM_ERROR;
        }

        esp_vfs_fat_mount_config_t mount_cfg = {
            .format_if_mount_failed = true,
            .max_files = 5,
            .allocation_unit_size = 16 * 1024,
        };

        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        host.slot = (spi_host_device_t)host_id;
        host.max_freq_khz = TKL_SD_MAX_FREQ_KHZ;

        sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_cfg.host_id = (spi_host_device_t)host_id;
        slot_cfg.gpio_cs = (cs >= 0) ? cs : GPIO_NUM_NC;

        sdmmc_card_t *card = NULL;
        esp_err_t ret = esp_vfs_fat_sdspi_mount(path, &host, &slot_cfg, &mount_cfg, &card);
        if (ret != ESP_OK) {
            ESP_LOGE("tkl_fs", "esp_vfs_fat_sdspi_mount failed: 0x%x", ret);
            return OPRT_COM_ERROR;
        }

        ESP_LOGI("tkl_fs", "SD mounted at %s, OCR=0x%08"PRIx32", freq=%"PRIu32"kHz, sectors=%"PRIu32,
                 path, card->ocr, card->max_freq_khz, (uint32_t)(card->csd.capacity));
        return OPRT_OK;
    }

    return OPRT_NOT_SUPPORTED;
}

int tkl_fs_unmount(const char *path)
{
    if (path == NULL) {
        return OPRT_INVALID_PARM;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(path, NULL);
    if (ret != ESP_OK) {
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

int tkl_fs_mkdir(const char *path)
{
    if (path == NULL) {
        return -1;
    }
    int ret = mkdir(path, 0777);
    if (ret && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int tkl_fs_remove(const char *path)
{
    if (path == NULL) {
        return -1;
    }

    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return -1;
    }

    if (S_ISREG(path_stat.st_mode)) {
        return unlink(path);
    } else if (S_ISDIR(path_stat.st_mode)) {
        return rmdir(path);
    }
    return -1;
}

int tkl_fs_mode(const char *path, uint32_t *mode)
{
    if (path == NULL || mode == NULL) {
        return -1;
    }

    struct stat statbuf = {0};
    if (stat(path, &statbuf) != 0) {
        return -1;
    }

    *mode = statbuf.st_mode;
    return 0;
}

int tkl_fs_is_exist(const char *path, BOOL_T *is_exist)
{
    if (path == NULL || is_exist == NULL) {
        return -1;
    }

    struct stat statbuf = {0};
    if (stat(path, &statbuf) != 0) {
        *is_exist = FALSE;
    } else {
        *is_exist = TRUE;
    }
    return 0;
}

int tkl_fs_rename(const char *path_old, const char *path_new)
{
    if (path_old == NULL || path_new == NULL) {
        return -1;
    }
    return rename(path_old, path_new);
}

int tkl_dir_open(const char *path, TUYA_DIR *dir)
{
    if (path == NULL || dir == NULL) {
        return -1;
    }

    DIR *d = opendir(path);
    if (d == NULL) {
        return -1;
    }

    *dir = d;
    return 0;
}

int tkl_dir_close(TUYA_DIR dir)
{
    if (dir == NULL) {
        return -1;
    }
    return closedir((DIR *)dir);
}

int tkl_dir_read(TUYA_DIR dir, TUYA_FILEINFO *info)
{
    if (dir == NULL || info == NULL) {
        return -1;
    }

    struct dirent *dp = readdir((DIR *)dir);
    if (dp == NULL) {
        return -1;
    }

    *info = dp;
    return 0;
}

int tkl_dir_name(TUYA_FILEINFO info, const char **name)
{
    if (info == NULL || name == NULL) {
        return -1;
    }

    *name = ((struct dirent *)info)->d_name;
    return 0;
}

int tkl_dir_is_directory(TUYA_FILEINFO info, BOOL_T *is_dir)
{
    if (info == NULL || is_dir == NULL) {
        return -1;
    }

    *is_dir = (((struct dirent *)info)->d_type == DT_DIR);
    return 0;
}

int tkl_dir_is_regular(TUYA_FILEINFO info, BOOL_T *is_regular)
{
    if (info == NULL || is_regular == NULL) {
        return -1;
    }

    *is_regular = (((struct dirent *)info)->d_type == DT_REG);
    return 0;
}

TUYA_FILE tkl_fopen(const char *path, const char *mode)
{
    if (path == NULL || mode == NULL) {
        return NULL;
    }

    FILE *fp = fopen(path, mode);
    return (TUYA_FILE)fp;
}

int tkl_fclose(TUYA_FILE file)
{
    if (file == NULL) {
        return -1;
    }
    return fclose((FILE *)file);
}

int tkl_fread(void *buf, int bytes, TUYA_FILE file)
{
    if (buf == NULL || file == NULL || bytes <= 0) {
        return -1;
    }
    return fread(buf, 1, bytes, (FILE *)file);
}

int tkl_fwrite(void *buf, int bytes, TUYA_FILE file)
{
    if (buf == NULL || file == NULL || bytes <= 0) {
        return -1;
    }
    return fwrite(buf, 1, bytes, (FILE *)file);
}

int tkl_fsync(int fd)
{
    (void)fd;
    return 0;
}

char *tkl_fgets(char *buf, int len, TUYA_FILE file)
{
    if (buf == NULL || file == NULL || len <= 0) {
        return NULL;
    }
    return fgets(buf, len, (FILE *)file);
}

int tkl_feof(TUYA_FILE file)
{
    if (file == NULL) {
        return -1;
    }
    return feof((FILE *)file);
}

int tkl_fseek(TUYA_FILE file, int64_t offs, int whence)
{
    if (file == NULL) {
        return -1;
    }

    int origin;
    switch (whence) {
        case 0: origin = SEEK_SET; break;
        case 1: origin = SEEK_CUR; break;
        case 2: origin = SEEK_END; break;
        default: return -1;
    }

    return fseek((FILE *)file, offs, origin);
}

int64_t tkl_ftell(TUYA_FILE file)
{
    if (file == NULL) {
        return -1;
    }
    return ftell((FILE *)file);
}

int tkl_fgetsize(const char *filepath)
{
    if (filepath == NULL) {
        return -1;
    }

    struct stat statbuf;
    if (stat(filepath, &statbuf) != 0) {
        return -1;
    }
    return statbuf.st_size;
}

int tkl_faccess(const char *filepath, int mode)
{
    if (filepath == NULL) {
        return -1;
    }
    return access(filepath, mode);
}

int tkl_fgetc(TUYA_FILE file)
{
    if (file == NULL) {
        return -1;
    }
    return fgetc((FILE *)file);
}

int tkl_fflush(TUYA_FILE file)
{
    if (file == NULL) {
        return -1;
    }
    return fflush((FILE *)file);
}

int tkl_fileno(TUYA_FILE file)
{
    if (file == NULL) {
        return -1;
    }
    return fileno((FILE *)file);
}

int tkl_ftruncate(int fd, uint64_t length)
{
    return ftruncate(fd, length);
}
