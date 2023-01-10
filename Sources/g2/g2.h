
// This implementation was adapted from tizilogic's krink and robdangerous' kha, licensed under zlib/libpng license
// https://github.com/tizilogic/krink/blob/main/Sources/krink/graphics2/graphics.h
// https://github.com/Kode/Kha/tree/main/Sources/kha/graphics2

#pragma once

#include <kinc/math/matrix.h>
#include <kinc/graphics4/texture.h>
#include <kinc/graphics4/rendertarget.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct g2_font_image g2_font_image_t;

typedef struct g2_font {
	unsigned char *blob;
	g2_font_image_t *images;
	size_t m_capacity;
	size_t m_images_len;
	int offset;
} g2_font_t;

void g2_init(void);
void g2_begin(void);
void g2_draw_scaled_sub_image(kinc_g4_texture_t *tex, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh);
void g2_draw_scaled_image(kinc_g4_texture_t *tex, float dx, float dy, float dw, float dh);
void g2_draw_sub_image(kinc_g4_texture_t *tex, float sx, float sy, float sw, float sh, float x, float y);
void g2_draw_image(kinc_g4_texture_t *tex, float x, float y);
void g2_draw_scaled_sub_render_target(kinc_g4_render_target_t *rt, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh);
void g2_draw_scaled_render_target(kinc_g4_render_target_t *rt, float dx, float dy, float dw, float dh);
void g2_draw_sub_render_target(kinc_g4_render_target_t *rt, float sx, float sy, float sw, float sh, float x, float y);
void g2_draw_render_target(kinc_g4_render_target_t *rt, float x, float y);
void g2_fill_triangle(float x1, float y1, float x2, float y2, float x3, float y3);
void g2_fill_rect(float x, float y, float width, float height);
void g2_draw_rect(float x, float y, float width, float height, float strength);
void g2_draw_line(float x1, float y1, float x2, float y2, float strength);
void g2_draw_string(const char *text, float x, float y);
void g2_end(void);
void g2_set_color(uint32_t color);
uint32_t g2_get_color();
void g2_set_font(g2_font_t *font, int size);
void g2_font_init(g2_font_t *font, const char *font_path);
bool g2_font_has_glyph(int glyph);
void g2_font_add_glyphs(int *glyphs, int count);
float g2_font_height(g2_font_t *font, int font_size);
float g2_sub_string_width(g2_font_t *font, int font_size, const char *text, int start, int length);
float g2_string_width(g2_font_t *font, int font_size, const char *text);
void g2_set_bilinear_filter(bool bilinear);
void g2_restore_render_target(void);
void g2_set_render_target(kinc_g4_render_target_t *target);