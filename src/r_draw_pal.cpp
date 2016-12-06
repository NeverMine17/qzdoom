
#include "templates.h"
#include "doomtype.h"
#include "doomdef.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"
#include "r_draw_pal.h"

/*
	[RH] This translucency algorithm is based on DOSDoom 0.65's, but uses
	a 32k RGB table instead of an 8k one. At least on my machine, it's
	slightly faster (probably because it uses only one shift instead of
	two), and it looks considerably less green at the ends of the
	translucency range. The extra size doesn't appear to be an issue.

	The following note is from DOSDoom 0.65:

	New translucency algorithm, by Erik Sandberg:

	Basically, we compute the red, green and blue values for each pixel, and
	then use a RGB table to check which one of the palette colours that best
	represents those RGB values. The RGB table is 8k big, with 4 R-bits,
	5 G-bits and 4 B-bits. A 4k table gives a bit too bad precision, and a 32k
	table takes up more memory and results in more cache misses, so an 8k
	table seemed to be quite ultimate.

	The computation of the RGB for each pixel is accelerated by using two
	1k tables for each translucency level.
	The xth element of one of these tables contains the r, g and b values for
	the colour x, weighted for the current translucency level (for example,
	the weighted rgb values for background colour at 75% translucency are 1/4
	of the original rgb values). The rgb values are stored as three
	low-precision fixed point values, packed into one long per colour:
	Bit 0-4:   Frac part of blue  (5 bits)
	Bit 5-8:   Int  part of blue  (4 bits)
	Bit 9-13:  Frac part of red   (5 bits)
	Bit 14-17: Int  part of red   (4 bits)
	Bit 18-22: Frac part of green (5 bits)
	Bit 23-27: Int  part of green (5 bits)
	Bit 28-31: All zeros          (4 bits)

	The point of this format is that the two colours now can be added, and
	then be converted to a RGB table index very easily: First, we just set
	all the frac bits and the four upper zero bits to 1. It's now possible
	to get the RGB table index by anding the current value >> 5 with the
	current value >> 19. When asm-optimised, this should be the fastest
	algorithm that uses RGB tables.
*/

namespace swrenderer
{
	PalWall1Command::PalWall1Command()
	{
		using namespace drawerargs;

		_iscale = dc_iscale;
		_texturefrac = dc_texturefrac;
		_colormap = dc_colormap;
		_count = dc_count;
		_source = dc_source;
		_dest = dc_dest;
		_vlinebits = vlinebits;
		_mvlinebits = mvlinebits;
		_tmvlinebits = tmvlinebits;
		_pitch = dc_pitch;
		_srcblend = dc_srcblend;
		_destblend = dc_destblend;
	}

	PalWall4Command::PalWall4Command()
	{
		using namespace drawerargs;

		_dest = dc_dest;
		_count = dc_count;
		_pitch = dc_pitch;
		_vlinebits = vlinebits;
		_mvlinebits = mvlinebits;
		_tmvlinebits = tmvlinebits;
		for (int col = 0; col < 4; col++)
		{
			_palookupoffse[col] = palookupoffse[col];
			_bufplce[col] = bufplce[col];
			_vince[col] = vince[col];
			_vplce[col] = vplce[col];
		}
		_srcblend = dc_srcblend;
		_destblend = dc_destblend;
	}

	void DrawWall1PalCommand::Execute(DrawerThread *thread)
	{
		uint32_t fracstep = _iscale;
		uint32_t frac = _texturefrac;
		uint8_t *colormap = _colormap;
		int count = _count;
		const uint8_t *source = _source;
		uint8_t *dest = _dest;
		int bits = _vlinebits;
		int pitch = _pitch;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		do
		{
			*dest = colormap[source[frac >> bits]];
			frac += fracstep;
			dest += pitch;
		} while (--count);
	}

	void DrawWall4PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int bits = _vlinebits;
		uint32_t place;
		auto pal0 = _palookupoffse[0];
		auto pal1 = _palookupoffse[1];
		auto pal2 = _palookupoffse[2];
		auto pal3 = _palookupoffse[3];
		auto buf0 = _bufplce[0];
		auto buf1 = _bufplce[1];
		auto buf2 = _bufplce[2];
		auto buf3 = _bufplce[3];
		auto vince0 = _vince[0];
		auto vince1 = _vince[1];
		auto vince2 = _vince[2];
		auto vince3 = _vince[3];
		auto vplce0 = _vplce[0];
		auto vplce1 = _vplce[1];
		auto vplce2 = _vplce[2];
		auto vplce3 = _vplce[3];
		auto pitch = _pitch;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		vplce0 += vince0 * skipped;
		vplce1 += vince1 * skipped;
		vplce2 += vince2 * skipped;
		vplce3 += vince3 * skipped;
		vince0 *= thread->num_cores;
		vince1 *= thread->num_cores;
		vince2 *= thread->num_cores;
		vince3 *= thread->num_cores;
		pitch *= thread->num_cores;

		do
		{
			dest[0] = pal0[buf0[(place = vplce0) >> bits]]; vplce0 = place + vince0;
			dest[1] = pal1[buf1[(place = vplce1) >> bits]]; vplce1 = place + vince1;
			dest[2] = pal2[buf2[(place = vplce2) >> bits]]; vplce2 = place + vince2;
			dest[3] = pal3[buf3[(place = vplce3) >> bits]]; vplce3 = place + vince3;
			dest += pitch;
		} while (--count);
	}

	void DrawWallMasked1PalCommand::Execute(DrawerThread *thread)
	{
		uint32_t fracstep = _iscale;
		uint32_t frac = _texturefrac;
		uint8_t *colormap = _colormap;
		int count = _count;
		const uint8_t *source = _source;
		uint8_t *dest = _dest;
		int bits = _mvlinebits;
		int pitch = _pitch;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		do
		{
			uint8_t pix = source[frac >> bits];
			if (pix != 0)
			{
				*dest = colormap[pix];
			}
			frac += fracstep;
			dest += pitch;
		} while (--count);
	}

	void DrawWallMasked4PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int bits = _mvlinebits;
		uint32_t place;
		auto pal0 = _palookupoffse[0];
		auto pal1 = _palookupoffse[1];
		auto pal2 = _palookupoffse[2];
		auto pal3 = _palookupoffse[3];
		auto buf0 = _bufplce[0];
		auto buf1 = _bufplce[1];
		auto buf2 = _bufplce[2];
		auto buf3 = _bufplce[3];
		auto vince0 = _vince[0];
		auto vince1 = _vince[1];
		auto vince2 = _vince[2];
		auto vince3 = _vince[3];
		auto vplce0 = _vplce[0];
		auto vplce1 = _vplce[1];
		auto vplce2 = _vplce[2];
		auto vplce3 = _vplce[3];
		auto pitch = _pitch;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		vplce0 += vince0 * skipped;
		vplce1 += vince1 * skipped;
		vplce2 += vince2 * skipped;
		vplce3 += vince3 * skipped;
		vince0 *= thread->num_cores;
		vince1 *= thread->num_cores;
		vince2 *= thread->num_cores;
		vince3 *= thread->num_cores;
		pitch *= thread->num_cores;

		do
		{
			uint8_t pix;

			pix = buf0[(place = vplce0) >> bits]; if (pix) dest[0] = pal0[pix]; vplce0 = place + vince0;
			pix = buf1[(place = vplce1) >> bits]; if (pix) dest[1] = pal1[pix]; vplce1 = place + vince1;
			pix = buf2[(place = vplce2) >> bits]; if (pix) dest[2] = pal2[pix]; vplce2 = place + vince2;
			pix = buf3[(place = vplce3) >> bits]; if (pix) dest[3] = pal3[pix]; vplce3 = place + vince3;
			dest += pitch;
		} while (--count);
	}

	void DrawWallAdd1PalCommand::Execute(DrawerThread *thread)
	{
		uint32_t fracstep = _iscale;
		uint32_t frac = _texturefrac;
		uint8_t *colormap = _colormap;
		int count = _count;
		const uint8_t *source = _source;
		uint8_t *dest = _dest;
		int bits = _tmvlinebits;
		int pitch = _pitch;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		do
		{
			uint8_t pix = source[frac >> bits];
			if (pix != 0)
			{
				uint32_t fg = fg2rgb[colormap[pix]];
				uint32_t bg = bg2rgb[*dest];
				fg = (fg + bg) | 0x1f07c1f;
				*dest = RGB32k.All[fg & (fg >> 15)];
			}
			frac += fracstep;
			dest += pitch;
		} while (--count);
	}

	void DrawWallAdd4PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int bits = _tmvlinebits;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		uint32_t vplce[4] = { _vplce[0], _vplce[1], _vplce[2], _vplce[3] };
		uint32_t vince[4] = { _vince[0], _vince[1], _vince[2], _vince[3] };

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		for (int i = 0; i < 4; i++)
		{
			vplce[i] += vince[i] * skipped;
			vince[i] *= thread->num_cores;
		}
		pitch *= thread->num_cores;

		do
		{
			for (int i = 0; i < 4; ++i)
			{
				uint8_t pix = _bufplce[i][vplce[i] >> bits];
				if (pix != 0)
				{
					uint32_t fg = fg2rgb[_palookupoffse[i][pix]];
					uint32_t bg = bg2rgb[dest[i]];
					fg = (fg + bg) | 0x1f07c1f;
					dest[i] = RGB32k.All[fg & (fg >> 15)];
				}
				vplce[i] += vince[i];
			}
			dest += pitch;
		} while (--count);
	}

	void DrawWallAddClamp1PalCommand::Execute(DrawerThread *thread)
	{
		uint32_t fracstep = _iscale;
		uint32_t frac = _texturefrac;
		uint8_t *colormap = _colormap;
		int count = _count;
		const uint8_t *source = _source;
		uint8_t *dest = _dest;
		int bits = _tmvlinebits;
		int pitch = _pitch;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		do
		{
			uint8_t pix = source[frac >> bits];
			if (pix != 0)
			{
				uint32_t a = fg2rgb[colormap[pix]] + bg2rgb[*dest];
				uint32_t b = a;

				a |= 0x01f07c1f;
				b &= 0x40100400;
				a &= 0x3fffffff;
				b = b - (b >> 5);
				a |= b;
				*dest = RGB32k.All[a & (a >> 15)];
			}
			frac += fracstep;
			dest += pitch;
		} while (--count);
	}

	void DrawWallAddClamp4PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int bits = _tmvlinebits;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		uint32_t vplce[4] = { _vplce[0], _vplce[1], _vplce[2], _vplce[3] };
		uint32_t vince[4] = { _vince[0], _vince[1], _vince[2], _vince[3] };

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		for (int i = 0; i < 4; i++)
		{
			vplce[i] += vince[i] * skipped;
			vince[i] *= thread->num_cores;
		}
		pitch *= thread->num_cores;

		do
		{
			for (int i = 0; i < 4; ++i)
			{
				uint8_t pix = _bufplce[i][vplce[i] >> bits];
				if (pix != 0)
				{
					uint32_t a = fg2rgb[_palookupoffse[i][pix]] + bg2rgb[dest[i]];
					uint32_t b = a;

					a |= 0x01f07c1f;
					b &= 0x40100400;
					a &= 0x3fffffff;
					b = b - (b >> 5);
					a |= b;
					dest[i] = RGB32k.All[a & (a >> 15)];
				}
				vplce[i] += vince[i];
			}
			dest += pitch;
		} while (--count);
	}

	void DrawWallSubClamp1PalCommand::Execute(DrawerThread *thread)
	{
		uint32_t fracstep = _iscale;
		uint32_t frac = _texturefrac;
		uint8_t *colormap = _colormap;
		int count = _count;
		const uint8_t *source = _source;
		uint8_t *dest = _dest;
		int bits = _tmvlinebits;
		int pitch = _pitch;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		do
		{
			uint8_t pix = source[frac >> bits];
			if (pix != 0)
			{
				uint32_t a = (fg2rgb[colormap[pix]] | 0x40100400) - bg2rgb[*dest];
				uint32_t b = a;

				b &= 0x40100400;
				b = b - (b >> 5);
				a &= b;
				a |= 0x01f07c1f;
				*dest = RGB32k.All[a & (a >> 15)];
			}
			frac += fracstep;
			dest += pitch;
		} while (--count);
	}

	void DrawWallSubClamp4PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int bits = _tmvlinebits;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		uint32_t vplce[4] = { _vplce[0], _vplce[1], _vplce[2], _vplce[3] };
		uint32_t vince[4] = { _vince[0], _vince[1], _vince[2], _vince[3] };

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		for (int i = 0; i < 4; i++)
		{
			vplce[i] += vince[i] * skipped;
			vince[i] *= thread->num_cores;
		}
		pitch *= thread->num_cores;

		do
		{
			for (int i = 0; i < 4; ++i)
			{
				uint8_t pix = _bufplce[i][vplce[i] >> bits];
				if (pix != 0)
				{
					uint32_t a = (fg2rgb[_palookupoffse[i][pix]] | 0x40100400) - bg2rgb[dest[i]];
					uint32_t b = a;

					b &= 0x40100400;
					b = b - (b >> 5);
					a &= b;
					a |= 0x01f07c1f;
					dest[i] = RGB32k.All[a & (a >> 15)];
				}
				vplce[i] += vince[i];
			}
			dest += pitch;
		} while (--count);
	}

	void DrawWallRevSubClamp1PalCommand::Execute(DrawerThread *thread)
	{
		uint32_t fracstep = _iscale;
		uint32_t frac = _texturefrac;
		uint8_t *colormap = _colormap;
		int count = _count;
		const uint8_t *source = _source;
		uint8_t *dest = _dest;
		int bits = _tmvlinebits;
		int pitch = _pitch;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		do
		{
			uint8_t pix = source[frac >> bits];
			if (pix != 0)
			{
				uint32_t a = (bg2rgb[*dest] | 0x40100400) - fg2rgb[colormap[pix]];
				uint32_t b = a;

				b &= 0x40100400;
				b = b - (b >> 5);
				a &= b;
				a |= 0x01f07c1f;
				*dest = RGB32k.All[a & (a >> 15)];
			}
			frac += fracstep;
			dest += pitch;
		} while (--count);
	}

	void DrawWallRevSubClamp4PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int bits = _tmvlinebits;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		uint32_t vplce[4] = { _vplce[0], _vplce[1], _vplce[2], _vplce[3] };
		uint32_t vince[4] = { _vince[0], _vince[1], _vince[2], _vince[3] };

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		for (int i = 0; i < 4; i++)
		{
			vplce[i] += vince[i] * skipped;
			vince[i] *= thread->num_cores;
		}
		pitch *= thread->num_cores;

		do
		{
			for (int i = 0; i < 4; ++i)
			{
				uint8_t pix = _bufplce[i][vplce[i] >> bits];
				if (pix != 0)
				{
					uint32_t a = (bg2rgb[dest[i]] | 0x40100400) - fg2rgb[_palookupoffse[i][pix]];
					uint32_t b = a;

					b &= 0x40100400;
					b = b - (b >> 5);
					a &= b;
					a |= 0x01f07c1f;
					dest[i] = RGB32k.All[a & (a >> 15)];
				}
				vplce[i] += vince[i];
			}
			dest += _pitch;
		} while (--count);
	}

	/////////////////////////////////////////////////////////////////////////

	PalSkyCommand::PalSkyCommand(uint32_t solid_top, uint32_t solid_bottom) : solid_top(solid_top), solid_bottom(solid_bottom)
	{
		using namespace drawerargs;

		_dest = dc_dest;
		_count = dc_count;
		_pitch = dc_pitch;
		for (int col = 0; col < 4; col++)
		{
			_bufplce[col] = bufplce[col];
			_bufplce2[col] = bufplce2[col];
			_bufheight[col] = bufheight[col];
			_vince[col] = vince[col];
			_vplce[col] = vplce[col];
		}
	}

	void DrawSingleSky1PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int pitch = _pitch;
		const uint8_t *source0 = _bufplce[0];
		int textureheight0 = _bufheight[0];

		int32_t frac = _vplce[0];
		int32_t fracstep = _vince[0];

		int start_fade = 2; // How fast it should fade out

		int solid_top_r = RPART(solid_top);
		int solid_top_g = GPART(solid_top);
		int solid_top_b = BPART(solid_top);
		int solid_bottom_r = RPART(solid_bottom);
		int solid_bottom_g = GPART(solid_bottom);
		int solid_bottom_b = BPART(solid_bottom);

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * skipped;
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		for (int index = 0; index < count; index++)
		{
			uint32_t sample_index = (((((uint32_t)frac) << 8) >> FRACBITS) * textureheight0) >> FRACBITS;
			uint8_t fg = source0[sample_index];

			int alpha_top = MAX(MIN(frac >> (16 - start_fade), 256), 0);
			int alpha_bottom = MAX(MIN(((2 << 24) - frac) >> (16 - start_fade), 256), 0);

			if (alpha_top == 256 && alpha_bottom == 256)
			{
				*dest = fg;
			}
			else
			{
				int inv_alpha_top = 256 - alpha_top;
				int inv_alpha_bottom = 256 - alpha_bottom;

				const auto &c = GPalette.BaseColors[fg];
				int c_red = c.r;
				int c_green = c.g;
				int c_blue = c.b;
				c_red = (c_red * alpha_top + solid_top_r * inv_alpha_top) >> 8;
				c_green = (c_green * alpha_top + solid_top_g * inv_alpha_top) >> 8;
				c_blue = (c_blue * alpha_top + solid_top_b * inv_alpha_top) >> 8;
				c_red = (c_red * alpha_bottom + solid_bottom_r * inv_alpha_bottom) >> 8;
				c_green = (c_green * alpha_bottom + solid_bottom_g * inv_alpha_bottom) >> 8;
				c_blue = (c_blue * alpha_bottom + solid_bottom_b * inv_alpha_bottom) >> 8;
				*dest = RGB32k.RGB[(c_red >> 3)][(c_green >> 3)][(c_blue >> 3)];
			}

			frac += fracstep;
			dest += pitch;
		}
	}

	void DrawSingleSky4PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int pitch = _pitch;
		const uint8_t *source0[4] = { _bufplce[0], _bufplce[1], _bufplce[2], _bufplce[3] };
		int textureheight0 = _bufheight[0];
		const uint32_t *palette = (const uint32_t *)GPalette.BaseColors;
		int32_t frac[4] = { (int32_t)_vplce[0], (int32_t)_vplce[1], (int32_t)_vplce[2], (int32_t)_vplce[3] };
		int32_t fracstep[4] = { (int32_t)_vince[0], (int32_t)_vince[1], (int32_t)_vince[2], (int32_t)_vince[3] };
		uint8_t output[4];

		int start_fade = 2; // How fast it should fade out

		int solid_top_r = RPART(solid_top);
		int solid_top_g = GPART(solid_top);
		int solid_top_b = BPART(solid_top);
		int solid_bottom_r = RPART(solid_bottom);
		int solid_bottom_g = GPART(solid_bottom);
		int solid_bottom_b = BPART(solid_bottom);
		uint32_t solid_top_fill = RGB32k.RGB[(solid_top_r >> 3)][(solid_top_g >> 3)][(solid_top_b >> 3)];
		uint32_t solid_bottom_fill = RGB32k.RGB[(solid_bottom_r >> 3)][(solid_bottom_g >> 3)][(solid_bottom_b >> 3)];
		solid_top_fill = (solid_top_fill << 24) | (solid_top_fill << 16) | (solid_top_fill << 8) | solid_top_fill;
		solid_bottom_fill = (solid_bottom_fill << 24) | (solid_bottom_fill << 16) | (solid_bottom_fill << 8) | solid_bottom_fill;

		// Find bands for top solid color, top fade, center textured, bottom fade, bottom solid color:
		int fade_length = (1 << (24 - start_fade));
		int start_fadetop_y = (-frac[0]) / fracstep[0];
		int end_fadetop_y = (fade_length - frac[0]) / fracstep[0];
		int start_fadebottom_y = ((2 << 24) - fade_length - frac[0]) / fracstep[0];
		int end_fadebottom_y = ((2 << 24) - frac[0]) / fracstep[0];
		for (int col = 1; col < 4; col++)
		{
			start_fadetop_y = MIN(start_fadetop_y, (-frac[0]) / fracstep[0]);
			end_fadetop_y = MAX(end_fadetop_y, (fade_length - frac[0]) / fracstep[0]);
			start_fadebottom_y = MIN(start_fadebottom_y, ((2 << 24) - fade_length - frac[0]) / fracstep[0]);
			end_fadebottom_y = MAX(end_fadebottom_y, ((2 << 24) - frac[0]) / fracstep[0]);
		}
		start_fadetop_y = clamp(start_fadetop_y, 0, count);
		end_fadetop_y = clamp(end_fadetop_y, 0, count);
		start_fadebottom_y = clamp(start_fadebottom_y, 0, count);
		end_fadebottom_y = clamp(end_fadebottom_y, 0, count);

		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		for (int col = 0; col < 4; col++)
		{
			frac[col] += fracstep[col] * skipped;
			fracstep[col] *= thread->num_cores;
		}
		pitch *= thread->num_cores;
		int num_cores = thread->num_cores;
		int index = skipped;

		// Top solid color:
		while (index < start_fadetop_y)
		{
			*((uint32_t*)dest) = solid_top_fill;
			dest += pitch;
			for (int col = 0; col < 4; col++)
				frac[col] += fracstep[col];
			index += num_cores;
		}

		// Top fade:
		while (index < end_fadetop_y)
		{
			for (int col = 0; col < 4; col++)
			{
				uint32_t sample_index = (((((uint32_t)frac[col]) << 8) >> FRACBITS) * textureheight0) >> FRACBITS;
				uint8_t fg = source0[col][sample_index];

				uint32_t c = palette[fg];
				int alpha_top = MAX(MIN(frac[col] >> (16 - start_fade), 256), 0);
				int inv_alpha_top = 256 - alpha_top;
				int c_red = RPART(c);
				int c_green = GPART(c);
				int c_blue = BPART(c);
				c_red = (c_red * alpha_top + solid_top_r * inv_alpha_top) >> 8;
				c_green = (c_green * alpha_top + solid_top_g * inv_alpha_top) >> 8;
				c_blue = (c_blue * alpha_top + solid_top_b * inv_alpha_top) >> 8;
				output[col] = RGB32k.RGB[(c_red >> 3)][(c_green >> 3)][(c_blue >> 3)];

				frac[col] += fracstep[col];
			}
			*((uint32_t*)dest) = *((uint32_t*)output);
			dest += pitch;
			index += num_cores;
		}

		// Textured center:
		while (index < start_fadebottom_y)
		{
			for (int col = 0; col < 4; col++)
			{
				uint32_t sample_index = (((((uint32_t)frac[col]) << 8) >> FRACBITS) * textureheight0) >> FRACBITS;
				output[col] = source0[col][sample_index];

				frac[col] += fracstep[col];
			}

			*((uint32_t*)dest) = *((uint32_t*)output);
			dest += pitch;
			index += num_cores;
		}

		// Fade bottom:
		while (index < end_fadebottom_y)
		{
			for (int col = 0; col < 4; col++)
			{
				uint32_t sample_index = (((((uint32_t)frac[col]) << 8) >> FRACBITS) * textureheight0) >> FRACBITS;
				uint8_t fg = source0[col][sample_index];

				uint32_t c = palette[fg];
				int alpha_bottom = MAX(MIN(((2 << 24) - frac[col]) >> (16 - start_fade), 256), 0);
				int inv_alpha_bottom = 256 - alpha_bottom;
				int c_red = RPART(c);
				int c_green = GPART(c);
				int c_blue = BPART(c);
				c_red = (c_red * alpha_bottom + solid_bottom_r * inv_alpha_bottom) >> 8;
				c_green = (c_green * alpha_bottom + solid_bottom_g * inv_alpha_bottom) >> 8;
				c_blue = (c_blue * alpha_bottom + solid_bottom_b * inv_alpha_bottom) >> 8;
				output[col] = RGB32k.RGB[(c_red >> 3)][(c_green >> 3)][(c_blue >> 3)];

				frac[col] += fracstep[col];
			}
			*((uint32_t*)dest) = *((uint32_t*)output);
			dest += pitch;
			index += num_cores;
		}

		// Bottom solid color:
		while (index < count)
		{
			*((uint32_t*)dest) = solid_bottom_fill;
			dest += pitch;
			index += num_cores;
		}
	}

	void DrawDoubleSky1PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int pitch = _pitch;
		const uint8_t *source0 = _bufplce[0];
		const uint8_t *source1 = _bufplce2[0];
		int textureheight0 = _bufheight[0];
		uint32_t maxtextureheight1 = _bufheight[1] - 1;

		int32_t frac = _vplce[0];
		int32_t fracstep = _vince[0];

		int start_fade = 2; // How fast it should fade out

		int solid_top_r = RPART(solid_top);
		int solid_top_g = GPART(solid_top);
		int solid_top_b = BPART(solid_top);
		int solid_bottom_r = RPART(solid_bottom);
		int solid_bottom_g = GPART(solid_bottom);
		int solid_bottom_b = BPART(solid_bottom);

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * skipped;
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		for (int index = 0; index < count; index++)
		{
			uint32_t sample_index = (((((uint32_t)frac) << 8) >> FRACBITS) * textureheight0) >> FRACBITS;
			uint8_t fg = source0[sample_index];
			if (fg == 0)
			{
				uint32_t sample_index2 = MIN(sample_index, maxtextureheight1);
				fg = source1[sample_index2];
			}

			int alpha_top = MAX(MIN(frac >> (16 - start_fade), 256), 0);
			int alpha_bottom = MAX(MIN(((2 << 24) - frac) >> (16 - start_fade), 256), 0);

			if (alpha_top == 256 && alpha_bottom == 256)
			{
				*dest = fg;
			}
			else
			{
				int inv_alpha_top = 256 - alpha_top;
				int inv_alpha_bottom = 256 - alpha_bottom;

				const auto &c = GPalette.BaseColors[fg];
				int c_red = c.r;
				int c_green = c.g;
				int c_blue = c.b;
				c_red = (c_red * alpha_top + solid_top_r * inv_alpha_top) >> 8;
				c_green = (c_green * alpha_top + solid_top_g * inv_alpha_top) >> 8;
				c_blue = (c_blue * alpha_top + solid_top_b * inv_alpha_top) >> 8;
				c_red = (c_red * alpha_bottom + solid_bottom_r * inv_alpha_bottom) >> 8;
				c_green = (c_green * alpha_bottom + solid_bottom_g * inv_alpha_bottom) >> 8;
				c_blue = (c_blue * alpha_bottom + solid_bottom_b * inv_alpha_bottom) >> 8;
				*dest = RGB32k.RGB[(c_red >> 3)][(c_green >> 3)][(c_blue >> 3)];
			}

			frac += fracstep;
			dest += pitch;
		}
	}

	void DrawDoubleSky4PalCommand::Execute(DrawerThread *thread)
	{
		uint8_t *dest = _dest;
		int count = _count;
		int pitch = _pitch;
		const uint8_t *source0[4] = { _bufplce[0], _bufplce[1], _bufplce[2], _bufplce[3] };
		const uint8_t *source1[4] = { _bufplce2[0], _bufplce2[1], _bufplce2[2], _bufplce2[3] };
		int textureheight0 = _bufheight[0];
		uint32_t maxtextureheight1 = _bufheight[1] - 1;
		const uint32_t *palette = (const uint32_t *)GPalette.BaseColors;
		int32_t frac[4] = { (int32_t)_vplce[0], (int32_t)_vplce[1], (int32_t)_vplce[2], (int32_t)_vplce[3] };
		int32_t fracstep[4] = { (int32_t)_vince[0], (int32_t)_vince[1], (int32_t)_vince[2], (int32_t)_vince[3] };
		uint8_t output[4];

		int start_fade = 2; // How fast it should fade out

		int solid_top_r = RPART(solid_top);
		int solid_top_g = GPART(solid_top);
		int solid_top_b = BPART(solid_top);
		int solid_bottom_r = RPART(solid_bottom);
		int solid_bottom_g = GPART(solid_bottom);
		int solid_bottom_b = BPART(solid_bottom);
		uint32_t solid_top_fill = RGB32k.RGB[(solid_top_r >> 3)][(solid_top_g >> 3)][(solid_top_b >> 3)];
		uint32_t solid_bottom_fill = RGB32k.RGB[(solid_bottom_r >> 3)][(solid_bottom_g >> 3)][(solid_bottom_b >> 3)];
		solid_top_fill = (solid_top_fill << 24) | (solid_top_fill << 16) | (solid_top_fill << 8) | solid_top_fill;
		solid_bottom_fill = (solid_bottom_fill << 24) | (solid_bottom_fill << 16) | (solid_bottom_fill << 8) | solid_bottom_fill;

		// Find bands for top solid color, top fade, center textured, bottom fade, bottom solid color:
		int fade_length = (1 << (24 - start_fade));
		int start_fadetop_y = (-frac[0]) / fracstep[0];
		int end_fadetop_y = (fade_length - frac[0]) / fracstep[0];
		int start_fadebottom_y = ((2 << 24) - fade_length - frac[0]) / fracstep[0];
		int end_fadebottom_y = ((2 << 24) - frac[0]) / fracstep[0];
		for (int col = 1; col < 4; col++)
		{
			start_fadetop_y = MIN(start_fadetop_y, (-frac[0]) / fracstep[0]);
			end_fadetop_y = MAX(end_fadetop_y, (fade_length - frac[0]) / fracstep[0]);
			start_fadebottom_y = MIN(start_fadebottom_y, ((2 << 24) - fade_length - frac[0]) / fracstep[0]);
			end_fadebottom_y = MAX(end_fadebottom_y, ((2 << 24) - frac[0]) / fracstep[0]);
		}
		start_fadetop_y = clamp(start_fadetop_y, 0, count);
		end_fadetop_y = clamp(end_fadetop_y, 0, count);
		start_fadebottom_y = clamp(start_fadebottom_y, 0, count);
		end_fadebottom_y = clamp(end_fadebottom_y, 0, count);

		int skipped = thread->skipped_by_thread(_dest_y);
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		for (int col = 0; col < 4; col++)
		{
			frac[col] += fracstep[col] * skipped;
			fracstep[col] *= thread->num_cores;
		}
		pitch *= thread->num_cores;
		int num_cores = thread->num_cores;
		int index = skipped;

		// Top solid color:
		while (index < start_fadetop_y)
		{
			*((uint32_t*)dest) = solid_top_fill;
			dest += pitch;
			for (int col = 0; col < 4; col++)
				frac[col] += fracstep[col];
			index += num_cores;
		}

		// Top fade:
		while (index < end_fadetop_y)
		{
			for (int col = 0; col < 4; col++)
			{
				uint32_t sample_index = (((((uint32_t)frac[col]) << 8) >> FRACBITS) * textureheight0) >> FRACBITS;
				uint8_t fg = source0[col][sample_index];
				if (fg == 0)
				{
					uint32_t sample_index2 = MIN(sample_index, maxtextureheight1);
					fg = source1[col][sample_index2];
				}
				output[col] = fg;

				uint32_t c = palette[fg];
				int alpha_top = MAX(MIN(frac[col] >> (16 - start_fade), 256), 0);
				int inv_alpha_top = 256 - alpha_top;
				int c_red = RPART(c);
				int c_green = GPART(c);
				int c_blue = BPART(c);
				c_red = (c_red * alpha_top + solid_top_r * inv_alpha_top) >> 8;
				c_green = (c_green * alpha_top + solid_top_g * inv_alpha_top) >> 8;
				c_blue = (c_blue * alpha_top + solid_top_b * inv_alpha_top) >> 8;
				output[col] = RGB32k.RGB[(c_red >> 3)][(c_green >> 3)][(c_blue >> 3)];

				frac[col] += fracstep[col];
			}
			*((uint32_t*)dest) = *((uint32_t*)output);
			dest += pitch;
			index += num_cores;
		}

		// Textured center:
		while (index < start_fadebottom_y)
		{
			for (int col = 0; col < 4; col++)
			{
				uint32_t sample_index = (((((uint32_t)frac[col]) << 8) >> FRACBITS) * textureheight0) >> FRACBITS;
				uint8_t fg = source0[col][sample_index];
				if (fg == 0)
				{
					uint32_t sample_index2 = MIN(sample_index, maxtextureheight1);
					fg = source1[col][sample_index2];
				}
				output[col] = fg;

				frac[col] += fracstep[col];
			}

			*((uint32_t*)dest) = *((uint32_t*)output);
			dest += pitch;
			index += num_cores;
		}

		// Fade bottom:
		while (index < end_fadebottom_y)
		{
			for (int col = 0; col < 4; col++)
			{
				uint32_t sample_index = (((((uint32_t)frac[col]) << 8) >> FRACBITS) * textureheight0) >> FRACBITS;
				uint8_t fg = source0[col][sample_index];
				if (fg == 0)
				{
					uint32_t sample_index2 = MIN(sample_index, maxtextureheight1);
					fg = source1[col][sample_index2];
				}
				output[col] = fg;

				uint32_t c = palette[fg];
				int alpha_bottom = MAX(MIN(((2 << 24) - frac[col]) >> (16 - start_fade), 256), 0);
				int inv_alpha_bottom = 256 - alpha_bottom;
				int c_red = RPART(c);
				int c_green = GPART(c);
				int c_blue = BPART(c);
				c_red = (c_red * alpha_bottom + solid_bottom_r * inv_alpha_bottom) >> 8;
				c_green = (c_green * alpha_bottom + solid_bottom_g * inv_alpha_bottom) >> 8;
				c_blue = (c_blue * alpha_bottom + solid_bottom_b * inv_alpha_bottom) >> 8;
				output[col] = RGB32k.RGB[(c_red >> 3)][(c_green >> 3)][(c_blue >> 3)];

				frac[col] += fracstep[col];
			}
			*((uint32_t*)dest) = *((uint32_t*)output);
			dest += pitch;
			index += num_cores;
		}

		// Bottom solid color:
		while (index < count)
		{
			*((uint32_t*)dest) = solid_bottom_fill;
			dest += pitch;
			index += num_cores;
		}
	}

	/////////////////////////////////////////////////////////////////////////

	PalColumnCommand::PalColumnCommand()
	{
		using namespace drawerargs;

		_count = dc_count;
		_dest = dc_dest;
		_pitch = dc_pitch;
		_iscale = dc_iscale;
		_texturefrac = dc_texturefrac;
		_colormap = dc_colormap;
		_source = dc_source;
		_translation = dc_translation;
		_color = dc_color;
		_srcblend = dc_srcblend;
		_destblend = dc_destblend;
		_srccolor = dc_srccolor;
	}

	void DrawColumnPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;

		// Framebuffer destination address.
		dest = _dest;

		// Determine scaling,
		//	which is the only mapping to be done.
		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		// [RH] Get local copies of these variables so that the compiler
		//		has a better chance of optimizing this well.
		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;

		// Inner loop that does the actual texture mapping,
		//	e.g. a DDA-lile scaling.
		// This is as fast as it gets.
		do
		{
			// Re-map color indices from wall texture column
			//	using a lighting/special effects LUT.
			*dest = colormap[source[frac >> FRACBITS]];

			dest += pitch;
			frac += fracstep;

		} while (--count);
	}

	void FillColumnPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;

		count = _count;
		dest = _dest;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		pitch *= thread->num_cores;

		uint8_t color = _color;
		do
		{
			*dest = color;
			dest += pitch;
		} while (--count);
	}

	void FillColumnAddPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;

		count = _count;
		dest = _dest;
		uint32_t *bg2rgb;
		uint32_t fg;

		bg2rgb = _destblend;
		fg = _srccolor;
		int pitch = _pitch;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		pitch *= thread->num_cores;

		do
		{
			uint32_t bg;
			bg = (fg + bg2rgb[*dest]) | 0x1f07c1f;
			*dest = RGB32k.All[bg & (bg >> 15)];
			dest += pitch;
		} while (--count);

	}

	void FillColumnAddClampPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;

		count = _count;

		dest = _dest;
		uint32_t *bg2rgb;
		uint32_t fg;

		bg2rgb = _destblend;
		fg = _srccolor;
		int pitch = _pitch;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		pitch *= thread->num_cores;

		do
		{
			uint32_t a = fg + bg2rgb[*dest];
			uint32_t b = a;

			a |= 0x01f07c1f;
			b &= 0x40100400;
			a &= 0x3fffffff;
			b = b - (b >> 5);
			a |= b;
			*dest = RGB32k.All[a & (a >> 15)];
			dest += pitch;
		} while (--count);
	}

	void FillColumnSubClampPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;

		count = _count;

		dest = _dest;
		uint32_t *bg2rgb;
		uint32_t fg;

		bg2rgb = _destblend;
		fg = _srccolor | 0x40100400;
		int pitch = _pitch;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		pitch *= thread->num_cores;

		do
		{
			uint32_t a = fg - bg2rgb[*dest];
			uint32_t b = a;

			b &= 0x40100400;
			b = b - (b >> 5);
			a &= b;
			a |= 0x01f07c1f;
			*dest = RGB32k.All[a & (a >> 15)];
			dest += pitch;
		} while (--count);
	}

	void FillColumnRevSubClampPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;

		count = _count;
		if (count <= 0)
			return;

		dest = _dest;
		uint32_t *bg2rgb;
		uint32_t fg;

		bg2rgb = _destblend;
		fg = _srccolor;
		int pitch = _pitch;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		pitch *= thread->num_cores;

		do
		{
			uint32_t a = (bg2rgb[*dest] | 0x40100400) - fg;
			uint32_t b = a;

			b &= 0x40100400;
			b = b - (b >> 5);
			a &= b;
			a |= 0x01f07c1f;
			*dest = RGB32k.All[a & (a >> 15)];
			dest += pitch;
		} while (--count);
	}

	void DrawColumnAddPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;
		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;

		do
		{
			uint32_t fg = colormap[source[frac >> FRACBITS]];
			uint32_t bg = *dest;

			fg = fg2rgb[fg];
			bg = bg2rgb[bg];
			fg = (fg + bg) | 0x1f07c1f;
			*dest = RGB32k.All[fg & (fg >> 15)];
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	void DrawColumnTranslatedPalCommand::Execute(DrawerThread *thread)
	{
		int 				count;
		uint8_t*				dest;
		fixed_t 			frac;
		fixed_t 			fracstep;

		count = _count;

		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		// [RH] Local copies of global vars to improve compiler optimizations
		const uint8_t *colormap = _colormap;
		const uint8_t *translation = _translation;
		const uint8_t *source = _source;

		do
		{
			*dest = colormap[translation[source[frac >> FRACBITS]]];
			dest += pitch;

			frac += fracstep;
		} while (--count);
	}

	void DrawColumnTlatedAddPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;
		const uint8_t *translation = _translation;
		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;

		do
		{
			uint32_t fg = colormap[translation[source[frac >> FRACBITS]]];
			uint32_t bg = *dest;

			fg = fg2rgb[fg];
			bg = bg2rgb[bg];
			fg = (fg + bg) | 0x1f07c1f;
			*dest = RGB32k.All[fg & (fg >> 15)];
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	void DrawColumnShadedPalCommand::Execute(DrawerThread *thread)
	{
		int  count;
		uint8_t *dest;
		fixed_t frac, fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		const uint8_t *source = _source;
		const uint8_t *colormap = _colormap;
		uint32_t *fgstart = &Col2RGB8[0][_color];

		do
		{
			uint32_t val = colormap[source[frac >> FRACBITS]];
			uint32_t fg = fgstart[val << 8];
			val = (Col2RGB8[64 - val][*dest] + fg) | 0x1f07c1f;
			*dest = RGB32k.All[val & (val >> 15)];

			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	void DrawColumnAddClampPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		do
		{
			uint32_t a = fg2rgb[colormap[source[frac >> FRACBITS]]] + bg2rgb[*dest];
			uint32_t b = a;

			a |= 0x01f07c1f;
			b &= 0x40100400;
			a &= 0x3fffffff;
			b = b - (b >> 5);
			a |= b;
			*dest = RGB32k.All[a & (a >> 15)];
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	void DrawColumnAddClampTranslatedPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		const uint8_t *translation = _translation;
		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		do
		{
			uint32_t a = fg2rgb[colormap[translation[source[frac >> FRACBITS]]]] + bg2rgb[*dest];
			uint32_t b = a;

			a |= 0x01f07c1f;
			b &= 0x40100400;
			a &= 0x3fffffff;
			b = b - (b >> 5);
			a |= b;
			*dest = RGB32k.All[(a >> 15) & a];
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	void DrawColumnSubClampPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		do
		{
			uint32_t a = (fg2rgb[colormap[source[frac >> FRACBITS]]] | 0x40100400) - bg2rgb[*dest];
			uint32_t b = a;

			b &= 0x40100400;
			b = b - (b >> 5);
			a &= b;
			a |= 0x01f07c1f;
			*dest = RGB32k.All[a & (a >> 15)];
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	void DrawColumnSubClampTranslatedPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		const uint8_t *translation = _translation;
		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		do
		{
			uint32_t a = (fg2rgb[colormap[translation[source[frac >> FRACBITS]]]] | 0x40100400) - bg2rgb[*dest];
			uint32_t b = a;

			b &= 0x40100400;
			b = b - (b >> 5);
			a &= b;
			a |= 0x01f07c1f;
			*dest = RGB32k.All[(a >> 15) & a];
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	void DrawColumnRevSubClampPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		do
		{
			uint32_t a = (bg2rgb[*dest] | 0x40100400) - fg2rgb[colormap[source[frac >> FRACBITS]]];
			uint32_t b = a;

			b &= 0x40100400;
			b = b - (b >> 5);
			a &= b;
			a |= 0x01f07c1f;
			*dest = RGB32k.All[a & (a >> 15)];
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	void DrawColumnRevSubClampTranslatedPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;
		fixed_t frac;
		fixed_t fracstep;

		count = _count;
		dest = _dest;

		fracstep = _iscale;
		frac = _texturefrac;

		count = thread->count_for_thread(_dest_y, count);
		if (count <= 0)
			return;

		int pitch = _pitch;
		dest = thread->dest_for_thread(_dest_y, pitch, dest);
		frac += fracstep * thread->skipped_by_thread(_dest_y);
		fracstep *= thread->num_cores;
		pitch *= thread->num_cores;

		const uint8_t *translation = _translation;
		const uint8_t *colormap = _colormap;
		const uint8_t *source = _source;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		do
		{
			uint32_t a = (bg2rgb[*dest] | 0x40100400) - fg2rgb[colormap[translation[source[frac >> FRACBITS]]]];
			uint32_t b = a;

			b &= 0x40100400;
			b = b - (b >> 5);
			a &= b;
			a |= 0x01f07c1f;
			*dest = RGB32k.All[(a >> 15) & a];
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}

	/////////////////////////////////////////////////////////////////////////

	DrawFuzzColumnPalCommand::DrawFuzzColumnPalCommand()
	{
		using namespace drawerargs;

		_yl = dc_yl;
		_yh = dc_yh;
		_x = dc_x;
		_destorg = dc_destorg;
		_pitch = dc_pitch;
	}

	void DrawFuzzColumnPalCommand::Execute(DrawerThread *thread)
	{
		int count;
		uint8_t *dest;

		// Adjust borders. Low...
		if (_yl == 0)
			_yl = 1;

		// .. and high.
		if (_yh > fuzzviewheight)
			_yh = fuzzviewheight;

		count = _yh - _yl;

		// Zero length.
		if (count < 0)
			return;

		count++;

		dest = ylookup[_yl] + _x + _destorg;

		// colormap #6 is used for shading (of 0-31, a bit brighter than average)
		{
			// [RH] Make local copies of global vars to try and improve
			//		the optimizations made by the compiler.
			int pitch = _pitch;
			int fuzz = fuzzpos;
			int cnt;
			uint8_t *map = &NormalLight.Maps[6 * 256];

			// [RH] Split this into three separate loops to minimize
			// the number of times fuzzpos needs to be clamped.
			if (fuzz)
			{
				cnt = MIN(FUZZTABLE - fuzz, count);
				count -= cnt;
				do
				{
					*dest = map[dest[fuzzoffset[fuzz++]]];
					dest += pitch;
				} while (--cnt);
			}
			if (fuzz == FUZZTABLE || count > 0)
			{
				while (count >= FUZZTABLE)
				{
					fuzz = 0;
					cnt = FUZZTABLE;
					count -= FUZZTABLE;
					do
					{
						*dest = map[dest[fuzzoffset[fuzz++]]];
						dest += pitch;
					} while (--cnt);
				}
				fuzz = 0;
				if (count > 0)
				{
					do
					{
						*dest = map[dest[fuzzoffset[fuzz++]]];
						dest += pitch;
					} while (--count);
				}
			}
			fuzzpos = fuzz;
		}
	}

	/////////////////////////////////////////////////////////////////////////

	PalSpanCommand::PalSpanCommand()
	{
		using namespace drawerargs;

		_source = ds_source;
		_colormap = ds_colormap;
		_xfrac = ds_xfrac;
		_yfrac = ds_yfrac;
		_y = ds_y;
		_x1 = ds_x1;
		_x2 = ds_x2;
		_destorg = dc_destorg;
		_xstep = ds_xstep;
		_ystep = ds_ystep;
		_xbits = ds_xbits;
		_ybits = ds_ybits;
		_srcblend = dc_srcblend;
		_destblend = dc_destblend;
		_color = ds_color;
	}

	void DrawSpanPalCommand::Execute(DrawerThread *thread)
	{
		if (thread->skipped_by_thread(_y))
			return;

		dsfixed_t xfrac;
		dsfixed_t yfrac;
		dsfixed_t xstep;
		dsfixed_t ystep;
		uint8_t *dest;
		const uint8_t *source = _source;
		const uint8_t *colormap = _colormap;
		int count;
		int spot;

		xfrac = _xfrac;
		yfrac = _yfrac;

		dest = ylookup[_y] + _x1 + _destorg;

		count = _x2 - _x1 + 1;

		xstep = _xstep;
		ystep = _ystep;

		if (_xbits == 6 && _ybits == 6)
		{
			// 64x64 is the most common case by far, so special case it.
			do
			{
				// Current texture index in u,v.
				spot = ((xfrac >> (32 - 6 - 6))&(63 * 64)) + (yfrac >> (32 - 6));

				// Lookup pixel from flat texture tile,
				//  re-index using light/colormap.
				*dest++ = colormap[source[spot]];

				// Next step in u,v.
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
		else
		{
			uint8_t yshift = 32 - _ybits;
			uint8_t xshift = yshift - _xbits;
			int xmask = ((1 << _xbits) - 1) << _ybits;

			do
			{
				// Current texture index in u,v.
				spot = ((xfrac >> xshift) & xmask) + (yfrac >> yshift);

				// Lookup pixel from flat texture tile,
				//  re-index using light/colormap.
				*dest++ = colormap[source[spot]];

				// Next step in u,v.
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
	}

	void DrawSpanMaskedPalCommand::Execute(DrawerThread *thread)
	{
		if (thread->skipped_by_thread(_y))
			return;

		dsfixed_t xfrac;
		dsfixed_t yfrac;
		dsfixed_t xstep;
		dsfixed_t ystep;
		uint8_t *dest;
		const uint8_t *source = _source;
		const uint8_t *colormap = _colormap;
		int count;
		int spot;

		xfrac = _xfrac;
		yfrac = _yfrac;

		dest = ylookup[_y] + _x1 + _destorg;

		count = _x2 - _x1 + 1;

		xstep = _xstep;
		ystep = _ystep;

		if (_xbits == 6 && _ybits == 6)
		{
			// 64x64 is the most common case by far, so special case it.
			do
			{
				int texdata;

				spot = ((xfrac >> (32 - 6 - 6))&(63 * 64)) + (yfrac >> (32 - 6));
				texdata = source[spot];
				if (texdata != 0)
				{
					*dest = colormap[texdata];
				}
				dest++;
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
		else
		{
			uint8_t yshift = 32 - _ybits;
			uint8_t xshift = yshift - _xbits;
			int xmask = ((1 << _xbits) - 1) << _ybits;
			do
			{
				int texdata;

				spot = ((xfrac >> xshift) & xmask) + (yfrac >> yshift);
				texdata = source[spot];
				if (texdata != 0)
				{
					*dest = colormap[texdata];
				}
				dest++;
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
	}

	void DrawSpanTranslucentPalCommand::Execute(DrawerThread *thread)
	{
		if (thread->skipped_by_thread(_y))
			return;

		dsfixed_t xfrac;
		dsfixed_t yfrac;
		dsfixed_t xstep;
		dsfixed_t ystep;
		uint8_t *dest;
		const uint8_t *source = _source;
		const uint8_t *colormap = _colormap;
		int count;
		int spot;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		xfrac = _xfrac;
		yfrac = _yfrac;

		dest = ylookup[_y] + _x1 + _destorg;

		count = _x2 - _x1 + 1;

		xstep = _xstep;
		ystep = _ystep;

		if (_xbits == 6 && _ybits == 6)
		{
			// 64x64 is the most common case by far, so special case it.
			do
			{
				spot = ((xfrac >> (32 - 6 - 6))&(63 * 64)) + (yfrac >> (32 - 6));
				uint32_t fg = colormap[source[spot]];
				uint32_t bg = *dest;
				fg = fg2rgb[fg];
				bg = bg2rgb[bg];
				fg = (fg + bg) | 0x1f07c1f;
				*dest++ = RGB32k.All[fg & (fg >> 15)];
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
		else
		{
			uint8_t yshift = 32 - _ybits;
			uint8_t xshift = yshift - _xbits;
			int xmask = ((1 << _xbits) - 1) << _ybits;
			do
			{
				spot = ((xfrac >> xshift) & xmask) + (yfrac >> yshift);
				uint32_t fg = colormap[source[spot]];
				uint32_t bg = *dest;
				fg = fg2rgb[fg];
				bg = bg2rgb[bg];
				fg = (fg + bg) | 0x1f07c1f;
				*dest++ = RGB32k.All[fg & (fg >> 15)];
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
	}

	void DrawSpanMaskedTranslucentPalCommand::Execute(DrawerThread *thread)
	{
		if (thread->skipped_by_thread(_y))
			return;

		dsfixed_t xfrac;
		dsfixed_t yfrac;
		dsfixed_t xstep;
		dsfixed_t ystep;
		uint8_t *dest;
		const uint8_t *source = _source;
		const uint8_t *colormap = _colormap;
		int count;
		int spot;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		xfrac = _xfrac;
		yfrac = _yfrac;

		dest = ylookup[_y] + _x1 + _destorg;

		count = _x2 - _x1 + 1;

		xstep = _xstep;
		ystep = _ystep;

		if (_xbits == 6 && _ybits == 6)
		{
			// 64x64 is the most common case by far, so special case it.
			do
			{
				uint8_t texdata;

				spot = ((xfrac >> (32 - 6 - 6))&(63 * 64)) + (yfrac >> (32 - 6));
				texdata = source[spot];
				if (texdata != 0)
				{
					uint32_t fg = colormap[texdata];
					uint32_t bg = *dest;
					fg = fg2rgb[fg];
					bg = bg2rgb[bg];
					fg = (fg + bg) | 0x1f07c1f;
					*dest = RGB32k.All[fg & (fg >> 15)];
				}
				dest++;
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
		else
		{
			uint8_t yshift = 32 - _ybits;
			uint8_t xshift = yshift - _xbits;
			int xmask = ((1 << _xbits) - 1) << _ybits;
			do
			{
				uint8_t texdata;

				spot = ((xfrac >> xshift) & xmask) + (yfrac >> yshift);
				texdata = source[spot];
				if (texdata != 0)
				{
					uint32_t fg = colormap[texdata];
					uint32_t bg = *dest;
					fg = fg2rgb[fg];
					bg = bg2rgb[bg];
					fg = (fg + bg) | 0x1f07c1f;
					*dest = RGB32k.All[fg & (fg >> 15)];
				}
				dest++;
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
	}

	void DrawSpanAddClampPalCommand::Execute(DrawerThread *thread)
	{
		if (thread->skipped_by_thread(_y))
			return;

		dsfixed_t xfrac;
		dsfixed_t yfrac;
		dsfixed_t xstep;
		dsfixed_t ystep;
		uint8_t *dest;
		const uint8_t *source = _source;
		const uint8_t *colormap = _colormap;
		int count;
		int spot;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		xfrac = _xfrac;
		yfrac = _yfrac;

		dest = ylookup[_y] + _x1 + _destorg;

		count = _x2 - _x1 + 1;

		xstep = _xstep;
		ystep = _ystep;

		if (_xbits == 6 && _ybits == 6)
		{
			// 64x64 is the most common case by far, so special case it.
			do
			{
				spot = ((xfrac >> (32 - 6 - 6))&(63 * 64)) + (yfrac >> (32 - 6));
				uint32_t a = fg2rgb[colormap[source[spot]]] + bg2rgb[*dest];
				uint32_t b = a;

				a |= 0x01f07c1f;
				b &= 0x40100400;
				a &= 0x3fffffff;
				b = b - (b >> 5);
				a |= b;
				*dest++ = RGB32k.All[a & (a >> 15)];
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
		else
		{
			uint8_t yshift = 32 - _ybits;
			uint8_t xshift = yshift - _xbits;
			int xmask = ((1 << _xbits) - 1) << _ybits;
			do
			{
				spot = ((xfrac >> xshift) & xmask) + (yfrac >> yshift);
				uint32_t a = fg2rgb[colormap[source[spot]]] + bg2rgb[*dest];
				uint32_t b = a;

				a |= 0x01f07c1f;
				b &= 0x40100400;
				a &= 0x3fffffff;
				b = b - (b >> 5);
				a |= b;
				*dest++ = RGB32k.All[a & (a >> 15)];
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
	}

	void DrawSpanMaskedAddClampPalCommand::Execute(DrawerThread *thread)
	{
		if (thread->skipped_by_thread(_y))
			return;

		dsfixed_t xfrac;
		dsfixed_t yfrac;
		dsfixed_t xstep;
		dsfixed_t ystep;
		uint8_t *dest;
		const uint8_t *source = _source;
		const uint8_t *colormap = _colormap;
		int count;
		int spot;
		uint32_t *fg2rgb = _srcblend;
		uint32_t *bg2rgb = _destblend;

		xfrac = _xfrac;
		yfrac = _yfrac;

		dest = ylookup[_y] + _x1 + _destorg;

		count = _x2 - _x1 + 1;

		xstep = _xstep;
		ystep = _ystep;

		if (_xbits == 6 && _ybits == 6)
		{
			// 64x64 is the most common case by far, so special case it.
			do
			{
				uint8_t texdata;

				spot = ((xfrac >> (32 - 6 - 6))&(63 * 64)) + (yfrac >> (32 - 6));
				texdata = source[spot];
				if (texdata != 0)
				{
					uint32_t a = fg2rgb[colormap[texdata]] + bg2rgb[*dest];
					uint32_t b = a;

					a |= 0x01f07c1f;
					b &= 0x40100400;
					a &= 0x3fffffff;
					b = b - (b >> 5);
					a |= b;
					*dest = RGB32k.All[a & (a >> 15)];
				}
				dest++;
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
		else
		{
			uint8_t yshift = 32 - _ybits;
			uint8_t xshift = yshift - _xbits;
			int xmask = ((1 << _xbits) - 1) << _ybits;
			do
			{
				uint8_t texdata;

				spot = ((xfrac >> xshift) & xmask) + (yfrac >> yshift);
				texdata = source[spot];
				if (texdata != 0)
				{
					uint32_t a = fg2rgb[colormap[texdata]] + bg2rgb[*dest];
					uint32_t b = a;

					a |= 0x01f07c1f;
					b &= 0x40100400;
					a &= 0x3fffffff;
					b = b - (b >> 5);
					a |= b;
					*dest = RGB32k.All[a & (a >> 15)];
				}
				dest++;
				xfrac += xstep;
				yfrac += ystep;
			} while (--count);
		}
	}

	void FillSpanPalCommand::Execute(DrawerThread *thread)
	{
		if (thread->skipped_by_thread(_y))
			return;

		memset(ylookup[_y] + _x1 + _destorg, _color, _x2 - _x1 + 1);
	}

	/////////////////////////////////////////////////////////////////////////

	DrawTiltedSpanPalCommand::DrawTiltedSpanPalCommand(int y, int x1, int x2, const FVector3 &plane_sz, const FVector3 &plane_su, const FVector3 &plane_sv, bool plane_shade, int planeshade, float planelightfloat, fixed_t pviewx, fixed_t pviewy)
	{
	}

	void DrawTiltedSpanPalCommand::Execute(DrawerThread *thread)
	{
	}

	/////////////////////////////////////////////////////////////////////////

	DrawColoredSpanPalCommand::DrawColoredSpanPalCommand(int y, int x1, int x2)
	{
	}

	void DrawColoredSpanPalCommand::Execute(DrawerThread *thread)
	{
	}

	/////////////////////////////////////////////////////////////////////////

	DrawSlabPalCommand::DrawSlabPalCommand(int dx, fixed_t v, int dy, fixed_t vi, const uint8_t *vptr, uint8_t *p, const uint8_t *colormap)
	{
	}

	void DrawSlabPalCommand::Execute(DrawerThread *thread)
	{
	}

	/////////////////////////////////////////////////////////////////////////

	DrawFogBoundaryLinePalCommand::DrawFogBoundaryLinePalCommand(int y, int y2, int x1)
	{
	}

	void DrawFogBoundaryLinePalCommand::Execute(DrawerThread *thread)
	{
	}
}