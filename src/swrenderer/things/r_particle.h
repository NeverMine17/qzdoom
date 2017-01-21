//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//

#pragma once

#include "r_visiblesprite.h"
#include "swrenderer/scene/r_opaque_pass.h"

struct particle_t;

namespace swrenderer
{
	class RenderParticle : public VisibleSprite
	{
	public:
		static void Project(particle_t *, const sector_t *sector, int shade, WaterFakeSide fakeside, bool foggy);

	protected:
		bool IsParticle() const override { return true; }
		void Render(short *cliptop, short *clipbottom, int minZ, int maxZ) override;

	private:
		void DrawMaskedSegsBehindParticle();

		fixed_t xscale;
		fixed_t	startfrac; // horizontal position of x1
		int y1, y2;

		uint32_t Translation;
		uint32_t FillColor;
	};
}