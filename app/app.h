#ifndef PHOTON_APP_APP_H
#define PHOTON_APP_APP_H

typedef enum {
	APP_SW_RANGE_INT,
	APP_SW_RANGE_FLOAT,
	APP_SW_COORDS,
	APP_SW_COLOUR_PICKER,

	APP_SW_INVALID,
} ShaderWidgetTypes;

typedef struct {
	ShaderWidgetTypes type;

	union {
		struct {
			int val, min, max;
			double sliderVal;
		} irange;

		struct {
			double val, min, max;
			double sliderVal;
		} frange;

		struct {
			double x, y;
			double minX,minY, maxX,maxY;
			double sliderX, sliderY;
		} coords;

		struct {
			double val;
			double sliderVal;
		} colourPicker[4];
	};
} shaderUniformInterface_t;

void widgetRangeInt(int id, shaderUniformInterface_t* interface);
void widgetRangeFloat(int id, shaderUniformInterface_t* interface);
void widgetCoords(int id, shaderUniformInterface_t* interface);
void widgetColourPicker(int id, shaderUniformInterface_t* interface);

#endif