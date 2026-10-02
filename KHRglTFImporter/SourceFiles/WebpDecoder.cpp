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
#include <iostream>
#include <fstream>

#include "webp/decode.h"

#pragma comment(lib, "libwebpdecoder.lib")

//===================================================================
//===================================================================
BOOL glTFImporter_Core::WebpDecode(const tstring &fname, tstring &retname)
{
    //size_t data_size;
    int width;
    int height;

    std::ifstream file(fname, std::ios::binary);
    if (!file.is_open()) return FALSE;

    std::vector< std::uint8_t > data
    ((std::istreambuf_iterator< char >(file))
        , (std::istreambuf_iterator< char >())
    );
    file.close();

    int ret = WebPGetInfo(data.data(), data.size(), &width, &height);

    if (!ret) return FALSE;

    uint8_t *buf = WebPDecodeRGBA(data.data(), data.size(), &width, &height);
    if (!buf) return FALSE;

    std::filesystem::path destname(fname);
    retname = m_WorkImageFolder + tstring(destname.stem()) + _T(".png");

    // Check if alpha map is needed?
    bool has_alpha = false;
    uint8_t* p_check = buf;
    for (int i = 0; i < width * height; ++i) {
        p_check += 3; // sSkip RGB channel
        uint8_t alpha = *p_check++;
        if (alpha < 255) { //
            has_alpha = true;
            break;
        }
    }

    BitmapInfo bi;
    bi.SetHeight(height);
    bi.SetWidth(width);
    bi.SetName(retname.c_str());

    bi.SetType(BMM_TRUE_64);
    bi.SetFlags(MAP_HAS_ALPHA);

    Bitmap* pBmp = TheManager->Create(&bi);
    uint8_t* p = buf;
    float rate = 65535.0f / 255.0f;;

    for (int h = 0; h < height;h++) {
        for (int w = 0; w < width; w++) {
            BMM_Color_64 pix;
            BMM_Color_64 col;
            col.r = static_cast<uint16_t>(*p++ * rate);
            col.g = static_cast<uint16_t>(*p++ * rate);
            col.b = static_cast<uint16_t>(*p++ * rate);
            col.a = static_cast<uint16_t>(*p++ * rate);

            //pix.r = static_cast<uint16_t>(pow(static_cast<float>(col.r) / 65535.0f, 2.2f) * 65535.0f + 0.5f);
            //pix.g = static_cast<uint16_t>(pow(static_cast<float>(col.g) / 65535.0f, 2.2f) * 65535.0f + 0.5f);
            //pix.b = static_cast<uint16_t>(pow(static_cast<float>(col.b) / 65535.0f, 2.2f) * 65535.0f + 0.5f);

            pix = col;

            pBmp->PutPixels(w, h, 1, &pix);
        }
    }

    m_pPNG_BmpIO->SetType(BMM_TRUE_24);
    m_pPNG_BmpIO->SetAlpha(has_alpha);
    //SetPNGInfo(m_pPNG_BmpIO, pBmp);

    pBmp->OpenOutput(&bi);
    pBmp->Write(&bi);
    pBmp->Close(&bi);
    pBmp->DeleteThis();

    WebPFree(buf);

    return TRUE;
}
