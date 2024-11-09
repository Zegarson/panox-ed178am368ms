// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2013, The Linux Foundation. All rights reserved.

static const struct drm_display_mode panox_ed178am368ms_mode = {
	.clock = (368 + 20 + 20 + 40) * (448 + 20 + 4 + 12) * 60 / 1000,
	.hdisplay = 368,
	.hsync_start = 368 + 20,
	.hsync_end = 368 + 20 + 20,
	.htotal = 368 + 20 + 20 + 40,
	.vdisplay = 448,
	.vsync_start = 448 + 20,
	.vsync_end = 448 + 20 + 4,
	.vtotal = 448 + 20 + 4 + 12,
	.width_mm = 28.7,
	.height_mm = 34.94,
	.type = DRM_MODE_TYPE_DRIVER,
};

static const struct panel_desc_dsi panox_ed178am368ms = {
	.desc = {
		.modes = &panox_ed178am368ms_mode,
		.num_modes = 1,
		.bpc = 8,
		.size = {
			.width = 287,
			.height = 349,
		},
		.connector_type = DRM_MODE_CONNECTOR_DSI,
	},
	.flags = MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_NO_EOT_PACKET | MIPI_DSI_CLOCK_NON_CONTINUOUS,
	.format = MIPI_DSI_FMT_RGB888,
	.lanes = 1,
};
