
function shader_data_create(raw: shader_data_t, done: (sd: shader_data_t)=>void, override_context: shader_override_t = null) {
	raw._contexts = [];
	for (let c of raw.contexts) {
		raw._contexts.push(null);
	}

	let contexts_loaded: i32 = 0;
	for (let i: i32 = 0; i < raw.contexts.length; ++i) {
		let c: shader_context_t = raw.contexts[i];
		shader_context_create(c, function (con: shader_context_t) {
			raw._contexts[i] = con;
			contexts_loaded++;
			if (contexts_loaded == raw.contexts.length) {
				done(raw);
			}
		}, override_context);
	}
}

function shader_data_ext(): string {
	///if krom_vulkan
	return ".spirv";
	///elseif (krom_android || krom_wasm)
	return ".essl";
	///elseif krom_opengl
	return ".glsl";
	///elseif krom_metal
	return ".metal";
	///else
	return ".d3d11";
	///end
}

function shader_data_parse(file: string, name: string, done: (sd: shader_data_t)=>void, override_context: shader_override_t = null) {
	data_get_scene_raw(file, function (format: scene_t) {
		let raw: shader_data_t = data_get_shader_raw_by_name(format.shader_datas, name);
		if (raw == null) {
			krom_log(`Shader data "${name}" not found!`);
			done(null);
		}
		shader_data_create(raw, done, override_context);
	});
}

function shader_data_delete(raw: shader_data_t) {
	for (let c of raw._contexts) {
		shader_context_delete(c);
	}
}

function shader_data_get_context(raw: shader_data_t, name: string): shader_context_t {
	for (let c of raw._contexts) {
		if (c.name == name) {
			return c;
		}
	}
	return null;
}

function shader_context_create(raw: shader_context_t, done: (sc: shader_context_t)=>void, override_context: shader_override_t = null) {
	///if (!arm_voxels)
	if (raw.name == "voxel") {
		done(raw);
		return;
	}
	///end
	raw._override_context = override_context;
	shader_context_parse_vertex_struct(raw);
	shader_context_compile(raw, done);
}

function shader_context_compile(raw: shader_context_t, done: (sc: shader_context_t)=>void) {
	if (raw._pipe_state != null) {
		g4_pipeline_delete(raw._pipe_state);
	}
	raw._pipe_state = g4_pipeline_create();
	raw._constants = [];
	raw._tex_units = [];

	if (raw._instancing_type > 0) { // Instancing
		let inst_struct: vertex_struct_t = g4_vertex_struct_create();
		g4_vertex_struct_add(inst_struct, "ipos", vertex_data_t.F32_3X);
		if (raw._instancing_type == 2 || raw._instancing_type == 4) {
			g4_vertex_struct_add(inst_struct, "irot", vertex_data_t.F32_3X);
		}
		if (raw._instancing_type == 3 || raw._instancing_type == 4) {
			g4_vertex_struct_add(inst_struct, "iscl", vertex_data_t.F32_3X);
		}
		inst_struct.instanced = true;
		raw._pipe_state.input_layout = [raw._structure, inst_struct];
	}
	else { // Regular
		raw._pipe_state.input_layout = [raw._structure];
	}

	// Depth
	raw._pipe_state.depth_write = raw.depth_write;
	raw._pipe_state.depth_mode = shader_context_get_compare_mode(raw.compare_mode);

	// Cull
	raw._pipe_state.cull_mode = shader_context_get_cull_mode(raw.cull_mode);

	// Blending
	if (raw.blend_source != null) {
		raw._pipe_state.blend_source = shader_context_get_blend_fac(raw.blend_source);
	}
	if (raw.blend_destination != null) {
		raw._pipe_state.blend_dest = shader_context_get_blend_fac(raw.blend_destination);
	}
	if (raw.alpha_blend_source != null) {
		raw._pipe_state.alpha_blend_source = shader_context_get_blend_fac(raw.alpha_blend_source);
	}
	if (raw.alpha_blend_destination != null) {
		raw._pipe_state.alpha_blend_dest = shader_context_get_blend_fac(raw.alpha_blend_destination);
	}

	// Per-target color write mask
	if (raw.color_writes_red != null) {
		for (let i: i32 = 0; i < raw.color_writes_red.length; ++i) {
			raw._pipe_state.color_write_masks_red[i] = raw.color_writes_red[i];
		}
	}
	if (raw.color_writes_green != null) {
		for (let i: i32 = 0; i < raw.color_writes_green.length; ++i) {
			raw._pipe_state.color_write_masks_green[i] = raw.color_writes_green[i];
		}
	}
	if (raw.color_writes_blue != null) {
		for (let i: i32 = 0; i < raw.color_writes_blue.length; ++i) {
			raw._pipe_state.color_write_masks_blue[i] = raw.color_writes_blue[i];
		}
	}
	if (raw.color_writes_alpha != null) {
		for (let i: i32 = 0; i < raw.color_writes_alpha.length; ++i) {
			raw._pipe_state.color_write_masks_alpha[i] = raw.color_writes_alpha[i];
		}
	}

	// Color attachment format
	if (raw.color_attachments != null) {
		raw._pipe_state.color_attachment_count = raw.color_attachments.length;
		for (let i: i32 = 0; i < raw.color_attachments.length; ++i) {
			raw._pipe_state.color_attachments[i] = shader_context_get_tex_format(raw.color_attachments[i]);
		}
	}

	// Depth attachment format
	if (raw.depth_attachment != null) {
		raw._pipe_state.depth_attachment = shader_context_get_depth_format(raw.depth_attachment);
	}

	// Shaders
	if (raw.shader_from_source) {
		raw._pipe_state.vertex_shader = g4_shader_from_source(raw.vertex_shader, shader_type_t.VERTEX);
		raw._pipe_state.fragment_shader = g4_shader_from_source(raw.fragment_shader, shader_type_t.FRAGMENT);

		// Shader compile error
		if (raw._pipe_state.vertex_shader.shader_ == null || raw._pipe_state.fragment_shader.shader_ == null) {
			done(null);
			return;
		}
		shader_context_finish_compile(raw, done);
	}
	else {

		///if arm_noembed // Load shaders manually

		let shaders_loaded: i32 = 0;
		let num_shaders: i32 = 2;
		if (raw.geometry_shader != null) {
			num_shaders++;
		}

		function load_shader(file: string, type: i32) {
			let path: string = file + shader_data_ext();
			data_get_blob(path, function (b: ArrayBuffer) {
				if (type == 0) {
					raw._pipe_state.vertex_shader = g4_shader_create(b, shader_type_t.VERTEX);
				}
				else if (type == 1) {
					raw._pipe_state.fragment_shader = g4_shader_create(b, shader_type_t.FRAGMENT);
				}
				else if (type == 2) {
					raw._pipe_state.geometry_shader = g4_shader_create(b, shader_type_t.GEOMETRY);
				}
				shaders_loaded++;
				if (shaders_loaded >= num_shaders) {
					shader_context_finish_compile(raw, done);
				}
			});
		}
		load_shader(raw.vertex_shader, 0);
		load_shader(raw.fragment_shader, 1);
		if (raw.geometry_shader != null) {
			load_shader(raw.geometry_shader, 2);
		}

		///else

		raw._pipe_state.fragment_shader = sys_get_shader(raw.fragment_shader);
		raw._pipe_state.vertex_shader = sys_get_shader(raw.vertex_shader);
		if (raw.geometry_shader != null) {
			raw._pipe_state.geometry_shader = sys_get_shader(raw.geometry_shader);
		}
		shader_context_finish_compile(raw, done);

		///end
	}
}

function shader_context_finish_compile(raw: shader_context_t, done: (sc: shader_context_t)=>void) {
	// Override specified values
	if (raw._override_context != null) {
		if (raw._override_context.cull_mode != null) {
			raw._pipe_state.cull_mode = shader_context_get_cull_mode(raw._override_context.cull_mode);
		}
	}

	g4_pipeline_compile(raw._pipe_state);

	if (raw.constants != null) {
		for (let c of raw.constants) {
			shader_context_add_const(raw, c);
		}
	}

	if (raw.texture_units != null) {
		for (let tu of raw.texture_units) {
			shader_context_add_tex(raw, tu);
		}
	}

	done(raw);
}

function shader_context_parse_data(data: string): vertex_data_t {
	if (data == "float1") {
		return vertex_data_t.F32_1X;
	}
	else if (data == "float2") {
		return vertex_data_t.F32_2X;
	}
	else if (data == "float3") {
		return vertex_data_t.F32_3X;
	}
	else if (data == "float4") {
		return vertex_data_t.F32_4X;
	}
	else if (data == "short2norm") {
		return vertex_data_t.I16_2X_NORM;
	}
	else if (data == "short4norm") {
		return vertex_data_t.I16_4X_NORM;
	}
	return vertex_data_t.F32_1X;
}

function shader_context_parse_vertex_struct(raw: shader_context_t) {
	raw._structure = g4_vertex_struct_create();
	let ipos: bool = false;
	let irot: bool = false;
	let iscl: bool = false;
	for (let elem of raw.vertex_elements) {
		if (elem.name == "ipos") {
			ipos = true;
			continue;
		}
		if (elem.name == "irot") {
			irot = true;
			continue;
		}
		if (elem.name == "iscl") {
			iscl = true;
			continue;
		}
		g4_vertex_struct_add(raw._structure, elem.name, shader_context_parse_data(elem.data));
	}
	if (ipos && !irot && !iscl) {
		raw._instancing_type = 1;
	}
	else if (ipos && irot && !iscl) {
		raw._instancing_type = 2;
	}
	else if (ipos && !irot && iscl) {
		raw._instancing_type = 3;
	}
	else if (ipos && irot && iscl) {
		raw._instancing_type = 4;
	}
}

function shader_context_delete(raw: shader_context_t) {
	if (raw._pipe_state.fragment_shader != null) {
		g4_shader_delete(raw._pipe_state.fragment_shader);
	}
	if (raw._pipe_state.vertex_shader != null) {
		g4_shader_delete(raw._pipe_state.vertex_shader);
	}
	if (raw._pipe_state.geometry_shader != null) {
		g4_shader_delete(raw._pipe_state.geometry_shader);
	}
	g4_pipeline_delete(raw._pipe_state);
}

function shader_context_get_compare_mode(s: string): compare_mode_t {
	if (s == "always") {
		return compare_mode_t.ALWAYS;
	}
	if (s == "never") {
		return compare_mode_t.NEVER;
	}
	if (s == "less") {
		return compare_mode_t.LESS;
	}
	if (s == "less_equal") {
		return compare_mode_t.LESS_EQUAL;
	}
	if (s == "greater") {
		return compare_mode_t.GREATER;
	}
	if (s == "greater_equal") {
		return compare_mode_t.GREATER_EQUAL;
	}
	if (s == "equal") {
		return compare_mode_t.EQUAL;
	}
	if (s == "not_equal") {
		return compare_mode_t.NOT_EQUAL;
	}
	return compare_mode_t.LESS;
}

function shader_context_get_cull_mode(s: string): cull_mode_t {
	if (s == "none") {
		return cull_mode_t.NONE;
	}
	if (s == "clockwise") {
		return cull_mode_t.CLOCKWISE;
	}
	return cull_mode_t.COUNTER_CLOCKWISE;
}

function shader_context_get_blend_fac(s: string): blend_factor_t {
	if (s == "blend_one") {
		return blend_factor_t.BLEND_ONE;
	}
	if (s == "blend_zero") {
		return blend_factor_t.BLEND_ZERO;
	}
	if (s == "source_alpha") {
		return blend_factor_t.SOURCE_ALPHA;
	}
	if (s == "destination_alpha") {
		return blend_factor_t.DEST_ALPHA;
	}
	if (s == "inverse_source_alpha") {
		return blend_factor_t.INV_SOURCE_ALPHA;
	}
	if (s == "inverse_destination_alpha") {
		return blend_factor_t.INV_DEST_ALPHA;
	}
	if (s == "source_color") {
		return blend_factor_t.SOURCE_COLOR;
	}
	if (s == "destination_color") {
		return blend_factor_t.DEST_COLOR;
	}
	if (s == "inverse_source_color") {
		return blend_factor_t.INV_SOURCE_COLOR;
	}
	if (s == "inverse_destination_color") {
		return blend_factor_t.INV_DEST_COLOR;
	}
	return blend_factor_t.BLEND_ONE;
}

function shader_context_get_tex_addresing(s: string): tex_addressing_t {
	if (s == "repeat") {
		return tex_addressing_t.REPEAT;
	}
	if (s == "mirror") {
		return tex_addressing_t.MIRROR;
	}
	return tex_addressing_t.CLAMP;
}

function shader_context_get_tex_filter(s: string): tex_filter_t {
	if (s == "point") {
		return tex_filter_t.POINT;
	}
	if (s == "linear") {
		return tex_filter_t.LINEAR;
	}
	return tex_filter_t.ANISOTROPIC;
}

function shader_context_get_mipmap_filter(s: string): mip_map_filter_t {
	if (s == "no") {
		return mip_map_filter_t.NONE;
	}
	if (s == "point") {
		return mip_map_filter_t.POINT;
	}
	return mip_map_filter_t.LINEAR;
}

function shader_context_get_tex_format(s: string): tex_format_t {
	if (s == "RGBA32") {
		return tex_format_t.RGBA32;
	}
	if (s == "RGBA64") {
		return tex_format_t.RGBA64;
	}
	if (s == "RGBA128") {
		return tex_format_t.RGBA128;
	}
	if (s == "DEPTH16") {
		return tex_format_t.DEPTH16;
	}
	if (s == "R32") {
		return tex_format_t.R32;
	}
	if (s == "R16") {
		return tex_format_t.R16;
	}
	if (s == "R8") {
		return tex_format_t.R8;
	}
	return tex_format_t.RGBA32;
}

function shader_context_get_depth_format(s: string): depth_format_t {
	if (s == "DEPTH32") {
		return depth_format_t.DEPTH24;
	}
	if (s == "NONE") {
		return depth_format_t.NO_DEPTH;
	}
	return depth_format_t.DEPTH24;
}

function shader_context_add_const(raw: shader_context_t, c: shader_const_t) {
	raw._constants.push(g4_pipeline_get_const_loc(raw._pipe_state, c.name));
}

function shader_context_add_tex(raw: shader_context_t, tu: tex_unit_t) {
	let unit = g4_pipeline_get_tex_unit(raw._pipe_state, tu.name);
	raw._tex_units.push(unit);
}

function shader_context_set_tex_params(raw: shader_context_t, unit_index: i32, tex: bind_tex_t) {
	// This function is called for samplers set using material context
	let unit: any = raw._tex_units[unit_index];
	g4_set_tex_params(unit,
		tex.u_addressing == null ? tex_addressing_t.REPEAT : shader_context_get_tex_addresing(tex.u_addressing),
		tex.v_addressing == null ? tex_addressing_t.REPEAT : shader_context_get_tex_addresing(tex.v_addressing),
		tex.min_filter == null ? tex_filter_t.LINEAR : shader_context_get_tex_filter(tex.min_filter),
		tex.mag_filter == null ? tex_filter_t.LINEAR : shader_context_get_tex_filter(tex.mag_filter),
		tex.mipmap_filter == null ? mip_map_filter_t.NONE : shader_context_get_mipmap_filter(tex.mipmap_filter));
}