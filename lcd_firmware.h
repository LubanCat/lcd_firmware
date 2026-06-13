#ifndef __LCD_FIRMWARE_H
#define __LCD_FIRMWARE_H

#include <stdbool.h>
#include <stdint.h>

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
};

#endif