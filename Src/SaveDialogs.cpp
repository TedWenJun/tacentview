// SaveDialogs.cpp
//
// Modal dialogs save-as and save-all.
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

#include "imgui.h"
#include "SaveDialogs.h"
#include "Image.h"
#include "TacentView.h"
using namespace tStd;
using namespace tSystem;
using namespace tMath;
using namespace tImage;


namespace Viewer
{
	void SaveAllImages(const tString& destDir, const tString& extension, float percent, int width, int height);
	void GetFilesNeedingOverwrite(const tString& destDir, tList<tStringItem>& overwriteFiles, const tString& extension);
	void AddSavedImageIfNecessary(const tString& savedFile);

	// This function saves the picture to the filename specified.
	bool SaveImageAs(Image&, const tString& outFile, int width, int height, float scale = 1.0f, Settings::SizeMode = Settings::SizeMode::SetWidthAndHeight);
}


tString Viewer::DoSubFolder()
{
	// Output sub-folder
	char subFolder[256]; tMemset(subFolder, 0, 256);
	tStrncpy(subFolder, Config.SaveSubFolder.Chars(), 255);
	ImGui::InputText("SubFolder", subFolder, 256);
	Config.SaveSubFolder.Set(subFolder);
	tString destDir = ImagesDir;
	if (!Config.SaveSubFolder.IsEmpty())
		destDir += Config.SaveSubFolder + "/";
	tString toolTipText;
	tsPrintf(toolTipText, "Save to %s", destDir.Chars());
	ShowToolTip(toolTipText.Chars());
	ImGui::SameLine();
	if (ImGui::Button("Default"))
		Config.SaveSubFolder.Set("Saved");
	ImGui::SameLine();
	if (ImGui::Button("Here"))
		Config.SaveSubFolder.Clear();

	return destDir;
}


tString Viewer::DoSaveFiletype()
{
	const char* fileTypeItems[] = { "tga", "png", "bmp", "jpg" };
	ImGui::Combo("File Type", &Config.SaveFileType, fileTypeItems, tNumElements(fileTypeItems));
	ImGui::SameLine();
	ShowHelpMark("Output image format. TGA, PNG, and BMP support an alpha channel.");

	tString extension = ".tga";
	switch (Config.SaveFileType)
	{
		case 0: extension = ".tga"; break;
		case 1: extension = ".png"; break;
		case 2: extension = ".bmp"; break;
		case 3: extension = ".jpg"; break;
	}

	if (Config.SaveFileType == 0)
		ImGui::Checkbox("RLE Compression", &Config.SaveFileTargaRLE);
	else if (Config.SaveFileType == 3)
		ImGui::SliderInt("Quality", &Config.SaveFileJpegQuality, 1, 100, "%d");

	return extension;
}


bool Viewer::SaveImageAs(Image& img, const tString& outFile, int width, int height, float scale, Settings::SizeMode sizeMode)
{
	// We make sure to maintain the loaded/unloaded state. This function may be called many times in succession
	// so we don't want them all in memory at once by indiscriminantly loading them all.
	bool imageLoaded = img.IsLoaded();
	if (!imageLoaded)
		img.Load();

	tPicture* currPic = img.GetCurrentPic();
	if (!currPic)
		return false;

	// Make a temp copy we can safely resize.
	tImage::tPicture outPic;
	outPic.Set(*currPic);

	// Restore loadedness.
	if (!imageLoaded)
		img.Unload();

	int outW = outPic.GetWidth();
	int outH = outPic.GetHeight();
	float aspect = float(outW) / float(outH);

	switch (sizeMode)
	{
		case Settings::SizeMode::Percent:
			if (tMath::tApproxEqual(scale, 1.0f, 0.01f))
				break;
			outW = int( tRound(float(outW)*scale) );
			outH = int( tRound(float(outH)*scale) );
			break;

		case Settings::SizeMode::SetWidthAndHeight:
			outW = width;
			outH = height;
			break;

		case Settings::SizeMode::SetWidthRetainAspect:
			outW = width;
			outH = int( tRound(float(width) / aspect) );
			break;

		case Settings::SizeMode::SetHeightRetainAspect:
			outH = height;
			outW = int( tRound(float(height) * aspect) );
			break;
	};
	tMath::tiClampMin(outW, 4);
	tMath::tiClampMin(outH, 4);

	if ((outPic.GetWidth() != outW) || (outPic.GetHeight() != outH))
		outPic.Resample(outW, outH, tImage::tPicture::tFilter(Config.ResampleFilter));

	bool success = false;
	tImage::tPicture::tColourFormat colourFmt = outPic.IsOpaque() ? tImage::tPicture::tColourFormat::Colour : tImage::tPicture::tColourFormat::ColourAndAlpha;
	if (Config.SaveFileType == 0)
		success = outPic.SaveTGA(outFile, tImage::tImageTGA::tFormat::Auto, Config.SaveFileTargaRLE ? tImage::tImageTGA::tCompression::RLE : tImage::tImageTGA::tCompression::None);
	else
		success = outPic.Save(outFile, colourFmt, Config.SaveFileJpegQuality);

	if (success)
		tPrintf("Saved image as %s\n", outFile.Chars());
	else
		tPrintf("Failed to save image %s\n", outFile.Chars());

	return success;
}


void Viewer::DoSaveAsModalDialog(bool justOpened)
{
	tAssert(CurrImage);
	tPicture* picture = CurrImage->GetCurrentPic();
	tAssert(picture);

	static int dstW = 512;
	static int dstH = 512;
	int srcW = picture->GetWidth();
	int srcH = picture->GetHeight();

	if (justOpened)
	{
		dstW = picture->GetWidth();
		dstH = picture->GetHeight();
	}

	float aspect = float(srcW) / float(srcH);
	static bool lockAspect = true;

	ImGui::PushItemWidth(100);
	if (ImGui::InputInt("Width", &dstW) && lockAspect)
		dstH = int( float(dstW) / aspect );
	ImGui::PopItemWidth();
	tiClampMin(dstW, 4); tiClampMin(dstH, 4);

	static char lo[32];
	static char hi[32];

	int loP2W = tNextLowerPower2(dstW);		tiClampMin(loP2W, 4);	tsPrintf(lo, "w%d", loP2W);
	int hiP2W = tNextHigherPower2(dstW);							tsPrintf(hi, "w%d", hiP2W);
	ImGui::SameLine(); if (ImGui::Button(lo))
		{ dstW = loP2W; if (lockAspect) dstH = int( float(dstW) / aspect ); }
	ImGui::SameLine(); if (ImGui::Button(hi))
		{ dstW = hiP2W; if (lockAspect) dstH = int( float(dstW) / aspect ); }
	ImGui::SameLine(); ShowHelpMark("Final output width in pixels.\nIf dimensions match current no scaling.");

	if (ImGui::Checkbox("Lock Aspect", &lockAspect) && lockAspect)
	{
		dstW = srcW;
		dstH = srcH;
	}

	ImGui::PushItemWidth(100);
	if (ImGui::InputInt("Height", &dstH) && lockAspect)
		dstW = int( float(dstH) * aspect );
	ImGui::PopItemWidth();
	tiClampMin(dstW, 4); tiClampMin(dstH, 4);

	int loP2H = tNextLowerPower2(dstH);		tiClampMin(loP2H, 4);	tsPrintf(lo, "h%d", loP2H);
	int hiP2H = tNextHigherPower2(dstH);							tsPrintf(hi, "h%d", hiP2H);
	ImGui::SameLine(); if (ImGui::Button(lo))
		{ dstH = loP2H; if (lockAspect) dstW = int( float(dstH) * aspect ); }
	ImGui::SameLine(); if (ImGui::Button(hi))
		{ dstH = hiP2H; if (lockAspect) dstW = int( float(dstH) * aspect ); }
	ImGui::SameLine(); ShowHelpMark("Final output height in pixels.\nIf dimensions match current no scaling.");

	if (ImGui::Button("Reset"))
	{
		dstW = srcW;
		dstH = srcH;
	}

	ImGui::Separator();

	if ((dstW != srcW) || (dstH != srcH))
	{
		// Matches tImage::tPicture::tFilter.
		const char* filterItems[] = { "NearestNeighbour", "Box", "Bilinear", "Bicubic", "Quadratic", "Hamming" };
		ImGui::Combo("Filter", &Config.ResampleFilter, filterItems, tNumElements(filterItems));
		ImGui::SameLine();
		ShowHelpMark("Filtering method to use when resizing images.");
	}

	tString extension = DoSaveFiletype();
	ImGui::Separator();
	tString destDir = DoSubFolder();

	static char filename[128] = "Filename";
	if (justOpened)
	{
		tString baseName = tSystem::tGetFileBaseName(CurrImage->Filename);
		tStrcpy(filename, baseName.Chars());
	}
	ImGui::InputText("Filename", filename, tNumElements(filename));
	ImGui::SameLine(); ShowHelpMark("The output filename without extension.");

	ImGui::NewLine();
	if (ImGui::Button("Cancel", tVector2(100, 0)))
		ImGui::CloseCurrentPopup();
	ImGui::SameLine();
	
	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 100.0f);

	tString outFile = destDir + tString(filename) + extension;
	bool closeThisModal = false;
	if (ImGui::Button("Save", tVector2(100, 0)))
	{
		bool dirExists = tDirExists(destDir);
		if (!dirExists)
		{
			dirExists = tCreateDir(destDir);
			PopulateImagesSubDirs();
		}

		if (dirExists)
		{
			if (tFileExists(outFile) && Config.ConfirmFileOverwrites)
			{
				ImGui::OpenPopup("Overwrite File");
			}
			else
			{
				bool ok = SaveImageAs(*CurrImage, outFile, dstW, dstH);
				if (ok)
				{
					// This gets a bit tricky. Image A may be saved as the same name as image B also in the list. We need to search for it.
					// If it's not found, we need to add it to the list iff it was saved to the current folder.
					Image* foundImage = FindImage(outFile);
					if (foundImage)
					{
						foundImage->Unload(true);
						foundImage->ClearDirty();
						foundImage->RequestInvalidateThumbnail();
					}
					else
						AddSavedImageIfNecessary(outFile);

					SortImages(Settings::SortKeyEnum(Config.SortKey), Config.SortAscending);
					SetCurrentImage(outFile);
				}
				closeThisModal = true;
			}
		}
	}

	// The unused isOpen bool is just so we get a close button in ImGui. 
	bool isOpen = true;
	if (ImGui::BeginPopupModal("Overwrite File", &isOpen, ImGuiWindowFlags_AlwaysAutoResize))
	{
		bool pressedOK = false, pressedCancel = false;
		DoOverwriteFileModal(outFile, pressedOK, pressedCancel);
		if (pressedOK)
		{
			bool ok = SaveImageAs(*CurrImage, outFile, dstW, dstH);
			if (ok)
			{
				Image* foundImage = FindImage(outFile);
				if (foundImage)
				{
					foundImage->Unload(true);
					foundImage->ClearDirty();
					foundImage->RequestInvalidateThumbnail();
				}
				else
					AddSavedImageIfNecessary(outFile);

				SortImages(Settings::SortKeyEnum(Config.SortKey), Config.SortAscending);
				SetCurrentImage(outFile);
			}
		}
		if (pressedOK || pressedCancel)
			closeThisModal = true;
	}

	if (closeThisModal)
		ImGui::CloseCurrentPopup();

	ImGui::EndPopup();
}


void Viewer::DoSaveAllModalDialog(bool justOpened)
{
	ImGui::Text("Save all %d images to the image type you select.", Images.GetNumItems()); ImGui::SameLine();
	ShowHelpMark
	(
		"Images may be resized based on the Size Mode:\n"
		"\n"
		"  Percent of Original\n"
		"  Use 100% for no scaling/resampling. Less\n"
		"  than 100% downscales. Greater than upscales.\n"
		"\n"
		"  Set Width and Height\n"
		"  Scales all images to specified width and\n"
		"  height, possibly non-uniformly.\n"
		"\n"
		"  Set Width - Retain Aspect\n"
		"  All images will have specified width. Always\n"
		"  uniform scale. Varying height.\n"
		"\n"
		"  Set Height - Retain Aspect\n"
		"  All images will have specified height. Always\n"
		"  uniform scale. Varying width.\n"
	);

	ImGui::Separator();

	static int width = 512;
	static int height = 512;
	static float percent = 100.0f;
	const char* sizeModeNames[] = { "Percent of Original", "Set Width and Height", "Set Width - Retain Aspect", "Set Height - Retain Aspect" };
	ImGui::Combo("Size Mode", &Config.SaveAllSizeMode, sizeModeNames, tNumElements(sizeModeNames));
	switch (Settings::SizeMode(Config.SaveAllSizeMode))
	{
		case Settings::SizeMode::Percent:
			ImGui::InputFloat("Percent", &percent, 1.0f, 10.0f, "%.1f");	ImGui::SameLine();	ShowHelpMark("Percent of original size.");
			break;

		case Settings::SizeMode::SetWidthAndHeight:
			ImGui::InputInt("Width", &width);	ImGui::SameLine();	ShowHelpMark("Output width in pixels for all images.");
			ImGui::InputInt("Height", &height);	ImGui::SameLine();	ShowHelpMark("Output height in pixels for all images.");
			break;

		case Settings::SizeMode::SetWidthRetainAspect:
			ImGui::InputInt("Width", &width);	ImGui::SameLine();	ShowHelpMark("Output width in pixels for all images.");
			break;

		case Settings::SizeMode::SetHeightRetainAspect:
			ImGui::InputInt("Height", &height);	ImGui::SameLine();	ShowHelpMark("Output height in pixels for all images.");
			break;
	};

	ImGui::Separator();
	if (!((Settings::SizeMode(Config.SaveAllSizeMode) == Settings::SizeMode::Percent) && (percent == 100.0f)))
	{
		// Matches tImage::tPicture::tFilter.
		const char* filterItems[] = { "NearestNeighbour", "Box", "Bilinear", "Bicubic", "Quadratic", "Hamming" };
		ImGui::Combo("Filter", &Config.ResampleFilter, filterItems, tNumElements(filterItems));
		ImGui::SameLine();
		ShowHelpMark("Filtering method to use when resizing images.");
	}
	tMath::tiClampMin(width, 4);
	tMath::tiClampMin(height, 4);

	tString extension = DoSaveFiletype();
	ImGui::Separator();
	tString destDir = DoSubFolder();

	ImGui::NewLine();
	if (ImGui::Button("Cancel", tVector2(100, 0)))
		ImGui::CloseCurrentPopup();

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 100.0f);

	// This needs to be static since DoSaveModalDialog is called for every frame the modal is open.
	static tList<tStringItem> overwriteFiles(tListMode::Static);
	bool closeThisModal = false;
	if (ImGui::Button("Save All", tVector2(100, 0)))
	{
		bool dirExists = tDirExists(destDir);
		if (!dirExists)
		{
			dirExists = tCreateDir(destDir);
			PopulateImagesSubDirs();
		}

		if (dirExists)
		{
			overwriteFiles.Empty();
			GetFilesNeedingOverwrite(destDir, overwriteFiles, extension);
			if (!overwriteFiles.IsEmpty() && Config.ConfirmFileOverwrites)
			{
				ImGui::OpenPopup("Overwrite Multiple Files");
			}
			else
			{
				SaveAllImages(destDir, extension, percent, width, height);
				closeThisModal = true;
			}
		}
	}

	// The unused isOpen bool is just so we get a close button in ImGui. 
	bool isOpen = true;
	if (ImGui::BeginPopupModal("Overwrite Multiple Files", &isOpen, ImGuiWindowFlags_AlwaysAutoResize))
	{
		bool pressedOK = false, pressedCancel = false;
		DoOverwriteMultipleFilesModal(overwriteFiles, pressedOK, pressedCancel);
		if (pressedOK)
			SaveAllImages(destDir, extension, percent, width, height);

		if (pressedOK || pressedCancel)
			closeThisModal = true;
	}

	if (closeThisModal)
	{
		overwriteFiles.Empty();
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}


void Viewer::GetFilesNeedingOverwrite(const tString& destDir, tList<tStringItem>& overwriteFiles, const tString& extension)
{
	for (Image* image = Images.First(); image; image = image->Next())
	{
		tString baseName = tSystem::tGetFileBaseName(image->Filename);
		tString outFile = destDir + tString(baseName) + extension;

		// Only add unique items to the list.
		if (tSystem::tFileExists(outFile) && !overwriteFiles.Contains(outFile))
			overwriteFiles.Append(new tStringItem(outFile));
	}
}


void Viewer::DoOverwriteMultipleFilesModal(const tList<tStringItem>& overwriteFiles, bool& pressedOK, bool& pressedCancel)
{
	tAssert(!overwriteFiles.IsEmpty());
	tString dir = tSystem::tGetDir(*overwriteFiles.First());
	ImGui::Text("The Following Files");
	ImGui::Indent();
	int fnum = 0;
	const int maxToShow = 6;
	for (tStringItem* filename = overwriteFiles.First(); filename && (fnum < maxToShow); filename = filename->Next(), fnum++)
	{
		tString file = tSystem::tGetFileName(*filename);
		ImGui::Text("%s", file.Chars());
	}
	int remaining = overwriteFiles.GetNumItems() - fnum;
	if (remaining > 0)
		ImGui::Text("And %d more.", remaining);
	ImGui::Unindent();
	ImGui::Text("Already Exist In Folder");
	ImGui::Indent(); ImGui::Text("%s", dir.Chars()); ImGui::Unindent();
	ImGui::NewLine();
	ImGui::Text("Overwrite Files?");
	ImGui::NewLine();
	ImGui::Separator();
	ImGui::NewLine();
	ImGui::Checkbox("Confirm file overwrites in the future?", &Config.ConfirmFileOverwrites);
	ImGui::NewLine();

	if (ImGui::Button("Cancel", tVector2(100, 0)))
	{
		pressedCancel = true;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 100.0f);
	if (ImGui::Button("Overwrite", tVector2(100, 0)))
	{
		pressedOK = true;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}


void Viewer::SaveAllImages(const tString& destDir, const tString& extension, float percent, int width, int height)
{
	float scale = percent/100.0f;
	tString currFile = CurrImage ? CurrImage->Filename : tString();

	bool anySaved = false;
	for (Image* image = Images.First(); image; image = image->Next())
	{
		tString baseName = tSystem::tGetFileBaseName(image->Filename);
		tString outFile = destDir + tString(baseName) + extension;

		bool ok = SaveImageAs(*image, outFile, width, height, scale, Settings::SizeMode(Config.SaveAllSizeMode));
		if (ok)
		{
			Image* foundImage = FindImage(outFile);
			if (foundImage)
			{
				foundImage->Unload(true);
				foundImage->ClearDirty();
				foundImage->RequestInvalidateThumbnail();
			}
			else
				AddSavedImageIfNecessary(outFile);
			anySaved = true;
		}
	}

	// If we saved to the same dir we are currently viewing we need to reload and set the current image again.
	if (anySaved)
	{
		SortImages(Settings::SortKeyEnum(Config.SortKey), Config.SortAscending);
		SetCurrentImage(currFile);
	}
}


void Viewer::AddSavedImageIfNecessary(const tString& savedFile)
{
	#ifdef PLATFORM_LINUX
	if (ImagesDir.IsEqual(tGetDir(savedFile)))
	#else
	if (ImagesDir.IsEqualCI(tGetDir(savedFile)))
	#endif
	{
		// Add to list. It's still unloaded.
		Image* newImg = new Image(savedFile);
		Images.Append(newImg);
		ImagesLoadTimeSorted.Append(newImg);
	}
}


void Viewer::DoOverwriteFileModal(const tString& outFile, bool& pressedOK, bool& pressedCancel)
{
	tString file = tSystem::tGetFileName(outFile);
	tString dir = tSystem::tGetDir(outFile);
	ImGui::Text("Overwrite file");
		ImGui::Indent(); ImGui::Text("%s", file.Chars()); ImGui::Unindent();
	ImGui::Text("In Folder");
		ImGui::Indent(); ImGui::Text("%s", dir.Chars()); ImGui::Unindent();
	ImGui::NewLine();
	ImGui::Separator();

	ImGui::NewLine();
	ImGui::Checkbox("Confirm file overwrites in the future?", &Config.ConfirmFileOverwrites);
	ImGui::NewLine();

	if (ImGui::Button("Cancel", tVector2(100, 0)))
	{
		pressedCancel = true;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 100.0f);
	if (ImGui::Button("OK", tVector2(100, 0)))
	{
		pressedOK = true;
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}
