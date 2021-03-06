/*
**  Triangle drawers
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

#include "r_data/r_translate.h"
#include "r_data/colormaps.h"
#include "screen_triangle.h"

class FTexture;
struct TriMatrix;

enum class PolyDrawMode
{
	Triangles,
	TriangleFan,
	TriangleStrip
};

class PolyClipPlane
{
public:
	PolyClipPlane() : A(0.0f), B(0.0f), C(0.0f), D(1.0f) { }
	PolyClipPlane(float a, float b, float c, float d) : A(a), B(b), C(c), D(d) { }

	float A, B, C, D;
};

class PolyDrawArgs
{
public:
	void SetClipPlane(const PolyClipPlane &plane);
	void SetTexture(const uint8_t *texels, int width, int height);
	void SetTexture(FTexture *texture);
	void SetTexture(FTexture *texture, uint32_t translationID, bool forcePal = false);
	void SetLight(FSWColormap *basecolormap, uint32_t lightlevel, double globVis, bool fixed);
	void SetSubsectorDepth(uint32_t subsectorDepth) { mSubsectorDepth = subsectorDepth; }
	void SetSubsectorDepthTest(bool enable) { mSubsectorTest = enable; }
	void SetStencilTestValue(uint8_t stencilTestValue) { mStencilTestValue = stencilTestValue; }
	void SetWriteColor(bool enable) { mWriteColor = enable; }
	void SetWriteStencil(bool enable, uint8_t stencilWriteValue = 0) { mWriteStencil = enable; mStencilWriteValue = stencilWriteValue; }
	void SetWriteSubsectorDepth(bool enable) { mWriteSubsector = enable; }
	void SetFaceCullCCW(bool counterclockwise) { mFaceCullCCW = counterclockwise; }
	void SetStyle(TriBlendMode blendmode, double srcalpha = 1.0, double destalpha = 1.0) { mBlendMode = blendmode; mSrcAlpha = (uint32_t)(srcalpha * 256.0 + 0.5); mDestAlpha = (uint32_t)(destalpha * 256.0 + 0.5); }
	void SetStyle(const FRenderStyle &renderstyle, double alpha, uint32_t fillcolor, uint32_t translationID, FTexture *texture, bool fullbright);
	void SetTransform(const TriMatrix *objectToClip) { mObjectToClip = objectToClip; }
	void SetColor(uint32_t bgra, uint8_t palindex);
	void DrawArray(const TriVertex *vertices, int vcount, PolyDrawMode mode = PolyDrawMode::Triangles);

	const TriMatrix *ObjectToClip() const { return mObjectToClip; }
	const float *ClipPlane() const { return mClipPlane; }

	const TriVertex *Vertices() const { return mVertices; }
	int VertexCount() const { return mVertexCount; }
	PolyDrawMode DrawMode() const { return mDrawMode; }

	bool FaceCullCCW() const { return mFaceCullCCW; }
	bool WriteColor() const { return mWriteColor; }

	const uint8_t *TexturePixels() const { return mTexturePixels; }
	int TextureWidth() const { return mTextureWidth; }
	int TextureHeight() const { return mTextureHeight; }
	const uint8_t *Translation() const { return mTranslation; }

	bool WriteStencil() const { return mWriteStencil; }
	uint8_t StencilTestValue() const { return mStencilTestValue; }
	uint8_t StencilWriteValue() const { return mStencilWriteValue; }

	bool SubsectorTest() const { return mSubsectorTest; }
	bool WriteSubsector() const { return mWriteSubsector; }
	uint32_t SubsectorDepth() const { return mSubsectorDepth; }

	TriBlendMode BlendMode() const { return mBlendMode; }
	uint32_t Color() const { return mColor; }
	uint32_t SrcAlpha() const { return mSrcAlpha; }
	uint32_t DestAlpha() const { return mDestAlpha; }

	float GlobVis() const { return mGlobVis; }
	uint32_t Light() const { return mLight; }
	const uint8_t *BaseColormap() const { return mColormaps; }
	uint16_t ShadeLightAlpha() const { return mLightAlpha; }
	uint16_t ShadeLightRed() const { return mLightRed; }
	uint16_t ShadeLightGreen() const { return mLightGreen; }
	uint16_t ShadeLightBlue() const { return mLightBlue; }
	uint16_t ShadeFadeAlpha() const { return mFadeAlpha; }
	uint16_t ShadeFadeRed() const { return mFadeRed; }
	uint16_t ShadeFadeGreen() const { return mFadeGreen; }
	uint16_t ShadeFadeBlue() const { return mFadeBlue; }
	uint16_t ShadeDesaturate() const { return mDesaturate; }

	bool SimpleShade() const { return mSimpleShade; }
	bool NearestFilter() const { return mNearestFilter; }
	bool FixedLight() const { return mFixedLight; }

private:
	const TriMatrix *mObjectToClip = nullptr;
	const TriVertex *mVertices = nullptr;
	int mVertexCount = 0;
	PolyDrawMode mDrawMode = PolyDrawMode::Triangles;
	bool mFaceCullCCW = false;
	bool mSubsectorTest = false;
	bool mWriteStencil = true;
	bool mWriteColor = true;
	bool mWriteSubsector = true;
	const uint8_t *mTexturePixels = nullptr;
	int mTextureWidth = 0;
	int mTextureHeight = 0;
	const uint8_t *mTranslation = nullptr;
	uint8_t mStencilTestValue = 0;
	uint8_t mStencilWriteValue = 0;
	const uint8_t *mColormaps = nullptr;
	float mClipPlane[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	TriBlendMode mBlendMode = TriBlendMode::FillOpaque;
	uint32_t mLight = 0;
	uint32_t mSubsectorDepth = 0;
	uint32_t mColor = 0;
	uint32_t mSrcAlpha = 0;
	uint32_t mDestAlpha = 0;
	uint16_t mLightAlpha = 0;
	uint16_t mLightRed = 0;
	uint16_t mLightGreen = 0;
	uint16_t mLightBlue = 0;
	uint16_t mFadeAlpha = 0;
	uint16_t mFadeRed = 0;
	uint16_t mFadeGreen = 0;
	uint16_t mFadeBlue = 0;
	uint16_t mDesaturate = 0;
	float mGlobVis = 0.0f;
	bool mSimpleShade = true;
	bool mNearestFilter = true;
	bool mFixedLight = false;
};

class RectDrawArgs
{
public:
	void SetTexture(const uint8_t *texels, int width, int height);
	void SetTexture(FTexture *texture);
	void SetTexture(FTexture *texture, uint32_t translationID, bool forcePal = false);
	void SetLight(FSWColormap *basecolormap, uint32_t lightlevel);
	void SetStyle(TriBlendMode blendmode, double srcalpha = 1.0, double destalpha = 1.0) { mBlendMode = blendmode; mSrcAlpha = (uint32_t)(srcalpha * 256.0 + 0.5); mDestAlpha = (uint32_t)(destalpha * 256.0 + 0.5); }
	void SetStyle(const FRenderStyle &renderstyle, double alpha, uint32_t fillcolor, uint32_t translationID, FTexture *texture, bool fullbright);
	void SetColor(uint32_t bgra, uint8_t palindex);
	void Draw(double x0, double x1, double y0, double y1, double u0, double u1, double v0, double v1);

	const uint8_t *TexturePixels() const { return mTexturePixels; }
	int TextureWidth() const { return mTextureWidth; }
	int TextureHeight() const { return mTextureHeight; }
	const uint8_t *Translation() const { return mTranslation; }

	TriBlendMode BlendMode() const { return mBlendMode; }
	uint32_t Color() const { return mColor; }
	uint32_t SrcAlpha() const { return mSrcAlpha; }
	uint32_t DestAlpha() const { return mDestAlpha; }

	uint32_t Light() const { return mLight; }
	const uint8_t *BaseColormap() const { return mColormaps; }
	uint16_t ShadeLightAlpha() const { return mLightAlpha; }
	uint16_t ShadeLightRed() const { return mLightRed; }
	uint16_t ShadeLightGreen() const { return mLightGreen; }
	uint16_t ShadeLightBlue() const { return mLightBlue; }
	uint16_t ShadeFadeAlpha() const { return mFadeAlpha; }
	uint16_t ShadeFadeRed() const { return mFadeRed; }
	uint16_t ShadeFadeGreen() const { return mFadeGreen; }
	uint16_t ShadeFadeBlue() const { return mFadeBlue; }
	uint16_t ShadeDesaturate() const { return mDesaturate; }
	bool SimpleShade() const { return mSimpleShade; }

	float X0() const { return mX0; }
	float X1() const { return mX1; }
	float Y0() const { return mY0; }
	float Y1() const { return mY1; }
	float U0() const { return mU0; }
	float U1() const { return mU1; }
	float V0() const { return mV0; }
	float V1() const { return mV1; }

private:
	const uint8_t *mTexturePixels = nullptr;
	int mTextureWidth = 0;
	int mTextureHeight = 0;
	const uint8_t *mTranslation = nullptr;
	const uint8_t *mColormaps = nullptr;
	TriBlendMode mBlendMode = TriBlendMode::FillOpaque;
	uint32_t mLight = 0;
	uint32_t mColor = 0;
	uint32_t mSrcAlpha = 0;
	uint32_t mDestAlpha = 0;
	uint16_t mLightAlpha = 0;
	uint16_t mLightRed = 0;
	uint16_t mLightGreen = 0;
	uint16_t mLightBlue = 0;
	uint16_t mFadeAlpha = 0;
	uint16_t mFadeRed = 0;
	uint16_t mFadeGreen = 0;
	uint16_t mFadeBlue = 0;
	uint16_t mDesaturate = 0;
	bool mSimpleShade = true;
	float mX0, mX1, mY0, mY1, mU0, mU1, mV0, mV1;
};
