
function g4_shader_create(buffer: ArrayBuffer, type: shader_type_t): shader_t {
	let raw: shader_t = {};
	if (buffer != null) {
		raw.shader_ = krom_g4_create_shader(buffer, type);
	}
	return raw;
}

function g4_shader_from_source(source: string, type: shader_type_t): shader_t {
	let shader: shader_t = g4_shader_create(null, 0);
	if (type == shader_type_t.VERTEX) {
		shader.shader_ = krom_g4_create_vertex_shader_from_source(source);
	}
	else if (type == shader_type_t.FRAGMENT) {
		shader.shader_ = krom_g4_create_fragment_shader_from_source(source);
	}
	return shader;
}

function g4_shader_delete(raw: shader_t) {
	krom_g4_delete_shader(raw.shader_);
}

function g4_pipeline_create(): pipeline_t {
	let raw: pipeline_t = {};
	raw.cull_mode = cull_mode_t.NONE;
	raw.depth_write = false;
	raw.depth_mode = compare_mode_t.ALWAYS;

	raw.blend_source = blend_factor_t.BLEND_ONE;
	raw.blend_dest = blend_factor_t.BLEND_ZERO;
	raw.alpha_blend_source = blend_factor_t.BLEND_ONE;
	raw.alpha_blend_dest = blend_factor_t.BLEND_ZERO;

	raw.color_write_masks_red = [];
	raw.color_write_masks_green = [];
	raw.color_write_masks_blue = [];
	raw.color_write_masks_alpha = [];
	for (let i: i32 = 0; i < 8; ++i) {
		raw.color_write_masks_red.push(true);
	}
	for (let i: i32 = 0; i < 8; ++i) {
		raw.color_write_masks_green.push(true);
	}
	for (let i: i32 = 0; i < 8; ++i) {
		raw.color_write_masks_blue.push(true);
	}
	for (let i: i32 = 0; i < 8; ++i) {
		raw.color_write_masks_alpha.push(true);
	}

	raw.color_attachment_count = 1;
	raw.color_attachments = [];
	for (let i: i32 = 0; i < 8; ++i) {
		raw.color_attachments.push(tex_format_t.RGBA32);
	}
	raw.depth_attachment = depth_format_t.NO_DEPTH;

	raw.pipeline_ = krom_g4_create_pipeline();
	return raw;
}

function g4_pipeline_get_depth_buffer_bits(format: depth_format_t): i32 {
	if (format == depth_format_t.NO_DEPTH) {
		return 0;
	}
	if (format == depth_format_t.DEPTH24) {
		return 24;
	}
	if (format == depth_format_t.DEPTH16) {
		return 16;
	}
	return 0;
}

function g4_pipeline_delete(raw: pipeline_t) {
	krom_g4_delete_pipeline(raw.pipeline_);
}

function g4_pipeline_compile(raw: pipeline_t) {
	let structure0: vertex_struct_t = raw.input_layout.length > 0 ? raw.input_layout[0] : null;
	let structure1: vertex_struct_t = raw.input_layout.length > 1 ? raw.input_layout[1] : null;
	let structure2: vertex_struct_t = raw.input_layout.length > 2 ? raw.input_layout[2] : null;
	let structure3: vertex_struct_t = raw.input_layout.length > 3 ? raw.input_layout[3] : null;
	let gs: any = raw.geometry_shader != null ? raw.geometry_shader.shader_ : null;
	let color_attachments: i32[] = [];
	for (let i = 0; i < 8; ++i) {
		color_attachments.push(raw.color_attachments[i]);
	}
	krom_g4_compile_pipeline(raw.pipeline_, structure0, structure1, structure2, structure3, raw.input_layout.length, raw.vertex_shader.shader_, raw.fragment_shader.shader_, gs, {
		cull_mode: raw.cull_mode,
		depth_write: raw.depth_write,
		depth_mode: raw.depth_mode,
		blend_source: raw.blend_source,
		blend_dest: raw.blend_dest,
		alpha_blend_source: raw.alpha_blend_source,
		alpha_blend_dest: raw.alpha_blend_dest,
		color_write_masks_red: raw.color_write_masks_red,
		color_write_masks_green: raw.color_write_masks_green,
		color_write_masks_blue: raw.color_write_masks_blue,
		color_write_masks_alpha: raw.color_write_masks_alpha,
		color_attachment_count: raw.color_attachment_count,
		color_attachments: raw.color_attachments,
		depth_attachment_bits: g4_pipeline_get_depth_buffer_bits(raw.depth_attachment)
	});
}

function g4_pipeline_set(raw: pipeline_t) {
	krom_g4_set_pipeline(raw.pipeline_);
}

function g4_pipeline_get_const_loc(raw: pipeline_t, name: string): kinc_const_loc_t {
	return krom_g4_get_constant_location(raw.pipeline_, name);
}

function g4_pipeline_get_tex_unit(raw: pipeline_t, name: string): kinc_tex_unit_t {
	return krom_g4_get_texture_unit(raw.pipeline_, name);
}

function g4_vertex_buffer_create(vertex_count: i32, structure: vertex_struct_t, usage: usage_t, inst_data_step_rate: i32 = 0): vertex_buffer_t {
	let raw: vertex_buffer_t = {};
	raw.vertex_count = vertex_count;
	raw.buffer_ = krom_g4_create_vertex_buffer(vertex_count, structure.elements, usage, inst_data_step_rate);
	return raw;
}

function g4_vertex_buffer_delete(raw: vertex_buffer_t) {
	krom_g4_delete_vertex_buffer(raw.buffer_);
}

function g4_vertex_buffer_lock(raw: vertex_buffer_t): DataView {
	return new DataView(krom_g4_lock_vertex_buffer(raw.buffer_));
}

function g4_vertex_buffer_unlock(raw: vertex_buffer_t) {
	krom_g4_unlock_vertex_buffer(raw.buffer_);
}

function g4_vertex_buffer_set(raw: vertex_buffer_t) {
	krom_g4_set_vertex_buffer(raw.buffer_);
}

function g4_vertex_struct_create(): vertex_struct_t {
	let raw: vertex_struct_t = {};
	raw.elements = [];
	raw.instanced = false;
	return raw;
}

function g4_vertex_struct_add(raw: vertex_struct_t, name: string, data: vertex_data_t) {
	raw.elements.push({ name: name, data: data });
}

function g4_vertex_struct_byte_size(raw: vertex_struct_t): i32 {
	let byte_size = 0;
	for (let i: i32 = 0; i < raw.elements.length; ++i) {
		byte_size += g4_vertex_struct_data_byte_size(raw.elements[i].data);
	}
	return byte_size;
}

function g4_vertex_struct_data_byte_size(data: vertex_data_t): i32 {
	if (data == vertex_data_t.F32_1X) {
		return 1 * 4;
	}
	if (data == vertex_data_t.F32_2X) {
		return 2 * 4;
	}
	if (data == vertex_data_t.F32_3X) {
		return 3 * 4;
	}
	if (data == vertex_data_t.F32_4X) {
		return 4 * 4;
	}
	if (data == vertex_data_t.U8_4X_NORM) {
		return 4 * 1;
	}
	if (data == vertex_data_t.I16_2X_NORM) {
		return 2 * 2;
	}
	if (data == vertex_data_t.I16_4X_NORM) {
		return 4 * 2;
	}
	return 0;
}

function g4_index_buffer_create(index_count: i32): index_buffer_t {
	let raw: index_buffer_t = {};
	raw.buffer_ = krom_g4_create_index_buffer(index_count);
	return raw;
}

function g4_index_buffer_delete(raw: index_buffer_t) {
	krom_g4_delete_index_buffer(raw.buffer_);
}

function g4_index_buffer_lock(raw: index_buffer_t): Uint32Array {
	return krom_g4_lock_index_buffer(raw.buffer_);
}

function g4_index_buffer_unlock(raw: index_buffer_t) {
	krom_g4_unlock_index_buffer(raw.buffer_);
}

function g4_index_buffer_set(raw: index_buffer_t) {
	krom_g4_set_index_buffer(raw.buffer_);
}

function g4_begin(render_target: image_t, additional_targets: image_t[] = null) {
	krom_g4_begin(render_target, additional_targets);
}

function g4_end() {
	krom_g4_end();
}

function g4_clear(color: Color = null, depth: f32 = null) {
	let flags: i32 = 0;
	if (color != null) {
		flags |= 1;
	}
	if (depth != null) {
		flags |= 2;
	}
	krom_g4_clear(flags, color == null ? 0 : color, depth);
}

function g4_viewport(x: i32, y: i32, width: i32, height: i32) {
	krom_g4_viewport(x, y, width, height);
}

function g4_set_vertex_buffer(vb: vertex_buffer_t) {
	g4_vertex_buffer_set(vb);
}

function g4_set_vertex_buffers(vbs: vertex_buffer_t[]) {
	krom_g4_set_vertex_buffers(vbs);
}

function g4_set_index_buffer(ib: index_buffer_t) {
	g4_index_buffer_set(ib);
}

function g4_set_tex(unit: kinc_tex_unit_t, tex: image_t) {
	if (tex == null) {
		return;
	}
	tex.texture_ != null ? krom_g4_set_texture(unit, tex.texture_) : krom_g4_set_render_target(unit, tex.render_target_);
}

function g4_set_tex_depth(unit: kinc_tex_unit_t, tex: image_t) {
	if (tex == null) {
		return;
	}
	krom_g4_set_texture_depth(unit, tex.render_target_);
}

function g4_set_image_tex(unit: kinc_tex_unit_t, tex: image_t) {
	if (tex == null) {
		return;
	}
	krom_g4_set_image_texture(unit, tex.texture_);
}

function g4_set_tex_params(tex_unit: kinc_tex_unit_t, u_addressing: tex_addressing_t, v_addressing: tex_addressing_t, minification_filter: tex_filter_t, magnification_filter: tex_filter_t, mipmap_filter: mip_map_filter_t) {
	krom_g4_set_texture_parameters(tex_unit, u_addressing, v_addressing, minification_filter, magnification_filter, mipmap_filter);
}

function g4_set_tex_3d_params(tex_unit: kinc_tex_unit_t, u_addressing: tex_addressing_t, v_addressing: tex_addressing_t, w_addressing: tex_addressing_t, minification_filter: tex_filter_t, magnification_filter: tex_filter_t, mipmap_filter: mip_map_filter_t) {
	krom_g4_set_texture3d_parameters(tex_unit, u_addressing, v_addressing, w_addressing, minification_filter, magnification_filter, mipmap_filter);
}

function g4_set_pipeline(pipe: pipeline_t) {
	g4_pipeline_set(pipe);
}

function g4_set_bool(loc: kinc_const_loc_t, value: bool) {
	krom_g4_set_bool(loc, value);
}

function g4_set_int(loc: kinc_const_loc_t, value: i32) {
	krom_g4_set_int(loc, value);
}

function g4_set_float(loc: kinc_const_loc_t, value: f32) {
	krom_g4_set_float(loc, value);
}

function g4_set_float2(loc: kinc_const_loc_t, value1: f32, value2: f32) {
	krom_g4_set_float2(loc, value1, value2);
}

function g4_set_float3(loc: kinc_const_loc_t, value1: f32, value2: f32, value3: f32) {
	krom_g4_set_float3(loc, value1, value2, value3);
}

function g4_set_float4(loc: kinc_const_loc_t, value1: f32, value2: f32, value3: f32, value4: f32) {
	krom_g4_set_float4(loc, value1, value2, value3, value4);
}

function g4_set_floats(loc: kinc_const_loc_t, values: Float32Array) {
	krom_g4_set_floats(loc, values.buffer);
}

function g4_set_vec2(loc: kinc_const_loc_t, v: vec2_t) {
	krom_g4_set_float2(loc, v.x, v.y);
}

function g4_set_vec3(loc: kinc_const_loc_t, v: vec3_t) {
	krom_g4_set_float3(loc, v.x, v.y, v.z);
}

function g4_set_vec4(loc: kinc_const_loc_t, v: vec4_t) {
	krom_g4_set_float4(loc, v.x, v.y, v.z, v.w);
}

function g4_set_mat(loc: kinc_const_loc_t, mat: mat4_t) {
	krom_g4_set_matrix4(loc, mat.m.buffer);
}

function g4_set_mat3(loc: kinc_const_loc_t, mat: mat3_t) {
	krom_g4_set_matrix3(loc, mat.m.buffer);
}

function g4_draw(start: i32 = 0, count: i32 = -1) {
	krom_g4_draw_indexed_vertices(start, count);
}

function g4_draw_inst(inst_count: i32, start: i32 = 0, count: i32 = -1) {
	krom_g4_draw_indexed_vertices_instanced(inst_count, start, count);
}

function g4_scissor(x: i32, y: i32, width: i32, height: i32) {
	krom_g4_scissor(x, y, width, height);
}

function g4_disable_scissor() {
	krom_g4_disable_scissor();
}

function _image_create(tex: any): image_t {
	let raw: image_t = {};
	raw.texture_ = tex;
	return raw;
}

function _image_set_size(image: image_t, tex: any) {
	image.width = tex.width;
	image.height = tex.height;
	image.depth = tex.depth;
}

function image_get_depth_buffer_bits(format: depth_format_t): i32 {
	if (format == depth_format_t.NO_DEPTH) {
		return -1;
	}
	if (format == depth_format_t.DEPTH24) {
		return 24;
	}
	if (format == depth_format_t.DEPTH16) {
		return 16;
	}
	return 0;
}

function image_get_tex_format(format: tex_format_t): i32 {
	if (format == tex_format_t.RGBA32) {
		return 0;
	}
	if (format == tex_format_t.RGBA128) {
		return 3;
	}
	if (format == tex_format_t.RGBA64) {
		return 4;
	}
	if (format == tex_format_t.R32) {
		return 5;
	}
	if (format == tex_format_t.R16) {
		return 7;
	}
	return 1; // R8
}

function image_from_texture(tex: any): image_t {
	let image: image_t = _image_create(tex);
	_image_set_size(image, tex);
	return image;
}

function image_from_bytes(buffer: ArrayBuffer, width: i32, height: i32, format: tex_format_t = null, usage: usage_t = null): image_t {
	if (format == null) {
		format = tex_format_t.RGBA32;
	}
	let readable: bool = true;
	let image: image_t = _image_create(null);
	image.format = format;
	image.texture_ = krom_g4_create_texture_from_bytes(buffer, width, height, image_get_tex_format(format), readable);
	_image_set_size(image, image.texture_);
	return image;
}

function image_from_bytes_3d(buffer: ArrayBuffer, width: i32, height: i32, depth: i32, format: tex_format_t = null, usage: usage_t = null): image_t {
	if (format == null) {
		format = tex_format_t.RGBA32;
	}
	let readable: bool = true;
	let image: image_t = _image_create(null);
	image.format = format;
	image.texture_ = krom_g4_create_texture_from_bytes3d(buffer, width, height, depth, image_get_tex_format(format), readable);
	_image_set_size(image, image.texture_);
	return image;
}

function image_from_encoded_bytes(buffer: ArrayBuffer, format: string, done: (img: image_t)=>void, readable: bool = false) {
	let image: image_t = _image_create(null);
	image.texture_ = krom_g4_create_texture_from_encoded_bytes(buffer, format, readable);
	_image_set_size(image, image.texture_);
	done(image);
}

function image_create(width: i32, height: i32, format: tex_format_t = null, usage: usage_t = null): image_t {
	if (format == null) {
		format = tex_format_t.RGBA32;
	}
	let image: image_t = _image_create(null);
	image.format = format;
	image.texture_ = krom_g4_create_texture(width, height, image_get_tex_format(format));
	_image_set_size(image, image.texture_);
	return image;
}

function image_create_3d(width: i32, height: i32, depth: i32, format: tex_format_t = null, usage: usage_t = null): image_t {
	if (format == null) {
		format = tex_format_t.RGBA32;
	}
	let image: image_t = _image_create(null);
	image.format = format;
	image.texture_ = krom_g4_create_texture3d(width, height, depth, image_get_tex_format(format));
	_image_set_size(image, image.texture_);
	return image;
}

function image_create_render_target(width: i32, height: i32, format: tex_format_t = null, depth_format: depth_format_t = depth_format_t.NO_DEPTH): image_t {
	if (format == null) {
		format = tex_format_t.RGBA32;
	}
	let image: image_t = _image_create(null);
	image.format = format;
	image.render_target_ = krom_g4_create_render_target(width, height, format, image_get_depth_buffer_bits(depth_format), 0);
	_image_set_size(image, image.render_target_);
	return image;
}

function image_render_targets_inv_y(): bool {
	return krom_g4_render_targets_inverted_y();
}

function image_format_byte_size(format: tex_format_t): i32 {
	if (format == tex_format_t.RGBA32) {
		return 4;
	}
	if (format == tex_format_t.R8) {
		return 1;
	}
	if (format == tex_format_t.RGBA128) {
		return 16;
	}
	if (format == tex_format_t.DEPTH16) {
		return 2;
	}
	if (format == tex_format_t.RGBA64) {
		return 8;
	}
	if (format == tex_format_t.R32) {
		return 4;
	}
	if (format == tex_format_t.R16) {
		return 2;
	}
	return 4;
}

function image_unload(raw: image_t) {
	krom_unload_image(raw);
	raw.texture_ = null;
	raw.render_target_ = null;
}

function image_lock(raw: image_t, level: i32 = 0): ArrayBuffer {
	return krom_g4_lock_texture(raw.texture_, level);
}

function image_unlock(raw: image_t) {
	krom_g4_unlock_texture(raw.texture_);
}

function image_get_pixels(raw: image_t): ArrayBuffer {
	if (raw.render_target_ != null) {
		// Minimum size of 32x32 required after https://github.com/Kode/Kinc/commit/3797ebce5f6d7d360db3331eba28a17d1be87833
		let pixels_w: i32 = raw.width < 32 ? 32 : raw.width;
		let pixels_h: i32 = raw.height < 32 ? 32 : raw.height;
		if (raw.pixels == null) {
			raw.pixels = new ArrayBuffer(image_format_byte_size(raw.format) * pixels_w * pixels_h);
		}
		krom_g4_get_render_target_pixels(raw.render_target_, raw.pixels);
		return raw.pixels;
	}
	else {
		return krom_g4_get_texture_pixels(raw.texture_);
	}
}

function image_gen_mipmaps(raw: image_t, levels: i32) {
	raw.texture_ == null ? krom_g4_generate_render_target_mipmaps(raw.render_target_, levels) : krom_g4_generate_texture_mipmaps(raw.texture_, levels);
}

function image_set_mipmaps(raw: image_t, mipmaps: image_t[]) {
	krom_g4_set_mipmaps(raw.texture_, mipmaps);
}

function image_set_depth_from(raw: image_t, image: image_t) {
	krom_g4_set_depth_from(raw.render_target_, image.render_target_);
}

function image_clear(raw: image_t, x: i32, y: i32, z: i32, width: i32, height: i32, depth: i32, color: Color) {
	krom_g4_clear_texture(raw.texture_, x, y, z, width, height, depth, color);
}

type image_t = {
	texture_?: any;
	render_target_?: any;
	format?: tex_format_t;
	readable?: bool;
	pixels?: ArrayBuffer;
	width?: i32;
	height?: i32;
	depth?: i32;
};

type pipeline_t = {
	pipeline_?: any;
	input_layout?: vertex_struct_t[];
	vertex_shader?: shader_t;
	fragment_shader?: shader_t;
	geometry_shader?: shader_t;
	cull_mode?: cull_mode_t;
	depth_write?: bool;
	depth_mode?: compare_mode_t;
	blend_source?: blend_factor_t;
	blend_dest?: blend_factor_t;
	alpha_blend_source?: blend_factor_t;
	alpha_blend_dest?: blend_factor_t;
	color_write_masks_red?: bool[];
	color_write_masks_green?: bool[];
	color_write_masks_blue?: bool[];
	color_write_masks_alpha?: bool[];
	color_attachment_count?: i32;
	color_attachments?: tex_format_t[];
	depth_attachment?: depth_format_t;
};

type shader_t = {
	shader_?: any;
};

type vertex_buffer_t = {
	buffer_?: any;
	vertex_count?: i32;
};

type vertex_struct_t = {
	elements?: kinc_vertex_elem_t[];
	instanced?: bool;
};

type index_buffer_t = {
	buffer_?: any;
};

type kinc_vertex_elem_t = {
	name: string;
	data: vertex_data_t;
};

type kinc_const_loc_t = any;
type kinc_tex_unit_t = any;

enum tex_filter_t {
	POINT,
	LINEAR,
	ANISOTROPIC,
}

enum mip_map_filter_t {
	NONE,
	POINT,
	LINEAR,
}

enum tex_addressing_t {
	REPEAT,
	MIRROR,
	CLAMP,
}

enum usage_t {
	STATIC,
	DYNAMIC,
	READABLE,
}

enum tex_format_t {
	RGBA32,
	RGBA64,
	R32,
	RGBA128,
	DEPTH16,
	R8,
	R16,
}

enum depth_format_t {
	NO_DEPTH,
	DEPTH16,
	DEPTH24,
}

enum vertex_data_t {
	F32_1X = 1,
	F32_2X = 2,
	F32_3X = 3,
	F32_4X = 4,
	U8_4X_NORM = 17,
	I16_2X_NORM = 24,
	I16_4X_NORM = 28,
}

enum blend_factor_t {
	BLEND_ONE,
	BLEND_ZERO,
	SOURCE_ALPHA,
	DEST_ALPHA,
	INV_SOURCE_ALPHA,
	INV_DEST_ALPHA,
	SOURCE_COLOR,
	DEST_COLOR,
	INV_SOURCE_COLOR,
	INV_DEST_COLOR,
}

enum compare_mode_t {
	ALWAYS,
	NEVER,
	EQUAL,
	NOT_EQUAL,
	LESS,
	LESS_EQUAL,
	GREATER,
	GREATER_EQUAL,
}

enum cull_mode_t {
	CLOCKWISE,
	COUNTER_CLOCKWISE,
	NONE,
}

enum shader_type_t {
	FRAGMENT = 0,
	VERTEX = 1,
	GEOMETRY = 3,
}