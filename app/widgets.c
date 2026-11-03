#include "memory.h"
#include <corecrt.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
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

#include <clay/clay.h>

#include "../src/photon.h"

#include "app.h"

void (*widgetFunctions[])(int, shaderUniformInterface_t*) = {
	widgetRangeInt,
	widgetRangeFloat,
	widgetCoords,
	widgetColourPicker,
	nullptr
};


// id, value, min, max, intermediate x value
void widgetRangeInt(int id, shaderUniformInterface_t* interface) {
	
	int*    rValue = &interface->irange.val;
	int     rMin   =  interface->irange.min;
	int     rMax   =  interface->irange.max;
	double* xValue = &interface->irange.sliderVal;
	
	Clay_ElementId widgetID = CLAY_IDI("app.widgets.range-int", id);
	extern shader_ptr previewShader;
	string_t coordStr;
	CLAY({
		.layout.sizing.width = CLAY_SIZING_GROW(),
		.layout.sizing.height = CLAY_SIZING_FIT(),
		.backgroundColor = PH_COLOUR_ELEMENT_BG_A,
		.border = {.width={1,1,1,1}, .color = PH_COLOUR_OUTLINE},
		.layout.padding = CLAY_PADDING_ALL(PH_PADDING_B), .layout.childGap = PH_PADDING_B,
		.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
	}) {
		Clay_String str = (Clay_String){.chars = previewShader->uniforms[id].name.buf, .length = previewShader->uniforms[id].name.len, .isStaticallyAllocated = false};
		CLAY({}) {
			CLAY_TEXT(CLAY_STRING("int "), CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = PH_VARIABLE_TYPE_COLOUR}));
			CLAY_TEXT(
				str,
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = {255, 255, 255, 255}})
			);
		}

		CLAY({
			.id = widgetID,
			.layout.sizing.width = CLAY_SIZING_GROW(),
			.layout.sizing.height = CLAY_SIZING_FIT(),
			.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER,
			.layout.childGap = PH_PADDING_B,
		}) {
			// string_t sMin = new
			
			bool metaExisted;
			elementMetadata_t* meta = phguiGetElementMetadata(widgetID, (elementMetadata_t){}, &metaExisted);
			string_t minStr, maxStr;
			if (!metaExisted) {
				minStr = newstring_len(100); sprintf(minStr.buf, "%d", rMin);
				maxStr = newstring_len(100); sprintf(maxStr.buf, "%d", rMax);
				coordStr = newstring_len(100); sprintf(coordStr.buf, "value: %d", *rValue);
				cacheAddress(&meta->addresses, minStr.buf);
				cacheAddress(&meta->addresses, maxStr.buf);
				cacheAddress(&meta->addresses, coordStr.buf);
			} else {
				minStr = (string_t){.buf = meta->addresses.addresses[0], .len = strlen(meta->addresses.addresses[0])};
				maxStr = (string_t){.buf = meta->addresses.addresses[1], .len = strlen(meta->addresses.addresses[1])};
				coordStr = (string_t){.buf = meta->addresses.addresses[2], .len = strlen(meta->addresses.addresses[2])};
				sprintf(coordStr.buf, "value: %d", *rValue);
				coordStr.len = strlen(coordStr.buf);
			}
			Clay_String str = (Clay_String){.chars = minStr.buf, .length = minStr.len, .isStaticallyAllocated = false};
			CLAY_TEXT(
				str,
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = {255, 255, 255, 255}})
			);

			Clay_ElementId sliderID = CLAY_IDI("app.widgets.range-int.slider", id);
			CLAY({
				.id = sliderID,
				.layout.sizing.width = CLAY_SIZING_GROW(),
				.layout.sizing.height = CLAY_SIZING_FIXED(12),
				.backgroundColor = PH_COLOUR_ELEMENT_BG_B,
			}) {
				Clay_ElementId draggableID = CLAY_IDI("app.widgets.range-int.draggable", id);
				
				double timer = 0;
				phguiSlider(draggableID, phGetMouseDelta().x / (double)(Clay_GetElementData(sliderID).boundingBox.width - 12), &timer, xValue, nullptr);
				if (phGetTime() > 1) *xValue = phClamp_double(*xValue, 0, 1);
				*rValue = rMin + (*xValue) * (double)(rMax - rMin);

				CLAY({
					.id = draggableID,
					.layout.sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)},
					.backgroundColor = phLerpClayColour(PH_COLOUR_ELEMENT_FG, PH_COLOUR_ACCENT_A, timer),
					.floating = {
						.attachTo = CLAY_ATTACH_TO_PARENT,
						.attachPoints.parent = CLAY_ATTACH_POINT_LEFT_TOP,
						.offset = {
							(*xValue) * (Clay_GetElementData(sliderID).boundingBox.width - 12),
						},
						.clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
					},
				});
			}
			str = (Clay_String){.chars = maxStr.buf, .length = maxStr.len, .isStaticallyAllocated = false};
			CLAY_TEXT(
				str,
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = {255, 255, 255, 255}})
			);
		}
		str = (Clay_String){.chars = coordStr.buf, .length = coordStr.len, .isStaticallyAllocated = false};
		CLAY_TEXT(str,CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff), .textAlignment = CLAY_TEXT_ALIGN_CENTER, .wrapMode = CLAY_TEXT_WRAP_NONE}));
	}
}

void widgetRangeFloat(int id, shaderUniformInterface_t* interface) {
	
	double* rValue = &interface->frange.val;
	double  rMin   =  interface->frange.min;
	double  rMax   =  interface->frange.max;
	double* xValue = &interface->frange.sliderVal;
	
	Clay_ElementId widgetID = CLAY_IDI("app.widgets.range-float", id);
	extern shader_ptr previewShader;
	string_t coordStr;
	CLAY({
		.layout.sizing.width = CLAY_SIZING_GROW(),
		.layout.sizing.height = CLAY_SIZING_FIT(),
		.backgroundColor = PH_COLOUR_ELEMENT_BG_A,
		.border = {.width={1,1,1,1}, .color = PH_COLOUR_OUTLINE},
		.layout.padding = CLAY_PADDING_ALL(PH_PADDING_B), .layout.childGap = PH_PADDING_B,
		.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
	}) {
		Clay_String str = (Clay_String){.chars = previewShader->uniforms[id].name.buf, .length = previewShader->uniforms[id].name.len, .isStaticallyAllocated = false};
		CLAY({}) {
			CLAY_TEXT(
				CLAY_STRING("float "),
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = PH_VARIABLE_TYPE_COLOUR}));
			CLAY_TEXT(
				str,
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = {255, 255, 255, 255}})
			);
		}

		CLAY({
			.id = widgetID,
			.layout.sizing.width = CLAY_SIZING_GROW(),
			.layout.sizing.height = CLAY_SIZING_FIT(),
			.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER,
			.layout.childGap = PH_PADDING_B,
		}) {
			// string_t sMin = new
			
			bool metaExisted;
			elementMetadata_t* meta = phguiGetElementMetadata(widgetID, (elementMetadata_t){}, &metaExisted);
			string_t minStr, maxStr;
			if (!metaExisted) {
				minStr = newstring_len(100); sprintf(minStr.buf, "%.2f", rMin);
				maxStr = newstring_len(100); sprintf(maxStr.buf, "%.2f", rMax);
				coordStr = newstring_len(100); sprintf(coordStr.buf, "value: %.2f", *rValue);
				cacheAddress(&meta->addresses, minStr.buf);
				cacheAddress(&meta->addresses, maxStr.buf);
				cacheAddress(&meta->addresses, coordStr.buf);
			} else {
				minStr = (string_t){.buf = meta->addresses.addresses[0], .len = strlen(meta->addresses.addresses[0])};
				maxStr = (string_t){.buf = meta->addresses.addresses[1], .len = strlen(meta->addresses.addresses[1])};
				coordStr = (string_t){.buf = meta->addresses.addresses[2], .len = strlen(meta->addresses.addresses[2])};
				sprintf(coordStr.buf, "value: %.2f", *rValue);
				coordStr.len = strlen(coordStr.buf);
			}
			Clay_String str = (Clay_String){.chars = minStr.buf, .length = minStr.len, .isStaticallyAllocated = false};
			CLAY_TEXT(
				str,
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff)})
			);

			Clay_ElementId sliderID = CLAY_IDI("app.widgets.range-float.slider", id);
			CLAY({
				.id = sliderID,
				.layout.sizing.width = CLAY_SIZING_GROW(),
				.layout.sizing.height = CLAY_SIZING_FIXED(12),
				.backgroundColor = PH_COLOUR_ELEMENT_BG_B,
			}) {
				Clay_ElementId draggableID = CLAY_IDI("app.widgets.range-float.draggable", id);
				
				double timer = 0;
				phguiSlider(draggableID, phGetMouseDelta().x / (double)(Clay_GetElementData(sliderID).boundingBox.width - 12), &timer, xValue, nullptr);
				if (phGetTime() > 1) *xValue = phClamp_double(*xValue, 0, 1);
				*rValue = rMin + (*xValue) * (double)(rMax - rMin);

				CLAY({
					.id = draggableID,
					.layout.sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)},
					.backgroundColor = phLerpClayColour(PH_COLOUR_ELEMENT_FG, PH_COLOUR_ACCENT_A, timer),
					.floating = {
						.attachTo = CLAY_ATTACH_TO_PARENT,
						.attachPoints.parent = CLAY_ATTACH_POINT_LEFT_TOP,
						.offset = {
							(*xValue) * (Clay_GetElementData(sliderID).boundingBox.width - 12),
							// (*xValueY) * (Clay_GetElementData(sliderID).boundingBox.height - 12)
						},
						.clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
					},
				});
				// printf("%f\n", Clay_GetElementData(sliderID).boundingBox.width);
			}
			str = (Clay_String){.chars = maxStr.buf, .length = maxStr.len, .isStaticallyAllocated = false};
			CLAY_TEXT(
				str,
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff)})
			);
		}
		str = (Clay_String){.chars = coordStr.buf, .length = coordStr.len, .isStaticallyAllocated = false};
		CLAY_TEXT(str,CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff), .textAlignment = CLAY_TEXT_ALIGN_CENTER, .wrapMode = CLAY_TEXT_WRAP_NONE}));
	}
}
// value(xy), min(xy), max(xy), int intermediate xy values
void widgetCoords(int id, shaderUniformInterface_t* interface) {

	// va_list args;
	// va_start(args, id);
	double* cValueX = &interface->coords.x;    double* cValueY = &interface->coords.y;
	double  cMinX   =  interface->coords.minX; double  cMinY   =  interface->coords.minY;
	double  cMaxX   =  interface->coords.maxX; double  cMaxY   =  interface->coords.maxY;
	
	double* xValueX = &interface->coords.sliderX; double* xValueY = &interface->coords.sliderY;
	// va_end(args);
	
	Clay_ElementId widgetID = CLAY_IDI("app.widgets.coords", id);
	Clay_ElementId sliderID = CLAY_IDI("app.widgets.coords.slider", id);
	Clay_ElementId draggableXID = CLAY_IDI("app.widgets.coords.draggable-x", id);
	Clay_ElementId draggableYID = CLAY_IDI("app.widgets.coords.draggable-y", id);

	bool metaExisted;
	elementMetadata_t* meta = phguiGetElementMetadata(widgetID, (elementMetadata_t){}, &metaExisted);
	string_t minXStr,minYStr, maxXStr,maxYStr, coordStr;
	if (!metaExisted) {
		minXStr  = newstring_len(100); sprintf(minXStr.buf,  "%.1f", cMinX);
		minYStr  = newstring_len(100); sprintf(minYStr.buf,  "%.1f", cMinY);
		maxXStr  = newstring_len(100); sprintf(maxXStr.buf,  "%.1f", cMaxX);
		maxYStr  = newstring_len(100); sprintf(maxYStr.buf,  "%.1f", cMaxY);
		coordStr = newstring_len(100); sprintf(coordStr.buf, "value: (%.1f %.1f)", *cValueX, *cValueY);
		cacheAddress(&meta->addresses, minXStr.buf);
		cacheAddress(&meta->addresses, minYStr.buf);
		cacheAddress(&meta->addresses, maxXStr.buf);
		cacheAddress(&meta->addresses, maxYStr.buf);
		cacheAddress(&meta->addresses, coordStr.buf);
	} else {
		minXStr  = (string_t){.buf = meta->addresses.addresses[0], .len = strlen(meta->addresses.addresses[0])};
		minYStr  = (string_t){.buf = meta->addresses.addresses[1], .len = strlen(meta->addresses.addresses[1])};
		maxXStr  = (string_t){.buf = meta->addresses.addresses[2], .len = strlen(meta->addresses.addresses[2])};
		maxYStr  = (string_t){.buf = meta->addresses.addresses[3], .len = strlen(meta->addresses.addresses[3])};
		coordStr = (string_t){.buf = meta->addresses.addresses[4], .len = strlen(meta->addresses.addresses[4])};
		sprintf(coordStr.buf, "value: (%.1f %.1f)", *cValueX, *cValueY);
		coordStr.len = strlen(coordStr.buf);
	}
	extern shader_ptr previewShader;
	CLAY({
		.layout.sizing.width = CLAY_SIZING_GROW(),
		.layout.sizing.height = CLAY_SIZING_FIT(),
		.backgroundColor = PH_COLOUR_ELEMENT_BG_A,
		.border = {.width={1,1,1,1}, .color = PH_COLOUR_OUTLINE},
		.layout.padding = CLAY_PADDING_ALL(PH_PADDING_B), .layout.childGap = PH_PADDING_B,
		.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
	}) {
		Clay_String str = (Clay_String){.chars = previewShader->uniforms[id].name.buf, .length = previewShader->uniforms[id].name.len, .isStaticallyAllocated = false};
		CLAY({}) {
			CLAY_TEXT(
				CLAY_STRING("vec2 "),
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = PH_VARIABLE_TYPE_COLOUR})
			);
			CLAY_TEXT(
				str,
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = {255, 255, 255, 255}})
			);
		}
		CLAY({
			.id = widgetID,
			.layout.sizing.width = CLAY_SIZING_GROW(),
			.layout.sizing.height = CLAY_SIZING_FIT(),
			.layout.childAlignment.x = CLAY_ALIGN_X_CENTER,
			.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER,
			.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
			.layout.childGap = PH_PADDING_B,
		}) {
			str = (Clay_String){.chars = maxYStr.buf, .length = maxYStr.len, .isStaticallyAllocated = false};
			CLAY_TEXT(str,CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff)}));

			
			CLAY({
				.layout.sizing.width = CLAY_SIZING_GROW(),
				.layout.sizing.height = CLAY_SIZING_FIT(),
				.layout.childAlignment.x = CLAY_ALIGN_X_CENTER,
				.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER,
				.layout.layoutDirection = CLAY_LEFT_TO_RIGHT,
				.layout.childGap = PH_PADDING_B,
			}) {
				Clay_String str = (Clay_String){.chars = minXStr.buf, .length = minXStr.len, .isStaticallyAllocated = false};
				CLAY_TEXT(str,CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff), .textAlignment=CLAY_TEXT_ALIGN_CENTER}));

				CLAY({
					.id = sliderID,
					.layout.sizing.width = CLAY_SIZING_GROW(.min = 100),
					.aspectRatio.aspectRatio = (cMaxX-cMinX) / (cMaxY-cMinY),
					.backgroundColor = PH_COLOUR_ELEMENT_BG_B,
				}) {
					
					float lineSpacings[] = {0, 1, .75, .5, .25};
					for (int i = sizeof(lineSpacings) / sizeof(float) - 1; i >= 0; i--) {
						CLAY({
							.layout.sizing.width = CLAY_SIZING_FIXED( i==0 ? 1 : (Clay_GetElementData(sliderID).boundingBox.width*lineSpacings[i])),
							.layout.sizing.height = CLAY_SIZING_FIXED(Clay_GetElementData(sliderID).boundingBox.height),
							// .backgroundColor = phLerpClayColour(PH_COLOUR_ELEMENT_FG, PH_COLOUR_ACCENT_A, timer),
							.border = {.width=CLAY_BORDER_ALL(1), .color = i==0 ? clayRGB(0xccc) : clayRGB(0x888)},
							.floating = {
								.attachTo = CLAY_ATTACH_TO_PARENT,
								.attachPoints.element = CLAY_ATTACH_POINT_CENTER_TOP,
								.attachPoints.parent = CLAY_ATTACH_POINT_CENTER_TOP,
								.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
								.clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
							},
							
						});
						CLAY({
							.layout.sizing.width = CLAY_SIZING_FIXED(Clay_GetElementData(sliderID).boundingBox.width),
							.layout.sizing.height = CLAY_SIZING_FIXED( i==0 ? 1 : (Clay_GetElementData(sliderID).boundingBox.height*lineSpacings[i])),
							// .backgroundColor = phLerpClayColour(PH_COLOUR_ELEMENT_FG, PH_COLOUR_ACCENT_A, timer),
							.border = {.width=CLAY_BORDER_ALL(1), .color = i==0 ? clayRGB(0xccc) : clayRGB(0x888)},
							.floating = {
								.attachTo = CLAY_ATTACH_TO_PARENT,
								.attachPoints.element = CLAY_ATTACH_POINT_LEFT_CENTER,
								.attachPoints.parent = CLAY_ATTACH_POINT_LEFT_CENTER,
								.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
								.clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
							},
						});
					}
					double timer = 0;

					phguiSlider(draggableXID, phGetMouseDelta().x / (double)(Clay_GetElementData(sliderID).boundingBox.width - 12), &timer, xValueX, nullptr);
					if (phGetTime() > 1) *xValueX = phClamp_double(*xValueX, 0, 1);
					*cValueX = cMinX + (*xValueX) * (double)(cMaxX - cMinX);

					phguiSlider(draggableYID, phGetMouseDelta().y / (double)(Clay_GetElementData(sliderID).boundingBox.height - 12), &timer, xValueY, nullptr);
					if (phGetTime() > 1) *xValueY = phClamp_double(*xValueY, 0, 1);
					*cValueY = cMinY + (*xValueY) * (double)(cMaxY - cMinY);

					// phguiGrabbable(draggableXID, phGetMouseDelta().x, &timer, xValueX, nullptr, nullptr);
					// *xValueX = phClamp_int(*xValueX, 0, Clay_GetElementData(sliderID).boundingBox.width - 12);
					// *cValueX = cMinX + (double)(*xValueX) * (double)((cMaxX - cMinX) / (Clay_GetElementData(sliderID).boundingBox.width - 12));

					// phguiGrabbable(draggableYID, phGetMouseDelta().y, &timer, xValueY, nullptr, nullptr);
					// *xValueY = phClamp_int(*xValueY, 0, Clay_GetElementData(sliderID).boundingBox.height - 12);
					// *cValueY = cMaxY + (double)(*xValueY) * (double)((cMinY - cMaxY) / (Clay_GetElementData(sliderID).boundingBox.height - 12));

					CLAY({
						.id = draggableXID,
						.layout.sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)},
						.backgroundColor = phLerpClayColour(PH_COLOUR_ELEMENT_FG, PH_COLOUR_ACCENT_A, timer),
						.floating = {
							.attachTo = CLAY_ATTACH_TO_PARENT,
							.attachPoints.parent = CLAY_ATTACH_POINT_LEFT_TOP,
							.offset = {
								(*xValueX) * (Clay_GetElementData(sliderID).boundingBox.width - 12),
								(*xValueY) * (Clay_GetElementData(sliderID).boundingBox.height - 12)
							},
							.clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
						},
					});

					CLAY({
						.id = draggableYID,
						.layout.sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)},
						.backgroundColor = phLerpClayColour(PH_COLOUR_ELEMENT_FG, PH_COLOUR_ACCENT_A, timer),
						.floating = {
							.attachTo = CLAY_ATTACH_TO_PARENT,
							.attachPoints.parent = CLAY_ATTACH_POINT_LEFT_TOP,
							.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
							.offset = {
								(*xValueX) * (Clay_GetElementData(sliderID).boundingBox.width - 12),
								(*xValueY) * (Clay_GetElementData(sliderID).boundingBox.height - 12)
							},
							.clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
						},
					});

					// printf("%f\n", Clay_GetElementData(sliderID).boundingBox.width);
				}
				str = (Clay_String){.chars = maxXStr.buf, .length = maxXStr.len, .isStaticallyAllocated = false};
				CLAY_TEXT(str,CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff)}));
			}
			str = (Clay_String){.chars = minYStr.buf, .length = minYStr.len, .isStaticallyAllocated = false};
			CLAY_TEXT(str,CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff), .textAlignment=CLAY_TEXT_ALIGN_CENTER}));
		}
		str = (Clay_String){.chars = coordStr.buf, .length = coordStr.len, .isStaticallyAllocated = false};
		CLAY_TEXT(str,CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff), .textAlignment = CLAY_TEXT_ALIGN_CENTER, .wrapMode = CLAY_TEXT_WRAP_NONE}));
	}
}
bool colourPicker(int* red, int* green, int* blue);

void widgetColourPicker(int id, shaderUniformInterface_t* interface) {
	
	// double* rValue = &interface->frange.val;
	// double  rMin   =  interface->frange.min;
	// double  rMax   =  interface->frange.max;
	// int*    xValue = &interface->frange.tempSliderVal;
	
	string_t colStr;
	Clay_ElementId widgetID = CLAY_IDI("app.widgets.colour-picker", id);

	bool metaExisted;
	elementMetadata_t* meta = phguiGetElementMetadata(widgetID, (elementMetadata_t){}, &metaExisted);
	if (!metaExisted) {
		colStr = newstring_len(100); sprintf(colStr.buf, "alpha: %.2f", interface->colourPicker[3].val);
		cacheAddress(&meta->addresses, colStr.buf);
	} else {
		colStr = (string_t){.buf = meta->addresses.addresses[0], .len = strlen(meta->addresses.addresses[0])};
		sprintf(colStr.buf, "alpha: %.2f", interface->colourPicker[3].val);
		colStr.len = strlen(colStr.buf);
	}

	extern shader_ptr previewShader;
	CLAY({
		.id = widgetID,
		.layout.sizing.width = CLAY_SIZING_GROW(),
		.layout.sizing.height = CLAY_SIZING_FIT(),
		.backgroundColor = PH_COLOUR_ELEMENT_BG_A,
		.border = {.width={1,1,1,1}, .color = PH_COLOUR_OUTLINE},
		.layout.padding = CLAY_PADDING_ALL(PH_PADDING_B), .layout.childGap = PH_PADDING_B,
		.layout.layoutDirection = CLAY_TOP_TO_BOTTOM,
	}) {
		Clay_String str = (Clay_String){.chars = previewShader->uniforms[id].name.buf, .length = previewShader->uniforms[id].name.len, .isStaticallyAllocated = false};
		CLAY({}) {
			CLAY_TEXT(
				CLAY_STRING("vec4 "),
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = PH_VARIABLE_TYPE_COLOUR}));
			CLAY_TEXT(
				str,
				CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = {255, 255, 255, 255}})
			);
		}
		Clay_Color chanelColours[4] = {clayRGB(0xf24), clayRGB(0x2f4), clayRGB(0x24f), clayRGB(0xeff)};
		for (int chanel = 0; chanel < 4; chanel++) {
			CLAY({
				.layout.sizing.width = CLAY_SIZING_GROW(),
				.layout.sizing.height = CLAY_SIZING_FIT(),
				.layout.childAlignment.y = CLAY_ALIGN_Y_CENTER,
				.layout.childGap = PH_PADDING_B,
			}) {
				CLAY({
					.layout.sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)},
					.backgroundColor = chanelColours[chanel],
				});

				Clay_ElementId sliderID = CLAY_IDI("app.widgets.colour-picker.slider", id*4 + chanel);
				CLAY({
					.id = sliderID,
					.layout.sizing.width = CLAY_SIZING_GROW(),
					.layout.sizing.height = CLAY_SIZING_FIXED(12),
					.backgroundColor = PH_COLOUR_ELEMENT_BG_B,
				}) {

					Clay_ElementId draggableID = CLAY_IDI("app.widgets.colour-picker.draggable", id*4 + chanel);
					double timer = 0;
					double* rValue = &interface->colourPicker[chanel].val;
					double rMin =     0;
					double rMax =     1;
					double* xValue = &interface->colourPicker[chanel].sliderVal;
					bool active = phguiSlider(draggableID, phGetMouseDelta().x / (double)(Clay_GetElementData(sliderID).boundingBox.width - 12), &timer, xValue, nullptr);
					if (phGetTime() > 1) *xValue = phClamp_double(*xValue, 0, 1);
					if (active) {
						*rValue = rMin + (*xValue) * (double)(rMax - rMin);
					}
					

					CLAY({
						.id = draggableID,
						.layout.sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)},
						.backgroundColor = phLerpClayColour(PH_COLOUR_ELEMENT_FG, PH_COLOUR_ACCENT_A, timer),
						.floating = {
							.attachTo = CLAY_ATTACH_TO_PARENT,
							.attachPoints.parent = CLAY_ATTACH_POINT_LEFT_TOP,
							.offset = {
								(*xValue) * (Clay_GetElementData(sliderID).boundingBox.width - 12),
							},
							.clipTo = CLAY_CLIP_TO_ATTACHED_PARENT,
						},
					});
				}
			}
		}
		CLAY({.layout.layoutDirection = CLAY_LEFT_TO_RIGHT, .layout.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()}}) {
			Clay_ElementId pickableID = CLAY_IDI("app.widgets.colour-picker.pickable", id);
			double t = 0;
			if (phguiButton(pickableID, &t)) {
				int r, g, b;
				r = interface->colourPicker[0].val * 255;
				g = interface->colourPicker[1].val * 255;
				b = interface->colourPicker[2].val * 255;
				colourPicker(&r, &g, &b);
				interface->colourPicker[0].val = (double)(r)/255.;
				interface->colourPicker[1].val = (double)(g)/255.;
				interface->colourPicker[2].val = (double)(b)/255.;
				interface->colourPicker[0].sliderVal = interface->colourPicker[0].val;
				interface->colourPicker[1].sliderVal = interface->colourPicker[1].val;
				interface->colourPicker[2].sliderVal = interface->colourPicker[2].val;
				for (int i = 0; i < 3; i++) {
					elementMetadata_ptr meta = phguiGetElementMetadata(CLAY_IDI("app.widgets.colour-picker.draggable", id*4 + i), (elementMetadata_t){0}, nullptr);
					meta->slider.value = interface->colourPicker[i].val;

				}

			}
			CLAY({
				.id = pickableID,
				.layout.sizing = {CLAY_SIZING_FIXED(48), CLAY_SIZING_FIXED(24)},
				.backgroundColor = phLerpClayColour((Clay_Color){
					.r = interface->colourPicker[0].val * 255,
					.g = interface->colourPicker[1].val * 255,
					.b = interface->colourPicker[2].val * 255,
					.a = interface->colourPicker[3].val * 255,
				}, clayRGB(0x888), t*.5),
				.border.color = phLerpClayColour(PH_COLOUR_BLANK, PH_COLOUR_OUTLINE, t),
				.layout.childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
			}) {
				phguiIcon(PH_ICON_PENCIL, 0, phLerpColour(rgb(0xfff0), rgb(0xffff), t));
			}
			CLAY({.layout.sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIXED(12)},});

			str = (Clay_String){.chars = colStr.buf, .length = colStr.len, .isStaticallyAllocated = false};
			CLAY_TEXT(str,CLAY_TEXT_CONFIG({.fontId = 0, .fontSize = 24, .textColor = clayRGB(0xfff), .textAlignment = CLAY_TEXT_ALIGN_RIGHT, .wrapMode = CLAY_TEXT_WRAP_NONE}));
		}
	}
}
