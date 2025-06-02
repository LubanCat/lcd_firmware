#include "board_firmware.h"
#include "lcd_firmware.h"

static unsigned char init_seq[] = {
	0x15, 0x00, 0x02, 0xF0, 0xC3,
	0x15, 0x00, 0x02, 0xF0, 0x96,
	0x15, 0x00, 0x02, 0x36, 0x48,
	0x15, 0x00, 0x02, 0x3A, 0x77,
	0x15, 0x00, 0x02, 0xB4, 0x01,
	0x39, 0x00, 0x09, 0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33,
	0x15, 0x00, 0x02, 0xC1, 0x06,
	0x15, 0x00, 0x02, 0xC2, 0xA7,
	0x15, 0x00, 0x02, 0xC5, 0x18,
	0x39, 0x00, 0x0F, 0xE0, 0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B,
	0x39, 0x00, 0x0F, 0xE1, 0xF0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2D, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B,
	0x15, 0x00, 0x02, 0xF0, 0x3C,
	0x15, 0x00, 0x02, 0xF0, 0x69,
	0x05, 0x00, 0x01, 0x21,
	0x05, 0x78, 0x01, 0x11,
	0x05, 0x14, 0x01, 0x29,
};

static unsigned char exit_seq[] = {
	0x05, 0x78, 0x01, 0x28,
	0x05, 0x00, 0x01, 0x10,
};

static struct display_timing timing = {
    .pixelclock = 11784960,
    .hactive = 320,
    .vactive = 480,
    .hsync_len = 19,
    .hback_porch = 19,
    .hfront_porch = 38,
    .vsync_len = 4,
    .vback_porch = 4,
    .vfront_porch = 8,
    .flags = DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_DE_LOW | DISPLAY_FLAGS_PIXDATA_NEGEDGE,
};

static struct touchscreen_properties prop = {
	.invert_x = true,
	.invert_y = true,
	.swap_x_y = false,
};

static struct firmware_header lcd_frame_header = {
	.vendor = "Embedfire",
	.model = "HXR35027C20",
	.version = "v1.0",
	.timing_entry = {
		.offset = sizeof(struct firmware_header),
		.length = sizeof(struct display_timing),
    },

	.init_seq_entry = {
		.offset = sizeof(struct firmware_header) + sizeof(struct display_timing),
		.length = sizeof(init_seq),
    },

	.exit_seq_entry = {
		.offset = sizeof(struct firmware_header) + sizeof(struct display_timing) + sizeof(init_seq),
		.length = sizeof(exit_seq),
    },

	.touchscreen_entry = {
		.offset = sizeof(struct firmware_header) + sizeof(struct display_timing) + sizeof(init_seq) + sizeof(exit_seq),
		.length = sizeof(struct touchscreen_properties),
    },

	.firmware_size = sizeof(struct firmware_header) + sizeof(struct display_timing) + sizeof(init_seq) + sizeof(exit_seq) + sizeof(struct touchscreen_properties),
};

static struct lcd_firmware firmware = {
	.header = &lcd_frame_header,
	.timing = &timing,
	.touch_prop = &prop,
	.init_seq = init_seq,
	.exit_seq = exit_seq,
};

struct board_info ebf410630 = {
    .model = "EBF410630_3.5inch_320x480",
    .firmware = &firmware,
};
