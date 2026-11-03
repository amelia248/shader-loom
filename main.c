#include "memory.h"
#include <corecrt.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>
#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

#include <memory.h>
#include <errors.h>
#include <maths.h>

#include "STB/stb-write-image.h"

#include <clay/clay.h>

#define PHOTON_FEATURE__OPENGL_STACK_OPERATIONS
#include "src/photon.h"

#include "app/app.h"

void onResize(int width, int height) {
	// phDelTexture(preTextRender);
	// preTextRender = phNewTexture_render(width, height, GL_NEAREST);
	// phReloadTexture_render(&preTextRender, width, height, GL_NEAREST);

	Clay__debugViewWidth = width / 3;
}

shader_ptr previewShader;
shaderUniformInterface_t previewShaderUniforms[128];
bool canPanAndZoomShader = true;
bool exportShader;
int main(int argc, char *argv[]) {
	// printf("args: ");
	// for (int i = 0; i < argc; i++) {
	// 	printf("\"%s\", ", argv[i]);
	// }
	// printf("\n");
	string_t previewShaderVSPath = newstring("assets/shaders/330/main.vs");
	string_t previewShaderFSPath;
	/* create title & window*/ {
		string_t title = newstring("Shader Debugger");
		if (argc == 2) {
			stringcat_buf(&title, " - ");
			stringcat_buf(&title, argv[1]);
			previewShaderFSPath = newstring(argv[1]);
		} else {
			previewShaderFSPath = newstring("assets/shaders/330/main.fs");
		}
		error_t err = phInitWindow(title.buf, 1500, 900, 60);
		delstring(title);
		if (err) return PH_ERR_FAILURE;
	}

	phSetOnResizeFunction(onResize);
	phSetClearColour(rgb(0x242424));

	previewShader = phNewShaderFromPath(previewShaderVSPath, previewShaderFSPath);
	phAddUniforms(previewShader,
		"model", "view", "projection",
		"iResolution", "iMouse",
		"iTime", "iTimeDelta",
		"iFrameRate", "iFrame", "iDate"
	);
	string_t fragmentShaderData = newstring_readFile(previewShaderFSPath.buf);
	bool atNewline  = false;
	// int  lineNumber = 1;
	for (int i = 0; i < previewShader->uniformsLen; i++) {
		previewShaderUniforms[previewShader->uniforms[i].loc].type = APP_SW_INVALID;
	}
	for (int i = 0; i < fragmentShaderData.len; i++) {
		if ((i == 0 || (i > 0 && fragmentShaderData.buf[i-1] == '\n')) && strncmp(fragmentShaderData.buf + i, "uniform", sizeof("uniform")-1) == 0) {
			char fmt[] = " %s %[^; ]; // %[^\n]";
			int stringsLen = 3;
			char strings[stringsLen][100];
			for (int j=0; j < stringsLen; j++) memset(strings[j], 0, 100);
			sscanf(fragmentShaderData.buf + i + 7, fmt, strings[0], strings[1], strings[2], strings[3], strings[4]);
			// if ()/
			// for (int j=0; j < stringsLen; j++) printf("str[%d]: %s\n", j, strings[j]);
			int uniformLocation = -1;
			for (int j=0; j < previewShader->uniformsLen; j++) {
				if (strcmp(previewShader->uniforms[j].name.buf, strings[1]) == 0) { uniformLocation = previewShader->uniforms[j].loc; break; }
			}
			if (uniformLocation == -1) {
				phAddUniform(previewShader, strings[1]);
				for (int j=0; j < previewShader->uniformsLen; j++) {
					if (strcmp(previewShader->uniforms[j].name.buf, strings[1]) == 0) { uniformLocation = previewShader->uniforms[j].loc; break; }
				}
			}
			if (uniformLocation >= 0 && strings[1][0] == 'u') {
				ShaderWidgetTypes type = -1;
				if      (strcmp(strings[0], "int")   == 0) type = APP_SW_RANGE_INT;
				else if (strcmp(strings[0], "float") == 0) type = APP_SW_RANGE_FLOAT;
				else if (strcmp(strings[0], "vec2")  == 0) type = APP_SW_COORDS;
				else if (strcmp(strings[0], "vec4")  == 0) type = APP_SW_COLOUR_PICKER;
				else type = APP_SW_INVALID;

				previewShaderUniforms[uniformLocation].type = type;
				switch (type) {
				case APP_SW_RANGE_INT: {
					sscanf(strings[2], " %d %d %d ",
						&previewShaderUniforms[uniformLocation].irange.min,
						&previewShaderUniforms[uniformLocation].irange.val,
						&previewShaderUniforms[uniformLocation].irange.max
					);
					shaderUniformInterface_t interface = previewShaderUniforms[uniformLocation];
					previewShaderUniforms[uniformLocation].irange.sliderVal =
						(double)(interface.irange.val - interface.irange.min) / (double)(interface.irange.max - interface.irange.min);
				} break;
				case APP_SW_RANGE_FLOAT: {
					sscanf(strings[2], " %lf %lf %lf ",
						&previewShaderUniforms[uniformLocation].frange.min,
						&previewShaderUniforms[uniformLocation].frange.val,
						&previewShaderUniforms[uniformLocation].frange.max
					);
					shaderUniformInterface_t interface = previewShaderUniforms[uniformLocation];
					previewShaderUniforms[uniformLocation].frange.sliderVal =
						(double)(interface.frange.val - interface.frange.min) / (double)(interface.frange.max - interface.frange.min);
				} break;
				case APP_SW_COORDS: {
					sscanf(strings[2], " (%lf %lf) (%lf %lf) (%lf %lf) ",
						&previewShaderUniforms[uniformLocation].coords.minX, &previewShaderUniforms[uniformLocation].coords.minY,
						&previewShaderUniforms[uniformLocation].coords.x,    &previewShaderUniforms[uniformLocation].coords.y,
						&previewShaderUniforms[uniformLocation].coords.maxX, &previewShaderUniforms[uniformLocation].coords.maxY
					);
					shaderUniformInterface_t interface = previewShaderUniforms[uniformLocation];
					previewShaderUniforms[uniformLocation].coords.sliderX =
						(double)(interface.coords.x - interface.coords.minX) / (double)(interface.coords.maxX - interface.coords.minX);
					previewShaderUniforms[uniformLocation].coords.sliderY =
						(double)(interface.coords.y - interface.coords.minY) / (double)(interface.coords.maxY - interface.coords.minY);
				} break;
				case APP_SW_COLOUR_PICKER: {
					char str[10]; memset(str, 0, 10);
					sscanf(strings[2], " %[^(] ", str);
					if (strcmp(str, "rgba") == 0) {
						sscanf(strings[2], " rgba(%lf, %lf, %lf, %lf)",
							&previewShaderUniforms[uniformLocation].colourPicker[0].val,
							&previewShaderUniforms[uniformLocation].colourPicker[1].val,
							&previewShaderUniforms[uniformLocation].colourPicker[2].val,
							&previewShaderUniforms[uniformLocation].colourPicker[3].val
						);
						previewShaderUniforms[uniformLocation].colourPicker[0].val /= 255.0;
						previewShaderUniforms[uniformLocation].colourPicker[1].val /= 255.0;
						previewShaderUniforms[uniformLocation].colourPicker[2].val /= 255.0;

						previewShaderUniforms[uniformLocation].colourPicker[0].sliderVal = previewShaderUniforms[uniformLocation].colourPicker[0].val;
						previewShaderUniforms[uniformLocation].colourPicker[1].sliderVal = previewShaderUniforms[uniformLocation].colourPicker[1].val;
						previewShaderUniforms[uniformLocation].colourPicker[2].sliderVal = previewShaderUniforms[uniformLocation].colourPicker[2].val;
						previewShaderUniforms[uniformLocation].colourPicker[3].sliderVal = previewShaderUniforms[uniformLocation].colourPicker[3].val;
					}
					else if (strcmp(str, "rgb") == 0) {
						sscanf(strings[2], " rgb(%lf, %lf, %lf)",
							&previewShaderUniforms[uniformLocation].colourPicker[0].val,
							&previewShaderUniforms[uniformLocation].colourPicker[1].val,
							&previewShaderUniforms[uniformLocation].colourPicker[2].val
						);
						previewShaderUniforms[uniformLocation].colourPicker[0].val /= 255.0;
						previewShaderUniforms[uniformLocation].colourPicker[1].val /= 255.0;
						previewShaderUniforms[uniformLocation].colourPicker[2].val /= 255.0;
						previewShaderUniforms[uniformLocation].colourPicker[3].val  = 1;

						previewShaderUniforms[uniformLocation].colourPicker[0].sliderVal = previewShaderUniforms[uniformLocation].colourPicker[0].val;
						previewShaderUniforms[uniformLocation].colourPicker[1].sliderVal = previewShaderUniforms[uniformLocation].colourPicker[1].val;
						previewShaderUniforms[uniformLocation].colourPicker[2].sliderVal = previewShaderUniforms[uniformLocation].colourPicker[2].val;
						previewShaderUniforms[uniformLocation].colourPicker[3].sliderVal = previewShaderUniforms[uniformLocation].colourPicker[3].val;
					}
					// shaderUniformInterface_t interface = previewShaderUniforms[uniformLocation];
					// previewShaderUniforms[uniformLocation].coords.sliderX =
					// 	(double)(interface.coords.x - interface.coords.minX) / (double)(interface.coords.maxX - interface.coords.minX);
					// previewShaderUniforms[uniformLocation].coords.sliderY =
					// 	(double)(interface.coords.y - interface.coords.minY) / (double)(interface.coords.maxY - interface.coords.minY);
				} break;
				case APP_SW_INVALID: {
					// previewShaderUniforms[indexOfUniform].
				} break;
				}
			}
		}
		// if (fragmentShaderData.buf[i-1] == '\n') {
		// 	atNewline = true;
		// 	// lineNumber++;
		// }
		// // previewShader->uniforms.
		// previewShaderUniforms[i] = 
	}

	extern void (*widgetFunctions[])(int, shaderUniformInterface_t*);

	int SIDEBAR_WIDTH = 300;
	int MIN_SBW       = 250;
	int MAX_SBW       = 800;
	
	double lockPanZoomStateTimer;
	

	while (!phWindowShouldClose()) {
		if (phIsKey(GLFW_KEY_SPACE, KeyPressed)) {
			printf("resized to %dx%d\n", phGetWinWidth(), phGetWinHeight());
		}

		phguiBeginLayout();

		CLAY({
			.id = CLAY_ID("main"),
			.layout.layoutDirection = CLAY_LEFT_TO_RIGHT,
			.backgroundColor = PH_COLOUR_BACKGROUND,
			.layout.sizing.width = CLAY_SIZING_GROW(),
			.layout.sizing.height = CLAY_SIZING_GROW(),
		}) {
			CLAY({
				.id = CLAY_ID("main.sidebar"),
				.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.backgroundColor = PH_COLOUR_SIDE_PANEL,
				.layout.sizing.width = CLAY_SIZING_FIXED(SIDEBAR_WIDTH),
				.layout.sizing.height = CLAY_SIZING_GROW(),
				.layout.padding = CLAY_PADDING_ALL(PH_PADDING_A), .layout.childGap = PH_PADDING_A,
			}) {
				CLAY({
					.id = CLAY_ID("main.sidebar.settings"),
					.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.layout.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
					.layout.padding = CLAY_PADDING_ALL(PH_PADDING_A), .layout.childGap = PH_PADDING_A,
					.layout.childAlignment.x = CLAY_ALIGN_X_CENTER,
					// .backgroundColor = PH_COLOUR_SIDE_PANEL_SETTINGS,
				}) {
					/*Pan & Zoom Toggle */ {
						Clay_ElementId panAndZoomToggleID = CLAY_ID("main.sidebar.settings.pan-zoom-toggle");
						double t = 0;
						phguiToggle(panAndZoomToggleID, &t, &lockPanZoomStateTimer, &canPanAndZoomShader);
						CLAY({
							.id = panAndZoomToggleID,
							.layout.sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIXED(32)},
							.floating.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
							.layout.padding = CLAY_PADDING_ALL(PH_PADDING_B),
							.backgroundColor = PH_COLOUR_ELEMENT_BG_A,
							.border = {.width = CLAY_BORDER_ALL(1), .color = phLerpClayColour(phLerpClayColour(PH_COLOUR_OUTLINE, PH_COLOUR_ELEMENT_FG, lockPanZoomStateTimer), PH_COLOUR_ACCENT_A, t) },
							.cornerRadius = CLAY_CORNER_RADIUS(3),
							.layout.childAlignment = {.y = CLAY_ALIGN_Y_CENTER, .x = CLAY_ALIGN_X_CENTER},
							.layout.childGap = PH_PADDING_A
						}) {
							phguiIcon((canPanAndZoomShader) ? PH_ICON_RED_SWITCH_ON : PH_ICON_RED_SWITCH_OFF, 0, rgb(0xfff));
							CLAY_TEXT(
								CLAY_STRING("lock pan & zoom"),
								CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = {255, 255, 255, 255}})
							);
						}
					}

					/* Export Button */ {
						Clay_ElementId exportButtonID = CLAY_ID("main.sidebar.settings.export-button");
						double t = 0;
						exportShader = phguiButton(exportButtonID, &t);
						CLAY({
							.id = exportButtonID,
							.layout.sizing = {CLAY_SIZING_FIT(), CLAY_SIZING_FIXED(32)},
							.floating.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
							.layout.padding = CLAY_PADDING_ALL(PH_PADDING_B),
							.backgroundColor = PH_COLOUR_ELEMENT_BG_A,
							.border = {.width = CLAY_BORDER_ALL(1), .color = phLerpClayColour(PH_COLOUR_OUTLINE, PH_COLOUR_ACCENT_A, t) },
							.cornerRadius = CLAY_CORNER_RADIUS(3),
							.layout.childAlignment = {.y = CLAY_ALIGN_Y_CENTER, .x = CLAY_ALIGN_X_CENTER},
							.layout.childGap = PH_PADDING_A
						}) {
							phguiIcon(PH_ICON_FLOPPY_DISC, 0, rgb(0xfff));
							CLAY_TEXT(
								CLAY_STRING("export as image"),
								CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = {255, 255, 255, 255}})
							);
						}
					}



					CLAY({
						.layout.sizing = {CLAY_SIZING_FIXED(SIDEBAR_WIDTH), CLAY_SIZING_FIXED(2)},
						.backgroundColor = PH_COLOUR_OUTLINE,
						.floating.offset.x = - PH_PADDING_A,
						.floating.attachTo = CLAY_ATTACH_TO_PARENT,
						.floating.attachPoints.parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
					});
				}
				CLAY({
					.id = CLAY_ID("main.sidebar.widgets"),
					.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.layout.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
					.layout.childGap = PH_PADDING_A,
					// .layout.childAlignment.x = CLAY_ALIGN_X_CENTER,
					.clip = {.vertical = true, .childOffset = Clay_GetScrollOffset()},
					
				}) {
					phGLPushShader(previewShader);
					for (int i = 0; i < previewShader->uniformsLen; i++) {
						if (previewShader->uniforms[i].loc < 0) continue;
						int n = previewShader->uniforms[i].loc;
						if (previewShaderUniforms[n].type == APP_SW_INVALID) continue;
						void (*f)(int, shaderUniformInterface_t*) = widgetFunctions[previewShaderUniforms[n].type];
						if (f != nullptr) f(i, &previewShaderUniforms[n]);

						switch (previewShaderUniforms[n].type) {
						case APP_SW_RANGE_INT: {
							phSetUniformf(previewShader->uniforms[i].name.buf, 'i', previewShaderUniforms[n].irange.val);
						} break;

						case APP_SW_RANGE_FLOAT: {
							phSetUniformf(previewShader->uniforms[i].name.buf, 'f', previewShaderUniforms[n].frange.val);
						} break;

						case APP_SW_COORDS: {
							phSetUniformf(previewShader->uniforms[i].name.buf, 'v2', previewShaderUniforms[n].coords.x, previewShaderUniforms[n].coords.y);
						} break;

						case APP_SW_COLOUR_PICKER: {
							phSetUniformf(previewShader->uniforms[i].name.buf, 'v4',
								previewShaderUniforms[n].colourPicker[0].val,
								previewShaderUniforms[n].colourPicker[1].val,
								previewShaderUniforms[n].colourPicker[2].val,
								previewShaderUniforms[n].colourPicker[3].val
							);
						} break;

						default:
						}
					}
					phGLPopShader();
				}
				// widgetFunctions[APP_SW_RANGE_INT]  (0, &val,  0, 200, &xVal);
				// widgetFunctions[APP_SW_RANGE_FLOAT](1, &fval, -.5, 2.4, &xfVal);
				// widgetFunctions[APP_SW_COORDS](2, &cvalx, &cvaly,  -1.0,-1.0,  1.0,1.0, &xcvalx, &xcvaly);
				// printf("%d, %.2f, (%.2f, %.2f)\n", val, fval, cvalx,cvaly);

				/* sidebar sizer */ {
					Clay_ElementId sidebarSizerID       = CLAY_ID("main.sidebar.sizer");
					Clay_ElementId sidebarSizerVisualID = CLAY_ID("main.sidebar.sizer.visual");
					double sidebarSizerTimer;
					CLAY({
						.id = sidebarSizerID,
						.backgroundColor = PH_COLOUR_TRANSPARENT,
						.layout.sizing.width = CLAY_SIZING_FIXED(6),
						.layout.sizing.height = CLAY_SIZING_GROW(),
						.floating = {
							.attachTo = CLAY_ATTACH_TO_PARENT,
							.attachPoints = {.parent = CLAY_ATTACH_POINT_RIGHT_TOP},
							.offset = {.x=-3, .y=0},
						},
					}) {
						phguiGrabbable(sidebarSizerID, phGetMouseDelta().x, &sidebarSizerTimer, &SIDEBAR_WIDTH, nullptr, nullptr);
						// printf("%d\n", delta);
						SIDEBAR_WIDTH = phClamp_int(SIDEBAR_WIDTH, MIN_SBW, MAX_SBW);
					}
					CLAY({
						.id = sidebarSizerVisualID,
						.backgroundColor = phLerpClayColour(PH_COLOUR_OUTLINE, PH_COLOUR_ACCENT_A, sidebarSizerTimer),
						.layout.sizing.width = CLAY_SIZING_FIXED(2 + 2*sidebarSizerTimer),
						.layout.sizing.height = CLAY_SIZING_GROW(),
						.floating = {
							.attachTo = CLAY_ATTACH_TO_PARENT,
							.attachPoints = {.parent = CLAY_ATTACH_POINT_RIGHT_TOP},
							.offset = {.x=-(1 + 1*sidebarSizerTimer), .y=0},
							.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
						},
					});
				}
			}

			CLAY({
				.id = CLAY_ID("main.shader-area"),
				.clip = {.horizontal = true, .vertical = true},
				.backgroundColor = PH_COLOUR_TRANSPARENT,
				.layout.sizing.width = CLAY_SIZING_GROW(),
				.layout.sizing.height = CLAY_SIZING_GROW(),
				.border.color = PH_COLOUR_NOT_ALLOWED_RED,
				.border.width = CLAY_BORDER_ALL(lockPanZoomStateTimer*4),
			}) {
				/* shader preview */ {
					Clay_ElementId shaderPreviewID = CLAY_ID("main.shader-area.preview");
					dvec2_t pan = {200,200};
					double  zoom = 1;
					phguiPanAndZoom(CLAY_ID("main.shader-area"), shaderPreviewID, &pan, &zoom, !canPanAndZoomShader);
					// printf("%f\n", zoom);
					
					CLAY({
						.id = shaderPreviewID,
						.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
						.backgroundColor = clayRGB(0x0f8),
						.layout.sizing.width = CLAY_SIZING_FIXED(800 * zoom),
						.layout.sizing.height = CLAY_SIZING_FIXED(800 * zoom),
						// .border = {.width={1,1,1,1}, .color = PH_COLOUR_ACCENT_A},
						
						.floating = {
							.attachTo = CLAY_ATTACH_TO_PARENT,
							.attachPoints.element = CLAY_ATTACH_POINT_CENTER_CENTER,
							.attachPoints.parent  = CLAY_ATTACH_POINT_CENTER_CENTER,
							.offset = {pan.x,pan.y},
							.clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
							.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
						},
						// .custom.customData = (void*)1,
						.userData = (void*)1
					}) {}
				}
			}

		}
		bool includeMouseInExport = false;
		if (phIsKey(GLFW_KEY_LEFT_CONTROL, KeyDown) || phIsKey(GLFW_KEY_RIGHT_CONTROL, KeyDown)) {
			if (phIsKey(GLFW_KEY_S, KeyPressed)) {
			exportShader = true;
			includeMouseInExport = true;
			}
		}

		if (exportShader) {
			exportShader = false;

			texture_ptr ren = phNewTexture_render(800, 800, GL_POINT);

			glBindFramebuffer(GL_FRAMEBUFFER, ren->ren.framebuffer);
			phSetClearColour(rgb(0x0000));
			glClear(GL_COLOR_BUFFER_BIT);

			
			// extern shader_ptr previewShader;
			// Clay_RectangleRenderData renderData = cmd.renderData.rectangle;
			irec32_t bb = (irec32_t){.x=0,.y=ren->height + (phGetWinHeight() - ren->height), .w=ren->width, .h=-ren->height};
			window_t* win = phGetWindow();
			// colour_t backgroundColour = phClayColourToPhColour(renderData.backgroundColor);
			// colour_t borderColour = backgroundColour; borderColour.a = 0;
			
			phGLPushShader(previewShader);
			phGLPushMesh   (   win->primitives.quadMesh       );
			phGLPushTexture(0, win->primitives.blankTex       );
			// phSetUniformf("view",       'm4x4', win->primitives.matrices.view);
			// phSetUniformf("projection", 'm4x4', win->primitives.matrices.projection);
			
			mesh_ptr m = phGLPeekMesh();
			phSetMatrixToRec(&win->primitives.matrices.model, bb);
			// float* modelDat = memalloc(sizeof(float_t) * 4 * 4);
			// matrix_t model = {.n = 4,.m = 4, .data = modelDat};

			// model.data[0*4 + 0] = 
			phSetUniformf("model",      'm4x4', win->primitives.matrices.model);
			phSetUniformf("view",       'm4x4', win->primitives.matrices.view);
			phSetUniformf("projection", 'm4x4', win->primitives.matrices.projection);

			phSetUniformf("iResolution", 'v3', (double)ren->width, (double)ren->height, 1.0);
			double time = phGetTime();
			phSetUniformf("iTime",       '1f',  time);
			// printf("%f\n", time);
			// phSetUniformf(c_string_t name, uint32_t fmt, ...)
			phSetUniformf("iTimeDelta",  '1f',  (double)phGetDeltaTime());
			phSetUniformf("iFrameRate",  '1f',  (double)(1./phGetDeltaTime()));
			phSetUniformf("iFrame",      'i',  (int)(phGetFrame()));
			if (includeMouseInExport) {
				extern fvec2_t mousePosRelativeToShaderArea;
				bool isDown, clicked;
				isDown =
					phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MouseDown)   ||
					phIsMouseButton(GLFW_MOUSE_BUTTON_MIDDLE, MouseDown) ||
					phIsMouseButton(GLFW_MOUSE_BUTTON_RIGHT, MouseDown)  ;
				clicked =
					phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MousePressed)   ||
					phIsMouseButton(GLFW_MOUSE_BUTTON_MIDDLE, MousePressed) ||
					phIsMouseButton(GLFW_MOUSE_BUTTON_RIGHT, MousePressed)  ;
				phSetUniformf("iMouse",      'v4', mousePosRelativeToShaderArea.x * ren->width, mousePosRelativeToShaderArea.y * ren->height, (double)isDown, (double)clicked);
			}
			else phSetUniformf("iMouse",      'v4', 0, 0, 0, 0);
			// phSetUniformf("uColour",       'rgba', backgroundColour);
			glDrawElements(GL_TRIANGLES, m->indicesLen, GL_UNSIGNED_SHORT, NULL);
			phGLPopTexture(0);
			phGLPopMesh   ( );
			phGLPopShader ( );
			void* data = memalloc(ren->width * ren->height * 4);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, ren->texture);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glBindTexture(GL_TEXTURE_2D, 0);
			stbi_write_png("exported shader.png", ren->width, ren->height, 4, data, 0);
			// stbi_write_bmp("ttf-cache.bmp", textureSize, textureSize, bpp, glyphTextureData);


			// printf("export shader!!");

			memfree(data);
		}

		phguiEndLayout();
		
		phBeginDrawing();
			phguiRender();
		phEndDrawing();
	}

	phCloseWindow();
	
	return 0;
}