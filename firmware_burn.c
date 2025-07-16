#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "lcd_firmware.h"
#include "board_firmware.h"


#define NVMEM_PATH      "/sys/bus/nvmem/devices/eeprom1/nvmem"
// #define NVMEM_PATH      "/sys/bus/nvmem/devices/1-00510/nvmem"
#define FIRMWARE_MAGIC	0xDEAD5A5A


static struct board_info *board_info[] = {
    &ebf410125v1,
    &ebf410125,
    &ebf410173,
    &ebf410177,
    &ebf410574,
    &ebf410575,
    &ebf410630,
    NULL,
};

static struct lcd_firmware *selet_lcd_firmware_to_burn(struct board_info **board_info)
{
    int i, length;

    printf("\n===============================================\n");
    printf("LCD board list:\n");
    printf("\n");
    for (i = 0; board_info[i] != NULL; i++) {
        printf("[%d] %s\n", i+1, board_info[i]->model);
    }
    length = i;
    printf("\n");
    printf("Which board would you like to burn? [1-%d]: ", length);
    scanf("%d", &i);
    if (i < 1 || i > length) {
        printf("\nInvalid selection. Please try again.\n");
        return NULL;
    }
    printf("\nSelected board: %s\n", board_info[i-1]->model);

    return board_info[i-1]->firmware;
}

static int burn_lcd_firmware(int fd, struct lcd_firmware *firmware)
{
    struct firmware_header *header = firmware->header;
    struct display_timing *timing = firmware->timing;
    struct touchscreen_properties *touch_prop = firmware->touch_prop;
    unsigned char *init_seq = firmware->init_seq;
    unsigned char *exit_seq = firmware->exit_seq;
    int ret;

    header->magic = FIRMWARE_MAGIC;
    
    printf("LCD vendor: %s\n", header->vendor);
    printf("LCD model: %s\n", header->model);
    printf("Firmware version: %s\n", header->version);
    printf("Firmware Burning ...\n");

    /* write header */
    ret = write(fd, header, sizeof(struct firmware_header));
    if (ret < 0) {
        printf("write header failed\n");
        return -1;
    }

    ret = lseek(fd, header->timing_entry.offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek timing offset failed\n");
        return -1;
    }

    /* write timing */
    ret = write(fd, timing, header->timing_entry.length);
    if (ret < 0) {
        printf("write timing failed\n");
        return -1;
    }

    ret = lseek(fd, header->init_seq_entry.offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek init seq offset failed\n");
        return -1;
    }

    /* write init sequence */
    ret = write(fd, init_seq, header->init_seq_entry.length);
    if (ret < 0) {
        printf("write init sequence failed\n");
        return -1;
    }

    ret = lseek(fd, header->exit_seq_entry.offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek exit seq offset failed\n");
        return -1;
    }

    /* write eixt sequence */
    ret = write(fd, exit_seq, header->exit_seq_entry.length);
    if (ret < 0) {
        printf("write exit sequence failed\n");
        return -1;
    }

    ret = lseek(fd, header->touchscreen_entry.offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek touchscreen properties offset failed\n");
        return -1;
    }

    /* write touchscreen properties */
    ret = write(fd, touch_prop, header->touchscreen_entry.length);
    if (ret < 0) {
        printf("write touchscreen properties failed\n");
        return -1;
    }

    return 0;
}

static int get_firmware_header(int fd, struct firmware_header *header)
{
    int ret;

    ret = lseek(fd, 0, SEEK_SET);
    if (ret < 0) {
        printf("lseek header offset failed\n");
        return -1;
    }

    ret = read(fd, header, sizeof(struct firmware_header));
    if (ret < 0) {
        printf("read header failed\n");
    }

    return 0;
}

static int get_firmware_display_timing(int fd, struct entry entry, struct display_timing *timing)
{
    int ret;
    
    ret = lseek(fd, entry.offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek timing offset failed\n");
        return -1;
    }

    ret = read(fd, timing, entry.length);
    if (ret < 0) {
        printf("read timing failed\n");
        return -1;
    }

    return 0;
}

static int get_firmware_init_sequence(int fd, struct entry entry, unsigned char *init_seq)
{
    int ret;

    ret = lseek(fd, entry.offset, SEEK_SET);
    if (ret < 0) {
        printf("lseek init seq offset failed\n");
        return -1;
    }

    ret = read(fd, init_seq, entry.length);
    if (ret < 0) {
        printf("read init sequence failed\n");
        return -1;
    }

    return 0;
}


int main(int argc, char **argv)
{
    int fd;
    int ret;
    int i;
    struct lcd_firmware *firmware;

    if ((argc == 2 && argv[1] != NULL) || argc > 2) {
        if (strcmp(argv[1], "--help") == 0 || argc > 2) {
            printf("Usage: %s\n", argv[0]);
            printf("       %s [board number]\n", argv[0]);
            printf("       %s --help\n", argv[0]);
            return 0;
        } 
    }

    fd = open(NVMEM_PATH, O_RDWR);
    if (fd < 0) {
        printf("open %s failed\n", NVMEM_PATH);
        return -1;
    }

    if (argc == 2 && argv[1] != NULL) {
        i = strtol(argv[1], NULL, 10);
        if (i > 0 && board_info[i-1] != NULL) {
            firmware = board_info[i-1]->firmware;
            ret = burn_lcd_firmware(fd, firmware);
            if (ret < 0) {
                printf("\nFirmware burning failed\n");
                return -1;
            } else {
                printf("\nFirmware burning successful\n");
                return 0;
            }
        } else {
            printf("Invalid selection. Please input argv[1] in 1 to %ld\n", sizeof(board_info)/sizeof(board_info[0]) - 1);
            return -1;
        }
    }

    while (1) {
        firmware = selet_lcd_firmware_to_burn(board_info);
        if (!firmware) {
            continue;
        }

        ret = burn_lcd_firmware(fd, firmware);
        if (ret < 0) {
            printf("\nFirmware burning failed\n");
            break;
        } else {
            printf("\nFirmware burning successful\n");
            break;
        }
    }

    return 0;
}
