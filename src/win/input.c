#include "maths.h"
#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "../photon.h"


typedef enum {
	keyUp       = 0b00000001,
	keyPressed  = 0b00000010,
	keyDown     = 0b00000100,
	keyReleased = 0b00001000,
	keyRepeated = 0b00010000,
} keyState;

keyState keyStates[GLFW_KEY_LAST+1];
const keyState keyActionToStateMap[3] = {keyReleased,keyPressed,keyRepeated};
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	keyStates[key] = keyActionToStateMap[action];
}
inline bool phIsKey(int key, keyStateMask_t state) {
	return keyStates[key] & (keyState)state;
}

mouseButtonState_t mouseBtnStates[GLFW_MOUSE_BUTTON_LAST];
const mouseButtonState_t mouseActionToStateMap[2] = {MouseReleased, MousePressed};

dvec2_t mousePrevPosition;
dvec2_t mousePosition;
dvec2_t mouseWheelDelta;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	mouseBtnStates[button] = mouseActionToStateMap[action];
}
inline bool phIsMouseButton(int button, mouseButtonState_t state) {
	return mouseBtnStates[button] & state;
}

void mousePositionCallback(GLFWwindow* window, double x, double y) {
	// mousePrevPosition = mousePosition;
	mousePosition = (dvec2_t) {x, y};
}
void mouseWheelCallback(GLFWwindow* window, double x, double y) {
	mouseWheelDelta = (dvec2_t) {x, y};
}
inline dvec2_t phGetMousePosition() { return mousePosition; }
inline dvec2_t phGetMouseDelta() { return (dvec2_t){mousePosition.x - mousePrevPosition.x, mousePosition.y - mousePrevPosition.y}; }
inline dvec2_t phGetMouseWheelDelta() { return mouseWheelDelta; }

void updateInput() {
	// keyboard
	for (int i = 0; i < GLFW_KEY_LAST; i++) {
		if (keyStates[i] & keyPressed)
			keyStates[i] = keyDown;
		else if (keyStates[i] & keyReleased)
			keyStates[i] = keyUp;
	}

	// mouse buttons
	for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++) {
		if (mouseBtnStates[i] & MousePressed)
			mouseBtnStates[i] = MouseDown;
		else if (mouseBtnStates[i] & MouseReleased)
			mouseBtnStates[i] = MouseUp;
	}
}

void resizeCallback(GLFWwindow* window, int width, int height) {
	window_t* win = phGetWindow();
	matrix_t* proj = &win->primitives.matrices.projection;

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	//                                                      ┌            ┐
	phSetMatrixElement(proj, 0,0,  2./((double) width)); // │ 2/w 0  0 0 │
	phSetMatrixElement(proj, 1,1,  2./((double)height)); // │  0 2/h 0 0 │
	phSetMatrixElement(proj, 3,0, -1.);                  // │  0  0  1 0 │
	phSetMatrixElement(proj, 3,1,  1.);                  // │ -1  1  0 1 │
	//                                                      └            ┘

	win->width = width;
	win->height = height;
	
	phReloadTexture_render(&win->renderPasses.firstPass, width, height, GL_NEAREST);

	if (win->onResize != NULL) win->onResize(win->width, win->height);
}
