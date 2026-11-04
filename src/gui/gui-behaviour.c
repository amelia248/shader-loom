#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>


#include <memory.h>
#include <maths.h>

#define PHOTON_FEATURE__OPENGL_STACK_OPERATIONS
#include "../photon.h"

#include <clay/clay.h>

const elementMetadata_t DEFAULT_META_NONE = (elementMetadata_t) {
	.type = PH_META_ELE_BUTTON,
};
const elementMetadata_t DEFAULT_META_BUTTON = (elementMetadata_t) {
	.type      = PH_META_ELE_BUTTON,
	.timeDelta = 8,
	.button.hoverTime = 0,
	.button.mouseStartedOnElement = false,
};

const elementMetadata_t DEFAULT_META_TOGGLE = (elementMetadata_t) {
	.type      = PH_META_ELE_TOGGLE,
	.timeDelta = 8,
	.toggle.hoverTime = 0,
	.toggle.stateTime = 0,
	.toggle.mouseStartedOnElement = false,
	.toggle.state = false,
};

const elementMetadata_t DEFAULT_META_GRABBABLE = (elementMetadata_t) {
	.type      = PH_META_ELE_GRABBABLE,
	.timeDelta = 8,
	.grabbable.grabbed  = false,
	.grabbable.delta    = 0,
	.grabbable.snapshot = 0,
	.grabbable.timer    = 0,
};

const elementMetadata_t DEFAULT_META_SLIDER = (elementMetadata_t) {
	.type      = PH_META_ELE_GRABBABLE,
	.timeDelta = 8,
	.slider.grabbed  = false,
	.slider.value    = 0,
	.slider.timer    = 0,
};

const elementMetadata_t DEFAULT_META_PAN_AND_ZOOM = (elementMetadata_t) {
	.type      = PH_META_ELE_PAN_AND_ZOOM,
	.timeDelta = 8,
	.panAndZoom.grabbed = false,
	.panAndZoom.pan     = {0,0},
	.panAndZoom.zoom    = 1,
};

const elementMetadata_t DEFAULT_META_ICON = (elementMetadata_t) {
	.type = PH_META_ELE_ICON,
	.icon = (ivec2_t) {0, 0},
	.icon.rotation = 0,
	.icon.colour = (colour_t) { .value = 0xffffffff },
};


const elementMetadata_t DEFAULT_META_NINE_SLICED_ICON = (elementMetadata_t) {
	.type = PH_META_ELE_9_SLICED_ICON,
	.icon = (ivec2_t) {0, 0},
	.icon.rotation = 0,
	.icon.colour = (colour_t) { .value = 0xffffffff },
};

size_t genIDIndex;

// addressCache_t elementMetadataCache;
elementMetadata_t elementMetadataCache[PH_MAX_ELEMENT_METADATA_ENTRIES];
size_t            elementMetadataCacheLen = 0;
elementMetadata_ptr phguiGetElementMetadata(Clay_ElementId id, elementMetadata_t init, bool* existedAlready) {
	for (size_t i = 0; i < elementMetadataCacheLen; i++) {
		if (elementMetadataCache[i].id.id == id.id) {
			if (existedAlready != nullptr) *existedAlready = true;
			return &elementMetadataCache[i];
		}
	}
	if (existedAlready != nullptr) *existedAlready = false;
	elementMetadataCache[elementMetadataCacheLen] = init;

	elementMetadataCache[elementMetadataCacheLen].id        = id;
	elementMetadataCache[elementMetadataCacheLen].addresses = newCache(PH_ELEMENT_METADATA_MAX_ADDRESSES);

	return &elementMetadataCache[elementMetadataCacheLen++];
}

bool phguiButton(Clay_ElementId id, double* timer) {
	elementMetadata_ptr e = phguiGetElementMetadata(id, DEFAULT_META_BUTTON, nullptr);

	bool hovering = Clay_PointerOver(e->id);
	bool pressed  = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MousePressed);
	bool released = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MouseReleased);
	bool clicked;

	if (pressed && hovering) e->button.mouseStartedOnElement = true;

	e->button.hoverTime += (hovering ? 1 : -1) * e->timeDelta * phGetDeltaTime();
	e->button.hoverTime  = phClamp_double(e->button.hoverTime, 0, 1);

	if (timer != NULL) (*timer) = e->button.hoverTime;

	clicked = e->button.mouseStartedOnElement && hovering && released;
	// clicked = clicked || (hovering && isKey(GLFW_KEY_ENTER, KeyPressed));

	if (clicked) e->button.mouseStartedOnElement = false;

	return clicked;
}

bool phguiToggle(Clay_ElementId id, double* timer, double* stateTimer, bool* state) {
	elementMetadata_ptr e = phguiGetElementMetadata(id, DEFAULT_META_TOGGLE, nullptr);

	bool hovering = Clay_PointerOver(e->id);
	bool pressed  = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MousePressed);
	bool released = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MouseReleased);
	bool clicked;

	if (pressed && hovering) e->toggle.mouseStartedOnElement = true;

	e->toggle.hoverTime += (hovering ? 1 : -1) * e->timeDelta * phGetDeltaTime();
	e->toggle.hoverTime  = phClamp_double(e->toggle.hoverTime, 0, 1);

	if (timer != NULL) (*timer) = e->toggle.hoverTime;

	clicked = e->toggle.mouseStartedOnElement && hovering && released;
	// clicked = clicked || (hovering && isKey(GLFW_KEY_ENTER, KeyPressed));

	if (clicked) {
		e->toggle.mouseStartedOnElement = false;
		e->toggle.state = !e->toggle.state;
	}
	
	e->toggle.stateTime += (e->toggle.state ? 1 : -1) * e->timeDelta * phGetDeltaTime();
	e->toggle.stateTime  = phClamp_double(e->toggle.stateTime, 0, 1);

	if (stateTimer != NULL) (*stateTimer) = e->toggle.stateTime;
	if (state      != NULL) (*state)      = e->toggle.state;

	return clicked;
}

void phguiSetToggleState(Clay_ElementId id, bool state) {
	elementMetadata_ptr e = phguiGetElementMetadata(id, DEFAULT_META_TOGGLE, nullptr);
	
	e->toggle.state = state;
}

// returns true if just grabbed
bool phguiGrabbable(Clay_ElementId id, double sizingDelta, double* timer, int* snapshot, int* delta, bool* grabbed) {
	elementMetadata_ptr e = phguiGetElementMetadata(id, DEFAULT_META_GRABBABLE, nullptr);
	bool hovering = Clay_PointerOver(e->id);
	bool pressed  = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MousePressed);
	bool released = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MouseReleased);
	bool justGrabbed;

	double timerDir = 0;
	if (!e->grabbable.grabbed && !hovering) timerDir = -1;
	if ( e->grabbable.grabbed ||  hovering) timerDir = 1;
	e->grabbable.timer += timerDir * e->timeDelta * phGetDeltaTime();
	e->grabbable.timer  = phClamp_double(e->grabbable.timer, 0, 1);

	if (!e->grabbable.grabbed && hovering && pressed) {
		e->grabbable.grabbed = true;
		justGrabbed     = true;

		e->grabbable.snapshot = (*snapshot);
		e->grabbable.delta    = 0;
	}

	if (e->grabbable.grabbed) {
		e->grabbable.delta += sizingDelta;

		if (released) {
			e->grabbable.grabbed = false;
		}

		if (snapshot != nullptr) (*snapshot) = e->grabbable.snapshot + e->grabbable.delta;
	}

	if (grabbed != nullptr) (*grabbed) = e->grabbable.grabbed;
	if (delta   != nullptr) (*delta)   = e->grabbable.delta;
	if (timer   != nullptr) (*timer)   = e->grabbable.timer;

	return justGrabbed;
}

bool phguiSlider(Clay_ElementId id, double delta, double* timer, double* value, bool* grabbed) {
	bool existed;
	elementMetadata_ptr e = phguiGetElementMetadata(id, DEFAULT_META_SLIDER, &existed);
	if (!existed) {
		e->slider.value = *value;
	}
	// if ()
	bool hovering = Clay_PointerOver(e->id);
	bool pressed  = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MousePressed);
	bool released = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MouseReleased);
	bool justGrabbed;

	double timerDir = 0;
	if (!e->slider.grabbed && !hovering) timerDir = -1;
	if ( e->slider.grabbed ||  hovering) timerDir = 1;
	e->slider.timer += timerDir * e->timeDelta * phGetDeltaTime();
	e->slider.timer  = phClamp_double(e->slider.timer, 0, 1);

	if (!e->slider.grabbed && hovering && pressed) {
		// clamp that
		e->slider.grabbed = true;
		justGrabbed     = true;

		// e->grabbable.snapshot = (*snapshot);
		// e->slider.delta    = 0;
	}

	if (e->slider.grabbed) {
		e->slider.value += delta;

		if (released) {
			e->slider.grabbed = false;
		}
	}

	if (grabbed != nullptr) (*grabbed) = e->grabbable.grabbed;
	if (value   != nullptr) (*value)   = e->slider.value;
	if (timer   != nullptr) (*timer)   = e->grabbable.timer;

	return e->slider.grabbed;
}

bool phguiPanAndZoom(Clay_ElementId containerID, Clay_ElementId movableID, dvec2_t* pan, double* zoom, bool canInteract) {
	elementMetadata_ptr e = phguiGetElementMetadata(containerID, DEFAULT_META_PAN_AND_ZOOM, nullptr);
	bool hovering = Clay_PointerOver(e->id);
	bool pressed  = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MousePressed);
	bool released = phIsMouseButton(GLFW_MOUSE_BUTTON_LEFT, MouseReleased);
	Clay_BoundingBox bb = Clay_GetElementData(containerID).boundingBox;
	bool justGrabbed;

	if (!e->panAndZoom.grabbed && hovering && pressed && canInteract) {
		e->panAndZoom.grabbed = true;
		justGrabbed = true;
	}

	if(hovering && canInteract) {
		float wheel = phGetMouseWheelDelta().y;

		if (wheel != 0) {
			float oldZoom = e->panAndZoom.zoom;
			float newZoom = oldZoom * powf(1.05f, wheel);
			newZoom = phClamp_double(newZoom, 0.01, 2000000.0);
			float ratio = newZoom / oldZoom;

			// mouse relative to the FIXED viewport box, not the panned element
			float mx = phGetMousePosition().x - (bb.x + bb.width /2);
			float my = phGetMousePosition().y - (bb.y + bb.height/2);

			e->panAndZoom.pan.x = mx - (mx - e->panAndZoom.pan.x) * ratio;
			e->panAndZoom.pan.y = my - (my - e->panAndZoom.pan.y) * ratio;
			e->panAndZoom.zoom  = newZoom;
		}
		// e->panAndZoom.zoom += phGetMouseWheelDelta().y * .05;
		// if (phGetMouseWheelDelta().y < -.1 && phGetMouseWheelDelta().y > .1) {
		// 	e->panAndZoom.pan.x -= (phGetMousePosition().x - bb.x) * (e->panAndZoom.zoom - 1);
		// 	e->panAndZoom.pan.y -= (phGetMousePosition().y - bb.y) * (e->panAndZoom.zoom - 1);
		// }
	}

	if (e->grabbable.grabbed && canInteract) {
		dvec2_t mouseDelta = phGetMouseDelta();
		e->panAndZoom.pan.x += mouseDelta.x;
		e->panAndZoom.pan.y += mouseDelta.y;

		if (released) {
			e->grabbable.grabbed = false;
		}
	}

	if (pan  != nullptr) (*pan)  = e->panAndZoom.pan;
	if (zoom != nullptr) (*zoom) = e->panAndZoom.zoom;


	return justGrabbed;
}

void phguiIcon(icon_t icon, double rotation, colour_t c) {
	Clay_ElementId id = CLAY_IDI("phgui_icon", genIDIndex++);
	elementMetadata_ptr data = phguiGetElementMetadata(id, DEFAULT_META_ICON, nullptr);
	data->icon.x        = icon.x;
	data->icon.y        = icon.y;
	data->icon.rotation = rotation;
	data->icon.colour   = c;
	CLAY({
		.layout = {
			.sizing = {CLAY_SIZING_FIXED(phguiMeasures().iconSize),CLAY_SIZING_FIXED(phguiMeasures().iconSize)},
		},
		.id = id,
		.image.imageData = data,
		.backgroundColor = phColourToClayColour(c),
		.floating.pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
	});
}

void phguiNineSlicedIcon(icon_t icon, irec32_t rec, colour_t burn, int border) {
	Clay_ElementId id = CLAY_IDI("phgui_icon", genIDIndex++);
	elementMetadata_ptr data = phguiGetElementMetadata(id, DEFAULT_META_NINE_SLICED_ICON, nullptr);
	data->nineSlicedIcon.x        = icon.x;
	data->nineSlicedIcon.y        = icon.y;
	data->nineSlicedIcon.rotation = 0;
	data->nineSlicedIcon.burn     = burn;
	CLAY({
		.layout = {
			.sizing = {CLAY_SIZING_FIXED(rec.w),CLAY_SIZING_FIXED(rec.h)},
		},
		.id = id,
		.image.imageData = data,
		.backgroundColor = clayRGB(0xfff),
		.cornerRadius = CLAY_CORNER_RADIUS(10),
		.border.width = CLAY_BORDER_OUTSIDE(border),
		.border.color = phColourToClayColour(burn),
	});
}

// bool phguiDraggableWindow(Clay_ElementId id, ivec2_t* position) {
// 	// elementMetadata_ptr e = phguiGetElementMetadata(id, DEFAULT_META_TOGGLE);
// 	return 0;
// }
// phguiBeginFrame(PHGUI_FRAME_SIZER_FIT, )

void phguiBeginFrame(Clay_SizingAxis width, Clay_SizingAxis height) {
	Clay__OpenElement();

	Clay__ConfigureOpenElement(CLAY__CONFIG_WRAPPER(Clay_ElementDeclaration,{
		.layout.sizing.width=width,
		.layout.sizing.height=height,
	}));


}

void phguiEndFrame() {
	Clay__CloseElement();
}