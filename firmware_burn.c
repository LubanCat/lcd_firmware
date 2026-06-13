#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "lcd_firmware.h"
#include "board_firmware.h"

/*********************** storage ************************/
// rk3562、rk3576、rk3588
#define NVMEM_PATH      "/sys/bus/nvmem/devices/eeprom1/nvmem"
// #define NVMEM_PATH      "/sys/bus/nvmem/devices/eeprom2/nvmem"

// rk356x
// #define NVMEM_PATH      "/sys/bus/nvmem/devices/1-00510/nvmem"
// #define NVMEM_PATH      "/sys/bus/nvmem/devices/5-00510/nvmem"

#define FIRMWARE_MAGIC	0xDEAD5A5A


/*********************** boards ************************/
static struct board_info *board_info[] = {
	&ebf410125v1,
	&ebf410125,
	&ebf410173,
	&ebf410173v1,
	&ebf410177,
	&ebf410574,
	&ebf410575,
	&ebf410630,
	&lianxin_s8001280b,
	NULL,
};

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

	{
		unsigned char zero[0x100];

		memset(zero, 0, sizeof(zero));

		ret = write(fd, zero, sizeof(zero));
		if (ret != sizeof(zero)) {
			printf("tail erase failed\n");
			return -1;
		}
	}

	return 0;
}

static int burn_tlv_firmware(int fd, struct lcd_firmware *fw, struct tlv_desc *tlvs, int count)
{
	struct tlv_container_header hdr;
	struct firmware_header *v1 = fw->header;

	int start_pos;
	int payload_start;
	int end_pos;
	int tlv_base = (v1->firmware_size + 3) & ~3;
	int i;

	if (lseek(fd, tlv_base, SEEK_SET) < 0)
		return -1;

	start_pos = tlv_base;

	/* =========================
	 * 1. write header placeholder
	 * ========================= */
	memset(&hdr, 0, sizeof(hdr));
	hdr.magic = TLV_MAGIC;
	hdr.version = 1;
	hdr.flags = 0;
	hdr.total_length = 0;
	hdr.crc32 = 0;

	if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
		return -1;

	payload_start = lseek(fd, 0, SEEK_CUR);
	if (payload_start < 0)
		return -1;

	/* =========================
	 * 2. write TLV entries
	 * ========================= */
	for (i = 0; i < count; i++) {

		int pad = (4 - ((sizeof(struct tlv_entry) + tlvs[i].len) & 3)) & 3;
		int size = sizeof(struct tlv_entry) + tlvs[i].len + pad;

		struct tlv_entry *entry = calloc(1, size);
		if (!entry)
			return -1;

		entry->type = tlvs[i].type;
		entry->length = tlvs[i].len;

		if (tlvs[i].data && tlvs[i].len)
			memcpy(entry->value, tlvs[i].data, tlvs[i].len);

		if (write(fd, entry, size) != size) {
			free(entry);
			return -1;
		}

		free(entry);
	}

	/* =========================
	 * 3. END TLV
	 * ========================= */
	{
		struct tlv_entry end;

		memset(&end, 0, sizeof(end));
		end.type = TLV_TYPE_END;
		end.length = 0;

		if (write(fd, &end, sizeof(end)) != sizeof(end))
			return -1;
	}

	end_pos = lseek(fd, 0, SEEK_CUR);
	if (end_pos < 0)
		return -1;

	/* =========================
	 * 4. only update length
	 * ========================= */
	hdr.total_length = end_pos - payload_start;

	if (lseek(fd, start_pos, SEEK_SET) < 0)
		return -1;

	if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
		return -1;

	if (lseek(fd, end_pos, SEEK_SET) < 0)
		return -1;

	/* =========================
	 * 5. clear tail 0x100
	 * ========================= */
	{
		unsigned char zero[0x100] = {0};

		if (write(fd, zero, sizeof(zero)) != sizeof(zero))
			return -1;
	}

	return 0;
}

/*********************** main ************************/
int main(int argc, char **argv)
{
	int fd;
	int i;
	struct board_info *b = NULL;

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
		if (board_info[i - 1]) {
			b = board_info[i - 1];
		} else {
			printf("Invalid selection. Please input argv[1] in 1 to %zu\n", sizeof(board_info)/sizeof(board_info[0]) - 1);
			return -1;
		}

		goto burn;
	}

	while (1) {
		printf("\nselect board...\n");

		for (i = 0; board_info[i]; i++) {
			printf("[%d] %s\n", i + 1, board_info[i]->model);
		}

		scanf("%d", &i);

		if (i > 0 && board_info[i - 1]) {
			b = board_info[i - 1];
			break;
		}
	}

burn:

	if (!b)
		return -1;

	if (b->firmware) {
		if (burn_lcd_firmware(fd, b->firmware) < 0) {
			printf("v1 firmware burn failed\n");
			close(fd);
			return -1;
		}
		printf("v1 firmware burned OK\n");
	}

	if (b->tlvs && b->tlv_count > 0) {
		if (burn_tlv_firmware(fd, b->firmware, b->tlvs, b->tlv_count)) {
			printf("TLV burn failed\n");
			close(fd);
			return -1;
		}
		printf("TLV burned OK\n");
	}

	printf("ALL firmware burned successfully\n");
	close(fd);
	return 0;
}
