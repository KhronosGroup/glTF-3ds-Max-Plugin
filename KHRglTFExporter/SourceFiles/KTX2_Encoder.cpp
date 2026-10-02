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
#include <string>
#include <algorithm>

#define KTX_API_STATIC
#define KTX_API

#include <ktx.h>
#include <vkformat_enum.h>

#pragma comment(lib, "ktx.lib")
 
//======================================================================
//======================================================================
BOOL glTFExporter_Core::KTX2Encode(Texmap* pTex, KTX2TextureStruct& toKTX)
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

    if(bi.Width() <= 0 || bi.Height() <= 0)
        return FALSE;

#if MAX_RELEASE > 26000
	tstring retname = GetCOREInterface()->GetDir(APP_SYSTEM_IMAGE_DIR).data();
#else
	tstring retname = GetCOREInterface()->GetDir(APP_SYSTEM_IMAGE_DIR);
#endif
	retname += _T("\\") + tstring(std::filesystem::path(bi.Filename()).stem().c_str()) + _T(".ktx2");
    toKTX.originalPathStr = retname;

    size_t width = bi.Width();
    size_t height = bi.Height();

    ktxTexture2* texture;
    std::vector<uint8_t> rgbaData(width * height * 4, 0);

    // Copy Bitmap to rgbaData
    {
        for (int y = 0; y < height; ++y) {
            std::vector<BMM_Color_fl> linePixels(width);
            pBmp->GetPixels(0, y, width, linePixels.data());
            for (int x = 0; x < width; ++x) {
                size_t targetIdx = (static_cast<size_t>(y) * width + x) * 4;
                rgbaData[targetIdx + 0] = (uint8_t)std::min(255.0f, std::max(0.0f, linePixels[x].r * 255.0f + 0.5f));
                rgbaData[targetIdx + 1] = (uint8_t)std::min(255.0f, std::max(0.0f, linePixels[x].g * 255.0f + 0.5f));
                rgbaData[targetIdx + 2] = (uint8_t)std::min(255.0f, std::max(0.0f, linePixels[x].b * 255.0f + 0.5f));
                rgbaData[targetIdx + 3] = (uint8_t)std::min(255.0f, std::max(0.0f, linePixels[x].a * 255.0f + 0.5f));
            }
        }
    }

    // Create container
    {
        ktxTextureCreateInfo createInfo = {0};
        createInfo.glInternalformat = 0x8051;// GL_RGB8;   // Ignored if creating a ktxTexture2.
        createInfo.vkFormat = toKTX.isSRGB? VK_FORMAT_R8G8B8A8_SRGB: VK_FORMAT_R8G8B8A8_UNORM;   // Ignored if creating a ktxTexture1.
        createInfo.baseWidth = width;
        createInfo.baseHeight = height;
        createInfo.baseDepth = 1;
        createInfo.numDimensions = 2;
        createInfo.numLayers = 1;
        if (toKTX.mipmap) {
            int maxSide = std::max(width, height);
            createInfo.numLevels = (int)floor(log2(maxSide)) + 1;
        }
        else {
            createInfo.numLevels = 1;
        }
        createInfo.numFaces = 1;
        createInfo.isArray = KTX_FALSE;
        createInfo.generateMipmaps = KTX_FALSE;

        KTX_error_code result = ktxTexture2_Create(&createInfo,
            KTX_TEXTURE_CREATE_ALLOC_STORAGE,
            &texture);

        if (result != KTX_SUCCESS) return false;
    }

    // Set Image data
    {
        ktx_uint8_t* src = rgbaData.data();
        ktx_size_t srcSize = rgbaData.size();
        ktx_uint32_t level = 0;
        ktx_uint32_t layer = 0;
        ktx_uint32_t faceSlice = 0
            ;
        KTX_error_code result = ktxTexture_SetImageFromMemory(ktxTexture(texture),
            level, layer, faceSlice,
            rgbaData.data(), srcSize);

        if (result != KTX_SUCCESS) return false;
    }

    // Execute compress
    {
        ktxBasisParams params = { 0 };
        params.structSize = sizeof(params);
        params.uastc = toKTX.useUASTC;
        params.compressionLevel = toKTX.compression;
        params.qualityLevel = toKTX.quality;

        const char* orientString = "rd";
        ktxHashList_AddKVPair(&texture->kvDataHead, KTX_ORIENTATION_KEY,
            (ktx_uint32_t)strlen(orientString) + 1, orientString);

        KTX_error_code result = ktxTexture2_CompressBasisEx(texture, &params);

        if (result == KTX_SUCCESS) {
        }

    }


    {
        std::string fname = WStringToString(retname);
        if (ktxTexture_WriteToNamedFile(ktxTexture(texture), fname.c_str()) != KTX_SUCCESS) {
            ktxTexture_Destroy(ktxTexture(texture));
            return FALSE;
        }
    }

    ktxTexture_Destroy(ktxTexture(texture));

    return TRUE;
}
