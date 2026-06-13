#include "board_firmware.h"
#include "lcd_firmware.h"

static unsigned char init_seq[] = {
	0x15, 0x00, 0x02, 0x80, 0xAC,
	0x15, 0x00, 0x02, 0x81, 0xB8,
	0x15, 0x00, 0x02, 0x82, 0x09,
	0x15, 0x00, 0x02, 0x83, 0x78,
	0x15, 0x00, 0x02, 0x84, 0x7F,
	0x15, 0x00, 0x02, 0x85, 0xBB,
	0x15, 0x00, 0x02, 0x86, 0x70,
	0x05, 0xC8, 0x01, 0x11,
	0x05, 0xC8, 0x01, 0x29,
};

static unsigned char exit_seq[] = {
	0x05, 0x78, 0x01, 0x28,
	0x05, 0x00, 0x01, 0x10,
};

static struct display_timing timing = {
	.pixelclock = 51668640,
	.hactive = 1024,
	.vactive = 600,
	.hsync_len = 10,
	.hback_porch = 160,
	.hfront_porch = 160,
	.vsync_len = 1,
	.vback_porch = 23,
	.vfront_porch = 12,
	.flags = DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW | DISPLAY_FLAGS_DE_LOW | DISPLAY_FLAGS_PIXDATA_NEGEDGE,
};

static struct touchscreen_properties prop = {
	.invert_x = true,
	.invert_y = true,
	.swap_x_y = false,
};

static struct firmware_header lcd_frame_header = {
	.vendor = "Embedfire",
	.model = "STL7.0-60-132-K",
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

struct board_info ebf410173 = {
	.model = "EBF410173_7inch_1024x600",
	.firmware = &firmware,
};
