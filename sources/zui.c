#include "zui.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <kinc/graphics4/graphics.h>
#include <kinc/input/keyboard.h>
#include <kinc/input/mouse.h>
#include <kinc/system.h>
#include <kinc/window.h>
#include <kinc/log.h>
#include <kinc/graphics2/g2_ext.h>

void *gc_alloc(size_t size);

static zui_t *current = NULL;
static bool zui_key_repeat = true; // Emulate key repeat for non-character keys
static bool zui_dynamic_glyph_load = true; // Allow text input fields to push new glyphs into the font atlas
static float zui_key_repeat_time = 0.0;
static char zui_text_to_paste[1024];
static char zui_text_to_copy[1024];
static zui_t *zui_copy_receiver = NULL;
static int zui_copy_frame = 0;
static bool zui_combo_first = true;
static char temp[1024];
static zui_handle_t zui_combo_search_handle;
zui_t *zui_instances[ZUI_MAX_INSTANCES];
int zui_instances_count;
bool zui_always_redraw_window = true; // Redraw cached window texture each frame or on changes only
bool zui_touch_scroll = false; // Pan with finger to scroll
bool zui_touch_hold = false; // Touch and hold finger for right click
bool zui_touch_tooltip = false; // Show extra tooltips above finger / on-screen keyboard
bool zui_is_cut = false;
bool zui_is_copy = false;
bool zui_is_paste = false;
void (*zui_on_border_hover)(zui_handle_t *, int) = NULL; // Mouse over window border, use for resizing
void (*zui_on_text_hover)(void) = NULL; // Mouse over text input, use to set I-cursor
void (*zui_on_deselect_text)(void) = NULL; // Text editing finished
void (*zui_on_tab_drop)(zui_handle_t *, int, zui_handle_t *, int) = NULL; // Tab reorder via drag and drop
#ifdef WITH_EVAL
float krom_js_eval(char *str);
#endif

float ZUI_SCALE() {
	return current->ops->scale_factor;
}

float ZUI_ELEMENT_W() {
	return current->ops->theme->ELEMENT_W * ZUI_SCALE();
}

float ZUI_ELEMENT_H() {
	return current->ops->theme->ELEMENT_H * ZUI_SCALE();
}

float ZUI_ELEMENT_OFFSET() {
	return current->ops->theme->ELEMENT_OFFSET * ZUI_SCALE();
}

float ZUI_ARROW_SIZE() {
	return current->ops->theme->ARROW_SIZE * ZUI_SCALE();
}

float ZUI_BUTTON_H() {
	return current->ops->theme->BUTTON_H * ZUI_SCALE();
}

float ZUI_CHECK_SIZE() {
	return current->ops->theme->CHECK_SIZE * ZUI_SCALE();
}

float ZUI_CHECK_SELECT_SIZE() {
	return current->ops->theme->CHECK_SELECT_SIZE * ZUI_SCALE();
}

float ZUI_FONT_SIZE() {
	return current->ops->theme->FONT_SIZE * ZUI_SCALE();
}

float ZUI_SCROLL_W() {
	return current->ops->theme->SCROLL_W * ZUI_SCALE();
}

float ZUI_SCROLL_MINI_W() {
	return current->ops->theme->SCROLL_MINI_W * ZUI_SCALE();
}

float ZUI_TEXT_OFFSET() {
	return current->ops->theme->TEXT_OFFSET * ZUI_SCALE();
}

float ZUI_TAB_W() {
	return current->ops->theme->TAB_W * ZUI_SCALE();
}

float ZUI_HEADER_DRAG_H() {
	return 15.0 * ZUI_SCALE();
}

float ZUI_TOOLTIP_DELAY() {
	return 1.0;
}

zui_t *zui_get_current() {
	return current;
}

void zui_set_current(zui_t *_current) {
	current = _current;
}

zui_handle_t *zui_handle_create() {
	// zui_handle_t *h = (zui_handle_t *)malloc(sizeof(zui_handle_t));
	zui_handle_t *h = (zui_handle_t *)gc_alloc(sizeof(zui_handle_t));
	memset(h, 0, sizeof(zui_handle_t));
	h->redraws = 2;
	h->color = 0xffffffff;
	h->init = true;
	return h;
}

zui_handle_t *zui_nest(zui_handle_t *handle, int pos) {
	while(handle->children == NULL || pos >= handle->children->length) {
		zui_handle_t *h = zui_handle_create();
		if (handle->children == NULL) {
			handle->children = any_array_create(0);
			// handle->children = calloc(1, sizeof(any_array_t));
		}
		any_array_push(handle->children, h);
		if (pos == handle->children->length - 1) {
			// Return now so init stays true
			return h;
		}
	}
	// This handle already exists, set init to false
	handle->children->buffer[pos]->init = false;
	return handle->children->buffer[pos];
}

void zui_fade_color(float alpha) {
	uint32_t color = arm_g2_get_color();
	uint8_t r = (color & 0x00ff0000) >> 16;
	uint8_t g = (color & 0x0000ff00) >> 8;
	uint8_t b = (color & 0x000000ff);
	uint8_t a = (uint8_t)(255.0 * alpha);
	arm_g2_set_color((a << 24) | (r << 16) | (g << 8) | b);
}

void zui_fill(float x, float y, float w, float h, uint32_t color) {
	arm_g2_set_color(color);
	if (!current->enabled) zui_fade_color(0.25);
	arm_g2_fill_rect(current->_x + x * ZUI_SCALE(), current->_y + y * ZUI_SCALE() - 1, w * ZUI_SCALE(), h * ZUI_SCALE());
	arm_g2_set_color(0xffffffff);
}

void zui_rect(float x, float y, float w, float h, uint32_t color, float strength) {
	arm_g2_set_color(color);
	if (!current->enabled) zui_fade_color(0.25);
	arm_g2_draw_rect(current->_x + x * ZUI_SCALE(), current->_y + y * ZUI_SCALE(), w * ZUI_SCALE(), h * ZUI_SCALE(), strength);
	arm_g2_set_color(0xffffffff);
}

void zui_draw_rect(bool fill, float x, float y, float w, float h) {
	float strength = 1.0;
	if (!current->enabled) zui_fade_color(0.25);
	x = (int)x;
	y = (int)y;
	w = (int)w;
	h = (int)h;
	if (fill) {
		int r = current->filled_round_corner_image.width;
		if (current->ops->theme->ROUND_CORNERS && current->enabled && r > 0 && w >= r * 2.0) {
			y -= 1; // Make it pixel perfect with non-round draw
			h += 1;
			arm_g2_draw_scaled_render_target(&current->filled_round_corner_image, x, y, r, r);
			arm_g2_draw_scaled_render_target(&current->filled_round_corner_image, x, y + h, r, -r);
			arm_g2_draw_scaled_render_target(&current->filled_round_corner_image, x + w, y, -r, r);
			arm_g2_draw_scaled_render_target(&current->filled_round_corner_image, x + w, y + h, -r, -r);
			arm_g2_fill_rect(x + r, y, w - r * 2.0, h);
			arm_g2_fill_rect(x, y + r, w, h - r * 2.0);
		}
		else {
			arm_g2_fill_rect(x, y - 1, w, h + 1);
		}
	}
	else {
		int r = current->round_corner_image.width;
		if (current->ops->theme->ROUND_CORNERS && current->enabled && r > 0) {
			x -= 1;
			w += 1;
			y -= 1;
			h += 1;
			arm_g2_draw_scaled_render_target(&current->round_corner_image, x, y, r, r);
			arm_g2_draw_scaled_render_target(&current->round_corner_image, x, y + h, r, -r);
			arm_g2_draw_scaled_render_target(&current->round_corner_image, x + w, y, -r, r);
			arm_g2_draw_scaled_render_target(&current->round_corner_image, x + w, y + h, -r, -r);
			arm_g2_fill_rect(x + r, y, w - r * 2.0, strength);
			arm_g2_fill_rect(x + r, y + h - 1, w - r * 2.0, strength);
			arm_g2_fill_rect(x, y + r, strength, h - r * 2.0);
			arm_g2_fill_rect(x + w - 1, y + r, strength, h - r * 2.0);
		}
		else {
			arm_g2_draw_rect(x, y, w, h, strength);
		}
	}
}

bool zui_is_char(int code) {
	return (code >= 65 && code <= 90) || (code >= 97 && code <= 122);
}

int zui_check_start(int i, char *text, char **start, int start_count) {
	for (int x = 0; x < start_count; ++x) {
		if (strncmp(text + i, start[x], strlen(start[x])) == 0) {
			return strlen(start[x]);
		}
	}
	return 0;
}

zui_text_extract_t zui_extract_coloring(char *text, zui_coloring_t *col) {
	zui_text_extract_t res;
	res.colored[0] = '\0';
	res.uncolored[0] = '\0';
	bool coloring = false;
	int start_from = 0;
	int start_length = 0;
	for (int i = 0; i < strlen(text); ++i) {
		bool skip_first = false;
		// Check if upcoming text should be colored
		int length = zui_check_start(i, text, col->start->buffer, col->start->length);
		// Not touching another character
		bool separated_left = i == 0 || !zui_is_char(text[i - 1]);
		bool separated_right = i + length >= strlen(text) || !zui_is_char(text[i + length]);
		bool is_separated = separated_left && separated_right;
		// Start coloring
		if (length > 0 && (!coloring || col->end[0] == '\0') && (!col->separated || is_separated)) {
			coloring = true;
			start_from = i;
			start_length = length;
			if (col->end[0] != '\0' && col->end[0] != '\n') skip_first = true;
		}
		// End coloring
		else if (col->end[0] == '\0') {
			if (i == start_from + start_length) coloring = false;
		}
		else if (strncmp(text + i, col->end, strlen(col->end)) == 0) {
			coloring = false;
		}
		// If true, add current character to colored string
		int len_c = strlen(res.colored);
		int len_uc = strlen(res.uncolored);
		if (coloring && !skip_first) {
			res.colored[len_c] = text[i];
			res.colored[len_c + 1] = '\0';
			res.uncolored[len_uc] = ' ';
			res.uncolored[len_uc + 1] = '\0';
		}
		else {
			res.colored[len_c] = ' ';
			res.colored[len_c + 1] = '\0';
			res.uncolored[len_uc] = text[i];
			res.uncolored[len_uc + 1] = '\0';
		}
	}
	return res;
}

void zui_draw_string(char *text, float x_offset, float y_offset, int align, bool truncation) {
	if (text == NULL) return;
	if (truncation) {
		assert(strlen(text) < 1024 - 2);
		char *full_text = text;
		strcpy(temp, text);
		text = &temp[0];
		while (strlen(text) > 0 && arm_g2_string_width(current->ops->font->font_, current->font_size, text) > current->_w - 6.0 * ZUI_SCALE()) {
			text[strlen(text) - 1] = 0;
		}
		if (strlen(text) < strlen(full_text)) {
			strcat(text, "..");
			// Strip more to fit ".."
			while (strlen(text) > 2 && arm_g2_string_width(current->ops->font->font_, current->font_size, text) > current->_w - 10.0 * ZUI_SCALE()) {
				text[strlen(text) - 3] = 0;
				strcat(text, "..");
			}
			if (current->is_hovered) {
				zui_tooltip(full_text);
			}
		}
	}

	if (zui_dynamic_glyph_load) {
		int len = strlen(text);
		for (int i = 0; i < len; ++i) {
			if (text[i] > 126 && !arm_g2_font_has_glyph((int)text[i])) {
				int glyph = text[i];
				arm_g2_font_add_glyph(glyph);
			}
		}
	}

	if (x_offset < 0) x_offset = current->ops->theme->TEXT_OFFSET;
	x_offset *= ZUI_SCALE();
	arm_g2_set_font(current->ops->font->font_, current->font_size);
	if (align == ZUI_ALIGN_CENTER) x_offset = current->_w / 2.0 - arm_g2_string_width(current->ops->font->font_, current->font_size, text) / 2.0;
	else if (align == ZUI_ALIGN_RIGHT) x_offset = current->_w - arm_g2_string_width(current->ops->font->font_, current->font_size, text) - ZUI_TEXT_OFFSET();

	if (!current->enabled) zui_fade_color(0.25);

	if (current->text_coloring == NULL) {
		arm_g2_draw_string(text, current->_x + x_offset, current->_y + current->font_offset_y + y_offset);
	}
	else {
		// Monospace fonts only for now
		char tmp[512];
		strcpy(tmp, text);
		for (int i = 0; i < current->text_coloring->colorings->length; ++i) {
			zui_coloring_t *coloring = current->text_coloring->colorings->buffer[i];
			zui_text_extract_t result = zui_extract_coloring(tmp, coloring);
			if (result.colored[0] != '\0') {
				arm_g2_set_color(coloring->color);
				arm_g2_draw_string(result.colored, current->_x + x_offset, current->_y + current->font_offset_y + y_offset);
			}
			strcpy(tmp, result.uncolored);
		}
		arm_g2_set_color(current->text_coloring->default_color);
		arm_g2_draw_string(tmp, current->_x + x_offset, current->_y + current->font_offset_y + y_offset);
	}
}

bool zui_get_initial_hover(float elem_h) {
	if (current->scissor && current->input_y < current->_window_y + current->window_header_h) return false;
	return current->enabled && current->input_enabled &&
		current->input_started_x >= current->_window_x + current->_x && current->input_started_x < (current->_window_x + current->_x + current->_w) &&
		current->input_started_y >= current->_window_y + current->_y && current->input_started_y < (current->_window_y + current->_y + elem_h);
}

bool zui_get_hover(float elem_h) {
	if (current->scissor && current->input_y < current->_window_y + current->window_header_h) return false;
	current->is_hovered = current->enabled && current->input_enabled &&
		current->input_x >= current->_window_x + current->_x && current->input_x < (current->_window_x + current->_x + current->_w) &&
		current->input_y >= current->_window_y + current->_y && current->input_y < (current->_window_y + current->_y + elem_h);
	return current->is_hovered;
}

bool zui_get_released(float elem_h) { // Input selection
	current->is_released = current->enabled && current->input_enabled && current->input_released && zui_get_hover(elem_h) && zui_get_initial_hover(elem_h);
	return current->is_released;
}

bool zui_get_pushed(float elem_h) {
	current->is_pushed = current->enabled && current->input_enabled && current->input_down && zui_get_hover(elem_h) && zui_get_initial_hover(elem_h);
	return current->is_pushed;
}

bool zui_get_started(float elem_h) {
	current->is_started = current->enabled && current->input_enabled && current->input_started && zui_get_hover(elem_h);
	return current->is_started;
}

bool zui_is_visible(float elem_h) {
	if (current->current_window == NULL) return true;
	return (current->_y + elem_h > current->window_header_h && current->_y < current->current_window->texture.height);
}

float zui_get_ratio(float ratio, float dyn) {
	return ratio < 0 ? -ratio : ratio * dyn;
}

// Draw the upcoming elements in the same row
// Negative values will be treated as absolute, positive values as ratio to `window width`
void zui_row(f32_array_t *ratios) {
	if (ratios->length == 0) {
		current->ratios = NULL;
		return;
	}
	current->ratios = ratios;
	current->current_ratio = 0;
	current->x_before_split = current->_x;
	current->w_before_split = current->_w;
	current->_w = zui_get_ratio(ratios->buffer[current->current_ratio], current->_w);
}

void zui_indent() {
	current->_x += ZUI_TAB_W();
	current->_w -= ZUI_TAB_W();
}

void zui_unindent() {
	current->_x -= ZUI_TAB_W();
	current->_w += ZUI_TAB_W();
}

void zui_end_element_of_size(float element_size) {
	if (current->current_window == NULL || current->current_window->layout == ZUI_LAYOUT_VERTICAL) {
		if (current->current_ratio == -1 || (current->ratios != NULL && current->current_ratio == current->ratios->length - 1)) { // New line
			current->_y += element_size;

			if ((current->ratios != NULL && current->current_ratio == current->ratios->length - 1)) { // Last row element
				current->current_ratio = -1;
				current->ratios = NULL;
				current->_x = current->x_before_split;
				current->_w = current->w_before_split;
			}
		}
		else { // Row
			current->current_ratio++;
			current->_x += current->_w; // More row elements to place
			current->_w = zui_get_ratio(current->ratios->buffer[current->current_ratio], current->w_before_split);
		}
	}
	else { // Horizontal
		current->_x += current->_w + ZUI_ELEMENT_OFFSET();
	}
}

void zui_end_element() {
	zui_end_element_of_size(ZUI_ELEMENT_H() + ZUI_ELEMENT_OFFSET());
}

void zui_resize(zui_handle_t *handle, int w, int h) {
	handle->redraws = 2;
	if (handle->texture.width != 0) kinc_g4_render_target_destroy(&handle->texture);
	if (w < 1) w = 1;
	if (h < 1) h = 1;
	kinc_g4_render_target_init(&handle->texture, w, h, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
}

bool zui_input_in_rect(float x, float y, float w, float h) {
	return current->enabled && current->input_enabled &&
		current->input_x >= x && current->input_x < (x + w) &&
		current->input_y >= y && current->input_y < (y + h);
}

bool zui_input_changed() {
	return current->input_dx != 0 || current->input_dy != 0 || current->input_wheel_delta != 0 || current->input_started || current->input_started_r || current->input_released || current->input_released_r || current->input_down || current->input_down_r || current->is_key_pressed;
}

void zui_end_input() {
	if (zui_on_tab_drop != NULL && current->drag_tab_handle != NULL) {
		if (current->input_dx != 0 || current->input_dy != 0) {
			kinc_mouse_set_cursor(1); // Hand
		}
		if (current->input_released) {
			kinc_mouse_set_cursor(0); // Default
			current->drag_tab_handle = NULL;
		}
	}

	current->is_key_pressed = false;
	current->input_started = false;
	current->input_started_r = false;
	current->input_released = false;
	current->input_released_r = false;
	current->input_dx = 0;
	current->input_dy = 0;
	current->input_wheel_delta = 0;
	current->pen_in_use = false;
	if (zui_key_repeat && current->is_key_down && kinc_time() - zui_key_repeat_time > 0.05) {
		if (current->key_code == KINC_KEY_BACKSPACE || current->key_code == KINC_KEY_DELETE || current->key_code == KINC_KEY_LEFT || current->key_code == KINC_KEY_RIGHT || current->key_code == KINC_KEY_UP || current->key_code == KINC_KEY_DOWN) {
			zui_key_repeat_time = kinc_time();
			current->is_key_pressed = true;
		}
	}
	if (zui_touch_hold && current->input_down && current->input_x == current->input_started_x && current->input_y == current->input_started_y && current->input_started_time > 0 && kinc_time() - current->input_started_time > 0.7) {
		current->touch_hold_activated = true;
		current->input_released_r = true;
		current->input_started_time = 0;
	}
}

void zui_scroll(float delta) {
	current->current_window->scroll_offset -= delta;
}

int zui_line_count(char *str) {
	if (str == NULL) return 0;
	int i = 0;
	int count = 1;
	while (str[i] != '\0') {
		if (str[i] == '\n') count++;
		i++;
	}
	return count;
}

char *zui_extract_line(char *str, int line) {
	int pos = 0;
	int len = strlen(str);
	int line_i = 0;
	for (int i = 0; i < len; ++i) {
		if (str[i] == '\n') { line_i++; continue; }
		if (line_i < line) continue;
		if (line_i > line) break;
		temp[pos++] = str[i];
	}
	temp[pos] = 0;
	return temp;
}

char *zui_lower_case(char *dest, char *src) {
	int len = strlen(src);
	assert(len < 1024);
	for (int i = 0; i < len; ++i) {
		dest[i] = tolower(src[i]);
	}
	return dest;
}

void zui_draw_tooltip_text(bool bind_global_g) {
	arm_g2_set_color(current->ops->theme->TEXT_COL);
	int line_count = zui_line_count(current->tooltip_text);
	float tooltip_w = 0.0;
	for (int i = 0; i < line_count; ++i) {
		float line_tooltip_w = arm_g2_string_width(current->ops->font->font_, current->font_size, zui_extract_line(current->tooltip_text, i));
		if (line_tooltip_w > tooltip_w) tooltip_w = line_tooltip_w;
	}
	current->tooltip_x = fmin(current->tooltip_x, kinc_window_width(0) - tooltip_w - 20);
	if (bind_global_g) arm_g2_restore_render_target();
	float font_height = arm_g2_font_height(current->ops->font->font_, current->font_size);
	float off = 0;
	if (current->tooltip_img != NULL) {
		float w = current->tooltip_img->tex_width;
		if (current->tooltip_img_max_width != 0 && w > current->tooltip_img_max_width) w = current->tooltip_img_max_width;
		off = current->tooltip_img->tex_height * (w / current->tooltip_img->tex_width);
	}
	else if (current->tooltip_rt != NULL) {
		float w = current->tooltip_rt->width;
		if (current->tooltip_img_max_width != 0 && w > current->tooltip_img_max_width) w = current->tooltip_img_max_width;
		off = current->tooltip_rt->height * (w / current->tooltip_rt->width);
	}
	arm_g2_fill_rect(current->tooltip_x, current->tooltip_y + off, tooltip_w + 20, font_height * line_count);
	arm_g2_set_font(current->ops->font->font_, current->font_size);
	arm_g2_set_color(current->ops->theme->ACCENT_COL);
	for (int i = 0; i < line_count; ++i) {
		arm_g2_draw_string(zui_extract_line(current->tooltip_text, i), current->tooltip_x + 5, current->tooltip_y + off + i * current->font_size);
	}
}

void zui_draw_tooltip_image(bool bind_global_g) {
	float w = current->tooltip_img->tex_width;
	if (current->tooltip_img_max_width != 0 && w > current->tooltip_img_max_width) w = current->tooltip_img_max_width;
	float h = current->tooltip_img->tex_height * (w / current->tooltip_img->tex_width);
	current->tooltip_x = fmin(current->tooltip_x, kinc_window_width(0) - w - 20);
	current->tooltip_y = fmin(current->tooltip_y, kinc_window_height(0) - h - 20);
	if (bind_global_g) arm_g2_restore_render_target();
	arm_g2_set_color(0xff000000);
	arm_g2_fill_rect(current->tooltip_x, current->tooltip_y, w, h);
	arm_g2_set_color(0xffffffff);
	current->tooltip_invert_y ?
		arm_g2_draw_scaled_image(current->tooltip_img, current->tooltip_x, current->tooltip_y + h, w, -h) :
		arm_g2_draw_scaled_image(current->tooltip_img, current->tooltip_x, current->tooltip_y, w, h);
}

void zui_draw_tooltip_rt(bool bind_global_g) {
	float w = current->tooltip_rt->width;
	if (current->tooltip_img_max_width != 0 && w > current->tooltip_img_max_width) w = current->tooltip_img_max_width;
	float h = current->tooltip_rt->height * (w / current->tooltip_rt->width);
	current->tooltip_x = fmin(current->tooltip_x, kinc_window_width(0) - w - 20);
	current->tooltip_y = fmin(current->tooltip_y, kinc_window_height(0) - h - 20);
	if (bind_global_g) arm_g2_restore_render_target();
	arm_g2_set_color(0xff000000);
	arm_g2_fill_rect(current->tooltip_x, current->tooltip_y, w, h);
	arm_g2_set_color(0xffffffff);
	current->tooltip_invert_y ?
		arm_g2_draw_scaled_render_target(current->tooltip_rt, current->tooltip_x, current->tooltip_y + h, w, -h) :
		arm_g2_draw_scaled_render_target(current->tooltip_rt, current->tooltip_x, current->tooltip_y, w, h);
}

void zui_draw_tooltip(bool bind_global_g) {
	if (current->slider_tooltip) {
		if (bind_global_g) arm_g2_restore_render_target();
		arm_g2_set_font(current->ops->font->font_, current->font_size * 2);
		assert(sprintf(NULL, "%f", round(current->scroll_handle->value * 100.0) / 100.0) < 1024);
		sprintf(temp, "%f", round(current->scroll_handle->value * 100.0) / 100.0);
		char *text = temp;
		float x_off = arm_g2_string_width(current->ops->font->font_, current->font_size * 2.0, text) / 2.0;
		float y_off = arm_g2_font_height(current->ops->font->font_, current->font_size * 2.0);
		float x = fmin(fmax(current->slider_tooltip_x, current->input_x), current->slider_tooltip_x + current->slider_tooltip_w);
		arm_g2_set_color(current->ops->theme->ACCENT_COL);
		arm_g2_fill_rect(x - x_off, current->slider_tooltip_y - y_off, x_off * 2.0, y_off);
		arm_g2_set_color(current->ops->theme->TEXT_COL);
		arm_g2_draw_string(text, x - x_off, current->slider_tooltip_y - y_off);
	}
	if (zui_touch_tooltip && current->text_selected_handle != NULL) {
		if (bind_global_g) arm_g2_restore_render_target();
		arm_g2_set_font(current->ops->font->font_, current->font_size * 2.0);
		float x_off = arm_g2_string_width(current->ops->font->font_, current->font_size * 2.0, current->text_selected) / 2.0;
		float y_off = arm_g2_font_height(current->ops->font->font_, current->font_size * 2.0) / 2.0;
		float x = kinc_window_width(0) / 2.0;
		float y = kinc_window_height(0) / 3.0;
		arm_g2_set_color(current->ops->theme->ACCENT_COL);
		arm_g2_fill_rect(x - x_off, y - y_off, x_off * 2.0, y_off * 2.0);
		arm_g2_set_color(current->ops->theme->TEXT_COL);
		arm_g2_draw_string(current->text_selected, x - x_off, y - y_off);
	}

	if (current->tooltip_text[0] != '\0' || current->tooltip_img != NULL || current->tooltip_rt != NULL) {
		if (zui_input_changed()) {
			current->tooltip_shown = false;
			current->tooltip_wait = current->input_dx == 0 && current->input_dy == 0; // Wait for movement before showing up again
		}
		if (!current->tooltip_shown) {
			current->tooltip_shown = true;
			current->tooltip_x = current->input_x;
			current->tooltip_time = kinc_time();
		}
		if (!current->tooltip_wait && kinc_time() - current->tooltip_time > ZUI_TOOLTIP_DELAY()) {
			if (current->tooltip_img != NULL) zui_draw_tooltip_image(bind_global_g);
			else if (current->tooltip_rt != NULL) zui_draw_tooltip_rt(bind_global_g);
			if (current->tooltip_text[0] != '\0') zui_draw_tooltip_text(bind_global_g);
		}
	}
	else current->tooltip_shown = false;
}

void zui_draw_combo(bool begin /*= true*/) {
	if (current->combo_selected_handle == NULL) return;
	arm_g2_set_color(current->ops->theme->SEPARATOR_COL);
	if (begin) {
		arm_g2_restore_render_target();
	}

	float combo_h = (current->combo_selected_texts->length + (current->combo_selected_label != NULL ? 1 : 0) + (current->combo_search_bar ? 1 : 0)) * ZUI_ELEMENT_H();
	float dist_top = current->combo_selected_y - combo_h - ZUI_ELEMENT_H() - current->window_border_top;
	float dist_bottom = kinc_window_height(0) - current->window_border_bottom - (current->combo_selected_y + combo_h );
	bool unroll_up = dist_bottom < 0 && dist_bottom < dist_top;
	zui_begin_region(current, current->combo_selected_x, current->combo_selected_y, current->combo_selected_w);
	if (current->is_key_pressed || current->input_wheel_delta != 0) {
		int arrow_up = current->is_key_pressed && current->key_code == (unroll_up ? KINC_KEY_DOWN : KINC_KEY_UP);
		int arrow_down = current->is_key_pressed && current->key_code == (unroll_up ? KINC_KEY_UP : KINC_KEY_DOWN);
		int wheel_up = (unroll_up && current->input_wheel_delta > 0) || (!unroll_up && current->input_wheel_delta < 0);
		int wheel_down = (unroll_up && current->input_wheel_delta < 0) || (!unroll_up && current->input_wheel_delta > 0);
		if ((arrow_up || wheel_up) && current->combo_to_submit > 0) {
			int step = 1;
			if (current->combo_search_bar && strlen(current->text_selected) > 0) {
				char search[512];
				char str[512];
				zui_lower_case(search, current->text_selected);
				while (true) {
					zui_lower_case(str, current->combo_selected_texts->buffer[current->combo_to_submit - step]);
					if (strstr(str, search) == NULL && current->combo_to_submit - step > 0) {
						++step;
					}
					else break;
				}

				// Corner case: current position is the top one according to the search pattern
				zui_lower_case(str, current->combo_selected_texts->buffer[current->combo_to_submit - step]);
				if (strstr(str, search) == NULL) step = 0;
			}
			current->combo_to_submit -= step;
			current->submit_combo_handle = current->combo_selected_handle;
		}
		else if ((arrow_down || wheel_down) && current->combo_to_submit < current->combo_selected_texts->length - 1) {
			int step = 1;
			if (current->combo_search_bar && strlen(current->text_selected) > 0) {
				char search[512];
				char str[512];
				zui_lower_case(search, current->text_selected);
				while (true) {
					zui_lower_case(str, current->combo_selected_texts->buffer[current->combo_to_submit + step]);
					if (strstr(str, search) == NULL && current->combo_to_submit + step > 0) {
						++step;
					}
					else break;
				}

				// Corner case: current position is the top one according to the search pattern
				zui_lower_case(str, current->combo_selected_texts->buffer[current->combo_to_submit + step]);
				if (strstr(str, search) == NULL) step = 0;
			}

			current->combo_to_submit += step;
			current->submit_combo_handle = current->combo_selected_handle;
		}
		if (current->combo_selected_window != NULL) current->combo_selected_window->redraws = 2;
	}

	current->input_enabled = true;
	int _BUTTON_COL = current->ops->theme->BUTTON_COL;
	int _ELEMENT_OFFSET = current->ops->theme->ELEMENT_OFFSET;
	current->ops->theme->ELEMENT_OFFSET = 0;
	float unroll_right = current->_x + current->combo_selected_w * 2.0 < kinc_window_width(0) - current->window_border_right ? 1 : -1;
	bool reset_position = false;
	char search[512];
	search[0] = '\0';
	if (current->combo_search_bar) {
		if (unroll_up) current->_y -= ZUI_ELEMENT_H() * 2.0;
		if (zui_combo_first) zui_combo_search_handle.text[0] = '\0';
		zui_fill(0, 0, current->_w / ZUI_SCALE(), ZUI_ELEMENT_H() / ZUI_SCALE(), current->ops->theme->SEPARATOR_COL);
		strcpy(search, zui_text_input(&zui_combo_search_handle, "", ZUI_ALIGN_LEFT, true, true));
		zui_lower_case(search, search);
		if (current->is_released) zui_combo_first = true; // Keep combo open
		if (zui_combo_first) {
			#if !defined(KINC_ANDROID) && !defined(KINC_IOS)
			zui_start_text_edit(&zui_combo_search_handle, ZUI_ALIGN_LEFT); // Focus search bar
			#endif
		}
		reset_position = zui_combo_search_handle.changed;
	}

	for (int i = 0; i < current->combo_selected_texts->length; ++i) {
		char str[512];
		zui_lower_case(str, current->combo_selected_texts->buffer[i]);
		if (strlen(search) > 0 && strstr(str, search) == NULL) {
			continue; // Don't show items that don't fit the current search pattern
		}

		if (reset_position) { // The search has changed, select first entry that matches
			current->combo_to_submit = current->combo_selected_handle->position = i;
			current->submit_combo_handle = current->combo_selected_handle;
			reset_position = false;
		}
		if (unroll_up) current->_y -= ZUI_ELEMENT_H() * 2.0;
		current->ops->theme->BUTTON_COL = i == current->combo_selected_handle->position ? current->ops->theme->ACCENT_SELECT_COL : current->ops->theme->SEPARATOR_COL;
		zui_fill(0, 0, current->_w / ZUI_SCALE(), ZUI_ELEMENT_H() / ZUI_SCALE(), current->ops->theme->SEPARATOR_COL);
		if (zui_button(current->combo_selected_texts->buffer[i], current->combo_selected_align, "")) {
			current->combo_to_submit = i;
			current->submit_combo_handle = current->combo_selected_handle;
			if (current->combo_selected_window != NULL) current->combo_selected_window->redraws = 2;
			break;
		}
		if (current->_y + ZUI_ELEMENT_H() > kinc_window_height(0) - current->window_border_bottom || current->_y - ZUI_ELEMENT_H() * 2 < current->window_border_top) {
			current->_x += current->combo_selected_w * unroll_right; // Next column
			current->_y = current->combo_selected_y;
		}
	}
	current->ops->theme->BUTTON_COL = _BUTTON_COL;
	current->ops->theme->ELEMENT_OFFSET = _ELEMENT_OFFSET;

	if (current->combo_selected_label != NULL) { // Unroll down
		if (unroll_up) {
			current->_y -= ZUI_ELEMENT_H() * 2.0;
			zui_fill(0, 0, current->_w / ZUI_SCALE(), ZUI_ELEMENT_H() / ZUI_SCALE(), current->ops->theme->SEPARATOR_COL);
			arm_g2_set_color(current->ops->theme->LABEL_COL);
			zui_draw_string(current->combo_selected_label, current->ops->theme->TEXT_OFFSET, 0, ZUI_ALIGN_RIGHT, true);
			current->_y += ZUI_ELEMENT_H();
			zui_fill(0, 0, current->_w / ZUI_SCALE(), 1.0 * ZUI_SCALE(), current->ops->theme->ACCENT_SELECT_COL); // Separator
		}
		else {
			zui_fill(0, 0, current->_w / ZUI_SCALE(), ZUI_ELEMENT_H() / ZUI_SCALE(), current->ops->theme->SEPARATOR_COL);
			zui_fill(0, 0, current->_w / ZUI_SCALE(), 1.0 * ZUI_SCALE(), current->ops->theme->ACCENT_SELECT_COL); // Separator
			arm_g2_set_color(current->ops->theme->LABEL_COL);
			zui_draw_string(current->combo_selected_label, current->ops->theme->TEXT_OFFSET, 0, ZUI_ALIGN_RIGHT, true);
		}
	}

	if ((current->input_released || current->input_released_r || current->is_escape_down || current->is_return_down) && !zui_combo_first) {
		current->combo_selected_handle = NULL;
		zui_combo_first = true;
	}
	else zui_combo_first = false;
	current->input_enabled = current->combo_selected_handle == NULL;
	zui_end_region(false);
}

void zui_bake_elements() {
	if (current->check_select_image.width != 0) {
		kinc_g4_render_target_destroy(&current->check_select_image);
	}
	float r = ZUI_CHECK_SELECT_SIZE();
	kinc_g4_render_target_init(&current->check_select_image, r, r, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
	arm_g2_set_render_target(&current->check_select_image);
	kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0x00000000, 0, 0);
	arm_g2_set_color(0xffffffff);
	arm_g2_draw_line(0, r / 2.0, r / 2.0 - 2.0 * ZUI_SCALE(), r - 2.0 * ZUI_SCALE(), 2.0 * ZUI_SCALE());
	arm_g2_draw_line(r / 2.0 - 3.0 * ZUI_SCALE(), r - 3.0 * ZUI_SCALE(), r / 2.0 + 5.0 * ZUI_SCALE(), r - 11.0 * ZUI_SCALE(), 2.0 * ZUI_SCALE());
	arm_g2_end();

	if (current->radio_image.width != 0) {
		kinc_g4_render_target_destroy(&current->radio_image);
	}
	r = ZUI_CHECK_SIZE();
	kinc_g4_render_target_init(&current->radio_image, r, r, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
	arm_g2_set_render_target(&current->radio_image);
	kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0x00000000, 0, 0);
	arm_g2_set_color(0xffaaaaaa);
	arm_g2_fill_circle(r / 2.0, r / 2.0, r / 2.0, 0);
	arm_g2_set_color(0xffffffff);
	arm_g2_draw_circle(r / 2.0, r / 2.0, r / 2.0, 0, 1.0 * ZUI_SCALE());
	arm_g2_end();

	if (current->radio_select_image.width != 0) {
		kinc_g4_render_target_destroy(&current->radio_select_image);
	}
	r = ZUI_CHECK_SELECT_SIZE();
	kinc_g4_render_target_init(&current->radio_select_image, r, r, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
	arm_g2_set_render_target(&current->radio_select_image);
	kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0x00000000, 0, 0);
	arm_g2_set_color(0xffaaaaaa);
	arm_g2_fill_circle(r / 2.0, r / 2.0, 4.5 * ZUI_SCALE(), 0);
	arm_g2_set_color(0xffffffff);
	arm_g2_fill_circle(r / 2.0, r / 2.0, 4.0 * ZUI_SCALE(), 0);
	arm_g2_end();

	if (current->ops->theme->ROUND_CORNERS) {
		if (current->filled_round_corner_image.width != 0) {
			kinc_g4_render_target_destroy(&current->filled_round_corner_image);
		}
		r = 4.0 * ZUI_SCALE();
		kinc_g4_render_target_init(&current->filled_round_corner_image, r, r, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
		arm_g2_set_render_target(&current->filled_round_corner_image);
		kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0x00000000, 0, 0);
		arm_g2_set_color(0xffffffff);
		arm_g2_fill_circle(r, r, r, 0);
		arm_g2_end();

		if (current->round_corner_image.width != 0) {
			kinc_g4_render_target_destroy(&current->round_corner_image);
		}
		kinc_g4_render_target_init(&current->round_corner_image, r, r, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);
		arm_g2_set_render_target(&current->round_corner_image);
		kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0x00000000, 0, 0);
		arm_g2_set_color(0xffffffff);
		arm_g2_draw_circle(r, r, r, 0, 1);
		arm_g2_end();
	}

	arm_g2_restore_render_target();
	current->elements_baked = true;
}

void zui_begin_region(zui_t *ui, int x, int y, int w) {
	current = ui;
	if (!current->elements_baked) {
		zui_bake_elements();
	}
	current->changed = false;
	current->current_window = NULL;
	current->tooltip_text[0] = '\0';
	current->tooltip_img = NULL;
	current->tooltip_rt = NULL;
	current->_window_x = 0;
	current->_window_y = 0;
	current->_window_w = w;
	current->_x = x;
	current->_y = y;
	current->_w = w;
	current->_h = 0;
}

void zui_end_region(bool last) {
	zui_draw_tooltip(false);
	current->tab_pressed_handle = NULL;
	if (last) {
		zui_draw_combo(false); // Handle active combo
		zui_end_input();
	}
}

void zui_set_cursor_to_input(int align) {
	float off = align == ZUI_ALIGN_LEFT ? ZUI_TEXT_OFFSET() : current->_w - arm_g2_string_width(current->ops->font->font_, current->font_size, current->text_selected);
	float x = current->input_x - (current->_window_x + current->_x + off);
	current->cursor_x = 0;
	while (current->cursor_x < strlen(current->text_selected) && arm_g2_sub_string_width(current->ops->font->font_, current->font_size, current->text_selected, 0, current->cursor_x) < x) {
		current->cursor_x++;
	}
	current->highlight_anchor = current->cursor_x;
}

void zui_start_text_edit(zui_handle_t *handle, int align) {
	current->is_typing = true;
	current->submit_text_handle = current->text_selected_handle;
	strcpy(current->text_to_submit, current->text_selected);
	current->text_selected_handle = handle;
	strcpy(current->text_selected, handle->text);
	current->cursor_x = strlen(handle->text);
	if (current->tab_pressed) {
		current->tab_pressed = false;
		current->is_key_pressed = false; // Prevent text deselect after tab press
	}
	else if (!current->highlight_on_select) { // Set cursor to click location
		zui_set_cursor_to_input(align);
	}
	current->tab_pressed_handle = handle;
	current->highlight_anchor = current->highlight_on_select ? 0 : current->cursor_x;
	kinc_keyboard_show();
}

void zui_submit_text_edit() {
	current->changed = strcmp(current->submit_text_handle->text, current->text_to_submit) != 0;
	current->submit_text_handle->changed = current->changed;
	strcpy(current->submit_text_handle->text, current->text_to_submit);
	current->submit_text_handle = NULL;
	current->text_to_submit[0] = '\0';
	current->text_selected[0] = '\0';
}

void zui_deselect_text(zui_t *ui) {
	if (ui->text_selected_handle == NULL) return;
	ui->submit_text_handle = ui->text_selected_handle;
	strcpy(ui->text_to_submit, ui->text_selected);
	ui->text_selected_handle = NULL;
	ui->is_typing = false;
	if (ui->current_window != NULL) ui->current_window->redraws = 2;
	kinc_keyboard_hide();
	ui->highlight_anchor = ui->cursor_x;
	if (zui_on_deselect_text != NULL) zui_on_deselect_text();
}

void zui_remove_char_at(char *str, int at) {
	int len = strlen(str);
	for (int i = at; i < len; ++i) {
		str[i] = str[i + 1];
	}
}

void zui_remove_chars_at(char *str, int at, int count) {
	for (int i = 0; i < count; ++i) {
		zui_remove_char_at(str, at);
	}
}

void zui_insert_char_at(char *str, int at, char c) {
	int len = strlen(str);
	for (int i = len + 1; i > at; --i) {
		str[i] = str[i - 1];
	}
	str[at] = c;
}

void zui_insert_chars_at(char *str, int at, char *cs) {
	int len = strlen(cs);
	for (int i = 0; i < len; ++i) {
		zui_insert_char_at(str, at + i, cs[i]);
	}
}

void zui_update_text_edit(int align, bool editable, bool live_update) {
	char text[256];
	strcpy(text, current->text_selected);
	if (current->is_key_pressed) { // Process input
		if (current->key_code == KINC_KEY_LEFT) { // Move cursor
			if (current->cursor_x > 0) current->cursor_x--;
		}
		else if (current->key_code == KINC_KEY_RIGHT) {
			if (current->cursor_x < strlen(text)) current->cursor_x++;
		}
		else if (editable && current->key_code == KINC_KEY_BACKSPACE) { // Remove char
			if (current->cursor_x > 0 && current->highlight_anchor == current->cursor_x) {
				zui_remove_char_at(text, current->cursor_x - 1);
				current->cursor_x--;
			}
			else if (current->highlight_anchor < current->cursor_x) {
				int count = current->cursor_x - current->highlight_anchor;
				zui_remove_chars_at(text, current->highlight_anchor, count);
				current->cursor_x = current->highlight_anchor;
			}
			else {
				int count = current->highlight_anchor - current->cursor_x;
				zui_remove_chars_at(text, current->cursor_x, count);
			}
		}
		else if (editable && current->key_code == KINC_KEY_DELETE) {
			if (current->highlight_anchor == current->cursor_x) {
				zui_remove_char_at(text, current->cursor_x);
			}
			else if (current->highlight_anchor < current->cursor_x) {
				int count = current->cursor_x - current->highlight_anchor;
				zui_remove_chars_at(text, current->highlight_anchor, count);
				current->cursor_x = current->highlight_anchor;
			}
			else {
				int count = current->highlight_anchor - current->cursor_x;
				zui_remove_chars_at(text, current->cursor_x, count);
			}
		}
		else if (current->key_code == KINC_KEY_RETURN) { // Deselect
			zui_deselect_text(current);
		}
		else if (current->key_code == KINC_KEY_ESCAPE) { // Cancel
			strcpy(current->text_selected, current->text_selected_handle->text);
			zui_deselect_text(current);
		}
		else if (current->key_code == KINC_KEY_TAB && current->tab_switch_enabled && !current->is_ctrl_down) { // Next field
			current->tab_pressed = true;
			zui_deselect_text(current);
			current->key_code = 0;
		}
		else if (current->key_code == KINC_KEY_HOME) {
			current->cursor_x = 0;
		}
		else if (current->key_code == KINC_KEY_END) {
			current->cursor_x = strlen(text);
		}
		else if (current->is_ctrl_down && current->is_a_down) { // Select all
			current->cursor_x = strlen(text);
			current->highlight_anchor = 0;
		}
		else if (editable && // Write
				 current->key_code != KINC_KEY_SHIFT &&
				 current->key_code != KINC_KEY_CAPS_LOCK &&
				 current->key_code != KINC_KEY_CONTROL &&
				 current->key_code != KINC_KEY_META &&
				 current->key_code != KINC_KEY_ALT &&
				 current->key_code != KINC_KEY_UP &&
				 current->key_code != KINC_KEY_DOWN &&
				 current->key_char >= 32) {
			zui_remove_chars_at(text, current->highlight_anchor, current->cursor_x - current->highlight_anchor);
			zui_insert_char_at(text, current->highlight_anchor, current->key_char);

			current->cursor_x = current->cursor_x + 1 > strlen(text) ? strlen(text) : current->cursor_x + 1;
		}
		bool selecting = current->is_shift_down && (current->key_code == KINC_KEY_LEFT || current->key_code == KINC_KEY_RIGHT || current->key_code == KINC_KEY_SHIFT);
		// isCtrlDown && isAltDown is the condition for AltGr was pressed
		// AltGr is part of the German keyboard layout and part of key combinations like AltGr + e -> €
		if (!selecting && (!current->is_ctrl_down || (current->is_ctrl_down && current->is_alt_down))) {
			current->highlight_anchor = current->cursor_x;
		}
	}

	if (editable && zui_text_to_paste[0] != '\0') { // Process cut copy paste
		zui_remove_chars_at(text, current->highlight_anchor, current->cursor_x - current->highlight_anchor);
		zui_insert_chars_at(text, current->highlight_anchor, zui_text_to_paste);
		current->cursor_x += strlen(zui_text_to_paste);
		current->highlight_anchor = current->cursor_x;
		zui_text_to_paste[0] = 0;
		zui_is_paste = false;
	}
	if (current->highlight_anchor == current->cursor_x) {
		strcpy(zui_text_to_copy, text); // Copy
	}
	else if (current->highlight_anchor < current->cursor_x) {
		int len = current->cursor_x - current->highlight_anchor;
		strncpy(zui_text_to_copy, text + current->highlight_anchor, len);
		zui_text_to_copy[len] = '\0';
	}
	else {
		int len = current->highlight_anchor - current->cursor_x;
		strncpy(zui_text_to_copy, text + current->cursor_x, len);
		zui_text_to_copy[len] = '\0';
	}
	if (editable && zui_is_cut) { // Cut
		if (current->highlight_anchor == current->cursor_x) {
			text[0] = '\0';
		}
		else if (current->highlight_anchor < current->cursor_x) {
			zui_remove_chars_at(text, current->highlight_anchor, current->cursor_x - current->highlight_anchor);
			current->cursor_x = current->highlight_anchor;
		}
		else {
			zui_remove_chars_at(text, current->cursor_x, current->highlight_anchor - current->cursor_x);
		}
	}

	float off = ZUI_TEXT_OFFSET();
	float line_height = ZUI_ELEMENT_H();
	float cursor_height = line_height - current->button_offset_y * 3.0;
	// Draw highlight
	if (current->highlight_anchor != current->cursor_x) {
		float istart = current->cursor_x;
		float iend = current->highlight_anchor;
		if (current->highlight_anchor < current->cursor_x) {
			istart = current->highlight_anchor;
			iend = current->cursor_x;
		}

		float hlstrw = arm_g2_sub_string_width(current->ops->font->font_, current->font_size, text, istart, iend);
		float start_off = arm_g2_sub_string_width(current->ops->font->font_, current->font_size, text, 0, istart);
		float hl_start = align == ZUI_ALIGN_LEFT ? current->_x + start_off + off : current->_x + current->_w - hlstrw - off;
		if (align == ZUI_ALIGN_RIGHT) {
			hl_start -= arm_g2_sub_string_width(current->ops->font->font_, current->font_size, text, iend, strlen(text));
		}
		arm_g2_set_color(current->ops->theme->ACCENT_SELECT_COL);
		arm_g2_fill_rect(hl_start, current->_y + current->button_offset_y * 1.5, hlstrw, cursor_height);
	}

	// Draw cursor
	int str_start = align == ZUI_ALIGN_LEFT ? 0 : current->cursor_x;
	int str_length = align == ZUI_ALIGN_LEFT ? current->cursor_x : (strlen(text) - current->cursor_x);
	float strw = arm_g2_sub_string_width(current->ops->font->font_, current->font_size, text, str_start, str_length);
	float cursor_x = align == ZUI_ALIGN_LEFT ? current->_x + strw + off : current->_x + current->_w - strw - off;
	arm_g2_set_color(current->ops->theme->TEXT_COL); // Cursor
	arm_g2_fill_rect(cursor_x, current->_y + current->button_offset_y * 1.5, 1.0 * ZUI_SCALE(), cursor_height);

	strcpy(current->text_selected, text);
	if (live_update && current->text_selected_handle != NULL) {
		current->text_selected_handle->changed = strcmp(current->text_selected_handle->text, current->text_selected) != 0;
		strcpy(current->text_selected_handle->text, current->text_selected);
	}
}

void zui_set_hovered_tab_name(char *name) {
	if (zui_input_in_rect(current->_window_x, current->_window_y, current->_window_w, current->_window_h)) {
		strcpy(current->hovered_tab_name, name);
		current->hovered_tab_x = current->_window_x;
		current->hovered_tab_y = current->_window_y;
		current->hovered_tab_w = current->_window_w;
		current->hovered_tab_h = current->_window_h;
	}
}

void zui_draw_tabs() {
	current->input_x = current->restore_x;
	current->input_y = current->restore_y;
	if (current->current_window == NULL) return;
	float tab_x = 0.0;
	float tab_y = 0.0;
	float tab_h_min = ZUI_BUTTON_H() * 1.1;
	float header_h = current->current_window->drag_enabled ? ZUI_HEADER_DRAG_H() : 0;
	float tab_h = (current->ops->theme->FULL_TABS && current->tab_vertical) ? ((current->_window_h - header_h) / current->tab_count) : tab_h_min;
	float orig_y = current->_y;
	current->_y = header_h;
	current->tab_handle->changed = false;

	if (current->is_ctrl_down && current->is_tab_down) { // Next tab
		current->tab_handle->position++;
		if (current->tab_handle->position >= current->tab_count) current->tab_handle->position = 0;
		current->tab_handle->changed = true;
		current->is_tab_down = false;
	}

	if (current->tab_handle->position >= current->tab_count) current->tab_handle->position = current->tab_count - 1;

	arm_g2_set_color(current->ops->theme->SEPARATOR_COL); // Tab background
	if (current->tab_vertical) {
		arm_g2_fill_rect(0, current->_y, ZUI_ELEMENT_W(), current->_window_h);
	}
	else {
		arm_g2_fill_rect(0, current->_y, current->_window_w, current->button_offset_y + tab_h + 2);
	}

	arm_g2_set_color(current->ops->theme->ACCENT_COL); // Underline tab buttons
	if (current->tab_vertical) {
		arm_g2_fill_rect(ZUI_ELEMENT_W(), current->_y, 1, current->_window_h);
	}
	else {
		arm_g2_fill_rect(current->button_offset_y, current->_y + current->button_offset_y + tab_h + 2, current->_window_w - current->button_offset_y * 2.0, 1);
	}

	float base_y = current->tab_vertical ? current->_y : current->_y + 2;
	bool _enabled = current->enabled;

	for (int i = 0; i < current->tab_count; ++i) {
		current->enabled = current->tab_enabled[i];
		current->_x = tab_x;
		current->_y = base_y + tab_y;
		current->_w = current->tab_vertical ? (ZUI_ELEMENT_W() - 1 * ZUI_SCALE()) :
			 		  current->ops->theme->FULL_TABS ? (current->_window_w / current->tab_count) :
					  (arm_g2_string_width(current->ops->font->font_, current->font_size, current->tab_names[i]) + current->button_offset_y * 2.0 + 18.0 * ZUI_SCALE());
		bool released = zui_get_released(tab_h);
		bool started = zui_get_started(tab_h);
		bool pushed = zui_get_pushed(tab_h);
		bool hover = zui_get_hover(tab_h);
		if (zui_on_tab_drop != NULL) {
			if (started) {
				current->drag_tab_handle = current->tab_handle;
				current->drag_tab_position = i;
			}
			if (current->drag_tab_handle != NULL && hover && current->input_released) {
				zui_on_tab_drop(current->tab_handle, i, current->drag_tab_handle, current->drag_tab_position);
				current->tab_handle->position = i;
			}
		}
		if (released) {
			zui_handle_t *h = zui_nest(current->tab_handle, current->tab_handle->position); // Restore tab scroll
			h->scroll_offset = current->current_window->scroll_offset;
			h = zui_nest(current->tab_handle, i);
			current->tab_scroll = h->scroll_offset;
			current->tab_handle->position = i; // Set new tab
			current->current_window->redraws = 3;
			current->tab_handle->changed = true;
		}
		bool selected = current->tab_handle->position == i;

		arm_g2_set_color((pushed || hover) ? current->ops->theme->BUTTON_HOVER_COL :
			current->tab_colors[i] != -1 ? current->tab_colors[i] :
			selected ? current->ops->theme->WINDOW_BG_COL :
			current->ops->theme->SEPARATOR_COL);
		if (current->tab_vertical) {
			tab_y += tab_h + 1;
		}
		else {
			tab_x += current->_w + 1;
		}
		// zui_draw_rect(true, current->_x + current->button_offset_y, current->_y + current->button_offset_y, current->_w, tab_h); // Round corners
		arm_g2_fill_rect(current->_x + current->button_offset_y, current->_y + current->button_offset_y, current->_w, tab_h);
		arm_g2_set_color(current->ops->theme->BUTTON_TEXT_COL);
		if (!selected) zui_fade_color(0.65);
		zui_draw_string(current->tab_names[i], current->ops->theme->TEXT_OFFSET, (tab_h - tab_h_min) / 2.0, (current->ops->theme->FULL_TABS || !current->tab_vertical) ? ZUI_ALIGN_CENTER : ZUI_ALIGN_LEFT, true);

		if (selected) { // Hide underline for active tab
			if (current->tab_vertical) {
				// Hide underline
				// arm_g2_set_color(current->ops->theme->WINDOW_BG_COL);
				// arm_g2_fill_rect(current->_x + current->button_offset_y + current->_w - 1, current->_y + current->button_offset_y - 1, 2, tab_h + current->button_offset_y);
				// Highlight
				arm_g2_set_color(current->ops->theme->HIGHLIGHT_COL);
				arm_g2_fill_rect(current->_x + current->button_offset_y, current->_y + current->button_offset_y - 1, 2, tab_h + current->button_offset_y);
			}
			else {
				// Hide underline
				arm_g2_set_color(current->ops->theme->WINDOW_BG_COL);
				arm_g2_fill_rect(current->_x + current->button_offset_y, current->_y + current->button_offset_y + tab_h, current->_w, 1);
				// Highlight
				arm_g2_set_color(current->ops->theme->HIGHLIGHT_COL);
				arm_g2_fill_rect(current->_x + current->button_offset_y, current->_y + current->button_offset_y, current->_w, 2);
			}
		}

		// Tab separator
		if (i < current->tab_count - 1) {
			arm_g2_set_color(current->ops->theme->SEPARATOR_COL - 0x00050505);
			if (current->tab_vertical) {
				arm_g2_fill_rect(current->_x, current->_y + tab_h, current->_w, 1);
			}
			else {
				arm_g2_fill_rect(current->_x + current->button_offset_y + current->_w, current->_y, 1, tab_h);
			}
		}
	}

	current->enabled = _enabled;
	zui_set_hovered_tab_name(current->tab_names[current->tab_handle->position]);

	current->_x = 0; // Restore positions
	current->_y = orig_y;
	current->_w = !current->current_window->scroll_enabled ? current->_window_w : current->_window_w - ZUI_SCROLL_W();
}

void zui_draw_arrow(bool selected) {
	float x = current->_x + current->arrow_offset_x;
	float y = current->_y + current->arrow_offset_y;
	arm_g2_set_color(current->ops->theme->TEXT_COL);
	if (selected) {
		arm_g2_fill_triangle(x, y,
						 x + ZUI_ARROW_SIZE(), y,
						 x + ZUI_ARROW_SIZE() / 2.0, y + ZUI_ARROW_SIZE());
	}
	else {
		arm_g2_fill_triangle(x, y,
						 x, y + ZUI_ARROW_SIZE(),
						 x + ZUI_ARROW_SIZE(), y + ZUI_ARROW_SIZE() / 2.0);
	}
}

void zui_draw_tree(bool selected) {
	float SIGN_W = 7.0 * ZUI_SCALE();
	float x = current->_x + current->arrow_offset_x + 1;
	float y = current->_y + current->arrow_offset_y + 1;
	arm_g2_set_color(current->ops->theme->TEXT_COL);
	if (selected) {
		arm_g2_fill_rect(x, y + SIGN_W / 2.0 - 1, SIGN_W, SIGN_W / 8.0);
	}
	else {
		arm_g2_fill_rect(x, y + SIGN_W / 2.0 - 1, SIGN_W, SIGN_W / 8.0);
		arm_g2_fill_rect(x + SIGN_W / 2.0 - 1, y, SIGN_W / 8.0, SIGN_W);
	}
}

void zui_draw_check(bool selected, bool hover) {
	float x = current->_x + current->check_offset_x;
	float y = current->_y + current->check_offset_y;

	arm_g2_set_color(hover ? current->ops->theme->ACCENT_HOVER_COL : current->ops->theme->ACCENT_COL);
	zui_draw_rect(current->ops->theme->FILL_ACCENT_BG, x, y, ZUI_CHECK_SIZE(), ZUI_CHECK_SIZE()); // Bg

	if (selected) { // Check
		arm_g2_set_color(current->ops->theme->ACCENT_SELECT_COL);
		if (!current->enabled) zui_fade_color(0.25);
		int size = ZUI_CHECK_SELECT_SIZE();
		arm_g2_draw_scaled_render_target(&current->check_select_image, x + current->check_select_offset_x, y + current->check_select_offset_y, size, size);
	}
}

void zui_draw_radio(bool selected, bool hover) {
	float x = current->_x + current->radio_offset_x;
	float y = current->_y + current->radio_offset_y;
	arm_g2_set_color(hover ? current->ops->theme->ACCENT_HOVER_COL : current->ops->theme->ACCENT_COL);
	// Rect bg
	// zui_draw_rect(current->ops->theme->FILL_ACCENT_BG, x, y, ZUI_CHECK_SIZE(), ZUI_CHECK_SIZE());
	// Circle bg
	arm_g2_draw_render_target(&current->radio_image, x, y);

	if (selected) { // Check
		arm_g2_set_color(current->ops->theme->ACCENT_SELECT_COL);
		if (!current->enabled) zui_fade_color(0.25);
		// Rect
		// arm_g2_fill_rect(x + current->radio_select_offset_x, y + current->radio_select_offset_y, ZUI_CHECK_SELECT_SIZE(), ZUI_CHECK_SELECT_SIZE());
		// Circle
		arm_g2_draw_render_target(&current->radio_select_image, x + current->radio_select_offset_x, y + current->radio_select_offset_y);
	}
}

void zui_draw_slider(float value, float from, float to, bool filled, bool hover) {
	float x = current->_x + current->button_offset_y;
	float y = current->_y + current->button_offset_y;
	float w = current->_w - current->button_offset_y * 2.0;

	arm_g2_set_color(hover ? current->ops->theme->ACCENT_HOVER_COL : current->ops->theme->ACCENT_COL);
	zui_draw_rect(current->ops->theme->FILL_ACCENT_BG, x, y, w, ZUI_BUTTON_H()); // Bg

	arm_g2_set_color(hover ? current->ops->theme->ACCENT_HOVER_COL : current->ops->theme->ACCENT_COL);
	float offset = (value - from) / (to - from);
	float bar_w = 8.0 * ZUI_SCALE(); // Unfilled bar
	float slider_x = filled ? x : x + (w - bar_w) * offset;
	slider_x = fmax(fmin(slider_x, x + (w - bar_w)), x);
	float slider_w = filled ? w * offset : bar_w;
	slider_w = fmax(fmin(slider_w, w), 0);
	zui_draw_rect(true, slider_x, y, slider_w, ZUI_BUTTON_H());
}

void zui_set_scale(float factor) {
	current->ops->scale_factor = factor;
	current->font_size = ZUI_FONT_SIZE();
	float font_height = arm_g2_font_height(current->ops->font->font_, current->font_size);
	current->font_offset_y = (ZUI_ELEMENT_H() - font_height) / 2.0; // Precalculate offsets
	current->arrow_offset_y = (ZUI_ELEMENT_H() - ZUI_ARROW_SIZE()) / 2.0;
	current->arrow_offset_x = current->arrow_offset_y;
	current->title_offset_x = (current->arrow_offset_x * 2.0 + ZUI_ARROW_SIZE()) / ZUI_SCALE();
	current->button_offset_y = (ZUI_ELEMENT_H() - ZUI_BUTTON_H()) / 2.0;
	current->check_offset_y = (ZUI_ELEMENT_H() - ZUI_CHECK_SIZE()) / 2.0;
	current->check_offset_x = current->check_offset_y;
	current->check_select_offset_y = (ZUI_CHECK_SIZE() - ZUI_CHECK_SELECT_SIZE()) / 2.0;
	current->check_select_offset_x = current->check_select_offset_y;
	current->radio_offset_y = (ZUI_ELEMENT_H() - ZUI_CHECK_SIZE()) / 2.0;
	current->radio_offset_x = current->radio_offset_y;
	current->radio_select_offset_y = (ZUI_CHECK_SIZE() - ZUI_CHECK_SELECT_SIZE()) / 2.0;
	current->radio_select_offset_x = current->radio_select_offset_y;
	current->elements_baked = false;
}

void zui_init(zui_t *ui, zui_options_t *ops) {
	assert(zui_instances_count < ZUI_MAX_INSTANCES);
	zui_instances[zui_instances_count++] = ui;
	current = ui;
	memset(current, 0, sizeof(zui_t));
	current->ops = ops;
	zui_set_scale(ops->scale_factor);
	current->enabled = true;
	current->scroll_enabled = true;
	current->highlight_on_select = true;
	current->tab_switch_enabled = true;
	current->input_enabled = true;
	current->current_ratio = -1;
	current->image_scroll_align = true;
	current->window_ended = true;
	current->restore_x = -1;
	current->restore_y = -1;
}

void zui_begin(zui_t *ui) {
	current = ui;
	if (!current->elements_baked) {
		zui_bake_elements();
	}
	current->changed = false;
	current->_x = 0; // Reset cursor
	current->_y = 0;
	current->_w = 0;
	current->_h = 0;
}

// Sticky region ignores window scrolling
void zui_begin_sticky() {
	current->sticky = true;
	current->_y -= current->current_window->scroll_offset;
	if (current->current_window->scroll_enabled) current->_w += ZUI_SCROLL_W(); // Use full width since there is no scroll bar in sticky region
}

void zui_end_sticky() {
	arm_g2_end();
	current->sticky = false;
	current->scissor = true;
	kinc_g4_scissor(0, current->_y, current->_window_w, current->_window_h - current->_y);
	current->window_header_h += current->_y - current->window_header_h;
	current->_y += current->current_window->scroll_offset;
	current->is_hovered = false;
	if (current->current_window->scroll_enabled) current->_w -= ZUI_SCROLL_W();
}

void zui_end_window(bool bind_global_g) {
	zui_handle_t *handle = current->current_window;
	if (handle == NULL) return;
	if (handle->redraws > 0 || current->is_scrolling) {
		if (current->scissor) {
			current->scissor = false;
			kinc_g4_disable_scissor();
		}

		if (current->tab_count > 0) zui_draw_tabs();

		if (handle->drag_enabled) { // Draggable header
			arm_g2_set_color(current->ops->theme->SEPARATOR_COL);
			arm_g2_fill_rect(0, 0, current->_window_w, ZUI_HEADER_DRAG_H());
		}

		float window_size = handle->layout == ZUI_LAYOUT_VERTICAL ? current->_window_h - current->window_header_h : current->_window_w - current->window_header_w; // Exclude header
		float full_size = handle->layout == ZUI_LAYOUT_VERTICAL ? current->_y - current->window_header_h : current->_x - current->window_header_w;
		full_size -= handle->scroll_offset;

		if (full_size < window_size || !current->scroll_enabled) { // Disable scrollbar
			handle->scroll_enabled = false;
			handle->scroll_offset = 0;
		}
		else { // Draw window scrollbar if necessary
			if (handle->layout == ZUI_LAYOUT_VERTICAL) {
				handle->scroll_enabled = true;
			}
			if (current->tab_scroll < 0) { // Restore tab
				handle->scroll_offset = current->tab_scroll;
				current->tab_scroll = 0;
			}
			float wy = current->_window_y + current->window_header_h;
			float amount_to_scroll = full_size - window_size;
			float amount_scrolled = -handle->scroll_offset;
			float ratio = amount_scrolled / amount_to_scroll;
			float bar_h = window_size * fabs(window_size / full_size);
			bar_h = fmax(bar_h, ZUI_ELEMENT_H());

			float total_scrollable_area = window_size - bar_h;
			float e = amount_to_scroll / total_scrollable_area;
			float bar_y = total_scrollable_area * ratio + current->window_header_h;
			bool bar_focus = zui_input_in_rect(current->_window_x + current->_window_w - ZUI_SCROLL_W(), bar_y + current->_window_y, ZUI_SCROLL_W(), bar_h);

			if (handle->scroll_enabled && current->input_started && bar_focus) { // Start scrolling
				current->scroll_handle = handle;
				current->is_scrolling = true;
			}

			float scroll_delta = current->input_wheel_delta;
			if (handle->scroll_enabled && zui_touch_scroll && current->input_down && current->input_dy != 0 && current->input_x > current->_window_x + current->window_header_w && current->input_y > current->_window_y + current->window_header_h) {
				current->is_scrolling = true;
				scroll_delta = -current->input_dy / 20.0;
			}
			if (handle == current->scroll_handle) { // Scroll
				zui_scroll(current->input_dy * e);
			}
			else if (scroll_delta != 0 && current->combo_selected_handle == NULL && zui_input_in_rect(current->_window_x, wy, current->_window_w, window_size)) { // Wheel
				zui_scroll(scroll_delta * ZUI_ELEMENT_H());
			}

			// Stay in bounds
			if (handle->scroll_offset > 0) {
				handle->scroll_offset = 0;
			}
			else if (full_size + handle->scroll_offset < window_size) {
				handle->scroll_offset = window_size - full_size;
			}

			if (handle->layout == ZUI_LAYOUT_VERTICAL) {
				arm_g2_set_color(current->ops->theme->ACCENT_COL); // Bar
				bool scrollbar_focus = zui_input_in_rect(current->_window_x + current->_window_w - ZUI_SCROLL_W(), wy, ZUI_SCROLL_W(), window_size);
				float bar_w = (scrollbar_focus || handle == current->scroll_handle) ? ZUI_SCROLL_W() : ZUI_SCROLL_MINI_W();
				zui_draw_rect(true, current->_window_w - bar_w - current->scroll_align, bar_y, bar_w, bar_h);
			}
		}

		handle->last_max_x = current->_x;
		handle->last_max_y = current->_y;
		if (handle->layout == ZUI_LAYOUT_VERTICAL) handle->last_max_x += current->_window_w;
		else handle->last_max_y += current->_window_h;
		handle->redraws--;
	}

	current->window_ended = true;

	// Draw window texture
	if (zui_always_redraw_window || handle->redraws > -4) {
		if (bind_global_g) arm_g2_restore_render_target();
		arm_g2_set_color(current->ops->theme->WINDOW_TINT_COL);
		arm_g2_draw_render_target(&handle->texture, current->_window_x, current->_window_y);
		if (handle->redraws <= 0) handle->redraws--;
	}
}

bool zui_window_dirty(zui_handle_t *handle, int x, int y, int w, int h) {
	float wx = x + handle->drag_x;
	float wy = y + handle->drag_y;
	float input_changed = zui_input_in_rect(wx, wy, w, h) && zui_input_changed();
	return current->always_redraw || current->is_scrolling || input_changed;
}

bool zui_window(zui_handle_t *handle, int x, int y, int w, int h, bool drag) {
	if (handle->texture.width == 0 || w != handle->texture.width || h != handle->texture.height) {
		zui_resize(handle, w, h);
	}

	if (!current->window_ended) zui_end_window(true); // End previous window if necessary
	current->window_ended = false;

	arm_g2_set_render_target(&handle->texture);
	current->current_window = handle;
	current->_window_x = x + handle->drag_x;
	current->_window_y = y + handle->drag_y;
	current->_window_w = w;
	current->_window_h = h;
	current->window_header_w = 0;
	current->window_header_h = 0;

	if (zui_window_dirty(handle, x, y, w, h)) {
		handle->redraws = 2;
	}

	if (zui_on_border_hover != NULL) {
		if (zui_input_in_rect(current->_window_x - 4, current->_window_y, 8, current->_window_h)) {
			zui_on_border_hover(handle, 0);
		}
		else if (zui_input_in_rect(current->_window_x + current->_window_w - 4, current->_window_y, 8, current->_window_h)) {
			zui_on_border_hover(handle, 1);
		}
		else if (zui_input_in_rect(current->_window_x, current->_window_y - 4, current->_window_w, 8)) {
			zui_on_border_hover(handle, 2);
		}
		else if (zui_input_in_rect(current->_window_x, current->_window_y + current->_window_h - 4, current->_window_w, 8)) {
			zui_on_border_hover(handle, 3);
		}
	}

	if (handle->redraws <= 0) {
		return false;
	}

	if (handle->layout == ZUI_LAYOUT_VERTICAL) {
		current->_x = 0;
		current->_y = handle->scroll_offset;
	}
	else {
		current->_x = handle->scroll_offset;
		current->_y = 0;
	}
	if (handle->layout == ZUI_LAYOUT_HORIZONTAL) w = ZUI_ELEMENT_W();
	current->_w = !handle->scroll_enabled ? w : w - ZUI_SCROLL_W(); // Exclude scrollbar if present
	current->_h = h;
	current->tooltip_text[0] = 0;
	current->tooltip_img = NULL;
	current->tooltip_rt = NULL;
	current->tab_count = 0;

	if (current->ops->theme->FILL_WINDOW_BG) {
		kinc_g4_clear(KINC_G4_CLEAR_COLOR, current->ops->theme->WINDOW_BG_COL, 0, 0);
	}
	else {
		kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0x00000000, 0, 0);
		arm_g2_set_color(current->ops->theme->WINDOW_BG_COL);
		arm_g2_fill_rect(current->_x, current->_y - handle->scroll_offset, handle->last_max_x, handle->last_max_y);
	}

	handle->drag_enabled = drag;
	if (drag) {
		if (current->input_started && zui_input_in_rect(current->_window_x, current->_window_y, current->_window_w, ZUI_HEADER_DRAG_H())) {
			current->drag_handle = handle;
		}
		else if (current->input_released) {
			current->drag_handle = NULL;
		}
		if (handle == current->drag_handle) {
			handle->redraws = 2;
			handle->drag_x += current->input_dx;
			handle->drag_y += current->input_dy;
		}
		current->_y += ZUI_HEADER_DRAG_H(); // Header offset
		current->window_header_h += ZUI_HEADER_DRAG_H();
	}

	return true;
}

bool zui_button(char *text, int align, char *label/*, kinc_g4_texture_t *icon, int sx, int sy, int sw, int sh*/) {
	if (!zui_is_visible(ZUI_ELEMENT_H())) {
		zui_end_element();
		return false;
	}
	bool released = zui_get_released(ZUI_ELEMENT_H());
	bool pushed = zui_get_pushed(ZUI_ELEMENT_H());
	bool hover = zui_get_hover(ZUI_ELEMENT_H());
	if (released) current->changed = true;

	arm_g2_set_color(pushed ? current->ops->theme->BUTTON_PRESSED_COL :
				 	  hover ? current->ops->theme->BUTTON_HOVER_COL :
				 	  current->ops->theme->BUTTON_COL);

	zui_draw_rect(current->ops->theme->FILL_BUTTON_BG, current->_x + current->button_offset_y, current->_y + current->button_offset_y, current->_w - current->button_offset_y * 2, ZUI_BUTTON_H());

	arm_g2_set_color(current->ops->theme->BUTTON_TEXT_COL);
	zui_draw_string(text, current->ops->theme->TEXT_OFFSET, 0, align, true);
	if (label != NULL) {
		arm_g2_set_color(current->ops->theme->LABEL_COL);
		zui_draw_string(label, current->ops->theme->TEXT_OFFSET, 0, align == ZUI_ALIGN_RIGHT ? ZUI_ALIGN_LEFT : ZUI_ALIGN_RIGHT, true);
	}

	/*
	if (icon != NULL) {
		arm_g2_set_color(0xffffffff);
		if (current->image_invert_y) {
			arm_g2_draw_scaled_sub_image(icon, sx, sy, sw, sh, _x + current->button_offset_y, _y - 1 + sh, sw, -sh);
		}
		else {
			arm_g2_draw_scaled_sub_image(icon, sx, sy, sw, sh, _x + current->button_offset_y, _y - 1, sw, sh);
		}
	}
	*/

	zui_end_element();
	return released;
}

void zui_split_text(char *lines, int align, int bg) {
	int count = zui_line_count(lines);
	for (int i = 0; i < count; ++i) zui_text(zui_extract_line(lines, i), align, bg);
}

int zui_text(char *text, int align, int bg) {
	if (zui_line_count(text) > 1) {
		zui_split_text(text, align, bg);
		return ZUI_STATE_IDLE;
	}
	float h = fmax(ZUI_ELEMENT_H(), arm_g2_font_height(current->ops->font->font_, current->font_size));
	if (!zui_is_visible(h)) {
		zui_end_element_of_size(h + ZUI_ELEMENT_OFFSET());
		return ZUI_STATE_IDLE;
	}
	bool started = zui_get_started(h);
	bool down = zui_get_pushed(h);
	bool released = zui_get_released(h);
	bool hover = zui_get_hover(h);
	if (bg != 0x0000000) {
		arm_g2_set_color(bg);
		arm_g2_fill_rect(current->_x + current->button_offset_y, current->_y + current->button_offset_y, current->_w - current->button_offset_y * 2, ZUI_BUTTON_H());
	}
	arm_g2_set_color(current->ops->theme->TEXT_COL);
	zui_draw_string(text, current->ops->theme->TEXT_OFFSET, 0, align, true);

	zui_end_element_of_size(h + ZUI_ELEMENT_OFFSET());
	return started ? ZUI_STATE_STARTED : released ? ZUI_STATE_RELEASED : down ? ZUI_STATE_DOWN : ZUI_STATE_IDLE;
}

bool zui_tab(zui_handle_t *handle, char *text, bool vertical, uint32_t color) {
	if (current->tab_count == 0) { // First tab
		current->tab_handle = handle;
		current->tab_vertical = vertical;
		current->_w -= current->tab_vertical ? ZUI_ELEMENT_OFFSET() + ZUI_ELEMENT_W() - 1 * ZUI_SCALE() : 0; // Shrink window area by width of vertical tabs
		if (vertical) {
			current->window_header_w += ZUI_ELEMENT_W();
		}
		else {
			current->window_header_h += ZUI_BUTTON_H() + current->button_offset_y + ZUI_ELEMENT_OFFSET();
		}
		current->restore_x = current->input_x; // Mouse in tab header, disable clicks for tab content
		current->restore_y = current->input_y;
		if (!vertical && zui_input_in_rect(current->_window_x, current->_window_y, current->_window_w, current->window_header_h)) {
			current->input_x = current->input_y = -1;
		}
		if (vertical) {
			current->_x += current->window_header_w + 6;
			current->_w -= 6;
		}
		else {
			current->_y += current->window_header_h + 3;
		}
	}
	assert(current->tab_count < 16);
	strcpy(current->tab_names[current->tab_count], text);
	current->tab_colors[current->tab_count] = color;
	current->tab_enabled[current->tab_count] = current->enabled;
	current->tab_count++;
	return handle->position == current->tab_count - 1;
}

bool zui_panel(zui_handle_t *handle, char *text, bool is_tree, bool filled, bool pack) {
	if (!zui_is_visible(ZUI_ELEMENT_H())) {
		zui_end_element();
		return handle->selected;
	}
	if (zui_get_released(ZUI_ELEMENT_H())) {
		handle->selected = !handle->selected;
		handle->changed = current->changed = true;
	}
	if (filled) {
		arm_g2_set_color(current->ops->theme->PANEL_BG_COL);
		zui_draw_rect(true, current->_x, current->_y, current->_w, ZUI_ELEMENT_H());
	}

	if (is_tree) {
		zui_draw_tree(handle->selected);
	}
	else {
		zui_draw_arrow(handle->selected);
	}

	arm_g2_set_color(current->ops->theme->LABEL_COL); // Title
	zui_draw_string(text, current->title_offset_x, 0, ZUI_ALIGN_LEFT, true);

	zui_end_element();
	if (pack && !handle->selected) current->_y -= ZUI_ELEMENT_OFFSET();

	return handle->selected;
}

static int image_width(void *image, bool is_rt) {
	if (is_rt) {
		return ((kinc_g4_render_target_t *)image)->width;
	}
	else {
		return ((kinc_g4_texture_t *)image)->tex_width;
	}
}

static int image_height(void *image, bool is_rt) {
	if (is_rt) {
		return ((kinc_g4_render_target_t *)image)->height;
	}
	else {
		return ((kinc_g4_texture_t *)image)->tex_height;
	}
}

static void draw_scaled_image(void *image, bool is_rt, float dx, float dy, float dw, float dh) {
	if (is_rt) {
		arm_g2_draw_scaled_render_target((kinc_g4_render_target_t *)image, dx, dy, dw, dh);
	}
	else {
		arm_g2_draw_scaled_image((kinc_g4_texture_t *)image, dx, dy, dw, dh);
	}
}

static void draw_scaled_sub_image(void *image, bool is_rt, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh) {
	if (is_rt) {
		arm_g2_draw_scaled_sub_render_target((kinc_g4_render_target_t *)image, sx, sy, sw, sh, dx, dy, dw, dh);
	}
	else {
		arm_g2_draw_scaled_sub_image((kinc_g4_texture_t *)image, sx, sy, sw, sh, dx, dy, dw, dh);
	}
}

int zui_sub_image(/*kinc_g4_texture_t kinc_g4_render_target_t*/ void *image, bool is_rt, uint32_t tint, int h, int sx, int sy, int sw, int sh) {
	float iw = (sw > 0 ? sw : image_width(image, is_rt)) * ZUI_SCALE();
	float ih = (sh > 0 ? sh : image_height(image, is_rt)) * ZUI_SCALE();
	float w = fmin(iw, current->_w);
	float x = current->_x;
	float scroll = current->current_window != NULL ? current->current_window->scroll_enabled : false;
	float r = current->current_ratio == -1 ? 1.0 : zui_get_ratio(current->ratios->buffer[current->current_ratio], 1);
	if (current->image_scroll_align) { // Account for scrollbar size
		w = fmin(iw, current->_w - current->button_offset_y * 2.0);
		x += current->button_offset_y;
		if (!scroll) {
			w -= ZUI_SCROLL_W() * r;
			x += ZUI_SCROLL_W() * r / 2.0;
		}
	}
	else if (scroll) w += ZUI_SCROLL_W() * r;

	// Image size
	float ratio = h == -1 ?
		w / iw :
		h / ih;
	if (h == -1) {
		h = ih * ratio;
	}
	else {
		w = iw * ratio;
	}

	if (!zui_is_visible(h)) {
		zui_end_element_of_size(h);
		return ZUI_STATE_IDLE;
	}
	bool started = zui_get_started(h);
	bool down = zui_get_pushed(h);
	bool released = zui_get_released(h);
	bool hover = zui_get_hover(h);

	// Limit input to image width
	// if (current->current_ratio == -1 && (started || down || released || hover)) {
	// 	if (current->input_x < current->_window_x + current->_x || current->input_x > current->_window_x + current->_x + w) {
	// 		started = down = released = hover = false;
	// 	}
	// }

	arm_g2_set_color(tint);
	if (!current->enabled) zui_fade_color(0.25);
	if (sw > 0) { // Source rect specified
		if (current->image_invert_y) {
			draw_scaled_sub_image(image, is_rt, sx, sy, sw, sh, x, current->_y + h, w, -h);
		}
		else {
			draw_scaled_sub_image(image, is_rt, sx, sy, sw, sh, x, current->_y, w, h);
		}
	}
	else {
		if (current->image_invert_y) {
			draw_scaled_image(image, is_rt, x, current->_y + h, w, -h);
		}
		else {
			draw_scaled_image(image, is_rt, x, current->_y, w, h);
		}
	}

	zui_end_element_of_size(h);
	return started ? ZUI_STATE_STARTED : released ? ZUI_STATE_RELEASED : down ? ZUI_STATE_DOWN : hover ? ZUI_STATE_HOVERED : ZUI_STATE_IDLE;
}

int zui_image(/*kinc_g4_texture_t kinc_g4_render_target_t*/ void *image, bool is_rt, uint32_t tint, int h) {
	return zui_sub_image(image, is_rt, tint, h, 0, 0, image_width(image, is_rt), image_height(image, is_rt));
}

char *zui_text_input(zui_handle_t *handle, char *label, int align, bool editable, bool live_update) {
	if (!zui_is_visible(ZUI_ELEMENT_H())) {
		zui_end_element();
		return handle->text;
	}

	bool hover = zui_get_hover(ZUI_ELEMENT_H());
	if (hover && zui_on_text_hover != NULL) zui_on_text_hover();
	arm_g2_set_color(hover ? current->ops->theme->ACCENT_HOVER_COL : current->ops->theme->ACCENT_COL); // Text bg
	zui_draw_rect(current->ops->theme->FILL_ACCENT_BG, current->_x + current->button_offset_y, current->_y + current->button_offset_y, current->_w - current->button_offset_y * 2, ZUI_BUTTON_H());

	bool released = zui_get_released(ZUI_ELEMENT_H());
	if (current->submit_text_handle == handle && released) { // Keep editing selected text
		current->is_typing = true;
		current->text_selected_handle = current->submit_text_handle;
		current->submit_text_handle = NULL;
		zui_set_cursor_to_input(align);
	}
	bool start_edit = released || current->tab_pressed;
	handle->changed = false;

	if (current->text_selected_handle != handle && start_edit) zui_start_text_edit(handle, align);
	if (current->text_selected_handle == handle) zui_update_text_edit(align, editable, live_update);
	if (current->submit_text_handle == handle) zui_submit_text_edit();

	if (label[0] != '\0') {
		arm_g2_set_color(current->ops->theme->LABEL_COL); // Label
		int label_align = align == ZUI_ALIGN_RIGHT ? ZUI_ALIGN_LEFT : ZUI_ALIGN_RIGHT;
		zui_draw_string(label, label_align == ZUI_ALIGN_LEFT ? current->ops->theme->TEXT_OFFSET : 0, 0, label_align, true);
	}

	arm_g2_set_color(current->ops->theme->TEXT_COL); // Text
	if (current->text_selected_handle != handle) {
		zui_draw_string(handle->text, current->ops->theme->TEXT_OFFSET, 0, align, true);
	}
	else {
		zui_draw_string(current->text_selected, current->ops->theme->TEXT_OFFSET, 0, align, false);
	}

	zui_end_element();
	return handle->text;
}

bool zui_check(zui_handle_t *handle, char *text, char *label) {
	if (!zui_is_visible(ZUI_ELEMENT_H())) {
		zui_end_element();
		return handle->selected;
	}
	if (zui_get_released(ZUI_ELEMENT_H())) {
		handle->selected = !handle->selected;
		handle->changed = current->changed = true;
	}
	else handle->changed = false;

	bool hover = zui_get_hover(ZUI_ELEMENT_H());
	zui_draw_check(handle->selected, hover); // Check

	arm_g2_set_color(current->ops->theme->TEXT_COL); // Text
	zui_draw_string(text, current->title_offset_x, 0, ZUI_ALIGN_LEFT, true);

	if (label[0] != '\0') {
		arm_g2_set_color(current->ops->theme->LABEL_COL);
		zui_draw_string(label, current->ops->theme->TEXT_OFFSET, 0, ZUI_ALIGN_RIGHT, true);
	}

	zui_end_element();

	return handle->selected;
}

bool zui_radio(zui_handle_t *handle, int position, char *text, char *label) {
	if (!zui_is_visible(ZUI_ELEMENT_H())) {
		zui_end_element();
		return handle->position == position;
	}
	if (position == 0) {
		handle->changed = false;
	}
	if (zui_get_released(ZUI_ELEMENT_H())) {
		handle->position = position;
		handle->changed = current->changed = true;
	}

	bool hover = zui_get_hover(ZUI_ELEMENT_H());
	zui_draw_radio(handle->position == position, hover); // Radio

	arm_g2_set_color(current->ops->theme->TEXT_COL); // Text
	zui_draw_string(text, current->title_offset_x, 0, ZUI_ALIGN_LEFT, true);

	if (label[0] != '\0') {
		arm_g2_set_color(current->ops->theme->LABEL_COL);
		zui_draw_string(label, current->ops->theme->TEXT_OFFSET, 0, ZUI_ALIGN_RIGHT, true);
	}

	zui_end_element();

	return handle->position == position;
}

int zui_combo(zui_handle_t *handle, char_ptr_array_t *texts, char *label, bool show_label, int align, bool search_bar) {
	if (!zui_is_visible(ZUI_ELEMENT_H())) {
		zui_end_element();
		return handle->position;
	}
	if (zui_get_released(ZUI_ELEMENT_H())) {
		if (current->combo_selected_handle == NULL) {
			current->input_enabled = false;
			current->combo_selected_handle = handle;
			current->combo_selected_window = current->current_window;
			current->combo_selected_align = align;
			current->combo_selected_texts = texts;
			current->combo_selected_label = (char *)label;
			current->combo_selected_x = current->_x + current->_window_x;
			current->combo_selected_y = current->_y + current->_window_y + ZUI_ELEMENT_H();
			current->combo_selected_w = current->_w;
			current->combo_search_bar = search_bar;
			for (int i = 0; i < texts->length; ++i) { // Adapt combo list width to combo item width
				int w = (int)arm_g2_string_width(current->ops->font->font_, current->font_size, texts->buffer[i]) + 10;
				if (current->combo_selected_w < w) current->combo_selected_w = w;
			}
			if (current->combo_selected_w > current->_w * 2.0) current->combo_selected_w = current->_w * 2.0;
			if (current->combo_selected_w > current->_w) current->combo_selected_w += ZUI_TEXT_OFFSET();
			current->combo_to_submit = handle->position;
			current->combo_initial_value = handle->position;
		}
	}
	if (handle == current->combo_selected_handle && (current->is_escape_down || current->input_released_r)) {
		handle->position = current->combo_initial_value;
		handle->changed = current->changed = true;
		current->submit_combo_handle = NULL;
	}
	else if (handle == current->submit_combo_handle) {
		handle->position = current->combo_to_submit;
		current->submit_combo_handle = NULL;
		handle->changed = current->changed = true;
	}
	else handle->changed = false;

	bool hover = zui_get_hover(ZUI_ELEMENT_H());
	if (hover) { // Bg
		arm_g2_set_color(current->ops->theme->ACCENT_HOVER_COL);
		zui_draw_rect(current->ops->theme->FILL_ACCENT_BG, current->_x + current->button_offset_y, current->_y + current->button_offset_y, current->_w - current->button_offset_y * 2, ZUI_BUTTON_H());
	}
	else {
		arm_g2_set_color(current->ops->theme->ACCENT_COL);
		zui_draw_rect(current->ops->theme->FILL_ACCENT_BG, current->_x + current->button_offset_y, current->_y + current->button_offset_y, current->_w - current->button_offset_y * 2, ZUI_BUTTON_H());
	}

	int x = current->_x + current->_w - current->arrow_offset_x - 8;
	int y = current->_y + current->arrow_offset_y + 3;

	// if (handle == current->combo_selected_handle) {
	//	// Flip arrow when combo is open
	//	arm_g2_fill_triangle(x, y, x + ZUI_ARROW_SIZE(), y, x + ZUI_ARROW_SIZE() / 2.0, y - ZUI_ARROW_SIZE() / 2.0);
	// }
	// else {
		arm_g2_fill_triangle(x, y, x + ZUI_ARROW_SIZE(), y, x + ZUI_ARROW_SIZE() / 2.0, y + ZUI_ARROW_SIZE() / 2.0);
	// }

	if (show_label && label[0] != '\0') {
		if (align == ZUI_ALIGN_LEFT) current->_x -= 15;
		arm_g2_set_color(current->ops->theme->LABEL_COL);
		zui_draw_string(label, current->ops->theme->TEXT_OFFSET, 0, align == ZUI_ALIGN_LEFT ? ZUI_ALIGN_RIGHT : ZUI_ALIGN_LEFT, true);
		if (align == ZUI_ALIGN_LEFT) current->_x += 15;
	}

	if (align == ZUI_ALIGN_RIGHT) current->_x -= 15;
	arm_g2_set_color(current->ops->theme->TEXT_COL); // Value
	if (handle->position < texts->length) {
		zui_draw_string(texts->buffer[handle->position], current->ops->theme->TEXT_OFFSET, 0, align, true);
	}
	if (align == ZUI_ALIGN_RIGHT) current->_x += 15;

	zui_end_element();
	return handle->position;
}

static void strip_trailing_zeros(char *str) {
	int len = strlen(str);
	while (str[--len] == '0') {
		str[len] = '\0';
	}
	if (str[len] == '.') {
		str[len] = '\0';
	}
}

float zui_slider(zui_handle_t *handle, char *text, float from, float to, bool filled, float precision, bool display_value, int align, bool text_edit) {
	if (!zui_is_visible(ZUI_ELEMENT_H())) {
		zui_end_element();
		return handle->value;
	}
	if (zui_get_started(ZUI_ELEMENT_H())) {
		current->scroll_handle = handle;
		current->is_scrolling = true;
		current->changed = handle->changed = true;
		if (zui_touch_tooltip) {
			current->slider_tooltip = true;
			current->slider_tooltip_x = current->_x + current->_window_x;
			current->slider_tooltip_y = current->_y + current->_window_y;
			current->slider_tooltip_w = current->_w;
		}
	}
	else handle->changed = false;

	#if !defined(KINC_ANDROID) && !defined(KINC_IOS)
	if (handle == current->scroll_handle && current->input_dx != 0) { // Scroll
	#else
	if (handle == current->scroll_handle) { // Scroll
	#endif
		float range = to - from;
		float slider_x = current->_x + current->_window_x + current->button_offset_y;
		float slider_w = current->_w - current->button_offset_y * 2;
		float step = range / slider_w;
		float value = from + (current->input_x - slider_x) * step;
		handle->value = round(value * precision) / precision;
		if (handle->value < from) handle->value = from; // Stay in bounds
		else if (handle->value > to) handle->value = to;
		handle->changed = current->changed = true;
	}

	bool hover = zui_get_hover(ZUI_ELEMENT_H());
	zui_draw_slider(handle->value, from, to, filled, hover); // Slider

	// Text edit
	bool start_edit = (zui_get_released(ZUI_ELEMENT_H()) || current->tab_pressed) && text_edit;
	if (start_edit) { // Mouse did not move
		sprintf(handle->text, "%.2f", handle->value);
		strip_trailing_zeros(handle->text);
		zui_start_text_edit(handle, ZUI_ALIGN_LEFT);
		handle->changed = current->changed = true;
	}
	int lalign = align == ZUI_ALIGN_LEFT ? ZUI_ALIGN_RIGHT : ZUI_ALIGN_LEFT;
	if (current->text_selected_handle == handle) {
		zui_update_text_edit(lalign, true, false);
	}
	if (current->submit_text_handle == handle) {
		zui_submit_text_edit();
		#ifdef WITH_EVAL
		handle->value = krom_js_eval(handle->text);
		#else
		handle->value = atof(handle->text);
		#endif
		handle->changed = current->changed = true;
	}

	arm_g2_set_color(current->ops->theme->LABEL_COL); // Text
	zui_draw_string(text, current->ops->theme->TEXT_OFFSET, 0, align, true);

	if (display_value) {
		arm_g2_set_color(current->ops->theme->TEXT_COL); // Value
		if (current->text_selected_handle != handle) {
			sprintf(temp, "%.2f", round(handle->value * precision) / precision);
			strip_trailing_zeros(temp);
			zui_draw_string(temp, current->ops->theme->TEXT_OFFSET, 0, lalign, true);
		}
		else {
			zui_draw_string(current->text_selected, current->ops->theme->TEXT_OFFSET, 0, lalign, true);
		}
	}

	zui_end_element();
	return handle->value;
}

void zui_separator(int h, bool fill) {
	if (!zui_is_visible(ZUI_ELEMENT_H())) {
		current->_y += h * ZUI_SCALE();
		return;
	}
	if (fill) {
		arm_g2_set_color(current->ops->theme->SEPARATOR_COL);
		arm_g2_fill_rect(current->_x, current->_y, current->_w, h * ZUI_SCALE());
	}
	current->_y += h * ZUI_SCALE();
}

void zui_tooltip(char *text) {
	assert(strlen(text) < 512);
	strcpy(current->tooltip_text, text);
	current->tooltip_y = current->_y + current->_window_y;
}

void zui_tooltip_image(kinc_g4_texture_t *image, int max_width) {
	current->tooltip_img = image;
	current->tooltip_img_max_width = max_width;
	current->tooltip_invert_y = current->image_invert_y;
	current->tooltip_y = current->_y + current->_window_y;
}

void zui_tooltip_render_target(kinc_g4_render_target_t *image, int max_width) {
	current->tooltip_rt = image;
	current->tooltip_img_max_width = max_width;
	current->tooltip_invert_y = current->image_invert_y;
	current->tooltip_y = current->_y + current->_window_y;
}

void zui_end(bool last) {
	if (!current->window_ended) zui_end_window(true);
	zui_draw_combo(true); // Handle active combo
	zui_draw_tooltip(true);
	current->tab_pressed_handle = NULL;
	if (last) zui_end_input();
}

void zui_set_input_position(zui_t *ui, int x, int y) {
	ui->input_dx += x - ui->input_x;
	ui->input_dy += y - ui->input_y;
	ui->input_x = x;
	ui->input_y = y;
}

// Useful for drag and drop operations
char *zui_hovered_tab_name() {
	return zui_input_in_rect(current->hovered_tab_x, current->hovered_tab_y, current->hovered_tab_w, current->hovered_tab_h) ? current->hovered_tab_name : "";
}

void zui_mouse_down(zui_t *ui, int button, int x, int y) {
	if (ui->pen_in_use) return;
	if (button == 0) { ui->input_started = ui->input_down = true; }
	else { ui->input_started_r = ui->input_down_r = true; }
	ui->input_started_time = kinc_time();
	#if defined(KINC_ANDROID) || defined(KINC_IOS)
	zui_set_input_position(ui, x, y);
	#endif
	ui->input_started_x = x;
	ui->input_started_y = y;
}

void zui_mouse_move(zui_t *ui, int x, int y, int movement_x, int movement_y) {
	#if !defined(KINC_ANDROID) && !defined(KINC_IOS)
	zui_set_input_position(ui, x, y);
	#endif
}

void zui_mouse_up(zui_t *ui, int button, int x, int y) {
	if (ui->pen_in_use) return;

	if (ui->touch_hold_activated) {
		ui->touch_hold_activated = false;
		return;
	}

	if (ui->is_scrolling) { // Prevent action when scrolling is active
		ui->is_scrolling = false;
		ui->scroll_handle = NULL;
		ui->slider_tooltip = false;
		if (x == ui->input_started_x && y == ui->input_started_y) { // Mouse not moved
			if (button == 0) ui->input_released = true;
			else ui->input_released_r = true;
		}
	}
	else {
		if (button == 0) ui->input_released = true;
		else ui->input_released_r = true;
	}

	if (button == 0) ui->input_down = false;
	else ui->input_down_r = false;
	#if defined(KINC_ANDROID) || defined(KINC_IOS)
	zui_set_input_position(ui, x, y);
	#endif
	zui_deselect_text(ui);
}

void zui_mouse_wheel(zui_t *ui, int delta) {
	ui->input_wheel_delta = delta;
}

void zui_pen_down(zui_t *ui, int x, int y, float pressure) {
	#if defined(KINC_ANDROID) || defined(KINC_IOS)
	return;
	#endif

	zui_mouse_down(ui, 0, x, y);
}

void zui_pen_up(zui_t *ui, int x, int y, float pressure) {
	#if defined(KINC_ANDROID) || defined(KINC_IOS)
	return;
	#endif

	if (ui->input_started) { ui->input_started = false; ui->pen_in_use = true; return; }
	zui_mouse_up(ui, 0, x, y);
	ui->pen_in_use = true; // On pen release, additional mouse down & up events are fired at once - filter those out
}

void zui_pen_move(zui_t *ui, int x, int y, float pressure) {
	#if defined(KINC_IOS)
	// Listen to pen hover if no other input is active
	if (pressure == 0.0) {
		if (!ui->input_down && !ui->input_down_r) {
			zui_set_input_position(ui, x, y);
		}
		return;
	}
	#endif

	#if defined(KINC_ANDROID) || defined(KINC_IOS)
	return;
	#endif

	zui_mouse_move(ui, x, y, 0, 0);
}

void zui_key_down(zui_t *ui, int key_code) {
	ui->key_code = key_code;
	ui->is_key_pressed = true;
	ui->is_key_down = true;
	zui_key_repeat_time = kinc_time() + 0.4;
	switch (key_code) {
		case KINC_KEY_SHIFT: ui->is_shift_down = true; break;
		case KINC_KEY_CONTROL: ui->is_ctrl_down = true; break;
		#ifdef KINC_DARWIN
		case KINC_KEY_META: ui->is_ctrl_down = true; break;
		#endif
		case KINC_KEY_ALT: ui->is_alt_down = true; break;
		case KINC_KEY_BACKSPACE: ui->is_backspace_down = true; break;
		case KINC_KEY_DELETE: ui->is_delete_down = true; break;
		case KINC_KEY_ESCAPE: ui->is_escape_down = true; break;
		case KINC_KEY_RETURN: ui->is_return_down = true; break;
		case KINC_KEY_TAB: ui->is_tab_down = true; break;
		case KINC_KEY_A: ui->is_a_down = true; break;
		case KINC_KEY_SPACE: ui->key_char = ' '; break;
		#ifdef ZUI_ANDROID_RMB // Detect right mouse button on Android..
		case KINC_KEY_BACK: if (!ui->input_down_r) zui_mouse_down(ui, 1, ui->input_x, ui->input_y); break;
		#endif
	}
}

void zui_key_up(zui_t *ui, int key_code) {
	ui->is_key_down = false;
	switch (key_code) {
		case KINC_KEY_SHIFT: ui->is_shift_down = false; break;
		case KINC_KEY_CONTROL: ui->is_ctrl_down = false; break;
		#ifdef KINC_DARWIN
		case KINC_KEY_META: ui->is_ctrl_down = false; break;
		#endif
		case KINC_KEY_ALT: ui->is_alt_down = false; break;
		case KINC_KEY_BACKSPACE: ui->is_backspace_down = false; break;
		case KINC_KEY_DELETE: ui->is_delete_down = false; break;
		case KINC_KEY_ESCAPE: ui->is_escape_down = false; break;
		case KINC_KEY_RETURN: ui->is_return_down = false; break;
		case KINC_KEY_TAB: ui->is_tab_down = false; break;
		case KINC_KEY_A: ui->is_a_down = false; break;
		#ifdef ZUI_ANDROID_RMB
		case KINC_KEY_BACK: zui_mouse_down(ui, 1, ui->input_x, ui->input_y); break;
		#endif
	}
}

void zui_key_press(zui_t *ui, unsigned key_char) {
	ui->key_char = key_char;
	ui->is_key_pressed = true;
}

#if defined(KINC_ANDROID) || defined(KINC_IOS)
static float zui_pinch_distance = 0.0;
static float zui_pinch_total = 0.0;
static bool zui_pinch_started = false;

void zui_touch_down(zui_t *ui, int index, int x, int y) {
	// Reset movement delta on touch start
	if (index == 0) {
		ui->input_dx = 0;
		ui->input_dy = 0;
		ui->input_x = x;
		ui->input_y = y;
	}
	// Two fingers down - right mouse button
	else if (index == 1) {
		ui->input_down = false;
		zui_mouse_down(ui, 1, ui->input_x, ui->input_y);
		zui_pinch_started = true;
		zui_pinch_total = 0.0;
		zui_pinch_distance = 0.0;
	}
	// Three fingers down - middle mouse button
	else if (index == 2) {
		ui->input_down_r = false;
		zui_mouse_down(ui, 2, ui->input_x, ui->input_y);
	}
}

void zui_touch_up(zui_t *ui, int index, int x, int y) {
	if (index == 1) zui_mouse_up(ui, 1, ui->input_x, ui->input_y);
}

void zui_touch_move(zui_t *ui, int index, int x, int y) {
	if (index == 0) zui_set_input_position(ui, x, y);

	// Pinch to zoom - mouse wheel
	if (index == 1) {
		float last_distance = zui_pinch_distance;
		float dx = ui->input_x - x;
		float dy = ui->input_y - y;
		zui_pinch_distance = sqrt(dx * dx + dy * dy);
		zui_pinch_total += last_distance != 0 ? last_distance - zui_pinch_distance : 0;
		if (!zui_pinch_started) {
			ui->input_wheel_delta = zui_pinch_total / 50;
			if (ui->input_wheel_delta != 0) {
				zui_pinch_total = 0.0;
			}
		}
		zui_pinch_started = false;
	}
}
#endif

char *zui_copy() {
	zui_is_copy = true;
	return &zui_text_to_copy[0];
}

char *zui_cut() {
	zui_is_cut = true;
	return zui_copy();
}

void zui_paste(char *s) {
	zui_is_paste = true;
	strcpy(zui_text_to_paste, s);
}

void zui_theme_default(zui_theme_t *t) {
	t->WINDOW_BG_COL = 0xff292929;
	t->WINDOW_TINT_COL = 0xffffffff;
	t->ACCENT_COL = 0xff383838;
	t->ACCENT_HOVER_COL = 0xff434343;
	t->ACCENT_SELECT_COL = 0xff606060;
	t->BUTTON_COL = 0xff383838;
	t->BUTTON_TEXT_COL = 0xffe8e8e8;
	t->BUTTON_HOVER_COL = 0xff434343;
	t->BUTTON_PRESSED_COL = 0xff222222;
	t->TEXT_COL = 0xffe8e8e8;
	t->LABEL_COL = 0xffc8c8c8;
	t->SEPARATOR_COL = 0xff202020;
	t->HIGHLIGHT_COL = 0xff205d9c;
	t->CONTEXT_COL = 0xff222222;
	t->PANEL_BG_COL = 0xff3b3b3b;
	t->FONT_SIZE = 13;
	t->ELEMENT_W = 100;
	t->ELEMENT_H = 24;
	t->ELEMENT_OFFSET = 4;
	t->ARROW_SIZE = 5;
	t->BUTTON_H = 22;
	t->CHECK_SIZE = 16;
	t->CHECK_SELECT_SIZE = 12;
	t->SCROLL_W = 9;
	t->SCROLL_MINI_W = 3;
	t->TEXT_OFFSET = 8;
	t->TAB_W = 6;
	t->FILL_WINDOW_BG = true;
	t->FILL_BUTTON_BG = true;
	t->FILL_ACCENT_BG = false;
	t->LINK_STYLE = ZUI_LINK_STYLE_LINE;
	t->FULL_TABS = false;
	#if defined(KINC_ANDROID) || defined(KINC_IOS)
	t->ROUND_CORNERS = true;
	#else
	t->ROUND_CORNERS = false;
	#endif
}