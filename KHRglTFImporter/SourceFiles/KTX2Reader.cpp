/*
 * Copyright (c) 2024-2026 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 //**************************************************************************/
 // AUTHOR: Satoshi Hayashi 
 //***************************************************************************/


#include "KHRglTFImporter.h"
#include <vector>
#include <iostream>

// Declaration to treat libktx as a static library
#ifndef KHRONOS_STATIC
#define KHRONOS_STATIC
#endif
#ifndef KTX_API_STATIC
#define KTX_API_STATIC
#endif

#include <ktx.h>

#pragma comment(lib, "ktx.lib")

//===================================================================
// Converts raw data (RGBA) decompressed by libktx into a 3ds Max Bitmap
//===================================================================
Bitmap* CreateMaxBitmapFromRawData(unsigned char* pRawData, int width, int height)
{
	BitmapInfo bi;
	bi.SetWidth(width);
	bi.SetHeight(height);
	bi.SetType(BMM_TRUE_64);
	bi.SetFlags(MAP_HAS_ALPHA);

	Bitmap* pBitmap = TheManager->Create(&bi);
	if (!pBitmap) return nullptr;

	for (int y = 0; y < height; ++y) {
		std::vector<BMM_Color_64> row(width);

		for (int x = 0; x < width; ++x) {
			int offset = (y * width + x) * 4;

			// Convert 8-bit (0–255) to 16-bit (0–65535) and store
			row[x].r = (unsigned short)pRawData[offset] << 8;
			row[x].g = (unsigned short)pRawData[offset + 1] << 8;
			row[x].b = (unsigned short)pRawData[offset + 2] << 8;
			row[x].a = (unsigned short)pRawData[offset + 3] << 8;
		}

		pBitmap->PutPixels(0, y, width, row.data());
	}

	return pBitmap;
}

//===================================================================
//===================================================================
ktxBasisParams getParamsFromTexture(ktxTexture2* texture)
{
	ktxBasisParams params = { 0 };
	params.structSize = sizeof(params);


	params.compressionLevel = 2;
	params.qualityLevel = 128;

	return params;
}

//===================================================================
 // Function that loads a KTX2 file and returns a buffer in RGBA8888 format
//===================================================================
bool LoadKTX2ToRawRGBA(const tstring& ktxfilename, const tstring &filename, IBitmapIO_Png* pPNG_BmpIO)
{
	ktxTexture2* kTexture = nullptr;
	KTX_error_code result;

	std::string str = WStringToString(ktxfilename);
	result = ktxTexture2_CreateFromNamedFile(str.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);
	if (result != KTX_SUCCESS) return false;

	// Check if the data is Basis Universal compressed and transcode if necessary
	if (ktxTexture2_NeedsTranscoding(kTexture)) {
		result = ktxTexture2_TranscodeBasis(kTexture, KTX_TTF_RGBA32, 0);
		if (result != KTX_SUCCESS) {
			ktxTexture_Destroy(ktxTexture(kTexture));
			return false;
		}
	}

	// Retrieve only the data size for mipmap level 0 (largest size)
	std::vector<unsigned char> outData;
	ktx_size_t levelSize = ktxTexture_GetImageSize(ktxTexture(kTexture), 0);
	outData.resize(levelSize);

	// Obtain the memory offset indicating where the level 0 data resides
	ktx_size_t offset = 0;
	result = ktxTexture_GetImageOffset(ktxTexture(kTexture), 0, 0, 0, &offset);

	// Get a pointer to the start of all data, then copy from the position advanced by the offset
	if (result == KTX_SUCCESS) {
		ktx_uint8_t* pAllData = ktxTexture_GetData(ktxTexture(kTexture));
		memcpy(outData.data(), pAllData + offset, levelSize);
	}
	else {
		ktxTexture_Destroy(ktxTexture(kTexture));
		return false;
	}

	ktxBasisParams params = getParamsFromTexture(kTexture);

	int width = kTexture->baseWidth;
	int height = kTexture->baseHeight;
	Bitmap* pBmp = CreateMaxBitmapFromRawData(outData.data(), width, height);
	if (!pBmp) { ktxTexture_Destroy(ktxTexture(kTexture)); return false; }

	bool hasAlpha = (ktxTexture2_GetNumComponents(kTexture) == 4);

	BitmapInfo bi = pBmp->GetBitmapInfo();
/*
#if MAX_RELEASE >= 26000
	if (kTexture->vkFormat == 157){
		MaxSDK::ColorManagement::IColorPipelineMgr* cpm = (MaxSDK::ColorManagement::IColorPipelineMgr*)GetCOREInterface(COLORPIPELINEMGR_INTERFACE);

		auto settings = cpm->Settings();
		if (settings->IsOCIOBased()) {
			auto ret = bi.SetRequestedColorSpace(settings->GetDataColorSpaceName(), MaxSDK::ColorManagement::ColSpaceSource::User);
			bi.SetName(filename.c_str());
			bi.ResetCustomFlag(BMM_CUSTOM_FILEGAMMA);
			bi.SetCustomFlag(BMM_CUSTOM_GAMMA);
			bi.SetCustomGamma(1.0f);
		}
	}
#endif
*/


	pPNG_BmpIO->SetType(BMM_TRUE_24);
	pPNG_BmpIO->SetAlpha(hasAlpha);
	//SetPNGInfo(pPNG_BmpIO, pBmp);

	bi.SetName(filename.c_str());
	pBmp->OpenOutput(&bi);
	pBmp->Write(&bi);
	pBmp->Close(&bi);
	pBmp->DeleteThis();

	// Clean up
	ktxTexture_Destroy(ktxTexture(kTexture));

	return true;
}

//===================================================================
//===================================================================
BOOL glTFImporter_Core::KTX2ImageCreater(const tstring& ktxname, tstring& retname)
{
	std::filesystem::path destname(ktxname);
	retname = m_WorkImageFolder + tstring(destname.stem()) + _T(".png");

	BOOL ret = LoadKTX2ToRawRGBA(ktxname, retname, m_pPNG_BmpIO);

	return ret;
}





