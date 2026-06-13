#ifndef __LCD_FIRMWARE_H
#define __LCD_FIRMWARE_H

#include <stdbool.h>
#include <stdint.h>

/*********************** v1 firmware ************************/
#define BIT(x)          (1<<(x))

enum display_flags {
	DISPLAY_FLAGS_HSYNC_LOW     = BIT(0),
	DISPLAY_FLAGS_HSYNC_HIGH    = BIT(1),
	DISPLAY_FLAGS_VSYNC_LOW     = BIT(2),
	DISPLAY_FLAGS_VSYNC_HIGH    = BIT(3),

	/* data enable flag */
	DISPLAY_FLAGS_DE_LOW        = BIT(4),
	DISPLAY_FLAGS_DE_HIGH       = BIT(5),
	/* drive data on pos. edge */
	DISPLAY_FLAGS_PIXDATA_POSEDGE   = BIT(6),
	/* drive data on neg. edge */
	DISPLAY_FLAGS_PIXDATA_NEGEDGE   = BIT(7),
	DISPLAY_FLAGS_INTERLACED    = BIT(8),
	DISPLAY_FLAGS_DOUBLESCAN    = BIT(9),
	DISPLAY_FLAGS_DOUBLECLK     = BIT(10),
	/* drive sync on pos. edge */
	DISPLAY_FLAGS_SYNC_POSEDGE  = BIT(11),
	/* drive sync on neg. edge */
	DISPLAY_FLAGS_SYNC_NEGEDGE  = BIT(12),
};

struct display_timing {
	uint64_t pixelclock;

	unsigned int hactive;        /* hor. active video */
	unsigned int hfront_porch;   /* hor. front porch */
	unsigned int hback_porch;    /* hor. back porch */
	unsigned int hsync_len;      /* hor. sync len */

	unsigned int vactive;        /* ver. active video */
	unsigned int vfront_porch;   /* ver. front porch */
	unsigned int vback_porch;    /* ver. back porch */
	unsigned int vsync_len;      /* ver. sync len */

	enum display_flags flags;       /* display flags */
};

struct entry {
	unsigned int offset;
	unsigned int length;
};

struct touchscreen_properties {
	unsigned int max_x;
	unsigned int max_y;
	bool invert_x;
	bool invert_y;
	bool swap_x_y;
};

struct firmware_header {
	unsigned int magic;
	unsigned char vendor[16];
	unsigned char model[32];
	unsigned char version[8];
	struct entry timing_entry;
	struct entry init_seq_entry;
	struct entry exit_seq_entry;
	struct entry touchscreen_entry;
	unsigned int firmware_size;
};

struct lcd_firmware {
	struct firmware_header *header;
	struct display_timing *timing;
	struct touchscreen_properties *touch_prop;
	unsigned char *init_seq;
	unsigned char *exit_seq;
};

struct board_info {
	unsigned char model[64];
	struct lcd_firmware *firmware;

	/* v2 TLV扩展 */
	struct tlv_desc *tlvs;
	int tlv_count;
};

/*********************** v2 TLV firmware ************************/

// ┌──────────────────────────────────────────────┐ 0x000000
// │                v1 firmware                   │
// │  firmware_header                             │
// │  display_timing                              │
// │  init_seq                                    │
// │  exit_seq                                    │
// │  touchscreen_properties                      │
// │                                              │
// │  firmware_size = TLV base offset             │
// └──────────────────────────────────────────────┘
// │            TLV Container (v2 area)           │
// │                                              │
// │  TLV Container Header                        │
// │  ┌──────────────────────────────┐            │
// │  │ magic = TLV_MAGIC            │            │
// │  │ version =                    │            │
// │  │ total_length                 │            │
// │  └──────────────────────────────┘            │
// │                                              │
// │  TLV Entry #1                                │
// │  TLV Entry #2                                │
// │  TLV Entry #3                                │
// │   ...                                        │
// │                                              │
// │  TLV_TYPE_END  ← 解析停止点                   │
// │                                              │
// └──────────────────────────────────────────────┘
// │   Tail Padding (0x100 bytes ZERO CLEAN)      │
// │   00 00 00 00 00 00 00 00 ...                │
// └──────────────────────────────────────────────┘

#define TLV_MAGIC 0x544C5632 // VLT2

#define TLV_TYPE_FIRMWARE_VERSION  1
#define TLV_TYPE_TIMING            2
#define TLV_TYPE_TOUCH             3
#define TLV_TYPE_EXIT_SEQ          4
#define TLV_TYPE_INIT_SEQ_4LANE    5
#define TLV_TYPE_INIT_SEQ_2LANE    6
#define TLV_TYPE_END               0xFFFF

struct tlv_container_header {
	uint32_t magic;
	uint16_t version;
	uint16_t flags;
	uint32_t total_length; /* payload length */
	uint32_t crc32;        /* payload crc */
} __attribute__((packed));

struct tlv_firmware_version {
	uint16_t main_version;
	uint16_t sub_version;
	unsigned char vendor[16];
	unsigned char model[32];
}__attribute__((packed));

struct tlv_entry {
	uint16_t type;
	uint16_t length;
	uint8_t value[0];
};

struct tlv_desc {
	uint16_t type;
	const void *data;
	uint16_t len;
};

#endif