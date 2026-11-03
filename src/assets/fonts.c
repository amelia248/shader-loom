#ifdef INTELLISENSE
	#define GLAPI
	#define APIENTRY
	#define GL_GLEXT_PROTOTYPES
	#include <glad/glcorearb.h>
#endif
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "STB/stb-write-image.h"
#include <freetype/freetype.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include <memory.h>
#include <maths.h>

#include "../photon.h"

char asciiSet[96] =
" !\"#$%&'()*+,-./"
"0123456789:;<=>?"
"@ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ[\\]^_"
"`abcdefghijklmno"
"pqrstuvwxyz{|}~"
"\x62";

addressCache_t fontCache;
addressStack_t fontStack;

void phGLPushFont(font_ptr f) {
	push(&fontStack, f);
}
font_ptr phGLPopFont() {
	return pop(&fontStack);
}
font_ptr phGLPeekFont() { return peek(&fontStack); }

// if font type is .ttf, 
font_t loadFont_ttf(c_string_t name, uint32_t size, fontRenderMode_e renderMode);
texture_ptr loadFontTexture_png(c_string_t name, uint32_t size);

size_t fontID = 0;

font_ptr phNewFont(c_string_t name, uint32_t size, ...) {
	va_list args;
	font_t f;
	c_string_t filenameExtension;
	
	va_start(args, size);

	for (filenameExtension = name + strlen(name); filenameExtension[-1] != '.'; filenameExtension--);

	if (strcmp(filenameExtension, "ttf") == 0) {
		fontRenderMode_e mode = va_arg(args, fontRenderMode_e);
		f = loadFont_ttf(name, size, mode);
		if (f.texture == NULL) return NULL;
	}
	else if (strcmp(filenameExtension, "png") == 0) {
		// f = loadFontTexture_png(name, size);
	}
	// f.id = fontID++;

	font_ptr ptr = (font_ptr)memalloc(sizeof(font_t));
	*ptr = f;
	ptr->id = cacheAddress(&fontCache, ptr);
	return ptr;
}

FT_Library getFreetypeLibrary();

font_t loadFont_ttf(c_string_t name, uint32_t size, fontRenderMode_e renderMode) {
	font_t f;

	FT_Render_Mode ftRenderMode;
	textureFormat_e glyphTextureFormat;
	int bpp; // bytes per pixel

	switch (renderMode) {
	case PH_FONT_RENDER_MODE_NORMAL: {
		ftRenderMode = FT_RENDER_MODE_NORMAL;
		glyphTextureFormat = PH_TEXTURE_FORMAT_R;
		bpp = 1;
	} break;

	// case FT_RENDER_MODE_LIGHT: {
	// 	ftRenderMode = FT_RENDER_MODE_LIGHT;
	// 	glyphTextureFormat = PH_TEXTURE_FORMAT_R;
	// 	bpp = 1;
	// } break;

	// case FT_RENDER_MODE_MONO: {
	// 	ftRenderMode = FT_RENDER_MODE_MONO;
	// 	printf("render mode not implemented\n");
	// 	return (font_t){0};
	// } break;

	case PH_FONT_RENDER_MODE_LCD_RGB: {
		ftRenderMode = FT_RENDER_MODE_LCD;
		glyphTextureFormat = PH_TEXTURE_FORMAT_RGB;
		bpp = 3;
	} break;

	// case FT_RENDER_MODE_LCD_V: {
	// 	ftRenderMode = FT_RENDER_MODE_LCD_V;
	// 	glyphTextureFormat = PH_TEXTURE_FORMAT_RGB;
	// 	printf("render mode not implemented\n");
	// 	return (font_t){0};
	// } break;

	case PH_FONT_RENDER_MODE_SDF: {
		ftRenderMode = FT_RENDER_MODE_SDF;
		glyphTextureFormat = PH_TEXTURE_FORMAT_R;
		bpp = 1;
	} break;

	default: {
		ftRenderMode = FT_RENDER_MODE_NORMAL;
		glyphTextureFormat = PH_TEXTURE_FORMAT_R;
		bpp = 1;
	}
	}

	string_t filePath;
	FT_Library freetype = phGetWindow()->freetypeLibrary;
	FT_Error err;
	FT_Face face;

	uint8_t* glyphBitmaps[sizeof(asciiSet)];

	f.glyphs = (glyph_t*)memalloc(sizeof(glyph_t) * sizeof(asciiSet));
	f.glyphsCount = sizeof(asciiSet);
	f.size = size;
	f.renderer = nullptr;
	
	if (name[0] == '/' || name[1] == ':') {
		filePath = newstring(name);
	}
	else {
		filePath = newstring(PH_ASSETS_FONT_FOLDER);
		stringcat_buf(&filePath, name);
	}
	err = FT_New_Face(freetype, filePath.buf, 0, &face);
	if (err) {
		printf("Could not load font \"%s\"\n", filePath.buf);
		return (font_t){0};
	}
	delstring(filePath);
	FT_Set_Pixel_Sizes(face, 0, f.size);

	f.ascender  = phRound((double)face->size->metrics.ascender  / 64.0, 1);
	f.descender = phRound((double)face->size->metrics.descender / 64.0, 1);
	/*
	int ascender  = m->ascender  >> 6;   // positive, pixels above baseline
	int descender = m->descender >> 6;   // negative, pixels below baseline
	int lineHeight = ascender - descender; // total height, top to bottom
	*/

	// printf(
	// 	"Font family: %s | " "style: %s | "     "# of glyphs: %ld | " "Units per EM: %d\n",
	// 	face->family_name,    face->style_name,  face->num_glyphs,     face->units_per_EM
	// );

	

	for (size_t i = 0; i < sizeof(asciiSet); i++) {
		if (i < 95) err = FT_Load_Char( face, asciiSet[i], FT_LOAD_DEFAULT);
		else        err = FT_Load_Glyph(face, asciiSet[i], FT_LOAD_DEFAULT);
		if (err) return (font_t){0};

		err = FT_Render_Glyph(face->glyph, ftRenderMode);
		if (err) return (font_t){0};
		// face->glyph->metrics...
		FT_Bitmap *bitmap = &face->glyph->bitmap;

		glyphBitmaps[i] = (uint8_t*)memalloc(bitmap->width * bitmap->rows);
		f.glyphs[i].value = asciiSet[i];
		
		f.glyphs[i].rec.w = bitmap->width / bpp;
		f.glyphs[i].rec.h = bitmap->rows;

		f.glyphs[i].offset.x = face->glyph->bitmap_left;
		f.glyphs[i].offset.y = face->glyph->bitmap_top - f.ascender;

		// f.glyphs[i].advance.x = max(f.glyphs[i].rec.w, phRound(face->glyph->advance.x / 64.0, 1));
		// f.glyphs[i].advance.y = phRound(face->glyph->advance.y / 64.0, 1);
		f.glyphs[i].advance.x = face->glyph->advance.x;
		f.glyphs[i].advance.y = face->glyph->advance.y;
		

		if (f.glyphs[i].rec.w == 0 || f.glyphs[i].rec.h == 0) continue;
		
		memcpy_blocks(
			glyphBitmaps[i],
			bitmap->buffer,
			bitmap->rows, bitmap->width, bitmap->pitch
		);

		// black/grey/white bordered box
		// memset(glyphBitmaps[i], 32*(i%8) - 1, bitmap->width * bpp);
		// memset_blocks(glyphBitmaps[i] + (bitmap->width-1)*bpp, 32*(i%8) - 1, bitmap->rows-1, bitmap->width*bpp, 2*bpp);
		// memset(glyphBitmaps[i]+((bitmap->rows-1) * bitmap->width), 32*(i%8) - 1, bitmap->width * bpp);
	}
	const size_t maxResizeIterations = 5; // max size of 2048 (64 << 5, 2^11)
	size_t textureSize = 32;
	int* yPosColumns = memalloc(textureSize * sizeof(int));
	memset(yPosColumns, 0, textureSize * sizeof(int));
	ivec2_t position = (ivec2_t){.x=0, .y=0};
	for (size_t i=0, resizeIterations=0; i < sizeof(asciiSet) && resizeIterations < maxResizeIterations; i++) {
		if (position.x + f.glyphs[i].rec.w > textureSize) {
			position.x = 0;
		}
		position.y = 0;
		for (size_t j = 0; j < f.glyphs[i].rec.w; j++) {
			position.y = phMax_int(position.y, yPosColumns[position.x + j]);
		}
		if (position.y + f.glyphs[i].rec.h > textureSize) {
			textureSize = textureSize << 1;
			
			memfree(yPosColumns);
			yPosColumns = memalloc(textureSize * sizeof(int));
			memset(yPosColumns, 0, textureSize * sizeof(int));
			
			position = (ivec2_t){.x=0, .y=0};
			resizeIterations++;
			i = 0;
		}

		for (size_t j = 0; j < f.glyphs[i].rec.w; j++) {
			yPosColumns[position.x + j] = position.y + f.glyphs[i].rec.h;
		}
		f.glyphs[i].rec.x = position.x;
		f.glyphs[i].rec.y = position.y;
		position.x += f.glyphs[i].rec.w;
	}

	position = (ivec2_t){.x=0, .y=0};

	uint8_t* glyphTextureData = memalloc((textureSize * bpp) * textureSize);
	memset(glyphTextureData, 0, (textureSize * bpp) * textureSize);
	for (size_t i = 0; i < sizeof(asciiSet); i++) {
		// advanceY = max(advanceY, f.glyphs[i].rec.h);
		// if (position.x + f.glyphs[i].rec.w > textureSize) {
		// 	position.x = 0, position.y += advanceY;
		// }
		// f.glyphs[i].rec.x = position.x;
		// f.glyphs[i].rec.y = position.y;

		memcpy_blocks(
			glyphTextureData + (f.glyphs[i].rec.y * textureSize + f.glyphs[i].rec.x) * bpp,
			glyphBitmaps[i],
			f.glyphs[i].rec.h,
			textureSize * bpp,
			f.glyphs[i].rec.w * bpp
		);
	}

	// stbi_write_bmp("ttf-cache.bmp", textureSize, textureSize, bpp, glyphTextureData);

	f.texture = phNewTexture(
		glyphTextureData,
		textureSize, textureSize,
		glyphTextureFormat, GL_LINEAR, PH_TEXTURE_TYPE_STATIC
	);

	memfree(glyphTextureData);
	memfree(yPosColumns);
	for (size_t i = 0; i < sizeof(asciiSet); i++) {
		memfree(glyphBitmaps[i]);
	}
	
	FT_Done_Face(face);
	return f;
}
texture_ptr loadFontTexture_png(c_string_t name, uint32_t size) { return NULL; }

void phGenerateTextRenderer(font_ptr f) {
	textRenderer_t* tr = (textRenderer_t*)memalloc(sizeof(textRenderer_t));
	*tr = newTextRenderer(f);
	f->renderer = tr;
}

textRenderer_t newTextRenderer(font_ptr f) {
	textRenderer_t tr;

	/* vertex guide
		|i| x,y,z,   r,g,b,a,   u,v  | vertices |   uv's   |
		|-|--------|----------|------|----------|----------|
		|0| 0,0,1,   1,1,1,1,   0,1, | 0------1 | 01----11 |
		|1| 1,0,1,   1,1,1,1,   1,1, | |    / | | |    / | |
		|2| 0,1,1,   1,1,1,1,   0,0, | | /    | | | /    | |
		|3| 1,1,1,   1,1,1,1,   1,0, | 2------3 | 00----10 |
	*/
	vertexAttribute_t attributes[3] = {
		{.type = PH_TYPE_FLOAT, .length = 3}, // x, y, z
		{.type = PH_TYPE_FLOAT, .length = 4}, // r, g, b, a
		{.type = PH_TYPE_FLOAT, .length = 2}, // u, v
	};
	uint16_t indices[] = {0,1,2, 3,2,1};

	// tr.indicesPatternLen = sizeof(indices) / sizeof(indices);
	
	tr.vertices = memalloc(sizeof(fontMeshVertex_t) * 4 * f->glyphsCount);
	for (size_t i = 0; i < f->glyphsCount; i++) {
		size_t j = 4*i;
		glyph_t g = f->glyphs[i];

		frec_t r = (frec_t){
			((float)g.rec.x / (float)f->texture->width),
			((float)g.rec.y / (float)f->texture->height),
			((float)g.rec.w / (float)f->texture->width),
			((float)g.rec.h / (float)f->texture->height)
		};

		tr.vertices[j+0].x = g.offset.x;           /**/tr.vertices[j+0].y = g.offset.y-g.rec.h; /**/tr.vertices[j+0].z = 1;
		tr.vertices[j+1].x = g.offset.x+g.rec.w;  /**/ tr.vertices[j+1].y = g.offset.y-g.rec.h;/**/ tr.vertices[j+1].z = 1;
		tr.vertices[j+2].x = g.offset.x;         /**/  tr.vertices[j+2].y = g.offset.y;       /**/  tr.vertices[j+2].z = 1;
		tr.vertices[j+3].x = g.offset.x+g.rec.w;/**/   tr.vertices[j+3].y = g.offset.y;      /**/   tr.vertices[j+3].z = 1;
		/**************************************************************************************/
		tr.vertices[j+0].u = r.x;             /**/     tr.vertices[j+0].v = r.y+r.h;       /**/
		tr.vertices[j+1].u = r.x+r.w;        /**/      tr.vertices[j+1].v = r.y+r.h;      /**/
		tr.vertices[j+2].u = r.x;           /**/       tr.vertices[j+2].v = r.y;         /**/
		tr.vertices[j+3].u = r.x+r.w;      /**/        tr.vertices[j+3].v = r.y;        /**/
		/*********************************************************************************/
		tr.vertices[j+0].r=1; tr.vertices[j+0].g=1; tr.vertices[j+0].b=1; tr.vertices[j+0].a=1;
		tr.vertices[j+1].r=1; tr.vertices[j+1].g=1; tr.vertices[j+1].b=1; tr.vertices[j+1].a=1;
		tr.vertices[j+2].r=1; tr.vertices[j+2].g=1; tr.vertices[j+2].b=1; tr.vertices[j+2].a=1;
		tr.vertices[j+3].r=1; tr.vertices[j+3].g=1; tr.vertices[j+3].b=1; tr.vertices[j+3].a=1;
	}

	
	tr.batchedMesh = phNewBatchedMesh(
		attributes, sizeof(attributes)/sizeof(vertexAttribute_t),
		indices, sizeof(indices),
		PH_FONT_RENDERER_MAX_GLYPH_INSTANCES
	);
	
	return tr;
}

ivec2_t phMeasureText(font_ptr font, c_string_t text, size_t length, uint16_t size, int16_t hSpacing, int16_t vSpacing) {
	int maxLineWidth = 0;
	ivec2_t measure = {0};

	length = length == 0 ? strlen(text) : length;

	for (size_t i = 0; i < length; i++) {
		glyph_t g = font->glyphs[text[i] - ' '];

		measure.x += phRound((double)g.advance.x / 64.0, 1);

		if (text[i] == '\n' || i == length - 1) {
			maxLineWidth = phMax_int(maxLineWidth, measure.x);
			measure.x = 0;
			measure.y += font->ascender - font->descender; // phRound((double)g.advance.y / 64.0, 1) + vSpacing;
			// g.rec.y;
		}
	}

	measure.x = maxLineWidth;
	return measure;
}

void phDelFont(font_ptr f) {
	int found = removeAddress(&fontCache, f);
	if (!found) return;
	phDelTexture(f->texture);
	memfree(f->glyphs);
	if (f->renderer != nullptr) {
		// memfree(f->renderer->indices);
		memfree(f->renderer->vertices);
		memfree(f->renderer);
	}
	memfree(f);
}
