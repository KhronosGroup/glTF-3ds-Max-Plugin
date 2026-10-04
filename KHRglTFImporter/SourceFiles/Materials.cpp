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

//#define BUILDING_LIBCURL
#include "KHRglTFImporter.h"
#include <maxscript\maxscript.h>
#include <gamma.h>
#include <Materials\\TextureOutput.h>

#include "define.h"
#include "MimeTypes.h"
//#include "define.h"

#if MAX_RELEASE >= 26000
#include "ColorManagement\IColorPipelineMgr.h"
#endif

#include <filesystem>
namespace fs = std::filesystem;

//=============================================================================
//=============================================================================
tstring glTFImporter_Core::CreateTextureFileName(cgltf_texture* tex, tstring &originalFname)
{
	tstring fname;

	originalFname.clear();

	cgltf_image* image = NULL;
	if (tex->has_basisu) {
		image = tex->basisu_image;
	}
	else {
		image = tex->image;
	}
	if (tex->extensions_count > 0) {
		for (int i = 0; i < tex->extensions_count;i++) {
			char* name = tex->extensions[i].name;
			if (!strcmp(name, "EXT_texture_webp")) {
				char* data = tex->extensions[i].data;
				char* ptr = strchr(data, ':');
				if (!ptr) break;
				ptr++;
				int idx = atoi(ptr);
				image = &m_glTF_data->images[idx];
				//webp_flag = TRUE;
				break;
			}else if (!strcmp(name, "KHR_texture_basisu")) {
				char* data = tex->extensions[i].data;
				char* ptr = strchr(data, ':');
				//ktx2_flag = TRUE;
			}
		}
	}

	const char* uri = image->uri;
	//int size = image->buffer_view->size;
	if (uri) {
		if (strncmp(uri, "data:", 5) == 0) {
			const char* p = strchr(uri, ';');
			char buf[MAX_PATH];
			strncpy_s(buf, MAX_PATH, uri + 5, p - (uri + 5));
			buf[p - (uri + 5)] = '\0';
			const char* type = MimeTypes::getExtension(buf);
			p = strchr(p, ',') + 1;
			const char* endp = strchr(p, '=');
			size_t len1 = endp - p;
			size_t len2 = strlen(p);
			if (len1 <= 0) len1 = len2;
			size_t CharSize = (len1 < len2) ? len1 : len2; //endp - p;
			size_t ByteSize = (CharSize * 3) / 4;
			//if (CharSize % 4 != 0) ByteSize += 1;
			cgltf_options options = {};

			char* out_data = new char[ByteSize];
			cgltf_result ret = cgltf_load_buffer_base64(&options, ByteSize, p, reinterpret_cast<void**>(&out_data));

			char base_name[MAX_PATH];
			if (image->name) {
				strncpy_s(base_name, MAX_PATH, image->name, MAX_PATH - 1);
				base_name[MAX_PATH - 1] = '\0';
			}
			else if (tex->name) {
				strncpy_s(base_name, MAX_PATH, tex->name, MAX_PATH - 1);
				base_name[MAX_PATH - 1] = '\0';
			}
			else 
				sprintf_s(base_name, MAX_PATH, "texture_%d.%s", (UINT)m_TextureMap.size(), type);

			unsigned char* pp = (unsigned char*)base_name;
			for (int i = 0; i < strlen(base_name); i++) {
				if (!isalnum(*pp)) *pp = '_';
				pp++;
			}
			if (!strchr(base_name, '.')) {
				sprintf_s(buf, MAX_PATH, "%s_%d.%s", base_name, (UINT)m_TextureMap.size(), type);
				strcpy_s(base_name, MAX_PATH, buf);
			}
			char name[MAX_PATH];
			sprintf_s(name, MAX_PATH, "%s\\%s", WStringToString(m_WorkImageFolder).c_str(), base_name);

			FILE* fp = nullptr;
			errno_t err = fopen_s(&fp, name, "wb");
			
			if (err) { delete[] out_data; return StringToWString(name); }

			fwrite(out_data, ByteSize, 1, fp);
			fclose(fp);
			
			delete[] out_data;
			fname = StringToWString(name);
			m_EmbedFormat = TRUE;
		}
		else {
			/*
			//std::filesystem::path file(uri);
			fname = urlDecode(StringToWString(uri));
			fname = tstring(m_fullpath.parent_path()) + tstring(_T("\\")) + fname;
			*/
			tstring decodedUri = urlDecode(StringToWString(uri));

			fs::path baseDir = m_fullpath.parent_path().lexically_normal();
			fs::path fullPath = (baseDir / fs::path(decodedUri)).lexically_normal();

			try {
				// lexically_relative - calculates characters like ".." based on strings
				fs::path rel = fullPath.lexically_relative(baseDir);

				// An error will occur if the directory is empty or starts with ".." (i.e., points outside the base directory).
				// * Due to the specifications of std::filesystem, when pointing outside the base directory, the beginning will always be "..", like "../foo".
				if (rel.empty() || rel.native().rfind(L"..", 0) == 0 || rel.native() == L"..") {
					// Security Error (Directory Traversal)
					fname = _T("");
				}
				else {
					fname = fullPath.wstring();
				}
			}
			catch (...) {
				fname = _T("");
			}

#if 0
			tstring decodedUri = urlDecode(StringToWString(uri));

			fs::path baseDir = m_fullpath.parent_path();
			fs::path fullPath = baseDir / fs::path(decodedUri);

			try {
				fs::path canonicalPath = fs::weakly_canonical(fullPath);
				fs::path canonicalBase = fs::canonical(baseDir);
				auto rel = fs::relative(canonicalPath, canonicalBase);

				// Check if the file is out of directory
				if (rel.empty() || rel.string().find("..") != std::string::npos) {
					// Seculity Error
					fname = _T("");
				}
				else {
					fname = canonicalPath.wstring();
				}
			}
			catch (...) {
				fname = _T("");
			}
#endif

		}
	}
	else if (strlen(image->mime_type) > 1) {
		const char* type = MimeTypes::getExtension(image->mime_type);
		cgltf_buffer_view* bufferview = image->buffer_view;
		cgltf_buffer* buffer = bufferview->buffer;
		size_t offset = bufferview->offset;
		size_t size = bufferview->size;
		void* ptr = (char*)(buffer->data) + offset;

		char base_name[MAX_PATH];
		//char buf[MAX_PATH];
		if (image->name) {
			strncpy_s(base_name, MAX_PATH, image->name, MAX_PATH);
			char* ptr = strchr(base_name, '.');
			if (ptr) *(ptr + 1) = 0;
			strcat_s(base_name, MAX_PATH, type);
		}
		else if (tex->name)	strncpy_s(base_name, MAX_PATH, tex->name, MAX_PATH);
		else 				sprintf_s(base_name, MAX_PATH, "texture_%d.%s", (UINT)m_TextureMap.size(), type);

		tstring str = StringToWString(base_name);
		auto pos = str.rfind('\\');
		if (pos != tstring::npos) {
			str = str.substr(pos + 1, str.size());
		}

		if (str.rfind(_T(".")) == std::string::npos) {
			TCHAR buf[1000];
			_stprintf_s(buf, _countof(buf), _T("%s_%d.%s"), str.c_str(), (UINT)m_TextureMap.size(), StringToWString(type).c_str()); 
			str = tstring(buf);
		}
		//char name[MAX_PATH];
		//sprintf(name, "%s\\%s", WStringToString(m_WorkImageFolder).c_str(), base_name);
		fname = m_WorkImageFolder + tstring(_T("\\")) + str;

		FILE* fp = nullptr;
		errno_t err = _tfopen_s(&fp, fname.c_str(), _T("wb"));
		if (err == 0) {
			fwrite(ptr, size, 1, fp);
			fclose(fp);
		}
		//fname = StringToWString(name);

		m_EmbedFormat = TRUE;
	}

	if (fname.find(_T(".webp"))!=std::string::npos) {
		originalFname = fname;
		WebpDecode(originalFname, fname);
	}
	else if (fname.find(_T(".ktx2")) != std::string::npos) {
		originalFname = fname;
		KTX2ImageCreater(originalFname, fname);
	}

	return fname;
}

//=============================================================================
//=============================================================================
void glTFImporter_Core::CreateTextureTable(void)
{
	m_EmbedFormat = FALSE;
	tstring fname;
	m_TextureMap.clear();
	m_TextureViewMap.clear();

	SetTexImportStatus(0);

	int index = 0;
	
	for (int i = 0; i < m_glTF_data->textures_count; i++) {
		cgltf_texture* tex = &m_glTF_data->textures[i];
		if (!tex) continue;

		tstring originalFname;
		fname = CreateTextureFileName(tex, originalFname);

		BitmapTex* pBmpTex = NewDefaultBitmapTex();
		pBmpTex->GetUVGen()->SetCoordMapping(UVMAP_SCREEN_ENV);
		pBmpTex->GetUVGen()->SetTextureTiling(U_WRAP | V_WRAP);
		pBmpTex->GetUVGen()->InitSlotType(MAPSLOT_TEXTURE);
		pBmpTex->SetMapName(fname.c_str());
		pBmpTex->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED, TRUE);
		pBmpTex->ActivateTexDisplay(TRUE);

#if MAX_RELEASE > 27000
		/* {
			MaxSDK::ColorManagement::IColorPipelineMgr* pColMgr = (MaxSDK::ColorManagement::IColorPipelineMgr*)GetCOREInterface(COLORPIPELINEMGR_INTERFACE);
			MaxSDK::ColorManagement::ColorPipelineMode pp = pColMgr->GetColorPipelineMode();
			BitmapInfo bi = pBmpTex->GetBitmap(0)->GetBitmapInfo();
			MaxSDK::ColorManagement::ColSpaceStatus sss = bi.SetRequestedColorSpace(_T("Raw"), MaxSDK::ColorManagement::ColSpaceSource::User);
			pBmpTex->SetBitmapInfo(bi);
		}*/
#endif
		if (originalFname.find(_T(".webp")) != std::string::npos) {
			CreateWebpEncodingAttr(pBmpTex, originalFname, originalFname.size() > 0);
		}
		else if (originalFname.find(_T(".ktx2")) != std::string::npos) {
			CreateKTX2EncodingAttr(pBmpTex, originalFname, originalFname.size() > 0);
		}

		m_TextureMap.insert(std::make_pair(tex, pBmpTex));
		SetTexImportStatus(i+1);
	}

	SetTexImportStatus(-1);
}

//=============================================================================
//=============================================================================
BitmapTex *glTFImporter_Core::GetBitmapTexFromglTexture(cgltf_texture *tex)
{
	if (!tex) return NULL;

	if (tex->has_basisu) {
		cgltf_image *image = tex->basisu_image;
	}

	if (m_UniqueTexture) {
		BitmapTex *pTex = m_TextureMap[tex];
		RemapDir *pRemap = NewRemapDir();
		BitmapTex *pNewTex = (BitmapTex*)pTex->Clone(*pRemap);
		pRemap->DeleteThis();
		return pNewTex;
	}
	else {
		return m_TextureMap[tex];
	}
}

static float s_gamma = 1.0f;

//=============================================================================
//=============================================================================
BitmapTex* glTFImporter_Core::SplitOcclusionTexture(BitmapTex *pOrgTexBmp)
{
	gammaMgr.SetFileOutGamma(1.0f);

	Bitmap *pOriginalBmp = pOrgTexBmp->GetBitmap(0);
	if (!pOriginalBmp) return NULL;

	SetPNGInfo(m_pPNG_BmpIO, pOriginalBmp);

	BitmapInfo orgbi = pOriginalBmp->GetBitmapInfo();
	//std::filesystem::path fname = orgbi.Filename();
	std::filesystem::path fname = pOrgTexBmp->GetMapName();
	if (!fname.has_parent_path()) {
		fname = m_SourceImageFolder + orgbi.Filename();
	}

	tstring texFilePath = tstring(m_WorkImageFolder) + tstring(fname.stem()) + tstring(_T("_R")) + tstring(fname.extension());
	CopyFile(fname.c_str(), texFilePath.c_str(), FALSE);

	BitmapTex *pBmpTex = NewDefaultBitmapTex();
	pBmpTex->SetMapName(texFilePath.c_str());

	BitmapInfo bi(orgbi);
	bi.SetPath(texFilePath.c_str());
	bi.ResetCustomFlag(BMM_CUSTOM_GAMMA);

	//bi.SetGamma(s_gamma);
	BMMRES status;
	Bitmap *pBitmap = TheManager->Load(&bi, &status);
	for (int w = 0; w < bi.Width(); w++) {
		for (int h = 0; h < bi.Height(); h++) {
			BMM_Color_64 buff;
			//pBitmap->GetLinearPixels(w, h, 1, &buff);
			pBitmap->GetPixels(w, h, 1, &buff);
			buff.g = buff.r;
			buff.b = buff.r;
			buff.a = 0;
			pBitmap->PutPixels(w, h, 1, &buff);
		}
	}

	pBitmap->OpenOutput(&bi);
	pBitmap->Write(&bi);
	pBitmap->Close(&bi);
	pBitmap->DeleteThis();

	pBmpTex->GetUVGen()->SetCoordMapping(UVMAP_SCREEN_ENV);
	pBmpTex->GetUVGen()->SetTextureTiling(0);
	pBmpTex->GetUVGen()->InitSlotType(MAPSLOT_TEXTURE);

	int mapCh = pOrgTexBmp->GetTheUVGen()->GetMapChannel();
	pBmpTex->GetUVGen()->SetMapChannel(mapCh);

	pBmpTex->ReloadBitmapAndUpdate();

	gammaMgr.SetFileOutGamma(2.2f);

	return pBmpTex;
}

//=============================================================================
//=============================================================================
BitmapTex* glTFImporter_Core::SplitRoughnessTexture(BitmapTex *pOrgTexBmp)
{
	gammaMgr.SetFileOutGamma(1.0f);

	Bitmap *pOriginalBmp = pOrgTexBmp->GetBitmap(0);
	if (!pOriginalBmp) return NULL;

	SetPNGInfo(m_pPNG_BmpIO, pOriginalBmp);

	BitmapInfo orgbi = pOriginalBmp->GetBitmapInfo();
	//std::filesystem::path fname = orgbi.Filename();
	std::filesystem::path fname = pOrgTexBmp->GetMapName();
	if (!fname.has_parent_path()) {
		fname = m_SourceImageFolder + orgbi.Filename();
	}

	tstring texFilePath = tstring(m_WorkImageFolder) + tstring(fname.stem()) + tstring(_T("_G")) + tstring(fname.extension());
	CopyFile(fname.c_str(), texFilePath.c_str(), FALSE);

	BitmapTex *pBmpTex = NewDefaultBitmapTex();
	pBmpTex->SetMapName(texFilePath.c_str());

	BitmapInfo bi(orgbi);
	bi.SetPath(texFilePath.c_str());
	//bi.SetGamma(s_gamma);
	BMMRES status;
	Bitmap *pBitmap = TheManager->Load(&bi, &status);
	//pBmpTex->ReloadBitmapAndUpdate();
	for (int w = 0; w < bi.Width(); w++) {
		for (int h = 0; h < bi.Height(); h++) {
			BMM_Color_64 buff;
			pBitmap->GetLinearPixels(w, h, 1, &buff);
			//pBitmap->GetPixels(w, h, 1, &buff);
	/*
			{
				pBitmap->GetPixels(w, h, 1, &buff);
				COLORREF col = RGB(buff.r, buff.g, buff.b);
				COLORREF c = gammaMgr.DisplayGammaCorrect(col);
				buff.g = GetGValue(c);
			}
	*/
			buff.r = buff.g;
			buff.b = buff.g;
			buff.a = 0;
			//buff.r = buff.b = buff.g;
			pBitmap->PutPixels(w, h, 1, &buff);
		}
	}
	pBitmap->OpenOutput(&bi);
	pBitmap->Write(&bi);
	pBitmap->Close(&bi);
	pBitmap->DeleteThis();

	pBmpTex->GetUVGen()->SetCoordMapping(UVMAP_SCREEN_ENV);
	pBmpTex->GetUVGen()->SetTextureTiling(U_WRAP | V_WRAP);
	pBmpTex->GetUVGen()->InitSlotType(MAPSLOT_TEXTURE);

	int mapCh = pOrgTexBmp->GetTheUVGen()->GetMapChannel();
	pBmpTex->GetUVGen()->SetMapChannel(mapCh);

	pBmpTex->ReloadBitmapAndUpdate();

	if (m_CorrectGamma)
		CorrectBitmapGamma(pBmpTex, m_GammaValue);
//	else
//		CorrectBitmapGamma(pBmpTex, 2.2f, FALSE);

	gammaMgr.SetFileOutGamma(2.2f);

	return pBmpTex;
}

//=============================================================================
//=============================================================================
BitmapTex* glTFImporter_Core::SplitMetalnessTexture(BitmapTex *pOrgTexBmp)
{
	gammaMgr.SetFileOutGamma(1.0f);

	Bitmap *pOriginalBmp = pOrgTexBmp->GetBitmap(0);
	if (!pOriginalBmp) return NULL;

	SetPNGInfo(m_pPNG_BmpIO, pOriginalBmp);

	BitmapInfo orgbi = pOriginalBmp->GetBitmapInfo();
	orgbi.GetDeviceFlags();
	//std::filesystem::path fname = orgbi.Filename();
	std::filesystem::path fname = pOrgTexBmp->GetMapName();
	if (!fname.has_parent_path()) {
		fname = m_SourceImageFolder + orgbi.Filename();
	}

	tstring texFilePath = tstring(m_WorkImageFolder) + tstring(fname.stem()) + tstring(_T("_B")) + tstring(fname.extension());
	CopyFile(fname.c_str(), texFilePath.c_str(), FALSE);

	BitmapTex *pBmpTex = NewDefaultBitmapTex();
	pBmpTex->SetMapName(texFilePath.c_str());

	BitmapInfo bi(orgbi);
	bi.SetPath(texFilePath.c_str());
	bi.SetGamma(s_gamma);
	BMMRES status;
	Bitmap *pBitmap = TheManager->Load(&bi, &status);
	pBmpTex->ReloadBitmapAndUpdate();
	for (int w = 0; w < bi.Width(); w++) {
		for (int h = 0; h < bi.Height(); h++) {
			BMM_Color_64 buff;
			pBitmap->GetLinearPixels(w, h, 1, &buff);
			//pBitmap->GetPixels(w, h, 1, &buff);
			buff.r = buff.b;
			buff.g = buff.b;
			pBitmap->PutPixels(w, h, 1, &buff);
		}
	}
	pBitmap->OpenOutput(&bi);
	pBitmap->Write(&bi);
	pBitmap->Close(&bi);
	pBitmap->DeleteThis();

	pBmpTex->GetUVGen()->SetCoordMapping(UVMAP_SCREEN_ENV);
	pBmpTex->GetUVGen()->SetTextureTiling(U_WRAP | V_WRAP);
	pBmpTex->GetUVGen()->InitSlotType(MAPSLOT_TEXTURE);

	int mapCh = pOrgTexBmp->GetTheUVGen()->GetMapChannel();
	pBmpTex->GetUVGen()->SetMapChannel(mapCh);

	pBmpTex->ReloadBitmapAndUpdate();

	if (m_CorrectGamma)
		CorrectBitmapGamma(pBmpTex, m_GammaValue);
//	else
//		CorrectBitmapGamma(pBmpTex, 2.2f, FALSE);

	gammaMgr.SetFileOutGamma(2.2f);

	return pBmpTex;
}

//=============================================================================
//=============================================================================
Texmap* glTFImporter_Core::SetMetalRoughOccClrCorrectMap(Texmap* pOrgTexBmp, Texmap** pMetalTex, Texmap** pRoughTex, Texmap** pOccTex, BOOL Metallic, BOOL Roughness, BOOL Occlusion)
{
#define	clrCrctMap	1
#define	rewireMode	2
#define	rewireR		3
#define	rewireG		4
#define	rewireB		5
#define	rewireA		6

//	if (m_CorrectGamma)
//		CorrectBitmapGamma(pOrgTexBmp, m_GammaValue);
//	else
//		CorrectBitmapGamma(pOrgTexBmp, 2.2f, FALSE);

	if (Metallic) *pMetalTex = NULL;
	if (Roughness) *pRoughTex = NULL;
	if (Occlusion) *pOccTex = NULL;

//	TSTR name = pOrgTexBmp->GetMapName();
	//Texmap* pTex = CreateMetalRoughOccOSLNode(pOrgTexBmp, 0);
	//IParamBlock2* pBlock = pTex->GetParamBlock(1);
	//pBlock->SetValue(0, m_time, name);

	if (Metallic)
	{
		*pMetalTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, ColorCorrectTexID);
		IParamBlock2* pBlock = (*pMetalTex)->GetParamBlock(0);
		pBlock->SetValue(clrCrctMap, m_time, pOrgTexBmp);
		pBlock->SetValue(rewireMode, m_time, 3);
		pBlock->SetValue(rewireR, m_time, 2);
		pBlock->SetValue(rewireG, m_time, 2);
		pBlock->SetValue(rewireB, m_time, 2);
	}
	if (Roughness)
	{
		*pRoughTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, ColorCorrectTexID);
		IParamBlock2* pBlock = (*pRoughTex)->GetParamBlock(0);
		pBlock->SetValue(clrCrctMap, m_time, pOrgTexBmp);
		pBlock->SetValue(rewireMode, m_time, 3);
		pBlock->SetValue(rewireR, m_time, 1);
		pBlock->SetValue(rewireG, m_time, 1);
		pBlock->SetValue(rewireB, m_time, 1);
	}
	if (Occlusion)
	{
		*pOccTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, ColorCorrectTexID);
		IParamBlock2* pBlock = (*pOccTex)->GetParamBlock(0);
		pBlock->SetValue(clrCrctMap, m_time, pOrgTexBmp);
		pBlock->SetValue(rewireMode, m_time, 3);
		pBlock->SetValue(rewireR, m_time, 0);
		pBlock->SetValue(rewireG, m_time, 0);
		pBlock->SetValue(rewireB, m_time, 0);
	}

	return pOrgTexBmp;
}

//=============================================================================
//=============================================================================
Texmap* glTFImporter_Core::SetAlphaClrCorrectMap(Texmap* pOrgTexBmp, Texmap** pAlphaTex)
{
#define	clrCrctMap	1
#define	rewireMode	2
#define	rewireR		3
#define	rewireG		4
#define	rewireB		5
#define	rewireA		6

	*pAlphaTex = NULL;

	*pAlphaTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, ColorCorrectTexID);
	IParamBlock2* pBlock = (*pAlphaTex)->GetParamBlock(0);
	pBlock->SetValue(clrCrctMap, m_time, pOrgTexBmp);
	pBlock->SetValue(rewireMode, m_time, 3);
	pBlock->SetValue(rewireR, m_time, 3);
	pBlock->SetValue(rewireG, m_time, 3);
	pBlock->SetValue(rewireB, m_time, 3);

	return pOrgTexBmp;
}

//=============================================================================
//=============================================================================
#if 1
Texmap* glTFImporter_Core::SetMetalRoughOccOSLMap(BitmapTex* pOrgTexBmp, Texmap** pMetalTex, Texmap** pRoughTex, Texmap** pOccTex, BOOL Metallic, BOOL Roughness, BOOL Occlusion, cgltf_texture_view *texview)
{
#define	sourceMap			0
#define	outputChannelIndex	1

	* pMetalTex = NULL;
	*pRoughTex = NULL;
	*pOccTex = NULL;

	int mapCh = 1;
	cgltf_texture_transform *transform = NULL;
	if (texview) {
		if (texview->has_transform) transform = &texview->transform;
		mapCh = texview->texcoord + 1;
	}

	TSTR name = pOrgTexBmp->GetMapName();
	Texmap* pTex = NULL;
	if (TRUE) {	// transform -> TRUE
		pTex = CreateUberBitmapOSLNode(name);
		IParamBlock2 *pBlock1 = pTex->GetParamBlock(1);
		Point3 offset(0.0f, 0.0f, 0.0f);
		Point3 tiling(1.0f, 1.0f, 0.0f);
		float rot = 0.0f;
		if (transform) {
			offset.x = transform->offset[0];
			offset.y = transform->offset[1];
			tiling.x = transform->scale[0];
			tiling.y = transform->scale[1];
			rot = transform->rotation;
		}
		pBlock1->SetValue(UberBmp_Offset, m_time, offset);
		pBlock1->SetValue(UberBmp_Rotate, m_time, rot);
		pBlock1->SetValue(UberBmp_Tiling, m_time, tiling);
		pBlock1->SetValue(UberBmp_UVSet, m_time, mapCh);

		if (transform->has_texcoord) {
		}

		if (Metallic)
		{
			*pMetalTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
			IParamBlock2* pBlock = (*pMetalTex)->GetParamBlock(0);
			pBlock->SetValue(sourceMap, m_time, pTex);
			pBlock->SetValue(outputChannelIndex, m_time, 3);
		}
		if (Roughness)
		{
			*pRoughTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
			IParamBlock2* pBlock = (*pRoughTex)->GetParamBlock(0);
			pBlock->SetValue(sourceMap, m_time, pTex);
			pBlock->SetValue(outputChannelIndex, m_time, 2);
		}
		if (Occlusion)
		{
			*pOccTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
			IParamBlock2* pBlock = (*pOccTex)->GetParamBlock(0);
			pBlock->SetValue(sourceMap, m_time, pTex);
			pBlock->SetValue(outputChannelIndex, m_time, 1);
		}
	}
	else {
		pTex = CreateBitmapLookupOSLNode(name);
		//IParamBlock2* pBlock = pTex->GetParamBlock(1);
		//pBlock->SetValue(0, m_time, name);

		if (Metallic)
		{
			*pMetalTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
			IParamBlock2* pBlock = (*pMetalTex)->GetParamBlock(0);
			pBlock->SetValue(sourceMap, m_time, pTex);
			pBlock->SetValue(outputChannelIndex, m_time, 3);
		}
		if (Roughness)
		{
			*pRoughTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
			IParamBlock2* pBlock = (*pRoughTex)->GetParamBlock(0);
			pBlock->SetValue(sourceMap, m_time, pTex);
			pBlock->SetValue(outputChannelIndex, m_time, 2);
		}
		if (Occlusion)
		{
			*pOccTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
			IParamBlock2* pBlock = (*pOccTex)->GetParamBlock(0);
			pBlock->SetValue(sourceMap, m_time, pTex);
			pBlock->SetValue(outputChannelIndex, m_time, 1);
		}
	}

	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pOrgTexBmp, tstring(_T("Webp Encode")), pBlock);
	if (pBlock) {
		tstring str = pBlock->GetStr(2, m_time);
		CreateWebpEncodingAttr(pTex, str, str.size() > 0);
	}

	return pTex;
}

#else
Texmap* glTFImporter_Core::SetMetalRoughOccOSLMap(BitmapTex *pOrgTexBmp, Texmap **pMetalTex, Texmap **pRoughTex, Texmap **pOccTex, BOOL Metallic, BOOL Roughness, BOOL Occlusion)
{
#define	sourceMap			0
#define	outputChannelIndex	1

	*pMetalTex = NULL;
	*pRoughTex = NULL;
	*pOccTex = NULL;

	TSTR name = pOrgTexBmp->GetMapName();
	Texmap *pTex = CreateMetalRoughOccOSLNode(pOrgTexBmp,0);
	IParamBlock2 *pBlock = pTex->GetParamBlock(1);
	pBlock->SetValue(0, m_time, name);

	if (Metallic)
	{
		*pMetalTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
		IParamBlock2 *pBlock = (*pMetalTex)->GetParamBlock(0);
		pBlock->SetValue(sourceMap, m_time, pTex);
		pBlock->SetValue(outputChannelIndex, m_time, 1);
	}
	if (Roughness)
	{
		*pRoughTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
		IParamBlock2 *pBlock = (*pRoughTex)->GetParamBlock(0);
		pBlock->SetValue(sourceMap, m_time, pTex);
		pBlock->SetValue(outputChannelIndex, m_time, 0);
	}
	if (Occlusion)
	{
		*pOccTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, MULTIOUTPUTTOTEXMAP_CLASS_ID);
		IParamBlock2 *pBlock = (*pOccTex)->GetParamBlock(0);
		pBlock->SetValue(sourceMap, m_time, pTex);
		pBlock->SetValue(outputChannelIndex, m_time, 2);
	}

	return pTex;
}
#endif
//=============================================================================
//=============================================================================
int GetColorDepth(Bitmap *pBitmap)
{
	int type;
	void *pBfffer = pBitmap->GetStoragePtr(&type);
	int depth = 32;
	switch (type) {
	case BMM_BMP_4:		depth = 4; break;
	case BMM_TRUE_16:	depth = 16; break;
	case BMM_TRUE_24:	depth = 24; break;
	case BMM_TRUE_32:	depth = 32; break;
	case BMM_TRUE_64:	depth = 64; break;
	case BMM_LINE_ART:	depth = 1; break;
	case BMM_PALETTED:	depth = 8; break;
	}

	return depth;
}
//=============================================================================
//=============================================================================
void SetPNGInfo(IBitmapIO_Png *pPNG_BmpIO, Bitmap *pBitmap)
{
	if (!pPNG_BmpIO | !pBitmap) return;

	int type;
	pBitmap->GetStoragePtr(&type);
	pPNG_BmpIO->SetType(type);
	pPNG_BmpIO->SetAlpha(pBitmap->HasAlpha());
}

//=============================================================================
//=============================================================================
Texmap *CreateBaseColorMap(void)
{

	TSTR ComStr;
	ComStr.printf(_T("tex = CompositeMap(); tex.mapEnabled.count = 2; tex"));
	FPValue fpv;
#if MAX_RELEASE >= 24000
	ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, FALSE, &fpv);
#else
	ExecuteMAXScriptScript(ComStr, FALSE, &fpv);
#endif
	Texmap *pTex = fpv.tex;

	return pTex;
}
//=============================================================================
//=============================================================================
Texmap *CreateColorMap(AColor &c)
{
#define solidcolor 0
#define mapEnabled 2

	Texmap *pTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, ColorMapTexID);
	Control* pC = (Control*)GetCOREInterface()->CreateInstance(CTRL_POINT4_CLASS_ID, Class_ID(0x2012, 0x0));
	pTex->GetParamBlock(0)->SetControllerByIndex(solidcolor, 0, pC);

	pTex->GetParamBlock(0)->SetValue(solidcolor, 0, c);
	pTex->GetParamBlock(0)->SetValue(mapEnabled, 0, 0);

	return pTex;
}

//=============================================================================
//=============================================================================
cgltf_texture *glTFImporter_Core::GetglTFTexByTexmap(Texmap* pTex)
{
	for (auto it : m_TextureMap) {
		if (it.second == pTex) return it.first;
	}

	return NULL;
}


//=============================================================================
//=============================================================================
Point2 ApplyGltfTextureTransform(Texmap* pBmpTex, const cgltf_texture_view* textview, TimeValue t)
{
	Point2 offset(0.0f, 0.0f);
	if (!pBmpTex || !textview || !textview->texture) return offset;

	StdUVGen* pUVGen = GetUVGen(pBmpTex);
	if (!pUVGen) return offset;

	// UV chanel （Map Channel +1）
	pUVGen->SetMapChannel(textview->texcoord + 1);

	// Sampler's tiling setting
	UINT tiling = 0;
	cgltf_sampler* pSampler = textview->texture->sampler;
	if (pSampler) {
		if (pSampler->wrap_s == 10497) tiling |= U_WRAP;
		if (pSampler->wrap_t == 10497) tiling |= V_WRAP;
		if (pSampler->wrap_s == 33648) tiling |= U_WRAP | U_MIRROR;
		if (pSampler->wrap_t == 33648) tiling |= V_WRAP | V_MIRROR;
		pUVGen->SetTextureTiling(tiling);
	}

	// Transform not found or disabled
	if (!textview->has_transform) return offset;

	// glTF transform information
	float sclU = textview->transform.scale[0];
	float sclV = textview->transform.scale[1];
	float rot = textview->transform.rotation;
	offset.x = textview->transform.offset[0];
	offset.y = textview->transform.offset[1];

	// Adjust offset (Rptate origin: glTF=[0.5, 0.5], 3ds Max=[0.0, 0.0])
	float pivotU = 0.5f;
	float pivotV = 0.5f;

	float cosR = cosf(rot);
	float sinR = sinf(rot);

	float dU = pivotU - (pivotU * cosR - pivotV * sinR);
	float dV = pivotV - (pivotU * sinR + pivotV * cosR);

	offset.x += dU;
	offset.y += dV;

	// if scale<1.0, adjust scale origin
	if (sclU != 0.0f) offset.x = offset.x / sclU;
	if (sclV != 0.0f) offset.y = offset.y / sclV;

	//Set  UVGen
	pUVGen->SetUOffs(offset.x, t);
	pUVGen->SetVOffs(offset.y, t);
	pUVGen->SetUScl(sclU, t);
	pUVGen->SetVScl(sclV, t);
	pUVGen->SetWAng(rot, t); // expects radians

	if (pBmpTex->ClassID() == bmptexClassID) {
		((BitmapTex*)pBmpTex)->ReloadBitmapAndUpdate();
	}

	return offset;
}

#if 1	// Original
//=============================================================================
//=============================================================================
Point2 glTFImporter_Core::SetTextureUVoffset(Texmap *pBmpTex, cgltf_texture_view *textview)
{
	//return ApplyGltfTextureTransform(pBmpTex, textview, m_time);

	Point2 offset(0.0f, 0.0f);
	if (!pBmpTex) return offset;
	if (!textview) return offset;

	StdUVGen* pUVGen = GetUVGen(pBmpTex);
	if (pUVGen) pUVGen->SetMapChannel(textview->texcoord + 1);

	float sclU = 1.0f;
	float sclV = 1.0f;

	cgltf_sampler* sampler = textview->texture->sampler;
	if (sampler && pUVGen) {
		UINT Tiling = 0;
		if (sampler->wrap_s == 10497) Tiling += U_WRAP;
		if (sampler->wrap_t == 10497) Tiling += V_WRAP;
		if (sampler->wrap_s == 33648) {
			Tiling += U_WRAP + U_MIRROR;
			//sclU = -1.0f;
		}
		if (sampler->wrap_t == 33648) {
			Tiling += V_WRAP + V_MIRROR;
			//sclV = -1.0f;
		}

		pUVGen->SetUScl(sclU, m_time);
		pUVGen->SetVScl(sclV, m_time);

		pUVGen->SetTextureTiling(Tiling);
		int mapCh = textview->texcoord + 1;
		pUVGen->SetMapChannel(mapCh);
	}

	if (!textview->has_transform) return offset;
	if(m_Quantization) return offset;

	m_TextureViewMap.insert(std::make_pair(pBmpTex, textview));

	offset.x = textview->transform.offset[0];
	offset.y = textview->transform.offset[1];
	sclU *= textview->transform.scale[0];
	sclV *= textview->transform.scale[1];
	float rot = textview->transform.rotation;

	float localoffsetU = -0.5f * cos(rot) + 0.5f * sin(rot) + 0.5f;
	float localoffsetV = -0.5f * sin(rot) - 0.5f * cos(rot) + 0.5f;
	if (sclU == 0.0f) {
	}
	else if (sclU >= 1.0f) {
		localoffsetU += (1.0f - (1.0f / sclU)) / 2.0f;
		offset.x = -offset.x - localoffsetU;
	}
	else if (sclU < 1.0f) {
		offset.x = (1.0f / sclU - 1.0f) * 0.5f + (1.0f - offset.x) / sclU;
	}

	if (sclV == 0.0f) {
	}
	else if (sclV >= 1.0f) {
		localoffsetV += (1.0f - (1.0f / sclV)) / 2.0f;
		offset.y = offset.y + localoffsetV;
	}
	else if (sclV < 1.0f) {
		offset.y = (offset.y) / sclV-(1.0f / sclV - 1.0f) / 2.0f;
	}

	if (pUVGen) {
		pUVGen->SetUOffs(offset.x, m_time);
		pUVGen->SetVOffs(offset.y, m_time);
		pUVGen->SetUScl(sclU, m_time);
		pUVGen->SetVScl(sclV, m_time);
		pUVGen->SetWAng(rot, m_time);
	}

	if (pBmpTex->ClassID() == bmptexClassID) {
		((BitmapTex*)pBmpTex)->ReloadBitmapAndUpdate();
	}

	return offset;
}

#else // Gemini
//=============================================================================
//=============================================================================
Point2 glTFImporter_Core::SetTextureUVoffset(Texmap* pBmpTex, cgltf_texture_view* textview)
{
	Point2 p2(0.0f, 0.0f);
	if (textview && textview->has_transform)
	{
		float scaleU = textview->transform.scale[0];
		float scaleV = textview->transform.scale[1];
		float rot = textview->transform.rotation;
		float offU = textview->transform.offset[0];
		float offV = textview->transform.offset[1];

		if (pBmpTex) {
			StdUVGen* uvGen = GetUVGen(pBmpTex);
			if (uvGen) {
				TimeValue t = GetCOREInterface()->GetTime();
				cgltf_sampler* pSampler = textview->texture->sampler;
				if (pSampler) {
					UINT Tiling = 0;

					if (pSampler->wrap_s == 10497) Tiling += U_WRAP;
					if (pSampler->wrap_t == 10497) Tiling += V_WRAP;
					if (pSampler->wrap_s == 33648) Tiling += U_WRAP + U_MIRROR;
					if (pSampler->wrap_t == 33648) Tiling += V_WRAP + V_MIRROR;
					uvGen->SetTextureTiling(Tiling);
					int mapCh = textview->texcoord + 1;
					uvGen->SetMapChannel(mapCh);
				}

				// 1. Setting the tiling (scale)
				uvGen->SetUScl(scaleU, t);
				uvGen->SetVScl(scaleV, t);

				// 2. Rotation Settings
				// glTF rotates counterclockwise around the origin (0,0).
				// Max's WAng is similar, but the coordinate system is inverted vertically, 
				// so correction of the rotation direction and center may be necessary.
				uvGen->SetWAng(rot, t);

				// 3. Offset Calculation
				// In Max's UVGen, the Offset value is treated as "1 unit after tiling",
				// so the following formula is standard when there is no rotation:
				p2.x = offU;
				// V direction: Convert glTF's "offset from top edge" to Max's "offset from bottom edge"
				// Further consider the tiling (height) and adjust the starting point to the bottom edge.
				p2.y = 1.0f - scaleV - offV;
				uvGen->SetUOffs(p2.x, t);
				uvGen->SetVOffs(p2.y, t);

				// 4. Important: Setting the origin of the coordinate system
				// If you need to set the rotation and scaling center to (0,0) to conform to the glTF specifications,
				// Check the following flag (uncomment if necessary)
				// uvGen->SetFlag(U_OFFSET, 0);
			}
		}
	}
	return p2;
}
#endif

//=============================================================================
//=============================================================================
void glTFImporter_Core::RescaleUVOffset(Mtl *pMtl, Box2D &rect)
{
	return;


	Point2 CropSize;
	CropSize.x = rect.max.x - rect.min.x;
	CropSize.y = rect.max.y- rect.min.y;

	if (CropSize.x == 1.0f && CropSize.y == 1.0f) return;
	if (CropSize.x == 0.0f && CropSize.y == 0.0f) return;

	float cropRateU = 1.0f / CropSize.x;
	float cropRateV = 1.0f / CropSize.y;

	int texnum = pMtl->NumSubTexmaps();
	for (int i = 0; i < texnum; i++) {
		Texmap* pTex = pMtl->GetSubTexmap(i);
		if (!pTex) continue;

		//cgltf_texture* tex = GetglTFTexByTexmap(pTex);
		//if (!tex) return;

		cgltf_texture_view* textview = m_TextureViewMap[pTex];
		if (!textview) return;

		Point2 offset(0.0f, 0.0f);
		offset.x = textview->transform.offset[0];
		offset.y = textview->transform.offset[1];
		float sclU = textview->transform.scale[0];
		float sclV = textview->transform.scale[1];
		float rot = textview->transform.rotation;

		float localoffsetU = -0.5f * cos(rot) + 0.5f * sin(rot) + 0.5f;
		float localoffsetV = -0.5f * sin(rot) - 0.5f * cos(rot) + 0.5f;
		localoffsetU += (1.0f / cropRateU - (1.0f / sclU)) / 2.0f;
		localoffsetV += (1.0f / cropRateV - (1.0f / sclV)) / 2.0f;
		offset.x = -offset.x - localoffsetU;
		offset.y = offset.y + localoffsetV;

		Point2 newOffset(0.0f, 0.0f);
		newOffset.x =  offset.x;
		newOffset.y =  offset.y;

		{
			float rotoffsetU = offset.x * cos(rot) + offset.y * sin(rot);
			float rotoffsetV = offset.x * sin(rot) - offset.y * cos(rot);
			Point2 newOffset2(0.0f, 0.0f);
			newOffset2.x = (rotoffsetU - CropSize.x / 2.0f) / sclU;
			newOffset2.y = (rotoffsetV - CropSize.y / 2.0f) / sclV;
		}

		StdUVGen* pUVGen = GetUVGen(pTex);
		if(pUVGen){
			pUVGen->SetUOffs(newOffset.x, m_time);
			pUVGen->SetVOffs(newOffset.y, m_time);
		}
	}
}

//=============================================================================
//=============================================================================
void glTFImporter_Core::CorrectBitmapGamma(BitmapTex*& pBmpTex, float gamma, BOOL custom)
{
	if (!pBmpTex) return;

	if (pBmpTex->ClassID() == VRayBitmapID) {
	}
	else {
		IParamBlock2* pb2 = pBmpTex->GetParamBlock(0);
		// get the bitmap parameter
		int n = pb2->GetDesc()->NameToIndex(_T("bitmap"));
		ParamID id = pb2->GetDesc()->IndextoID(n);
		PBBitmap* pbBitmap = pb2->GetBitmap(id);
		if (custom) {
#if MAX_RELEASE >= 26000
			if (gamma == 1.0f) {

				Bitmap* pBmp = pBmpTex->GetBitmap(0);
				BitmapInfo* bi = &pBmp->GetBitmapInfo();

				{
					//auto cpm = MaxSDK::ColorManagement::IColorPipelineMgr::GetInstance();
					MaxSDK::ColorManagement::IColorPipelineMgr* cpm = (MaxSDK::ColorManagement::IColorPipelineMgr*)GetCOREInterface(COLORPIPELINEMGR_INTERFACE);

					auto settings = cpm->Settings();
					if (settings->IsOCIOBased())
					{
						BitmapInfo bmi(*bi);
						auto ret = bmi.SetRequestedColorSpace(settings->GetDataColorSpaceName(), MaxSDK::ColorManagement::ColSpaceSource::User);
						bmi.SetName(pBmpTex->GetMapName());
						bmi.ResetCustomFlag(BMM_CUSTOM_FILEGAMMA);
						bmi.SetCustomFlag(BMM_CUSTOM_GAMMA);
						bmi.SetCustomGamma(gamma);
						pBmpTex->SetBitmapInfo(bmi);
					}

				}
			}
#else
			pbBitmap->bi.ResetCustomFlag(BMM_CUSTOM_FILEGAMMA);
			pbBitmap->bi.SetCustomFlag(BMM_CUSTOM_GAMMA);
			pbBitmap->bi.SetCustomGamma(gamma);
#endif
		}
		else {
			pbBitmap->bi.ResetCustomFlag(BMM_CUSTOM_GAMMA);
		}
		// now reload the bitmap from the disk.
		pBmpTex->ReloadBitmapAndUpdate();
	}
}

//=============================================================================
//=============================================================================
StdUVGen* GetUVGen(Texmap* pTex)
{
	if (!pTex) return NULL;

	StdUVGen* pUVGen = NULL;
	if (pTex->ClassID() == bmptexClassID) {
		pUVGen = ((BitmapTex*)pTex)->GetUVGen();
	}
	else if (pTex->ClassID() == VRayBitmapID) {
		for (int i = 0; i < pTex->NumSubs(); i++) {
			Animatable* pAnim = pTex->SubAnim(i);
			if (pAnim->ClassID() == Class_ID(0x100, 0)) return (StdUVGen*)pAnim;
		}
	}
	else if (pTex->ClassID() == CoronaBitmapID) {
		for (int i = 0; i < pTex->NumSubs(); i++) {
			Animatable* pAnim = pTex->SubAnim(i);
			if (pAnim->ClassID() == Class_ID(0x100, 0)) return (StdUVGen*)pAnim;
		}
	}
	return pUVGen;
}

//=============================================================================
//=============================================================================
Texmap* CreateAlphaFilterMap(Texmap *pTarget, AColor col)
{
#define	clrColor	0
#define	clrCrctMap	1
#define	rewireMode	2
#define	rewireR		3
#define	rewireG		4
#define	rewireB		5
#define	rewireA		6

	Texmap *pTex = (Texmap*)GetCOREInterface()->CreateInstance(TEXMAP_CLASS_ID, ColorCorrectTexID);
	IParamBlock2* pBlock = pTex->GetParamBlock(0);
	pBlock->SetValue(clrColor, 0, col);
	if(pTarget) pBlock->SetValue(clrCrctMap, 0, pTarget);
	pBlock->SetValue(rewireMode, 0, 3);
	pBlock->SetValue(rewireR, 0, 3);
	pBlock->SetValue(rewireG, 0, 3);
	pBlock->SetValue(rewireB, 0, 3);

	return pTex;
}
//=============================================================================
//=============================================================================
void SetTextureOutputScale(Texmap* pTex, float scale)
{
	if (!pTex) return;

	TextureOutput* texout = NULL;
	if (pTex->ClassID() == VRayBitmapID) {
		texout = (TextureOutput*)pTex->SubAnim(4);
	}
	else if (pTex->ClassID() == bmptexClassID) {
		texout = ((BitmapTex*)pTex)->GetTexout();
	}

	if (texout) {
		texout->SetOutputLevel(0, scale);
	}
}