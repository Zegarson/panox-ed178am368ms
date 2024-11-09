// SPDX-License-Identifier: GPL-2.0-only

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct panox_ed178am368ms
{
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct gpio_desc *reset_gpio;
};

static inline struct panox_ed178am368ms *to_panox_ed178am368ms(struct drm_panel *panel)
{
	return container_of(panel, struct panox_ed178am368ms, panel);
}

static void panox_ed178am368ms_reset(struct panox_ed178am368ms *ctx)
{
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(15000, 16000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(15000, 16000);
}

static int panox_ed178am368ms_on(struct panox_ed178am368ms *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_write_seq(dsi, 0xfe, 0x05);
	mipi_dsi_dcs_write_seq(dsi, 0xa3, 0x80);
	mipi_dsi_dcs_write_seq(dsi, 0xfe, 0x00);

	ret = mipi_dsi_dcs_set_column_address(dsi, 0x0014, 0x0153);
	if (ret < 0)
	{
		dev_err(dev, "Failed to set column address: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_set_page_address(dsi, 0x0000, 0x0167);
	if (ret < 0)
	{
		dev_err(dev, "Failed to set page address: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_PARTIAL_ROWS,
						   0x00, 0x01, 0x01, 0x66);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_SET_PARTIAL_COLUMNS,
						   0x00, 0x15, 0x01, 0x52);

	ret = mipi_dsi_dcs_set_tear_on(dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
	if (ret < 0)
	{
		dev_err(dev, "Failed to set tear on: %d\n", ret);
		return ret;
	}

	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x20);
	mipi_dsi_dcs_write_seq(dsi, MIPI_DCS_ENTER_PARTIAL_MODE);

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0)
	{
		dev_err(dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}
	msleep(80);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0)
	{
		dev_err(dev, "Failed to set display on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int panox_ed178am368ms_off(struct panox_ed178am368ms *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0)
	{
		dev_err(dev, "Failed to set display off: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0)
	{
		dev_err(dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}
	msleep(96);

	mipi_dsi_dcs_write_seq(dsi, 0x4f, 0x01);

	return 0;
}

static int panox_ed178am368ms_prepare(struct drm_panel *panel)
{
	struct panox_ed178am368ms *ctx = to_panox_ed178am368ms(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	panox_ed178am368ms_reset(ctx);

	ret = panox_ed178am368ms_on(ctx);
	if (ret < 0)
	{
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		return ret;
	}

	return 0;
}

static int panox_ed178am368ms_unprepare(struct drm_panel *panel)
{
	struct panox_ed178am368ms *ctx = to_panox_ed178am368ms(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = panox_ed178am368ms_off(ctx);
	if (ret < 0)
		dev_err(dev, "Failed to un-initialize panel: %d\n", ret);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);

	return 0;
}

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

static int panox_ed178am368ms_get_modes(struct drm_panel *panel,
										struct drm_connector *connector)
{
	return drm_connector_helper_get_modes_fixed(connector, &panox_ed178am368ms_mode);
}

static const struct drm_panel_funcs panox_ed178am368ms_panel_funcs = {
	.prepare = panox_ed178am368ms_prepare,
	.unprepare = panox_ed178am368ms_unprepare,
	.get_modes = panox_ed178am368ms_get_modes,
};

static int panox_ed178am368ms_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

// TODO: Check if /sys/class/backlight/.../actual_brightness actually returns
// correct values. If not, remove this function.
static int panox_ed178am368ms_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness & 0xff;
}

static const struct backlight_ops panox_ed178am368ms_bl_ops = {
	.update_status = panox_ed178am368ms_bl_update_status,
	.get_brightness = panox_ed178am368ms_bl_get_brightness,
};

static struct backlight_device *
panox_ed178am368ms_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 255,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
										  &panox_ed178am368ms_bl_ops, &props);
}

static int panox_ed178am368ms_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct panox_ed178am368ms *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
							 "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 1;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO_BURST |
					  MIPI_DSI_MODE_NO_EOT_PACKET |
					  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel, dev, &panox_ed178am368ms_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight = panox_ed178am368ms_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
							 "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
	{
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void panox_ed178am368ms_remove(struct mipi_dsi_device *dsi)
{
	struct panox_ed178am368ms *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id panox_ed178am368ms_of_match[] = {
	{.compatible = "panox,ed178am368ms"}};
MODULE_DEVICE_TABLE(of, panox_ed178am368ms_of_match);

static struct mipi_dsi_driver panox_ed178am368ms_driver = {
	.probe = panox_ed178am368ms_probe,
	.remove = panox_ed178am368ms_remove,
	.driver = {
		.name = "panel-panox-ed178am368ms",
		.of_match_table = panox_ed178am368ms_of_match,
	},
};
module_mipi_dsi_driver(panox_ed178am368ms_driver);

MODULE_AUTHOR("Franciszek Lopuszanski <franopusz2006@gmail.com>");
MODULE_DESCRIPTION("DRM driver for panox ed178am368ms (RM69090) command mode dsi panel");
MODULE_LICENSE("GPL");
