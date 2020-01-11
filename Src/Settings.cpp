// Settings.cpp
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

#include <System/tFile.h>
#include <System/tScript.h>
#include <Math/tFundamentals.h>
#include "Settings.h"
using namespace tMath;


void Settings::Reset()
{
	WindowW					= 1280;
	WindowH					= 720;
	WindowX					= 100;
	WindowY					= 100;

	ShowLog					= false;
	InfoOverlayShow			= false;
	ContentViewShow			= false;
	ThumbnailSize			= 72.0f;
	OverlayCorner			= 3;
	Tile					= false;
	BackgroundExtend		= false;
	BackgroundStyle			= 1;
	ResampleFilter			= 2;
	ConfirmDeletes			= true;
	SlidehowFrameDuration	= 1.0/30.0;
	PreferredFileSaveType	= 0;
	MaxImageMemMB			= 1024;
}


void Settings::Reset(int screenW, int screenH)
{
	Reset();
	WindowX					= (screenW - WindowW) >> 1;
	WindowY					= (screenH - WindowH) >> 1;
}


void Settings::Load(const tString& filename, int screenW, int screenH)
{
	if (!tSystem::tFileExists(filename))
	{
		Reset(screenW, screenH);
	}
	else
	{
		tScriptReader reader(filename);
		for (tExpr e = reader.First(); e.IsValid(); e = e.Next())
		{
			switch (e.Command().Hash())
			{
				case tHashCT("WindowX"):
					WindowX = e.Arg1();
					break;

				case tHashCT("WindowY"):
					WindowY = e.Arg1();
					break;

				case tHashCT("WindowW"):
					WindowW = e.Arg1();
					break;

				case tHashCT("WindowH"):
					WindowH = e.Arg1();
					break;

				case tHashCT("ShowLog"):
					ShowLog = e.Arg1();
					break;

				case tHashCT("InfoOverlayShow"):
					InfoOverlayShow = e.Arg1();
					break;

				case tHashCT("ContentViewShow"):
					ContentViewShow = e.Arg1();
					break;

				case tHashCT("ThumbnailSize"):
					ThumbnailSize = e.Arg1();
					break;

				case tHashCT("OverlayCorner"):
					OverlayCorner = e.Arg1();
					break;

				case tHashCT("Tile"):
					Tile = e.Arg1();
					break;

				case tHashCT("BackgroundExtend"):
					BackgroundExtend = e.Arg1();
					break;

				case tHashCT("BackgroundStyle"):
					BackgroundStyle = e.Arg1();
					break;

				case tHashCT("ResampleFilter"):
					ResampleFilter = e.Arg1();
					break;

				case tHashCT("ConfirmDeletes"):
					ConfirmDeletes = e.Arg1();
					break;

				case tHashCT("SlidehowFrameDuration"):
					SlidehowFrameDuration = e.Arg1();
					break;

				case tHashCT("PreferredFileSaveType"):
					PreferredFileSaveType = e.Arg1();
					break;

				case tHashCT("MaxImageMemMB"):
					MaxImageMemMB = e.Arg1();
					break;
			}
		}
	}

	tiClamp(ResampleFilter, 0, 5);
	tiClamp(BackgroundStyle, 0, 4);
	tiClamp(WindowW, 640, screenW);
	tiClamp(WindowH, 360, screenH);
	tiClamp(WindowX, 0, screenW - WindowW);
	tiClamp(WindowY, 0, screenH - WindowH);
	tiClamp(OverlayCorner, 0, 3);
	tiClamp(PreferredFileSaveType, 0, 4);
	tiClamp(ThumbnailSize, 64.0f, 256.0f);
}


bool Settings::Save(const tString& filename)
{
	tScriptWriter writer(filename);
	writer.Rem("Tacit Texture Viewer Configuration File");
	writer.CR();

	writer.Comp("WindowX", WindowX);
	writer.Comp("WindowY", WindowY);
	writer.Comp("WindowW", WindowW);
	writer.Comp("WindowH", WindowH);
	writer.Comp("ShowLog", ShowLog);
	writer.Comp("InfoOverlayShow", InfoOverlayShow);
	writer.Comp("ContentViewShow", ContentViewShow);
	writer.Comp("ThumbnailSize", ThumbnailSize);
	writer.Comp("OverlayCorner", OverlayCorner);
	writer.Comp("Tile", Tile);
	writer.Comp("BackgroundExtend", BackgroundExtend);
	writer.Comp("BackgroundStyle", BackgroundStyle);
	writer.Comp("ResampleFilter", ResampleFilter);
	writer.Comp("ConfirmDeletes", ConfirmDeletes);
	writer.Comp("SlidehowFrameDuration", SlidehowFrameDuration);
	writer.Comp("PreferredFileSaveType", PreferredFileSaveType);
	writer.Comp("MaxImageMemMB", MaxImageMemMB);
	
	return true;
}