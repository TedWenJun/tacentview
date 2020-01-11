// Settings.h
//
// Viewer settings stored as human-readable symbolic expressions.
//
// Copyright (c) 2019, 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tString.h>


struct Settings
{
	Settings()							{ Reset(); }
	int WindowX;
	int WindowY;
	int WindowW;
	int WindowH;
	bool ShowLog;
	bool InfoOverlayShow;
	bool ContentViewShow;
	float ThumbnailSize;
	int OverlayCorner;
	bool Tile;

	enum class BGStyle
	{
		None,
		Checkerboard,
		Black,
		Grey,
		White
	};
	bool BackgroundExtend;				// Extend background past image bounds.
	int BackgroundStyle;
	int ResampleFilter;					// Matches tImage::tPicture::tFilter.
	bool ConfirmDeletes;
	double SlidehowFrameDuration;
	int PreferredFileSaveType;
	int MaxImageMemMB;					// Max image mem before unloading images.

	void Load(const tString& filename, int screenWidth, int screenHeight);
	bool Save(const tString& filename);
	void Reset();
	void Reset(int screenWidth, int screenHeight);
};