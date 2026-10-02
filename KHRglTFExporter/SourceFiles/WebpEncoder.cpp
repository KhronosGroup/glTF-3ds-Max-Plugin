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

#include "KHRglTFExporter.h"
#include <iostream>
#include <fstream>
#include <algorithm>

#include <webp/encode.h>

#pragma comment(lib, "libwebp.lib")
#pragma comment(lib, "libsharpyuv.lib")

BOOL glTFExporter_Core::WebpEncode(Texmap* pTex, WebpTextureStruct &str)
{

	Bitmap* pBmp = NULL;
	BitmapTex* pWorkBitmapTex = NULL;

	if (pTex->ClassID() == bmptexClassID) {
		pBmp = ((BitmapTex*)pTex)->GetBitmap(0);
	}
	else if (pTex->ClassID() == VRayBitmapID) {
		TSTR name = pTex->GetParamBlock(0)->GetStr(0);
		pWorkBitmapTex = NewDefaultBitmapTex();
		pWorkBitmapTex->SetMapName(name);
		pWorkBitmapTex->ReloadBitmapAndUpdate();
		pBmp = pWorkBitmapTex->GetBitmap(0);
	}
	else if (pTex->ClassID() == CoronaBitmapID) {
		TSTR name = pTex->GetParamBlock(0)->GetStr(0);
		pWorkBitmapTex = NewDefaultBitmapTex();
		pWorkBitmapTex->SetMapName(name);
		pWorkBitmapTex->ReloadBitmapAndUpdate();
		pBmp = pWorkBitmapTex->GetBitmap(0);
	}
	else if (GetOSLMapType(pTex) == OSL_UberBitmap) {
		TSTR name = pTex->GetParamBlock(1)->GetStr(10);
		pWorkBitmapTex = NewDefaultBitmapTex();
		pWorkBitmapTex->SetMapName(name);
		pWorkBitmapTex->ReloadBitmapAndUpdate();
		pBmp = pWorkBitmapTex->GetBitmap(0);
	}
	else {
		return FALSE;
	}

	BitmapInfo bi = pBmp->GetBitmapInfo();

	// safeguard agains WebP spec resolution limits (16383), Max bitmap could be larger)
	if(bi.Width() <= 0 || bi.Height() < 0 || bi.Width() > 16383 || bi.Height() > 16383)
		return FALSE;

	tstring filename = bi.Filename();
	std::filesystem::path destname(filename);
#if MAX_RELEASE > 26000
	tstring fl = GetCOREInterface()->GetDir(APP_SYSTEM_IMAGE_DIR).data();
#else
	tstring fl = GetCOREInterface()->GetDir(APP_SYSTEM_IMAGE_DIR);
#endif
	tstring retname = fl + _T("\\") + tstring(destname.stem()) + _T(".webp");

	size_t width = bi.Width();
	size_t height = bi.Height();
	size_t elements = 4;
	
	std::vector<std::uint8_t> in(width * height * elements);

	for (int y = 0; y < height; ++y){
		for (int x = 0; x < width; ++x)	{
			BMM_Color_fl pix;
			pBmp->GetPixels(x,y,1, &pix);
			in[y * width * elements + x * elements + 0] = static_cast<std::uint8_t>(std::clamp(pix.r * 255.0f, 0.0f, 255.0f));
			in[y * width * elements + x * elements + 1] = static_cast<std::uint8_t>(std::clamp(pix.g * 255.0f, 0.0f, 255.0f));
			in[y * width * elements + x * elements + 2] = static_cast<std::uint8_t>(std::clamp(pix.b * 255.0f, 0.0f, 255.0f));
			in[y * width * elements + x * elements + 3] = static_cast<std::uint8_t>(std::clamp(pix.a * 255.0f, 0.0f, 255.0f));
		}
	}

	std::uint8_t* data = nullptr;
	auto size = 0ull;
	size_t stride = width * elements;

	{
		if (str.LossLess) {
			size = WebPEncodeLosslessRGBA(in.data(), width, height, stride, &data);
		}
		else {
			size = WebPEncodeRGBA(in.data(), width, height, stride, str.QualityFactor, &data);
		}
		if (!data || size == 0) return FALSE;

		
		//write_file(retname);
		std::ofstream o(retname, std::ios::binary);
		o.write(reinterpret_cast<const char*>(data), size);
		o.close();

		WebPFree(data);
	}

	if (pWorkBitmapTex) pWorkBitmapTex->DeleteThis();


	str.originalPathStr = retname;

	return TRUE;
}
