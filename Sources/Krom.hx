extern class Krom {

	static function clear(flags: Int, color: Int, depth: Float, stencil: Int): Void;
	static function createVertexShader(data: js.lib.ArrayBuffer, name: String): Dynamic;
	static function createVertexShaderFromSource(source: String): Dynamic;
	static function createFragmentShader(data: js.lib.ArrayBuffer, name: String): Dynamic;
	static function createFragmentShaderFromSource(source: String): Dynamic;
	static function createGeometryShader(data: js.lib.ArrayBuffer, name: String): Dynamic;
	static function createTessellationControlShader(data: js.lib.ArrayBuffer, name: String): Dynamic;
	static function createTessellationEvaluationShader(data: js.lib.ArrayBuffer, name: String): Dynamic;
	static function deleteShader(shader: Dynamic): Dynamic;
	static function createPipeline(): Dynamic;
	static function deletePipeline(pipeline: Dynamic): Dynamic;
	static function compilePipeline(pipeline: Dynamic, structure0: Dynamic, structure1: Dynamic, structure2: Dynamic, structure3: Dynamic, length: Int, vertexShader: Dynamic, fragmentShader: Dynamic, geometryShader: Dynamic, tessellationControlShader: Dynamic, tessellationEvaluationShader: Dynamic, state: Dynamic): Void;
	static function setPipeline(pipeline: Dynamic): Void;
	static function getConstantLocation(pipeline: Dynamic, name: String): Dynamic;
	static function getTextureUnit(pipeline: Dynamic, name: String): Dynamic;
	static function setTexture(stage: kha.graphics4.TextureUnit, texture: Dynamic): Void;
	static function setRenderTarget(stage: kha.graphics4.TextureUnit, renderTarget: Dynamic): Void;
	static function setTextureDepth(unit: kha.graphics4.TextureUnit, texture: Dynamic): Void;
	static function setImageTexture(stage: kha.graphics4.TextureUnit, texture: Dynamic): Void;
	static function setTextureParameters(texunit: kha.graphics4.TextureUnit, uAddressing: Int, vAddressing: Int, minificationFilter: Int, magnificationFilter: Int, mipmapFilter: Int): Void;
	static function setTexture3DParameters(texunit: kha.graphics4.TextureUnit, uAddressing: Int, vAddressing: Int, wAddressing: Int, minificationFilter: Int, magnificationFilter: Int, mipmapFilter: Int): Void;
	static function setTextureCompareMode(texunit: kha.graphics4.TextureUnit, enabled: Bool): Void;
	static function setCubeMapCompareMode(texunit: kha.graphics4.TextureUnit, enabled: Bool): Void;
	static function setBool(location: kha.graphics4.ConstantLocation, value: Bool): Void;
	static function setInt(location: kha.graphics4.ConstantLocation, value: Int): Void;
	static function setFloat(location: kha.graphics4.ConstantLocation, value: Float): Void;
	static function setFloat2(location: kha.graphics4.ConstantLocation, value1: Float, value2: Float): Void;
	static function setFloat3(location: kha.graphics4.ConstantLocation, value1: Float, value2: Float, value3: Float): Void;
	static function setFloat4(location: kha.graphics4.ConstantLocation, value1: Float, value2: Float, value3: Float, value4: Float): Void;
	static function setFloats(location: kha.graphics4.ConstantLocation, values: js.lib.ArrayBuffer): Void;
	static function setMatrix(location: kha.graphics4.ConstantLocation, matrix: js.lib.ArrayBuffer): Void;
	static function setMatrix3(location: kha.graphics4.ConstantLocation, matrix: js.lib.ArrayBuffer): Void;

	static function begin(renderTarget: kha.Canvas, additionalRenderTargets: Array<kha.Canvas>): Void;
	static function beginFace(renderTarget: kha.Canvas, face: Int): Void;
	static function end(): Void;
	static function renderTargetsInvertedY(): Bool;
	static function viewport(x: Int, y: Int, width: Int, height: Int): Void;
	static function scissor(x: Int, y: Int, width: Int, height: Int): Void;
	static function disableScissor(): Void;
	static function createRenderTarget(width: Int, height: Int, format: Int, depthBufferBits: Int, stencilBufferBits: Int): Dynamic;
	static function createRenderTargetCubeMap(cubeMapSize: Int, format: Int, depthBufferBits: Int, stencilBufferBits: Int): Dynamic;
	static function createTexture(width: Int, height: Int, format: Int): Dynamic;
	static function createTexture3D(width: Int, height: Int, depth: Int, format: Int): Dynamic;
	static function createTextureFromBytes(data: js.lib.ArrayBuffer, width: Int, height: Int, format: Int, readable: Bool): Dynamic;
	static function createTextureFromBytes3D(data: js.lib.ArrayBuffer, width: Int, height: Int, depth: Int, format: Int, readable: Bool): Dynamic;
	static function createTextureFromEncodedBytes(data: js.lib.ArrayBuffer, format: String, readable: Bool): Dynamic;
	static function getTexturePixels(texture: Dynamic): js.lib.ArrayBuffer;
	static function getRenderTargetPixels(renderTarget: Dynamic, data: js.lib.ArrayBuffer): Void;
	static function lockTexture(texture: Dynamic, level: Int): js.lib.ArrayBuffer;
	static function unlockTexture(texture: Dynamic): Void;
	static function generateTextureMipmaps(texture: Dynamic, levels: Int): Void;
	static function generateRenderTargetMipmaps(renderTarget: Dynamic, levels: Int): Void;
	static function setMipmaps(texture: Dynamic, mipmaps: Array<kha.Image>): Void;
	static function setDepthStencilFrom(target: Dynamic, source: Dynamic): Void;
	static function clearTexture(target: Dynamic, x: Int, y: Int, z: Int, width: Int, height: Int, depth: Int, color: Int): Void;
	static function createIndexBuffer(count: Int): Dynamic;
	static function deleteIndexBuffer(buffer: Dynamic): Dynamic;
	static function lockIndexBuffer(buffer: Dynamic): kha.arrays.Uint32Array;
	static function unlockIndexBuffer(buffer: Dynamic): Void;
	static function setIndexBuffer(buffer: Dynamic): Void;
	static function createVertexBuffer(count: Int, structure: Array<kha.graphics4.VertexElement>, usage: Int, instanceDataStepRate: Int): Dynamic;
	static function deleteVertexBuffer(buffer: Dynamic): Dynamic;
	static function lockVertexBuffer(buffer: Dynamic, start: Int, count: Int): js.lib.ArrayBuffer;
	static function unlockVertexBuffer(buffer: Dynamic, count: Int): Void;
	static function setVertexBuffer(buffer: Dynamic): Void;
	static function setVertexBuffers(vertexBuffers: Array<kha.graphics4.VertexBuffer>): Void;
	static function drawIndexedVertices(start: Int, count: Int): Void;
	static function drawIndexedVerticesInstanced(instanceCount: Int, start: Int, count: Int): Void;

	static function loadImage(file: String, readable: Bool): Dynamic;
	static function unloadImage(image: kha.Image): Void;
	static function loadBlob(file: String): js.lib.ArrayBuffer;
	static function loadUrl(url: String): Void;
	static function copyToClipboard(text: String): Void;

	static function loadSound(file: String): Dynamic;
	static function unloadSound(sound: Dynamic): Void;
	static function playSound(sound: Dynamic, loop: Bool): Void;
	static function stopSound(sound: Dynamic): Void;

	static function init(title: String, width: Int, height: Int, samplesPerPixel: Int, vSync: Bool, windowMode: Int, windowFeatures: Int, kromApi: Int, x: Int, y: Int, frequency: Int): Void;
	static function setApplicationName(name: String): Void;
	static function log(v: Dynamic): Void;
	static function setCallback(callback: Void->Void): Void;
	static function setDropFilesCallback(callback: String->Void): Void;
	static function setCutCopyPasteCallback(cutCallback: Void->String, copyCallback: Void->String, pasteCallback: String->Void): Void;
	static function setApplicationStateCallback(foregroundCallback: Void->Void, resumeCallback: Void->Void, pauseCallback: Void->Void, backgroundCallback: Void->Void, shutdownCallback: Void->Void): Void;
	static function setKeyboardDownCallback(callback: Int->Void): Void;
	static function setKeyboardUpCallback(callback: Int->Void): Void;
	static function setKeyboardPressCallback(callback: Int->Void): Void;
	static function setMouseDownCallback(callback: Int->Int->Int->Void): Void;
	static function setMouseUpCallback(callback: Int->Int->Int->Void): Void;
	static function setMouseMoveCallback(callback: Int->Int->Int->Int->Void): Void;
	static function setMouseWheelCallback(callback: Int->Void): Void;
	static function setTouchDownCallback(callback: Int->Int->Int->Void): Void;
	static function setTouchUpCallback(callback: Int->Int->Int->Void): Void;
	static function setTouchMoveCallback(callback: Int->Int->Int->Void): Void;
	static function setPenDownCallback(callback: Int->Int->Float->Void): Void;
	static function setPenUpCallback(callback: Int->Int->Float->Void): Void;
	static function setPenMoveCallback(callback: Int->Int->Float->Void): Void;
	static function setGamepadAxisCallback(callback: Int->Int->Float->Void): Void;
	static function setGamepadButtonCallback(callback: Int->Int->Float->Void): Void;
	static function lockMouse(): Void;
	static function unlockMouse(): Void;
	static function canLockMouse(): Bool;
	static function isMouseLocked(): Bool;
	static function setMousePosition(windowId: Int, x: Int, y: Int): Void;
	static function showMouse(show: Bool): Void;
	static function showKeyboard(show: Bool): Void;
	static function getTime(): Float;
	static function windowWidth(id: Int): Int;
	static function windowHeight(id: Int): Int;
	static function setWindowTitle(id: Int, title: String): Void;
	static function getWindowMode(id: Int): Int;
	static function setWindowMode(id: Int, mode: Int): Void;
	static function resizeWindow(id: Int, width: Int, height: Int): Void;
	static function moveWindow(id: Int, x: Int, y: Int): Void;
	static function screenDpi(): Int;
	static function systemId(): String;
	static function requestShutdown(): Void;
	static function displayCount(): Int;
	static function displayWidth(index: Int): Int;
	static function displayHeight(index: Int): Int;
	static function displayX(index: Int): Int;
	static function displayY(index: Int): Int;
	static function displayFrequency(index: Int): Int;
	static function displayIsPrimary(index: Int): Bool;
	static function writeStorage(name: String, data: js.lib.ArrayBuffer): Void;
	static function readStorage(name: String): js.lib.ArrayBuffer;

	static function fileSaveBytes(path: String, bytes: js.lib.ArrayBuffer, ?length: Int): Void;
	static function sysCommand(cmd: String, ?args: Array<String>): Int;
	static function savePath(): String;
	static function getArgCount(): Int;
	static function getArg(index: Int): String;
	static function getFilesLocation(): String;
	static function httpRequest(url: String, size: Int, callback: js.lib.ArrayBuffer->Void): Void;

	static function setBoolCompute(location: kha.compute.ConstantLocation, value: Bool): Void;
	static function setIntCompute(location: kha.compute.ConstantLocation, value: Int): Void;
	static function setFloatCompute(location: kha.compute.ConstantLocation, value: Float): Void;
	static function setFloat2Compute(location: kha.compute.ConstantLocation, value1: Float, value2: Float): Void;
	static function setFloat3Compute(location: kha.compute.ConstantLocation, value1: Float, value2: Float, value3: Float): Void;
	static function setFloat4Compute(location: kha.compute.ConstantLocation, value1: Float, value2: Float, value3: Float, value4: Float): Void;
	static function setFloatsCompute(location: kha.compute.ConstantLocation, values: js.lib.ArrayBuffer): Void;
	static function setMatrixCompute(location: kha.compute.ConstantLocation, matrix: js.lib.ArrayBuffer): Void;
	static function setMatrix3Compute(location: kha.compute.ConstantLocation, matrix: js.lib.ArrayBuffer): Void;
	static function setTextureCompute(unit: kha.compute.TextureUnit, texture: Dynamic, access: Int): Void;
	static function setRenderTargetCompute(unit: kha.compute.TextureUnit, renderTarget: Dynamic, access: Int): Void;
	static function setSampledTextureCompute(unit: kha.compute.TextureUnit, texture: Dynamic): Void;
	static function setSampledRenderTargetCompute(unit: kha.compute.TextureUnit, renderTarget: Dynamic): Void;
	static function setSampledDepthTextureCompute(unit: kha.compute.TextureUnit, texture: Dynamic): Void;
	static function setTextureParametersCompute(texunit: kha.compute.TextureUnit, uAddressing: Int, vAddressing: Int, minificationFilter: Int, magnificationFilter: Int, mipmapFilter: Int): Void;
	static function setTexture3DParametersCompute(texunit: kha.compute.TextureUnit, uAddressing: Int, vAddressing: Int, wAddressing: Int, minificationFilter: Int, magnificationFilter: Int, mipmapFilter: Int): Void;
	static function setShaderCompute(shader: Dynamic): Void;
	static function deleteShaderCompute(shader: Dynamic): Void;
	static function createShaderCompute(bytes: js.lib.ArrayBuffer): Dynamic;
	static function getConstantLocationCompute(shader: Dynamic, name: String): Dynamic;
	static function getTextureUnitCompute(shader: Dynamic, name: String): Dynamic;
	static function compute(x: Int, y: Int, z: Int): Void;

	// Extended
	static function g2_init(image_vert: js.lib.ArrayBuffer, image_frag: js.lib.ArrayBuffer, colored_vert: js.lib.ArrayBuffer, colored_frag: js.lib.ArrayBuffer, text_vert: js.lib.ArrayBuffer, text_frag: js.lib.ArrayBuffer): Void;
	static function g2_begin(): Void;
	static function g2_end(): Void;
	static function g2_draw_scaled_sub_image(image: kha.Image, sx: Float, sy: Float, sw: Float, sh: Float, dx: Float, dy: Float, dw: Float, dh: Float): Void;
	static function g2_fill_triangle(x0: Float, y0: Float, x1: Float, y1: Float, x2: Float, y2: Float): Void;
	static function g2_fill_rect(x: Float, y: Float, width: Float, height: Float): Void;
	static function g2_draw_rect(x: Float, y: Float, width: Float, height: Float, strength: Float): Void;
	static function g2_draw_line(x0: Float, y0: Float, x1: Float, y1: Float, strength: Float): Void;
	static function g2_draw_string(text: String, x: Float, y: Float): Void;
	static function g2_set_font(font: Dynamic, size: Int): Void;
	static function g2_font_init(blob: js.lib.ArrayBuffer, font_index: Int): Dynamic;
	static function g2_font_13(blob: js.lib.ArrayBuffer): Dynamic;
	static function g2_font_set_glyphs(glyphs: Array<Int>): Void;
	static function g2_font_count(font: Dynamic): Int;
	static function g2_font_height(font: Dynamic, size: Int): Int;
	static function g2_string_width(font: Dynamic, size: Int, text: String): Int;
	static function g2_set_bilinear_filter(bilinear: Bool): Void;
	static function g2_restore_render_target(): Void;
	static function g2_set_render_target(renderTarget: Dynamic): Void;
	static function g2_set_color(color: Int): Void;
	static function g2_set_pipeline(pipeline: Dynamic): Void;
	static function g2_set_transform(matrix: js.lib.ArrayBuffer): Void;
	static function g2_fill_circle(cx: Float, cy: Float, radius: Float, segments: Int): Void;
	static function g2_draw_circle(cx: Float, cy: Float, radius: Float, segments: Int, strength: Float): Void;
	static function g2_draw_cubic_bezier(x: Array<Float>, y: Array<Float>, segments: Int, strength: Float): Void;

	static function setSaveAndQuitCallback(callback: Bool->Void): Void;
	static function setMouseCursor(id: Int): Void;
	static function delayIdleSleep(): Void;
	static function raytraceSupported(): Bool;
	static function raytraceInit(shader: js.lib.ArrayBuffer, vb: Dynamic, ib: Dynamic, scale: Float): Void;
	static function raytraceSetTextures(tex0: kha.Image, tex1: kha.Image, tex2: kha.Image, texenv: Dynamic, tex_sobol: Dynamic, tex_scramble: Dynamic, tex_rank: Dynamic): Void;
	static function raytraceDispatchRays(target: Dynamic, cb: js.lib.ArrayBuffer): Void;
	static function saveDialog(filterList: String, defaultPath: String): String;
	static function openDialog(filterList: String, defaultPath: String, openMultiple: Bool): Array<String>;
	static function readDirectory(path: String, foldersOnly: Bool): String;
	static function fileExists(path: String): Bool;
	static function deleteFile(path: String): Void;
	static function inflate(bytes: js.lib.ArrayBuffer, raw: Bool): js.lib.ArrayBuffer;
	static function deflate(bytes: js.lib.ArrayBuffer, raw: Bool): js.lib.ArrayBuffer;
	static function writeJpg(path: String, bytes: js.lib.ArrayBuffer, w: Int, h: Int, format: Int, quality: Int): Void; // RGBA, R, RGB1, RRR1, GGG1, BBB1, AAA1
	static function writePng(path: String, bytes: js.lib.ArrayBuffer, w: Int, h: Int, format: Int): Void;
	static function encodeJpg(bytes: js.lib.ArrayBuffer, w: Int, h: Int, format: Int, quality: Int): js.lib.ArrayBuffer;
	static function encodePng(bytes: js.lib.ArrayBuffer, w: Int, h: Int, format: Int): js.lib.ArrayBuffer;
	static function windowX(id: Int): Int;
	static function windowY(id: Int): Int;
	static function language(): String;
	static function mlInference(model: js.lib.ArrayBuffer, tensors: Array<js.lib.ArrayBuffer>, ?inputShape: Array<Array<Int>>, ?outputShape: Array<Int>, ?useGpu: Bool): js.lib.ArrayBuffer;
	static function mlUnload(): Void;

	static function io_obj_parse(file_bytes: js.lib.ArrayBuffer, split_code: Int, start_pos: Int, udim: Bool): Dynamic;

	static function zui_init(ops: Dynamic): Dynamic;
	static function zui_get_scale(ui: Dynamic): Float;
	static function zui_set_scale(ui: Dynamic, factor: Float): Void;
	static function zui_set_font(ui: Dynamic, font: Dynamic): Void;
	static function zui_begin(ui: Dynamic): Void;
	static function zui_end(last: Bool): Void;
	static function zui_begin_region(ui: Dynamic, x: Int, y: Int, w: Int): Void;
	static function zui_end_region(last: Bool): Void;
	static function zui_begin_sticky(): Void;
	static function zui_end_sticky(): Void;
	static function zui_end_input(): Void;
	static function zui_end_window(bind_global_g: Bool): Void;
	static function zui_end_element(element_size: Float): Void;
	static function zui_start_text_edit(handle: Dynamic, align: Int): Void;
	static function zui_input_in_rect(x: Float, y: Float, w: Float, h: Float): Bool;
	static function zui_window(handle: Dynamic, x: Int, y: Int, w: Int, h: Int, drag: Bool): Bool;
	static function zui_button(text: String, align: Int, label: String): Bool;
	static function zui_check(handle: Dynamic, text: String, label: String): Bool;
	static function zui_radio(handle: Dynamic, position: Int, text: String, label: String): Bool;
	static function zui_combo(handle: Dynamic, texts: Array<String>, label: String, show_label: Bool, align: Int, search_bar: Bool): Int;
	static function zui_slider(handle: Dynamic, text: String, from: Float, to: Float, filled: Bool, precision: Float, display_value: Bool, align: Int, text_edit: Bool): Float;
	static function zui_image(image: kha.Image, tint: Int, h: Int, sx: Int, sy: Int, sw: Int, sh: Int): Int;
	static function zui_text(text: String, align: Int, bg: Int): Int;
	static function zui_text_input(handle: Dynamic, label: String, align: Int, editable: Bool, live_update: Bool): String;
	static function zui_tab(handle: Dynamic, text: String, vertical: Bool, color: Int): Bool;
	static function zui_panel(handle: Dynamic, text: String, isTree: Bool, filled: Bool, pack: Bool): Bool;
	static function zui_handle(ops: Dynamic): Dynamic;
	static function zui_separator(h: Int, fill: Bool): Void;
	static function zui_tooltip(text: String): Void;
	static function zui_tooltip_image(image: kha.Image, max_width: Int): Void;
	static function zui_row(ratios: Array<Float>): Void;
	static function zui_fill(x: Float, y: Float, w: Float, h: Float, color: Int): Void;
	static function zui_rect(x: Float, y: Float, w: Float, h: Float, color: Int, strength: Float): Void;
	static function zui_draw_rect(fill: Bool, x: Float, y: Float, w: Float, h: Float, strength: Float): Void;
	static function zui_draw_string(text: String, x_offset: Float, y_offset: Float, align: Int, truncation: Bool): Void;
	static function zui_get_hovered_tab_name(): String;
	static function zui_set_hovered_tab_name(name: String): Void;
	static function zui_begin_menu(): Void;
	static function zui_end_menu(): Void;
	static function zui_menu_button(text: String): Bool;
	static function zui_float_input(handle: Dynamic, label: String, align: Int, precision: Float): Float;
	static function zui_inline_radio(handle: Dynamic, texts: Array<String>, align: Int): Int;
	static function zui_color_wheel(handle: Dynamic, alpha: Bool, w: Float, h: Float, color_preview: Bool, picker: Void->Void): Int;
	static function zui_text_area(handle: Dynamic, align: Int, editable: Bool, label: String, word_wrap: Bool): String;
	static function zui_text_area_coloring(packed: js.lib.ArrayBuffer): Void;
	static function zui_nodes_init(): Dynamic;
	static function zui_node_canvas(nodes: Dynamic, packed: js.lib.ArrayBuffer): js.lib.ArrayBuffer;
	static function zui_nodes_rgba_popup(handle: Dynamic, val: js.lib.ArrayBuffer, x: Int, y: Int): Void;
	static function zui_nodes_scale(): Float;
	static function zui_nodes_pan_x(): Float;
	static function zui_nodes_pan_y(): Float;
	static function zui_get(ui: Dynamic, name: String): Dynamic;
	static function zui_set(ui: Dynamic, name: String, val: Dynamic): Void;
	static function zui_handle_get(handle: Dynamic, name: String): Dynamic;
	static function zui_handle_set(handle: Dynamic, name: String, val: Dynamic): Void;
	static function zui_handle_ptr(handle: Dynamic): Int;
	static function zui_theme_init(): Dynamic;
	static function zui_theme_get(theme: Dynamic, name: String): Dynamic;
	static function zui_theme_set(theme: Dynamic, name: String, val: Dynamic): Void;
	static function zui_nodes_get(nodes: Dynamic, name: String): Dynamic;
	static function zui_nodes_set(nodes: Dynamic, name: String, val: Dynamic): Void;
	static function zui_set_on_border_hover(f: Dynamic->Int->Void): Void;
	static function zui_set_on_text_hover(f: Void->Void): Void;
	static function zui_set_on_deselect_text(f: Void->Void): Void;
	static function zui_set_on_tab_drop(f: Dynamic->Int->Dynamic->Int->Void): Void;
	static function zui_nodes_set_enum_texts(f: String->Array<String>): Void;
	static function zui_nodes_set_on_custom_button(f: Int->String->Void): Void;
	static function zui_nodes_set_on_canvas_control(f: Void->Dynamic): Void;
	static function zui_nodes_set_on_canvas_released(f: Void->Void): Void;
	static function zui_nodes_set_on_socket_released(f: Int->Void): Void;
	static function zui_nodes_set_on_link_drag(f: Int->Bool->Void): Void;
}
