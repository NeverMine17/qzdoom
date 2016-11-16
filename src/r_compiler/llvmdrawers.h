/*
**  LLVM code generated drawers
**  Copyright (c) 2016 Magnus Norddahl
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
*/

#pragma once

struct WorkerThreadData
{
	int32_t core;
	int32_t num_cores;
	int32_t pass_start_y;
	int32_t pass_end_y;
	uint32_t *temp;
};

struct DrawWallArgs
{
	uint32_t *dest;
	const uint32_t *source[4];
	const uint32_t *source2[4];
	int32_t pitch;
	int32_t count;
	int32_t dest_y;
	uint32_t texturefrac[4];
	uint32_t texturefracx[4];
	uint32_t iscale[4];
	uint32_t textureheight[4];
	uint32_t light[4];
	uint32_t srcalpha;
	uint32_t destalpha;

	uint16_t light_alpha;
	uint16_t light_red;
	uint16_t light_green;
	uint16_t light_blue;
	uint16_t fade_alpha;
	uint16_t fade_red;
	uint16_t fade_green;
	uint16_t fade_blue;
	uint16_t desaturate;
	uint32_t flags;
	enum Flags
	{
		simple_shade = 1,
		nearest_filter = 2
	};

	FString ToString()
	{
		FString info;
		info.Format("dest_y = %i, count = %i, flags = %i, texturefrac[0] = %u, textureheight[0] = %u", dest_y, count, flags, texturefrac[0], textureheight[0]);
		return info;
	}
};

struct DrawSpanArgs
{
	uint32_t *destorg;
	const uint32_t *source;
	int32_t destpitch;
	int32_t xfrac;
	int32_t yfrac;
	int32_t xstep;
	int32_t ystep;
	int32_t x1;
	int32_t x2;
	int32_t y;
	int32_t xbits;
	int32_t ybits;
	uint32_t light;
	uint32_t srcalpha;
	uint32_t destalpha;

	uint16_t light_alpha;
	uint16_t light_red;
	uint16_t light_green;
	uint16_t light_blue;
	uint16_t fade_alpha;
	uint16_t fade_red;
	uint16_t fade_green;
	uint16_t fade_blue;
	uint16_t desaturate;
	uint32_t flags;
	enum Flags
	{
		simple_shade = 1,
		nearest_filter = 2
	};

	FString ToString()
	{
		FString info;
		info.Format("x1 = %i, x2 = %i, y = %i, flags = %i", x1, x2, y, flags);
		return info;
	}
};

struct DrawColumnArgs
{
	uint32_t *dest;
	const uint8_t *source;
	const uint8_t *source2;
	uint8_t *colormap;
	uint8_t *translation;
	const uint32_t *basecolors;
	int32_t pitch;
	int32_t count;
	int32_t dest_y;
	uint32_t iscale;
	uint32_t texturefracx;
	uint32_t textureheight;
	uint32_t texturefrac;
	uint32_t light;
	uint32_t color;
	uint32_t srccolor;
	uint32_t srcalpha;
	uint32_t destalpha;

	uint16_t light_alpha;
	uint16_t light_red;
	uint16_t light_green;
	uint16_t light_blue;
	uint16_t fade_alpha;
	uint16_t fade_red;
	uint16_t fade_green;
	uint16_t fade_blue;
	uint16_t desaturate;
	uint32_t flags;
	enum Flags
	{
		simple_shade = 1,
		nearest_filter = 2
	};

	FString ToString()
	{
		FString info;
		info.Format("dest_y = %i, count = %i, flags = %i, iscale = %i (%f), texturefrac = %i (%f)", dest_y, count, flags, iscale, ((fixed_t)iscale) / (float)FRACUNIT, texturefrac, ((fixed_t)texturefrac) / (float)FRACUNIT);
		return info;
	}
};

struct DrawSkyArgs
{
	uint32_t *dest;
	const uint32_t *source0[4];
	const uint32_t *source1[4];
	int32_t pitch;
	int32_t count;
	int32_t dest_y;
	uint32_t texturefrac[4];
	uint32_t iscale[4];
	uint32_t textureheight0;
	uint32_t textureheight1;
	uint32_t top_color;
	uint32_t bottom_color;

	FString ToString()
	{
		FString info;
		info.Format("dest_y = %i, count = %i", dest_y, count);
		return info;
	}
};

struct TriVertex
{
	TriVertex() { }
	TriVertex(float x, float y, float z, float w, float u, float v) : x(x), y(y), z(z), w(w) { varying[0] = u; varying[1] = v; }

	enum { NumVarying = 2 };
	float x, y, z, w;
	float varying[NumVarying];
};

struct TriMatrix
{
	static TriMatrix null();
	static TriMatrix identity();
	static TriMatrix translate(float x, float y, float z);
	static TriMatrix scale(float x, float y, float z);
	static TriMatrix rotate(float angle, float x, float y, float z);
	static TriMatrix swapYZ();
	static TriMatrix perspective(float fovy, float aspect, float near, float far);
	static TriMatrix frustum(float left, float right, float bottom, float top, float near, float far);

	static TriMatrix worldToView(); // Software renderer world to view space transform
	static TriMatrix viewToClip(); // Software renderer shearing projection

	TriVertex operator*(TriVertex v) const;
	TriMatrix operator*(const TriMatrix &m) const;

	float matrix[16];
};

struct TriUniforms
{
	uint32_t light;
	uint32_t subsectorDepth;

	uint16_t light_alpha;
	uint16_t light_red;
	uint16_t light_green;
	uint16_t light_blue;
	uint16_t fade_alpha;
	uint16_t fade_red;
	uint16_t fade_green;
	uint16_t fade_blue;
	uint16_t desaturate;
	uint32_t flags;
	enum Flags
	{
		simple_shade = 1,
		nearest_filter = 2,
		fixed_light = 4
	};

	TriMatrix objectToClip;
};

struct TriDrawTriangleArgs
{
	uint8_t *dest;
	int32_t pitch;
	TriVertex *v1;
	TriVertex *v2;
	TriVertex *v3;
	int32_t clipleft;
	int32_t clipright;
	int32_t cliptop;
	int32_t clipbottom;
	const uint8_t *texturePixels;
	uint32_t textureWidth;
	uint32_t textureHeight;
	uint32_t solidcolor;
	const TriUniforms *uniforms;
	uint8_t *stencilValues;
	uint32_t *stencilMasks;
	int32_t stencilPitch;
	uint8_t stencilTestValue;
	uint8_t stencilWriteValue;
	uint32_t *subsectorGBuffer;
};

enum class TriDrawVariant
{
	Draw,
	DrawMasked,
	Fill,
	DrawSubsector,
	Stencil,
};

class LLVMDrawers
{
public:
	virtual ~LLVMDrawers() { }

	static void Create();
	static void Destroy();
	static LLVMDrawers *Instance();

	void(*DrawColumn)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnAdd)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnShaded)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnAddClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnSubClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRevSubClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnTlatedAdd)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnAddClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnSubClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRevSubClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*FillColumn)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*FillColumnAdd)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*FillColumnAddClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*FillColumnSubClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*FillColumnRevSubClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1Copy)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1Add)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1Shaded)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1AddClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1SubClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1RevSubClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1Translated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1TlatedAdd)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1AddClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1SubClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt1RevSubClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4Copy)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4Add)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4Shaded)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4AddClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4SubClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4RevSubClamp)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4Translated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4TlatedAdd)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4AddClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4SubClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawColumnRt4RevSubClampTranslated)(const DrawColumnArgs *, const WorkerThreadData *) = nullptr;

	void(*DrawSpan)(const DrawSpanArgs *) = nullptr;
	void(*DrawSpanMasked)(const DrawSpanArgs *) = nullptr;
	void(*DrawSpanTranslucent)(const DrawSpanArgs *) = nullptr;
	void(*DrawSpanMaskedTranslucent)(const DrawSpanArgs *) = nullptr;
	void(*DrawSpanAddClamp)(const DrawSpanArgs *) = nullptr;
	void(*DrawSpanMaskedAddClamp)(const DrawSpanArgs *) = nullptr;

	void(*vlinec1)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*vlinec4)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*mvlinec1)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*mvlinec4)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*tmvline1_add)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*tmvline4_add)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*tmvline1_addclamp)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*tmvline4_addclamp)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*tmvline1_subclamp)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*tmvline4_subclamp)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*tmvline1_revsubclamp)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;
	void(*tmvline4_revsubclamp)(const DrawWallArgs *, const WorkerThreadData *) = nullptr;

	void(*DrawSky1)(const DrawSkyArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawSky4)(const DrawSkyArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawDoubleSky1)(const DrawSkyArgs *, const WorkerThreadData *) = nullptr;
	void(*DrawDoubleSky4)(const DrawSkyArgs *, const WorkerThreadData *) = nullptr;

	void(*TriDraw8)(const TriDrawTriangleArgs *, WorkerThreadData *) = nullptr;
	void(*TriDraw32)(const TriDrawTriangleArgs *, WorkerThreadData *) = nullptr;
	void(*TriDrawSubsector8)(const TriDrawTriangleArgs *, WorkerThreadData *) = nullptr;
	void(*TriDrawSubsector32)(const TriDrawTriangleArgs *, WorkerThreadData *) = nullptr;
	void(*TriFill8)(const TriDrawTriangleArgs *, WorkerThreadData *) = nullptr;
	void(*TriFill32)(const TriDrawTriangleArgs *, WorkerThreadData *) = nullptr;
	void(*TriStencil)(const TriDrawTriangleArgs *, WorkerThreadData *) = nullptr;

private:
	static LLVMDrawers *Singleton;
};