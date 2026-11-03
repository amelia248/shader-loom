#include "../photon.h"

void initMeasures() {
	window_t* win = phGetWindow();

	win->clay.measures = (phguiMeasures_t) {
		.guiScale = 2,
		.iconSize = 24,
	};
}

inline phguiMeasures_t phguiMeasures() {
	phguiMeasures_t m = phGetWindow()->clay.measures;

	m.iconSize *= m.guiScale;

	return m;
}
inline phguiMeasures_t phguiUncomputedMeasures() {
	return phGetWindow()->clay.measures;
}