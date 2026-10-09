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
#include <gamma.h>

#define MaterialSwitcherClassID		Class_ID(0x4ecd74a6, 0x0)
#define glTFMtlSwitcherClassID		Class_ID(0x9587a1a, 0x42bc9eb6)
#define PysicMtlSwitcherClassID		Class_ID(0x6af0dc4, 0x624849a5)
#define StdMtlSwitcherClassID		Class_ID(0x2fb3468d, 0x25fd57a2)
#define PBRMtlSwitcherClassID		Class_ID(0x2f4e61ce, 0x7a754a41)
#define USDMtlSwitcherClassID		Class_ID(0x506a52fd, 0x75430b22)

#define	MultiOutput_sourceMap			0


extern BOOL UVGenAnimated(StdUVGen* pUVGen);
extern BitmapTex* MergeRGBChannelTexture(BitmapTex* pTexR, BitmapTex* pTexG, BitmapTex* pTexB);
extern BitmapTex* MergeRGBChannelTexture(Texmap* pTexR, Texmap* pTexG, Texmap* pTexB, const tstring& Basename, const tstring &fileType);

//======================================================================
//======================================================================
StdUVGen* GetUVGen(Texmap* pTex)
{
	Texmap* pTex2 = GetBitmapTextureRec(pTex);
	if (!pTex2) return NULL;

	StdUVGen* pUVGen = NULL;
	if (pTex2->ClassID() == bmptexClassID) {
		pUVGen = ((BitmapTex*)pTex2)->GetUVGen();
	}
	else if (pTex2->ClassID() == VRayBitmapID) {
		for (int i = 0; i < pTex2->NumSubs(); i++) {
			Animatable* pAnim = pTex2->SubAnim(i);
			if (pAnim->ClassID() == Class_ID(0x100, 0)) return (StdUVGen*)pAnim;
		}
	}
	else if (pTex2->ClassID() == CoronaBitmapID) {
		for (int i = 0; i < pTex2->NumSubs(); i++) {
			Animatable* pAnim = pTex2->SubAnim(i);
			if (pAnim->ClassID() == Class_ID(0x100, 0)) return (StdUVGen*)pAnim;
		}
	}
	return pUVGen;
}


//===================================================
//===================================================
int GetOSLMapType(Texmap *pTex)
{
	if(!pTex)return OSL_UnSupport;
	if(pTex->ClassID() != OSLTex_CLASS_ID) return OSL_UnSupport;

	TSTR name = pTex->GetParamBlock(0)->GetStr(6, 0);
	if (name == _T("OSLBitmap2"))		return OSL_BitmapLookUp;
	if (name == _T("UberBitmap2"))		return OSL_UberBitmap;
	if (name == _T("UberBitmap2b"))		return OSL_UberBitmap;
	if (name == _T("HSFloatCutOff"))	return OSL_CutOff;
	if (name == _T("ColorMul"))			return OSL_ColorMultiply;

	return OSL_UnSupport;
}

//======================================================================
//======================================================================
int GetMapCh(Texmap* pTex)
{
	if (!pTex) return 0;

	StdUVGen * pUVGen = GetUVGen(pTex);
	//UVGen* pUVGen = pTex->GetTheUVGen();
	if (pUVGen) return pUVGen->GetMapChannel() - 1;

	return 0;
}

//======================================================================
//======================================================================
Texmap *GetBitmapTextureRec(Texmap* pTex)
{
	if (!pTex) return NULL;

	if (pTex->ClassID() == bmptexClassID) return pTex;
	if (pTex->ClassID() == VRayBitmapID) return pTex;

	if (pTex->ClassID() == ColorCorrectTexID) {
		return GetBitmapTextureRec(pTex->GetParamBlock(0)->GetTexmap(1));
	}
	if (pTex->ClassID() == RGBMultiTexID) {
		Texmap *p = pTex->GetParamBlock(0)->GetTexmap(2);
		if (p) return p;
		return pTex->GetParamBlock(0)->GetTexmap(3);
	}
	if (pTex->ClassID() == MixTexID) {
		return GetBitmapTextureRec(pTex->GetParamBlock(0)->GetTexmap(6));
	}

	if (pTex->ClassID() == CompositeTexClassID) {
		return GetBitmapTextureRec(pTex->GetParamBlock(0)->GetTexmap(9));
	}

	if (pTex->ClassID() == NormalBumpMapClassID) {
		return GetBitmapTextureRec(pTex->GetParamBlock(0)->GetTexmap(3));
	}

	if (pTex->ClassID() == VRayNormalMapID) {
		return GetBitmapTextureRec(pTex->GetParamBlock(0)->GetTexmap(0));
	}

	if (GetOSLMapType(pTex) == OSL_ColorMultiply) {
		return GetBitmapTextureRec(pTex->GetParamBlock(1)->GetTexmap(4));
	}

	return NULL;
}

//======================================================================
//======================================================================
BitmapTex* CreateBitmapTex(const tstring& texFilePath, Texmap* pTex, const IPoint2 &size)
{
	Texmap *pNewTex = NULL;
	if (pTex) {
		if(pTex->GetTheUVGen()){
			pNewTex = (Texmap*)pTex->Clone(DefaultRemapDir());
			if (pNewTex->GetTheUVGen()->IsStdUVGen()) {
				StdUVGen* pUVGen = (StdUVGen*)pNewTex->GetTheUVGen();
				pUVGen->SetUOffs(0.0f, 0);
				pUVGen->SetVOffs(0.0f, 0);
				pUVGen->SetUScl(1.0f, 0);
				pUVGen->SetVScl(1.0f, 0);
				pUVGen->SetWAng(0.0f, 0);
			}
		}
	}
	else {
		BitmapInfo bi;
		bi.SetHeight(size.y);
		bi.SetWidth(size.x);
		bi.SetName(texFilePath.c_str());
		bi.SetType(BMM_TRUE_64);
		bi.SetFlags(0);
		Bitmap* p = TheManager->Create(&bi);
		pNewTex = NewDefaultBitmapTex();
		((BitmapTex*)pNewTex)->SetBitmap(p);
		//p->DeleteThis();
	}

	BitmapInfo bi;
	bi.SetHeight(size.y);
	bi.SetWidth(size.x);
	bi.SetName(texFilePath.c_str());
	bi.SetType(BMM_TRUE_64);
	bi.SetFlags(0);
	Bitmap* p = TheManager->Create(&bi);
	if (pNewTex) {
		pNewTex->RenderBitmap(0, p, 100.0f, TRUE);
		pNewTex->DeleteThis();
	}
	else {
		pTex->RenderBitmap(0, p, 100.0f, TRUE);
	}
	BitmapTex* pBmpTex = NewDefaultBitmapTex();
	pBmpTex->SetMapName(texFilePath.c_str());
	p->OpenOutput(&bi);
	p->Write(&bi);
	p->Close(&bi);
	p->DeleteThis();


	return pBmpTex;
}
//===================================================
// Procedure class for enumerating dependency relationships
//===================================================
class HSMMDepEnumProc : public DependentEnumProc {
	ReferenceMaker *m_pRef;
	BOOL m_selected;
public:
	HSMMDepEnumProc(ReferenceMaker *pRef, BOOL selected) : m_pRef(pRef), m_selected(selected) { refTab.ZeroCount(); }

	virtual int proc(ReferenceMaker *rmaker) {
		if (rmaker == m_pRef) return 0;
		if (rmaker->SuperClassID() == BASENODE_CLASS_ID) {
			if (((INode*)rmaker)->Selected() == 0 && m_selected) return 0;
		}
		if (rmaker->SuperClassID() == BASENODE_CLASS_ID || rmaker->SuperClassID() == MATERIAL_CLASS_ID) {
			refTab.Append(1, &rmaker);
		}
		return 0;
	}

	Tab<ReferenceMaker*> refTab;
};

//======================================================================
//======================================================================
UINT glTFExporter_Core::CreateImage(const TCHAR *uri)
{
	std::string s = WStringToString(uri);
	const char* ext = MimeTypes::getType(strrchr(s.c_str(), '.') + 1);
	if (!ext) return -1;
	std::string mime(ext);
	if (mime != "image/jpeg" &&
		mime != "image/png" &&
		mime != "image/webp"&&
		mime != "image/ktx2"
		) return -1;

	auto it = std::find(m_imagePathTable.begin(), m_imagePathTable.end(), uri);
	if (it!= m_imagePathTable.end() ){
		return (UINT)std::distance(m_imagePathTable.begin(), it);
	}
	tinygltf::Image image;// = new tinygltf::Image;
	tstring buf;
	// For glb + bin format, the image file URI should be a relative path (filename only)
	if (m_ExportFileType == 1) {
		std::filesystem::path path(uri);
		buf = path.filename();
	}
	else if (m_ExportFileType == 2) {
		std::string s = WStringToString(uri);
		buf = GetURILFromFile(s);
		const char *ext = strrchr(s.c_str(), '.') + 1;
		tstring mime = StringToWString(MimeTypes::getType(ext));
		buf = _T("data:") + mime + _T(";base64,") + buf;
	}
	else {
		buf = uri;
		std::string s = WStringToString(buf);
		const char *ext = strrchr(s.c_str(), '.') + 1;
		std::string mime = MimeTypes::getType(ext);
		m_mimeTable.insert(make_pair(&image, mime));
	}
	m_imagePathTable.push_back(uri);
	//DWORD ret;
	//UrlCreateFromPath(uri, buf, &ret, NULL);
	image.uri = WStringToString(buf);
	m_model.images.push_back(image);

	return (int)(m_imagePathTable.size() - 1);
}

//======================================================================
//======================================================================
int glTFExporter_Core::findTextureIndex(Texmap* pTex, const TSTR& fname, BOOL KTX2isRGB, Texmap* pOSLMap)
{
	if (!pTex) {
		if (!PathFileExists(fname)) return -1;

		for (auto p : m_TextureTable) {
			if(p->ClassID() != bmptexClassID )
				break;

			// TODO: below code crashes on non-BitmapTex 
			BitmapTex* bmpTex = static_cast<BitmapTex*>(p);
			if(bmpTex) {
				TSTR n = bmpTex->GetMapName();
				if(n == fname) {
					pTex = p;
					break;
				}
			}
		}

		if (!pTex) {
			pTex = NewDefaultBitmapTex();
			static_cast<BitmapTex*>(pTex)->SetMapName(fname);
		}
	}
	auto it = std::find(m_TextureTable.begin(), m_TextureTable.end(), pTex);
	if (it != m_TextureTable.end()) {
		return (UINT)(std::distance(m_TextureTable.begin(), it));
	}
	else {
		tinygltf::Texture texture;
		tstring mapName(_T(""));

		if (pTex->ClassID() == bmptexClassID) {
			mapName = (static_cast<BitmapTex*>(pTex))->GetMapName();
		}
		else if (pTex->ClassID() == VRayBitmapID) {
			//pTex->GetParamBlock(0)->GetValueByName(_T("HDRIMapName"), m_time, *ptr, FOREVER);
			const TCHAR* ptr = pTex->GetParamBlock(0)->GetStr(0, m_time, FOREVER);
			mapName = ptr;
		}
		else if (pTex->ClassID() == CoronaBitmapID) {
			const TCHAR* ptr = pTex->GetParamBlock(0)->GetStr(101, m_time, FOREVER);
			mapName = ptr;
		}

		{
			WebpTextureStruct str;
			if(SetWebpTextureParams(pTex, str)){
				if (PathFileExists(str.originalPathStr.c_str()))
					mapName = str.originalPathStr;
				else {
					if(WebpEncode(pTex, str))
						mapName = str.originalPathStr;
				}
				m_TextureWebp_Used = TRUE;
			}
		}

		{
			KTX2TextureStruct str;
			str.compression = 4;
			str.quality = 128;
			str.useUASTC = FALSE;
			str.isSRGB = KTX2isRGB;
			if (SetKTX2TextureParams(pTex, str)) {
				if (PathFileExists(str.originalPathStr.c_str()))
					mapName = str.originalPathStr;
				else {
					if (KTX2Encode(pTex, str)) {
						mapName = str.originalPathStr;
					}
				}
				m_TexBasisu_Used = TRUE;
			}
		}

		if (mapName.size() > 1) {
			UINT idx = CreateImage(mapName.c_str());
			if (idx == -1) return -1;

			if(pOSLMap)
				texture.sampler = SetSampler(pOSLMap);
			else
				texture.sampler = SetSampler(pTex);

			if (mapName.find(_T(".webp")) != std::string::npos) {
				texture.source = idx;
				SetName(&texture, mapName.c_str());

				tinygltf::Value::Object obj;
				obj.insert(std::make_pair("source", tinygltf::Value((int)idx)));
				tinygltf::Value val(obj);
				texture.extensions.insert(std::make_pair("EXT_texture_webp", val));
			}
			else if (mapName.find(_T(".ktx2")) != std::string::npos) {
				texture.source = idx;
				SetName(&texture, mapName.c_str());

				tinygltf::Value::Object obj;
				obj.insert(std::make_pair("source", tinygltf::Value((int)idx)));
				tinygltf::Value val(obj);
				texture.extensions.insert(std::make_pair("KHR_texture_basisu", val));
			}
			else {
				texture.source = idx;
				SetName(&texture, mapName.c_str());
			}
			m_TextureTable.push_back(pTex);
			m_model.textures.push_back(texture);
			return (int)(m_model.textures.size() - 1);
		}
	}
	return -1;
}


//======================================================================
//======================================================================
int glTFExporter_Core::SetSampler(Texmap* pTex)
{
	UINT Tiling = 0;

	tinygltf::Sampler sampler;
	sampler.wrapS = TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
	sampler.wrapT = TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;

	if (pTex->ClassID()== OSLTex_CLASS_ID) {
		if (GetOSLMapType(pTex) != OSL_UberBitmap) return -1;
		IParamBlock2* pBlock1 = pTex->GetParamBlock(1);
		TSTR WrapMode = pBlock1->GetStr(UberBmp_WrapMode, m_time);
		if (WrapMode == _T("black")) {
			sampler.wrapS = sampler.wrapT = 0;
		}
		else if (WrapMode == _T("clamp")) {
			sampler.wrapS = sampler.wrapT = TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
		}
		else if (WrapMode == _T("periodic")) {
			sampler.wrapS = sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
		}
		else if (WrapMode == _T("mirror")) {
			sampler.wrapS = sampler.wrapT = TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT;
		}
		else{
			sampler.wrapS = sampler.wrapT = 0;
		}

	}
	else {
		StdUVGen* pUVGen = GetUVGen(pTex);
		if (pUVGen) {
			Tiling = pUVGen->GetTextureTiling();
		}

		if (Tiling) {
			if (Tiling & U_WRAP)
				sampler.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
			if (Tiling & V_WRAP)
				sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
		}
	}

	m_model.samplers.push_back(sampler);
	return (int)(m_model.samplers.size() - 1);
}

#if 0
//======================================================================
// *********************************************************************
//======================================================================
BOOL MyTest(Texmap *pTex)
{
	if (!pTex) return FALSE;

	Point3 offset(0.0,0.0,0.0);
	Point3 scale(0.0, 0.0, 1.0);
	Point3 rotationEuler(0.0,0.0,0.0);

	StdUVGen* pUVGen = GetUVGen(pTex);
	if (pUVGen) {
		offset.x = pUVGen->GetUOffs(m_time);
		offset.y = pUVGen->GetVOffs(m_time);
		scale.x = pUVGen->GetUScl(m_time);
		scale.y = pUVGen->GetVScl(m_time);
		rotationEuler.x = -pUVGen->GetUAng(m_time);
		rotationEuler.y = -pUVGen->GetVAng(m_time);
		rotationEuler.z = -pUVGen->GetWAng(m_time);
	}

	Point3 center(0.5, 0.5, 0.0); // max ref is center of the texture
	Point3 pivot = center + offset;

	{
		// lets define we have to set the offset manually without the help of rotation center
		Point3 origin(0.0, 1.0, 0.0); // upper left corner
		// inverse transforms to change reference 
		Matrix3 translateToOrigin;
		translateToOrigin.SetTrans(-pivot);
		Matrix3 rotate;
		rotate.RotateZ(-rotationEuler.z);
		Matrix3 scaling;
		scaling.Scale(scale);
		// because we want to keep the offset, so bring back to the center
		Matrix3 translateBack;
		translateBack.SetTrans(center);
		Matrix3 t = translateToOrigin * scaling * rotate * translateBack;
		offset = origin * t;
	}

	float uOffset = std::fmod(offset.x, 1);
	float vOffset = std::fmod((1.0 - offset.y), 1);
	float uScale = scale.x;
	float vScale = scale.y;
	float wAng = rotationEuler.z;

	//if (Path.GetExtension(babylonTexture.name).ToLower() == ".dds")
	//{
	//	babylonTexture.vScale *= -1; // Need to invert Y-axis for DDS texture
	//}


	babylonTexture.wrapU = BabylonTexture.AddressMode.CLAMP_ADDRESSMODE; // CLAMP
	if ((uvGen.TextureTiling & 1) != 0) // WRAP
	{
		babylonTexture.wrapU = BabylonTexture.AddressMode.WRAP_ADDRESSMODE;
	}
	else if ((uvGen.TextureTiling & 4) != 0) // MIRROR
	{
		babylonTexture.wrapU = BabylonTexture.AddressMode.MIRROR_ADDRESSMODE;
	}

	babylonTexture.wrapV = BabylonTexture.AddressMode.CLAMP_ADDRESSMODE; // CLAMP
	if ((uvGen.TextureTiling & 2) != 0) // WRAP
	{
		babylonTexture.wrapV = BabylonTexture.AddressMode.WRAP_ADDRESSMODE;
	}
	else if ((uvGen.TextureTiling & 8) != 0) // MIRROR
	{
		babylonTexture.wrapV = BabylonTexture.AddressMode.MIRROR_ADDRESSMODE;
	}

	return uvGen;
}
#endif
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateTextureTransformBlock(tinygltf::ExtensionMap& extension, Texmap* pTex, BOOL extent)
{
	if (!pTex) return FALSE;

	float offsetU = 0.0f;
	float offsetV = 0.0f;
	float sclU = 1.0f;
	float sclV = 1.0f;
	float rot = 0.0f;




	UINT force = IsUVAnimated(pTex);
#if TRUE
	StdUVGen* pUVGen = GetUVGen(pTex);
	if(pUVGen){
		offsetU = pUVGen->GetUOffs(m_time);
		offsetV = pUVGen->GetVOffs(m_time);
		sclU = pUVGen->GetUScl(m_time);
		sclV = pUVGen->GetVScl(m_time);
		rot = pUVGen->GetWAng(m_time);
	}

	float localoffsetU = -0.5f * cos(rot) + 0.5f * sin(rot) + 0.5f;
	float localoffsetV = -0.5f * sin(rot) - 0.5f * cos(rot) + 0.5f;

	if (rot == 0.0f && offsetU==0.0f && sclU > 1.0f) {
		localoffsetU = (sclU - 1.0f) / 2.0f;
		offsetU += localoffsetU;
	}
	else {
		if (sclU >= 1.0f) {
			localoffsetU += (1.0f - (1.0f / sclU)) / 2.0f;
			offsetU += localoffsetU;
		}
		else {
			localoffsetU += (1.0f - sclU) / 2.0f;
			offsetU = localoffsetU - offsetU * sclU;
			offsetU *= -1.0f;
		}
	}

	if (rot == 0.0f && offsetV == 0.0f && sclV > 1.0f) {
		localoffsetV = (sclV - 1.0f) / 2.0f;
		offsetV += localoffsetV;
	}
	else {
		if (sclV >= 1.0f) {
			localoffsetV += (1.0f - (1.0f / sclV)) / 2.0f;
			offsetV -= localoffsetV;
		}
		else {
			localoffsetV += (1.0f - sclV) / 2.0f;
			offsetV = localoffsetV + offsetV * sclV;
		}
	}
#else
	Point3 offset(0.0, 0.0, 0.0);
	Point3 scale(0.0, 0.0, 1.0);
	Point3 rotationEuler(0.0, 0.0, 0.0);

	StdUVGen* pUVGen = GetUVGen(pTex);
	if (pUVGen) {
		offset.x = pUVGen->GetUOffs(m_time);
		offset.y = pUVGen->GetVOffs(m_time);
		scale.x = pUVGen->GetUScl(m_time);
		scale.y = pUVGen->GetVScl(m_time);
		rotationEuler.x = -pUVGen->GetUAng(m_time);
		rotationEuler.y = -pUVGen->GetVAng(m_time);
		rotationEuler.z = -pUVGen->GetWAng(m_time);
	}

	Point3 center(0.5, 0.5, 0.0); // max ref is center of the texture
	Point3 pivot = center + offset;

	if (TRUE)
	{
		// fast and optimized track for Babylon format. using Matrix transform
		// https://github.com/BabylonJS/Babylon.js/blob/master/src/Materials/Textures/texture.ts#L561-L640
		//babylonTexture.uRotationCenter = pivot.X;
		//babylonTexture.vRotationCenter = pivot.Y;
		if (fabs(scale.x) != 1.0 || fabs(scale.y) != 1.0)
		{
			Matrix3 translate(1);
			translate.Translate(-center);
			Matrix3 scaling(1);
			scaling.Scale(scale);
			Matrix3 t = translate * scaling;
			offset = offset * t;
		}
		//note: Max is defining offset displacement when Babylon is defining origin of the texture
		//positiv offset in Max mean Negative offset in Babylon(Inverse transform)
		offset = -offset;
		scale.y = -scale.y;
	}
	else {
		// lets define we have to set the offset manually without the help of rotation center
		Point3 origin(0.0, 1.0, 0.0); // upper left corner
		// inverse transforms to change reference 
		Matrix3 translateToOrigin(1);
		translateToOrigin.SetTrans(-pivot);
		Matrix3 rotate(1);
		rotate.RotateZ(-rotationEuler.z);
		Matrix3 scaling(1);
		scaling.Scale(scale);
		// because we want to keep the offset, so bring back to the center
		Matrix3 translateBack(1);
		translateBack.SetTrans(center);
		Matrix3 t = translateToOrigin * scaling * rotate * translateBack;
		offset = origin * t;
	}

	offsetU = std::fmod(offset.x, 1);
	offsetV = std::fmod((1.0 - offset.y), 1);
	sclU = scale.x;
	sclV = scale.y;
	rot = rotationEuler.z;

#endif

	float QuantScale = 1.0f;
	Point2 QuantOffset(0.0f, 0.0f);
	if (m_Mesh_quantization_Used) {
		QuantizationInfo info;
		GetQuatizationInfo(pTex, info);
		QuantScale = info.uvmap1Scale;
		QuantOffset = info.uvmap1Offset;
	}

#if 1
	// Quantization の影響を Texture Transform 側に合成する
	// UV_final = (UV_quant * QuantScale + QuantOffset) * scale + offset
	//          = UV_quant * (QuantScale * scale) + (QuantOffset * scale + offset)

	// 1. スケールの合成
	float finalScaleU = sclU * QuantScale;
	float finalScaleV = sclV * QuantScale;

	// 2. オフセットの合成 (※3ds Max と glTF の軸・符号の扱いに合わせて適用)
	float finalOffsetU = offsetU + (QuantOffset.x * sclU);
	float finalOffsetV = offsetV + (QuantOffset.y * sclV);

	// 回転は Quantization による影響を受けないためそのまま保持
	float finalRot = rot;


	// ------------------------------------------------------------------
	// glTF JSON (KHR_texture_transform) の構築
	// ------------------------------------------------------------------
	tinygltf::Value::Object obj;

	// Quantization が適用されている場合、スケールやオフセットが変化しているため
	// 値が 0 や 1 であっても強制的に出力条件に含めるか判定します
	BOOL hasOffset = (finalOffsetU != 0.0f || finalOffsetV != 0.0f || (force & UV_ANIMATE_OFFSET) || (QuantOffset.x != 0.0f || QuantOffset.y != 0.0f));
	BOOL hasScale = (finalScaleU != 1.0f || finalScaleV != 1.0f || (force & UV_ANIMATE_SCALE) || (QuantScale != 1.0f));
	BOOL hasRot = (finalRot != 0.0f || (force & UV_ANIMATE_ROTATE));

	if (hasOffset) {
		tinygltf::Value::Array offsetArr;
		offsetArr.push_back(tinygltf::Value(truncateDecimal(-finalOffsetU)));
		offsetArr.push_back(tinygltf::Value(truncateDecimal(finalOffsetV)));
		obj.insert(std::make_pair("offset", tinygltf::Value(offsetArr)));
	}

	if (hasScale) {
		tinygltf::Value::Array sclArr;
		sclArr.push_back(tinygltf::Value(finalScaleU));
		sclArr.push_back(tinygltf::Value(finalScaleV));
		obj.insert(std::make_pair("scale", tinygltf::Value(sclArr)));
	}

	if (hasRot) {
		obj.insert(std::make_pair("rotation", tinygltf::Value(finalRot)));
	}

	if (obj.size() > 0) {
		tinygltf::Value val(obj);
		extension.insert(std::make_pair("KHR_texture_transform", val));
		m_TexTransform_Used = TRUE;
	}
	else if (UVGenAnimated(pUVGen))
	{
		tinygltf::Value val(obj);
		extension.insert(std::make_pair("KHR_texture_transform", val));
		m_TexTransform_Used = TRUE;
	}
#else
	tinygltf::Value::Object obj;
	if (offsetU != 0.0f || offsetV != 0.0f || (force & UV_ANIMATE_OFFSET)) {
		tinygltf::Value::Array offset;
		offset.push_back(tinygltf::Value(truncateDecimal(-offsetU)));
		offset.push_back(tinygltf::Value(truncateDecimal(offsetV)));
		obj.insert(std::make_pair("offset", tinygltf::Value(offset)));
	}
	if (sclU != 1.0f || sclV != 1.0f || (force & UV_ANIMATE_SCALE)) {
		tinygltf::Value::Array scl;
		scl.push_back(tinygltf::Value(truncateDecimal(sclU)));
		scl.push_back(tinygltf::Value(truncateDecimal(sclV)));
		obj.insert(std::make_pair("scale", tinygltf::Value(scl)));
	}
	if (rot != 0.0f || (force & UV_ANIMATE_ROTATE)) {
		obj.insert(std::make_pair("rotation", tinygltf::Value(rot)));
	}

	if(obj.size()>0) {
		tinygltf::Value val(obj);
		extension.insert(std::make_pair("KHR_texture_transform", val));
		m_TexTransform_Used = TRUE;
	}
	else if (UVGenAnimated(pUVGen))
	{
		tinygltf::Value val(obj);
		extension.insert(std::make_pair("KHR_texture_transform", val));
		m_TexTransform_Used = TRUE;
	}
#endif
	return (obj.size() > 0);
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateTextureTransformBlockEx(tinygltf::Value::Object& object, Texmap* pTex, BOOL extent)
{
	float offsetU = 0.0f;
	float offsetV = 0.0f;
	float sclU = 1.0f;
	float sclV = 1.0f;
	float rot = 0.0f;

	UINT force = IsUVAnimated(pTex);

	StdUVGen* pUVGen = GetUVGen(pTex);
	if (pUVGen) {
		offsetU = pUVGen->GetUOffs(m_time);
		offsetV = pUVGen->GetVOffs(m_time);
		sclU = pUVGen->GetUScl(m_time);
		sclV = pUVGen->GetVScl(m_time);
		rot = pUVGen->GetWAng(m_time);
	}

	tinygltf::Value::Object obj;
	if (offsetU != 0.0f || offsetV != 0.0f || (force & UV_ANIMATE_OFFSET)) {
		tinygltf::Value::Array offset;
		offset.push_back(tinygltf::Value(-offsetU));
		offset.push_back(tinygltf::Value(offsetV));
		obj.insert(std::make_pair("offset", tinygltf::Value(offset)));
	}
	if (sclU != 1.0f || sclV != 1.0f || (force & UV_ANIMATE_SCALE)) {
		tinygltf::Value::Array scl;
		scl.push_back(tinygltf::Value(sclU));
		scl.push_back(tinygltf::Value(sclV));
		obj.insert(std::make_pair("scale", tinygltf::Value(scl)));
	}
	if (rot != 0.0f || (force & UV_ANIMATE_ROTATE)) {
		obj.insert(std::make_pair("rotation", tinygltf::Value(rot)));
	}

	if (obj.size() > 0) {
		tinygltf::Value::Object extension;
		extension.insert(std::make_pair("KHR_texture_transform", tinygltf::Value(obj)));

		tinygltf::Value val(extension);
		if (extent)
			object.insert(std::make_pair("extensions", val));
		else
			object = extension;

		m_TexTransform_Used = TRUE;
	}

	return (obj.size() > 0);
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateTextureTransformBlockWithOSL(tinygltf::Value::Object& object, Texmap* pTex, BOOL extent)
{
	if (GetOSLMapType(pTex) != OSL_UberBitmap) return FALSE;
	IParamBlock2 *pBlock1 = pTex->GetParamBlock(1);

	Point3 offset(0.0f, 0.0f, 0.0f);
	Point3 scl(1.0f, 1.0f, 0.0f);
	//float scale = 1.0f;
	float rot = 0.0f;

	pBlock1->GetValue(UberBmp_Offset, m_time, offset, FOREVER);
	//pBlock1->GetValue(UberBmp_Scale, m_time, scale, FOREVER);
	pBlock1->GetValue(UberBmp_Rotate, m_time, rot, FOREVER);
	pBlock1->GetValue(UberBmp_Tiling, m_time, scl, FOREVER);

	float localoffsetU = -0.5f * cos(rot) + 0.5f * sin(rot) + 0.5f;
	float localoffsetV = -0.5f * sin(rot) - 0.5f * cos(rot) + 0.5f;
	localoffsetU += (1.0f - (1.0f / scl.x)) / 2.0f;
	localoffsetV += (1.0f - (1.0f / scl.y)) / 2.0f;

	offset.x += localoffsetU;
	offset.y += localoffsetV;
	offset.y = -1.0f - offset.y;

	tinygltf::Value::Object obj;
	if (offset.x != 0.0f || offset.y != 0.0f) {
		tinygltf::Value::Array of;
		of.push_back(tinygltf::Value(offset.x));
		of.push_back(tinygltf::Value(offset.y));
		obj.insert(std::make_pair("offset", tinygltf::Value(of)));
	}
	if (scl.x != 0.0f || scl.y != 0.0f) {
		tinygltf::Value::Array sc;
		sc.push_back(tinygltf::Value(scl.x));
		sc.push_back(tinygltf::Value(scl.y));
		obj.insert(std::make_pair("scale", tinygltf::Value(sc)));
	}
/*
	if (scale != 1.0f) {
		tinygltf::Value::Array scl;
		scl.push_back(tinygltf::Value(scale * 100.0f));
		scl.push_back(tinygltf::Value(scale * 100.0f));
		obj.insert(std::make_pair("scale", tinygltf::Value(scl)));
	}
*/
	if (rot != 0.0f) {
		obj.insert(std::make_pair("rotation", tinygltf::Value(rot)));
	}

	if (obj.size() > 0) {
		tinygltf::Value::Object extension;
		extension.insert(std::make_pair("KHR_texture_transform", tinygltf::Value(obj)));

		tinygltf::Value val(extension);
		if(extent)
			object.insert(std::make_pair("extensions", val));
		else
			object = extension;
		m_TexTransform_Used = TRUE;
	}

	return m_TexTransform_Used;
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateBaseColorTexture(tinygltf::Material &material, Texmap *pTex)
{
	if (!pTex) return FALSE;

	if (pTex->ClassID() == bmptexClassID) {
		material.pbrMetallicRoughness.baseColorTexture.index = findTextureIndex(pTex, _T(""), TRUE);
		int mapCh = pTex->GetTheUVGen()->GetMapChannel();
		material.pbrMetallicRoughness.baseColorTexture.texCoord = mapCh - 1;
		//material.pbrMetallicRoughness.baseColorTexture.texcoord = 0;
	}
	else if (pTex->ClassID() == VRayBitmapID) {
		material.pbrMetallicRoughness.baseColorTexture.index = findTextureIndex(pTex, _T(""), TRUE);
		int mapCh = GetUVGen(pTex)->GetMapChannel();
		material.pbrMetallicRoughness.baseColorTexture.texCoord = mapCh - 1;
	}
	else if (pTex->ClassID() == MULTIOUTPUTTOTEXMAP_CLASS_ID) {
		Texmap* pOSLMap;
		pTex->GetParamBlock(0)->GetValue(MultiOutput_sourceMap, m_time, pOSLMap, FOREVER);
		int type = GetOSLMapType(pOSLMap);
		if (type == OSL_BitmapLookUp) {
			TSTR name = pOSLMap->GetParamBlock(1)->GetStr(2, m_time);
			material.pbrMetallicRoughness.baseColorTexture.index = findTextureIndex(NULL, name, TRUE);
			int mapCh = pOSLMap->GetParamBlock(1)->GetInt(UberBmp_UVSet, m_time);
			material.pbrMetallicRoughness.baseColorTexture.texCoord = mapCh - 1;
		}
		else if (type == OSL_UberBitmap) {
			TSTR name = pOSLMap->GetParamBlock(1)->GetStr(10, m_time);
			material.pbrMetallicRoughness.baseColorTexture.index = findTextureIndex(NULL, name, TRUE, pOSLMap);
			tinygltf::Value::Object texIdx;
			CreateTextureTransformBlockWithOSL(texIdx, pOSLMap, FALSE);
			material.pbrMetallicRoughness.baseColorTexture.extensions = texIdx;
			int mapCh = pOSLMap->GetParamBlock(1)->GetInt(UberBmp_UVSet, m_time);
			material.pbrMetallicRoughness.baseColorTexture.texCoord = mapCh - 1;
		}
		else {
			std::filesystem::path texFilePath;
			texFilePath = ExportFolder() + tstring(_T("\\")) + tstring(_T("BaseColorMap")) + TextureTableCountStr() + tstring(_T(".jpg"));
			IPoint2 imageSize = GetBitmapSize();
			if (imageSize == IPoint2(0, 0))imageSize = IPoint2(1024,1024);
			CreateBitmapTex(texFilePath, pTex, imageSize);
			material.pbrMetallicRoughness.baseColorTexture.index = findTextureIndex(NULL, texFilePath.c_str(),TRUE);
		}
	}
	else {
		std::filesystem::path texFilePath;
		texFilePath = ExportFolder() + tstring(_T("\\")) + tstring(_T("BaseColorMap")) + TextureTableCountStr() + tstring(_T(".jpg"));
		IPoint2 imageSize = GetBitmapSize();
		if (imageSize == IPoint2(0, 0))imageSize = IPoint2(1024, 1024);
		CreateBitmapTex(texFilePath, pTex, imageSize);
		material.pbrMetallicRoughness.baseColorTexture.index = findTextureIndex(NULL, texFilePath.c_str(), TRUE);
	}

	CreateTextureTransformBlock(material.pbrMetallicRoughness.baseColorTexture.extensions, pTex);

	return TRUE;
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateNormalTexture(tinygltf::Material &material, Texmap *pTex, float scale)
{
	material.normalTexture.index = findTextureIndex(pTex, _T(""),FALSE);
	if (pTex->ClassID() == bmptexClassID) {
		int mapCh = pTex->GetTheUVGen()->GetMapChannel();
		material.normalTexture.texCoord = mapCh - 1;
	}
	else if (pTex->ClassID() == VRayBitmapID) {
		int mapCh = GetUVGen(pTex)->GetMapChannel();
		material.normalTexture.texCoord = mapCh - 1;
	}
	else {
		std::filesystem::path texFilePath;
		texFilePath = ExportFolder() + tstring(_T("\\")) + tstring(_T("NormalMap")) + TextureTableCountStr() + tstring(_T(".jpg"));
		IPoint2 imageSize = GetBitmapSize();
		if (imageSize == IPoint2(0, 0))imageSize = IPoint2(1024, 1024);
		CreateBitmapTex(texFilePath, pTex, imageSize);
		material.normalTexture.index = findTextureIndex(NULL, texFilePath.c_str(), FALSE);
		material.normalTexture.texCoord = 0;
	}
	CreateTextureTransformBlock(material.normalTexture.extensions, pTex);

	//material.normalTexture.texcoord = 0;
	material.normalTexture.scale = scale;

	return TRUE;
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateEmitTexture(tinygltf::Material &material, Texmap *pTex)
{
	material.emissiveTexture.index = findTextureIndex(pTex, _T(""), TRUE);
	if (pTex->ClassID() == bmptexClassID) {
		int mapCh = pTex->GetTheUVGen()->GetMapChannel();
		material.emissiveTexture.texCoord = mapCh - 1;
	}
	else if (pTex->ClassID() == VRayBitmapID) {
		int mapCh = GetUVGen(pTex)->GetMapChannel();
		material.emissiveTexture.texCoord = mapCh - 1;
	}

	CreateTextureTransformBlock(material.emissiveTexture.extensions, pTex);
	//material.emissiveTexture.texcoord = 0;

	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateEmitStrength(tinygltf::Material &material, float lum, BOOL animated)
{
	if (lum == 0.0f && !animated) return TRUE;

	tinygltf::Value::Object obj;

	obj.insert(std::make_pair("emissiveStrength", tinygltf::Value(truncateDecimal(lum))));

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_emissive_strength", val));

	m_MtlEmitStrength_Used = TRUE;

	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateTransmissionTexture(tinygltf::Material& material, const TransmissionStruct &str, BOOL animated)
{
	Texmap* pTex = GetBitmapTextureRec(str.pTex);

	tinygltf::Value::Object obj;
	if (pTex) {
		int index = findTextureIndex(pTex, _T(""),TRUE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if(!CreateTextureTransformBlockWithOSL(texIdx, pTex))
			CreateTextureTransformBlockEx(texIdx, pTex);
		obj.insert(std::make_pair("transmissionTexture", tinygltf::Value(texIdx)));
	}
	if(str.factor!=0.0f || animated) obj.insert(std::make_pair("transmissionFactor", tinygltf::Value(truncateDecimal(str.factor))));

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_transmission", val));

	m_MtlTransmission_Used = TRUE;

	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateSpecularTexture(tinygltf::Material& material, const SpecularStruct &str, BOOL animated)
{
	Texmap* pTex = GetBitmapTextureRec(str.pMap);

	tinygltf::Value::Object obj;
	if (pTex) {
		int index = findTextureIndex(pTex, _T(""), FALSE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pTex))
			CreateTextureTransformBlockEx(texIdx, pTex);
		obj.insert(std::make_pair("specularTexture", tinygltf::Value(texIdx)));
	}

	Texmap* pColTex = GetBitmapTextureRec(str.pColMap);
	if (pColTex) {
		int index = findTextureIndex(pColTex, _T(""), TRUE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pColTex))
			CreateTextureTransformBlockEx(texIdx, pColTex);
		obj.insert(std::make_pair("specularColorTexture", tinygltf::Value(texIdx)));
	}

	tinygltf::Value::Array col;
	col.push_back(tinygltf::Value(str.color.r));
	col.push_back(tinygltf::Value(str.color.g));
	col.push_back(tinygltf::Value(str.color.b));
	obj.insert(std::make_pair("specularColorFactor", tinygltf::Value(col)));
	obj.insert(std::make_pair("specularFactor", tinygltf::Value(truncateDecimal(str.factor))));

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_specular", val));

	m_MtlSpecular_Used = TRUE;
	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateVolumeTexture(tinygltf::Material& material, const VolumeStruct &str, BOOL animated)
{
	Texmap* pThicknessTex = GetBitmapTextureRec(str.pThicknessMap);

	tinygltf::Value::Object obj;
	if (pThicknessTex) {
		int index = findTextureIndex(pThicknessTex, _T(""),FALSE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pThicknessTex))
			CreateTextureTransformBlockEx(texIdx, pThicknessTex);
		obj.insert(std::make_pair("thicknessTexture", tinygltf::Value(texIdx)));
	}

	tinygltf::Value::Array col;
	col.push_back(tinygltf::Value(str.color.r));
	col.push_back(tinygltf::Value(str.color.g));
	col.push_back(tinygltf::Value(str.color.b));
	if (str.color != Color(1.0f, 1.0f, 1.0f) || animated) obj.insert(std::make_pair("attenuationColor", tinygltf::Value(col)));
	if (str.thickness != 0.0f || animated) obj.insert(std::make_pair("thicknessFactor", tinygltf::Value((str.thickness))));
	if (str.distance < KHR_FLT_MAX) {
		if (str.distance != 0.0f || animated) obj.insert(std::make_pair("attenuationDistance", tinygltf::Value(str.distance)));
	}
	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_volume", val));

	m_MtlVolume_Used = TRUE;
	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateSheenTexture(tinygltf::Material& material, const SheenStruct &str, BOOL animated)
{
	Texmap* pRoughnessTex = GetBitmapTextureRec(str.pRoughnessMap);

	tinygltf::Value::Object obj;
	if (pRoughnessTex) {
		int index = findTextureIndex(pRoughnessTex, _T(""),FALSE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pRoughnessTex))
			CreateTextureTransformBlockEx(texIdx, pRoughnessTex);
		obj.insert(std::make_pair("sheenRoughnessTexture", tinygltf::Value(texIdx)));
	}

	Texmap* pColTex = GetBitmapTextureRec(str.pColMap);
	if (pColTex) {
		int index = findTextureIndex(pColTex, _T(""),TRUE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pColTex))
			CreateTextureTransformBlockEx(texIdx, pColTex);
		obj.insert(std::make_pair("sheenColorTexture", tinygltf::Value(texIdx)));
	}

	tinygltf::Value::Array col;
	col.push_back(tinygltf::Value(str.color.r));
	col.push_back(tinygltf::Value(str.color.g));
	col.push_back(tinygltf::Value(str.color.b));
	if (str.color != Color(0.0f, 0.0f, 0.0f) || animated) obj.insert(std::make_pair("sheenColorFactor", tinygltf::Value(col)));
	if (str.roughness != 0.0f || animated) obj.insert(std::make_pair("sheenRoughnessFactor", tinygltf::Value(truncateDecimal(str.roughness))));

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_sheen", val));

	m_MtlSheen_Used = TRUE;
	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateClearCoatTexture(tinygltf::Material& material, const ClearCoatStruct &str, BOOL animated)
{
	//if (str.factor == 0.0f && str.roughness == 0.0f && !str.pMap && !str.pNormalMap && !str.pRoughnessMap) return;
	Texmap* pTex = GetBitmapTextureRec(str.pMap);

	tinygltf::Value::Object obj;
	if (pTex) {
		int index = findTextureIndex(pTex, _T(""), TRUE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pTex))
			CreateTextureTransformBlockEx(texIdx, pTex);
		obj.insert(std::make_pair("clearcoatTexture", tinygltf::Value(texIdx)));
	}

	Texmap* pRoughnessTex = GetBitmapTextureRec(str.pRoughnessMap);
	if (pRoughnessTex) {
		int index = findTextureIndex(pRoughnessTex, _T(""), FALSE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pRoughnessTex))
			CreateTextureTransformBlockEx(texIdx, pRoughnessTex);
		obj.insert(std::make_pair("clearcoatRoughnessTexture", tinygltf::Value(texIdx)));
	}

	Texmap* pNormalTex = GetBitmapTextureRec(str.pNormalMap);
	if (pNormalTex) {
		int index = findTextureIndex(pNormalTex, _T(""),FALSE);

		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pNormalTex))
			CreateTextureTransformBlockEx(texIdx, pNormalTex);
		obj.insert(std::make_pair("clearcoatNormalTexture", tinygltf::Value(texIdx)));
	}

	if (str.factor != 0.0f || animated) obj.insert(std::make_pair("clearcoatFactor", tinygltf::Value(truncateDecimal(str.factor))));
	if (str.roughness != 0.0f || animated) obj.insert(std::make_pair("clearcoatRoughnessFactor", tinygltf::Value(truncateDecimal(str.roughness))));
	//obj.insert(std::make_pair("clearcoatNormalFactor", tinygltf::Value(str.normalValue)));

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_clearcoat", val));

	m_MtlClearCoat_Used = TRUE;
	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateUnlitTexture(tinygltf::Material& material, const UnlitStruct& str, BOOL animated)
{
	if (!str.unlit) return FALSE;

	tinygltf::Value::Object obj;
	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_unlit", val));

	m_MtlUnlit_Used = TRUE;
	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateIORTexture(tinygltf::Material& material, const IORStruct& str, BOOL animated)
{
	if(!animated)
		if (fabs(str.ior - 1.5f) < 0.03f) return FALSE;

	tinygltf::Value::Object obj;
	if (str.ior != 1.5f || animated) obj.insert(std::make_pair("ior", tinygltf::Value(truncateDecimal(str.ior))));

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_ior", val));

	m_MtlIor_Used = TRUE;
	return TRUE;
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateIridescenceTexture(tinygltf::Material& material, const IridescenceStruct& str, BOOL animated)
{
	tinygltf::Value::Object obj;
	if(str.factor!=0.0f)
		obj.insert(std::make_pair("iridescenceFactor", tinygltf::Value(truncateDecimal(str.factor))));
	if (str.ior != 1.3f)
		obj.insert(std::make_pair("iridescenceIor", tinygltf::Value(truncateDecimal(str.ior))));
	if (str.minimum != 100.0f)
		obj.insert(std::make_pair("iridescenceThicknessMinimum", tinygltf::Value(truncateDecimal(str.minimum))));
	if (str.maximum != 400.0f)
		obj.insert(std::make_pair("iridescenceThicknessMaximum", tinygltf::Value(truncateDecimal(str.maximum))));

	Texmap* pTex = GetBitmapTextureRec(str.texture);
	if (pTex) {
		int index = findTextureIndex(pTex, _T(""), FALSE);
		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pTex))
			CreateTextureTransformBlockEx(texIdx, pTex);
		obj.insert(std::make_pair("iridescenceTexture", tinygltf::Value(texIdx)));
	}

	Texmap* pThicknessTex = GetBitmapTextureRec(str.thicknessTexture);
	if (pThicknessTex) {
		int index = findTextureIndex(pThicknessTex, _T(""),FALSE);
		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pThicknessTex))
			CreateTextureTransformBlockEx(texIdx, pThicknessTex);
		obj.insert(std::make_pair("iridescenceThicknessTexture", tinygltf::Value(texIdx)));
	}

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_iridescence", val));

	m_MtlIridescence_Used = TRUE;
	return TRUE;
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateEmissiveStrengthTexture(tinygltf::Material& material, const EmissiveStrengthStruct& str, BOOL animated)
{

	tinygltf::Value::Object obj;
	if (str.strength != 1.0f || animated) obj.insert(std::make_pair("emissiveStrength", tinygltf::Value(truncateDecimal(str.strength))));

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_emissive_strength", val));

	m_MtlEmitStrength_Used = TRUE;
	return TRUE;
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateDispersionTexture(tinygltf::Material& material, const DispersionStruct& str, BOOL animated)
{
	tinygltf::Value::Object obj;
	if (str.dispersion != 0.0f || animated) obj.insert(std::make_pair("dispersion", tinygltf::Value(truncateDecimal(str.dispersion))));

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_dispersion", val));

	m_MtlDispersion_Used = TRUE;
	return TRUE;
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateAnisotropyTexture(tinygltf::Material& material, const AnisotropyStruct& str, BOOL animated)
{
	tinygltf::Value::Object obj;
	if (str.strength != 0.0f || animated)
		obj.insert(std::make_pair("anisotropyStrength", tinygltf::Value(truncateDecimal(str.strength))));
	if (str.rotation != 0.0f || animated)
		obj.insert(std::make_pair("anisotropyRotation", tinygltf::Value(truncateDecimal(str.rotation))));

	Texmap* pTex = GetBitmapTextureRec(str.texture);
	if (pTex) {
		int index = findTextureIndex(pTex, _T(""),FALSE);
		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pTex))
			CreateTextureTransformBlockEx(texIdx, pTex);
		obj.insert(std::make_pair("anisotropyTexture", tinygltf::Value(texIdx)));
	}

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_anisotropy", val));

	m_MtlAnisotropy_Used = TRUE;
	return TRUE;
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateDiffuseTransmissionTexture(tinygltf::Material& material, const DiffuseTransmissionStruct& str, BOOL animated)
{
	tinygltf::Value::Object obj;
	if (str.TransmissionFactor != 0.0f || animated)
		obj.insert(std::make_pair("diffuseTransmissionFactor", tinygltf::Value(truncateDecimal(str.TransmissionFactor))));

	tinygltf::Value::Array col;
	col.push_back(tinygltf::Value(truncateDecimal(str.TransmissionColor.r)));
	col.push_back(tinygltf::Value(truncateDecimal(str.TransmissionColor.g)));
	col.push_back(tinygltf::Value(truncateDecimal(str.TransmissionColor.b)));
	if (str.TransmissionColor != Color(0.0f, 0.0f, 0.0f) || animated)
		obj.insert(std::make_pair("diffuseTransmissionColorFactor", tinygltf::Value(col)));

	Texmap* pTex = GetBitmapTextureRec(str.TransmissionColorTexture);
	if (pTex) {
		int index = findTextureIndex(pTex, _T(""), TRUE);
		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pTex))
			CreateTextureTransformBlockEx(texIdx, pTex);
		obj.insert(std::make_pair("diffuseTransmissionColorTexture", tinygltf::Value(texIdx)));
	}
	pTex = GetBitmapTextureRec(str.TransmissionTexture);
	if (pTex) {
		int index = findTextureIndex(pTex, _T(""),FALSE);
		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pTex))
			CreateTextureTransformBlockEx(texIdx, pTex);
		obj.insert(std::make_pair("diffuseTransmissionTexture", tinygltf::Value(texIdx)));
	}

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("KHR_materials_diffuse_transmission", val));

	m_MtlDiffuseTransmission_Used = TRUE;

	return TRUE;
}


//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateMaterialBumpTexture(tinygltf::Material& material, const MaterialBumpStruct& str, BOOL animated)
{
	tinygltf::Value::Object obj;
	if (str.bumpFactor != 1.0f || animated)
		obj.insert(std::make_pair("bumpFactor", tinygltf::Value(str.bumpFactor)));

	Texmap* pTex = GetBitmapTextureRec(str.bumpTexture);
	if (pTex) {
		int index = findTextureIndex(pTex, _T(""),FALSE);
		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(index)));
		if (!CreateTextureTransformBlockWithOSL(texIdx, pTex))
			CreateTextureTransformBlockEx(texIdx, pTex);
		obj.insert(std::make_pair("bumpTexture", tinygltf::Value(texIdx)));
	}

	if (obj.size() == 0) return TRUE;

	tinygltf::Value val(obj);
	material.extensions.insert(std::make_pair("EXT_materials_bump", val));

	m_MaterialBump_Used = TRUE;
	return TRUE;
}




//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateOcclusionTexture(tinygltf::Material &material, Texmap *pTex, float strength)
{
	if (m_MROMapExportMode != 0) return FALSE;

	if (pTex->ClassID() == bmptexClassID) {
		material.occlusionTexture.index = findTextureIndex(pTex, _T(""),FALSE);
		int mapCh = pTex->GetTheUVGen()->GetMapChannel();
		material.occlusionTexture.texCoord = mapCh - 1;
	}
	else if (pTex->ClassID() == MULTIOUTPUTTOTEXMAP_CLASS_ID) {
		Texmap *pOSLMap;
		pTex->GetParamBlock(0)->GetValue(MultiOutput_sourceMap, m_time, pOSLMap, FOREVER);
		int type = GetOSLMapType(pOSLMap);
		if (type == OSL_BitmapLookUp) {
			TSTR name = pOSLMap->GetParamBlock(1)->GetStr(2, m_time);
			material.occlusionTexture.index = findTextureIndex(NULL, name,FALSE);
			int mapCh = pOSLMap->GetParamBlock(1)->GetInt(UberBmp_UVSet, m_time);
			if(mapCh>0) material.occlusionTexture.texCoord = mapCh - 1;
		}
		else if (type == OSL_UberBitmap) {
			TSTR name = pOSLMap->GetParamBlock(1)->GetStr(10, m_time);
			material.occlusionTexture.index = findTextureIndex(NULL, name,FALSE);
			int mapCh = pOSLMap->GetParamBlock(1)->GetInt(UberBmp_UVSet, m_time);
			material.occlusionTexture.texCoord = mapCh - 1;
		}
		//pOSLMap->GetParamBlock(1)->GetValue(4, m_time, p, FOREVER);
		//material.occlusionTexture.index = findTextureIndex(p);
	}
	else if (pTex->ClassID() == ColorCorrectTexID) {
		material.occlusionTexture.index = findTextureIndex(pTex->GetParamBlock(0)->GetTexmap(1), _T(""),FALSE);
	}
	else if (pTex->ClassID() == VRayBitmapID) {
		material.occlusionTexture.index = findTextureIndex(pTex, _T(""),FALSE);
	}
	else if (pTex->ClassID() == CoronaBitmapID) {
		material.occlusionTexture.index = findTextureIndex(pTex, _T(""),FALSE);
	}
	else {
		material.occlusionTexture.index = -1;
	}
	CreateTextureTransformBlock(material.occlusionTexture.extensions, pTex);

	material.occlusionTexture.strength = strength;

	return TRUE;
}


//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateOpacityTexture(tinygltf::Material &material, Texmap *pTex)
{
	return TRUE;
}


//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateMetalRoughTexture(tinygltf::Material &material, Texmap *pTex1, Texmap *pTex2, int *mapCh1, Texmap *pTex3, int *mapCh2)
{

	BOOL VRayMode = FALSE;
	BOOL CoronaMode = FALSE;
	BOOL OSLMode = FALSE;
	BitmapTex *pBmpTex = NULL;
	BitmapTex *pOcclusionMap = NULL;
	Texmap* pUberBmpOSLMap = NULL;

	tstring fileType;
	switch (m_MROImageType)
	{
	case 0:fileType = _T(".jpg"); break;
	case 1:fileType = _T(".png"); break;
	default:fileType = _T(".jpg"); break;
	}

	if (m_MROMapExportMode != 0) {
		if (m_MROMapExportMode == 2) {
			pBmpTex = MergeRGBChannelTexture(NULL, pTex1, pTex2, _T("MetalRoughMap"), fileType);
			if (pTex3) {
				pOcclusionMap = MergeRGBChannelTexture(pTex3, pTex3, pTex3, _T("OcclusionMap"), fileType);
			}
		}
		else {
			pBmpTex = MergeRGBChannelTexture(pTex3, pTex1, pTex2, _T("MetalRoughOccMap"), fileType);
			if (pTex3) pOcclusionMap = pBmpTex;
		}
		if(pTex1)
			*mapCh1 = GetMapCh(pTex1);
		else if (pTex2)
			*mapCh1 = GetMapCh(pTex2);
		if (pTex3)
			*mapCh2 = GetMapCh(pTex3);

		material.pbrMetallicRoughness.metallicRoughnessTexture.index = findTextureIndex(pBmpTex, _T(""),FALSE);
		material.occlusionTexture.index = findTextureIndex(pOcclusionMap, _T(""),FALSE);

		tinygltf::Value::Object texIdx;
		if (pTex1)
			CreateTextureTransformBlock(texIdx, pTex1, FALSE);
		else if(pTex2)
			CreateTextureTransformBlock(texIdx, pTex2, FALSE);
		else if (pTex3)
			CreateTextureTransformBlock(texIdx, pTex3, FALSE);
		material.pbrMetallicRoughness.metallicRoughnessTexture.extensions = texIdx;

		return TRUE;
	}
	else {
		if (pTex1) {
			if (pTex1->ClassID() == bmptexClassID) {
				pBmpTex = MergeRGBChannelTexture((BitmapTex*)pTex3, (BitmapTex*)pTex1, (BitmapTex*)pTex2);
				*mapCh1 = GetMapCh(pTex1);
			}
			else if (pTex1->ClassID() == MULTIOUTPUTTOTEXMAP_CLASS_ID) {
				Texmap* pOSLMap;
				pTex1->GetParamBlock(0)->GetValue(MultiOutput_sourceMap, m_time, pOSLMap, FOREVER);
				//pOSLMap->GetParamBlock(1)->GetValue(4, m_time, p, FOREVER);
				int type = GetOSLMapType(pOSLMap);
				if (type == OSL_BitmapLookUp) {
					TSTR name = pOSLMap->GetParamBlock(1)->GetStr(2, m_time);
					pBmpTex = (BitmapTex*)m_TextureTable[findTextureIndex(NULL, name,FALSE)];
					OSLMode = TRUE;
				}
				else if (type == OSL_UberBitmap) {
					TSTR name = pOSLMap->GetParamBlock(1)->GetStr(10, m_time);
					pBmpTex = (BitmapTex*)m_TextureTable[findTextureIndex(NULL, name,FALSE)];
					pUberBmpOSLMap = pOSLMap;
					OSLMode = TRUE;
				}
			}
			else if (pTex1->ClassID() == ColorCorrectTexID) {
				//Texmap* p;
				//pTex1->GetParamBlock(0)->GetValue(1, m_time, p, FOREVER);
				//pBmpTex = (BitmapTex*)p;
				pBmpTex = (BitmapTex*)pTex1->GetParamBlock(0)->GetTexmap(1);
			}
			else if (pTex1->ClassID() == RGBMultiTexID) {
				pBmpTex = (BitmapTex*)pTex1->GetParamBlock(0)->GetTexmap(2);
				if (!pBmpTex)
					pBmpTex = (BitmapTex*)pTex1->GetParamBlock(0)->GetTexmap(3);
			}
			else if (pTex1->ClassID() == VRayBitmapID) {
				VRayMode = TRUE;
			}
			else if (pTex2->ClassID() == CoronaBitmapID) {
				CoronaMode = TRUE;
			}
		}
		else if (pTex2) {
			if (pTex2->ClassID() == bmptexClassID) {
				pBmpTex = MergeRGBChannelTexture((BitmapTex*)pTex3, (BitmapTex*)pTex1, (BitmapTex*)pTex2);
				*mapCh1 = GetMapCh(pTex2);
			}
			else if (pTex2->ClassID() == MULTIOUTPUTTOTEXMAP_CLASS_ID) {
				Texmap* pOSLMap;
				pTex2->GetParamBlock(0)->GetValue(MultiOutput_sourceMap, m_time, pOSLMap, FOREVER);
				//pOSLMap->GetParamBlock(1)->GetValue(4, m_time, p, FOREVER);
				int type = GetOSLMapType(pOSLMap);
				if (type == OSL_BitmapLookUp) {
					TSTR name = pOSLMap->GetParamBlock(1)->GetStr(2, m_time);
					pBmpTex = (BitmapTex*)m_TextureTable[findTextureIndex(NULL, name, FALSE)];
					OSLMode = TRUE;
				}
				if (type == OSL_UberBitmap) {
					TSTR name = pOSLMap->GetParamBlock(1)->GetStr(10, m_time);
					pBmpTex = (BitmapTex*)m_TextureTable[findTextureIndex(NULL, name,FALSE)];
					pUberBmpOSLMap = pOSLMap;
					OSLMode = TRUE;
				}
			}
			else if (pTex2->ClassID() == ColorCorrectTexID) {
				//Texmap* p;
				//pTex2->GetParamBlock(0)->GetValue(1, m_time, p, FOREVER);
				//pBmpTex = (BitmapTex*)p;
				pBmpTex = (BitmapTex*)pTex2->GetParamBlock(0)->GetTexmap(1);
			}
			else if (pTex2->ClassID() == RGBMultiTexID) {
				pBmpTex = (BitmapTex*)pTex2->GetParamBlock(0)->GetTexmap(2);
				if (!pBmpTex)
					pBmpTex = (BitmapTex*)pTex2->GetParamBlock(0)->GetTexmap(3);
			}
			else if (pTex2->ClassID() == VRayBitmapID) {
				VRayMode = TRUE;
			}
			else if (pTex2->ClassID() == CoronaBitmapID) {
				CoronaMode = TRUE;
			}
		}
		if (pTex3) {
			if (pTex3->ClassID() == bmptexClassID) {
				pOcclusionMap = MergeRGBChannelTexture((BitmapTex*)pTex3, (BitmapTex*)pTex1, (BitmapTex*)pTex2);
				*mapCh2 = pTex3->GetTheUVGen()->GetMapChannel() - 1;
			}
			else if (pTex3->ClassID() == MULTIOUTPUTTOTEXMAP_CLASS_ID) {
				Texmap* pOSLMap;
				pTex3->GetParamBlock(0)->GetValue(MultiOutput_sourceMap, m_time, pOSLMap, FOREVER);
				//pOSLMap->GetParamBlock(1)->GetValue(4, m_time, p, FOREVER);
				int type = GetOSLMapType(pOSLMap);
				if (type == OSL_BitmapLookUp) {
					TSTR name = pOSLMap->GetParamBlock(1)->GetStr(2, m_time);
					pOcclusionMap = (BitmapTex*)m_TextureTable[findTextureIndex(NULL, name,FALSE)];
					*mapCh2 = pOcclusionMap->GetTheUVGen()->GetMapChannel() - 1;
					OSLMode = TRUE;
				}
				if (type == OSL_UberBitmap) {
					TSTR name = pOSLMap->GetParamBlock(1)->GetStr(10, m_time);
					pOcclusionMap = (BitmapTex*)m_TextureTable[findTextureIndex(NULL, name,FALSE)];
					*mapCh2 = pOcclusionMap->GetTheUVGen()->GetMapChannel() - 1;
					pUberBmpOSLMap = pOSLMap;
					OSLMode = TRUE;
				}
			}
			else if (pTex3->ClassID() == ColorCorrectTexID) {
				pOcclusionMap = (BitmapTex*)pTex3->GetParamBlock(0)->GetTexmap(1);
				*mapCh2 = pOcclusionMap->GetTheUVGen()->GetMapChannel() - 1;
			}
			else if (pTex3->ClassID() == RGBMultiTexID) {
				pBmpTex = (BitmapTex*)pTex3->GetParamBlock(0)->GetTexmap(2);
				if (!pBmpTex)
					pBmpTex = (BitmapTex*)pTex3->GetParamBlock(0)->GetTexmap(3);
			}
			else if (pTex3->ClassID() == VRayBitmapID) {
				VRayMode = TRUE;
			}
			else if (pTex3->ClassID() == CoronaBitmapID) {
				CoronaMode = TRUE;
			}
		}

		if (VRayMode) {
			BitmapTex* pBmpTex1 = NULL;
			BitmapTex* pBmpTex2 = NULL;
			if (pTex1) {
				const TCHAR* ptr = pTex1->GetParamBlock(0)->GetStr(0, m_time, FOREVER);
				pBmpTex1 = NewDefaultBitmapTex();
				pBmpTex1->SetMapName(ptr);
			}
			if (pTex2) {
				const TCHAR* ptr = pTex2->GetParamBlock(0)->GetStr(0, m_time, FOREVER);
				pBmpTex2 = NewDefaultBitmapTex();
				pBmpTex2->SetMapName(ptr);
			}
			pBmpTex = MergeRGBChannelTexture(NULL, pBmpTex1, pBmpTex2);
		}
		if (CoronaMode) {
			BitmapTex* pBmpTex1 = NULL;
			BitmapTex* pBmpTex2 = NULL;
			if (pTex1) {
				const TCHAR* ptr = pTex1->GetParamBlock(0)->GetStr(101, m_time, FOREVER);
				pBmpTex1 = NewDefaultBitmapTex();
				pBmpTex1->SetMapName(ptr);
			}
			if (pTex2) {
				const TCHAR* ptr = pTex2->GetParamBlock(0)->GetStr(101, m_time, FOREVER);
				pBmpTex2 = NewDefaultBitmapTex();
				pBmpTex2->SetMapName(ptr);
			}
			pBmpTex = MergeRGBChannelTexture(NULL, pBmpTex1, pBmpTex2);
		}
	}

	if (pBmpTex) {
		if (pTex1 || pTex2) {
			int idx = findTextureIndex(pBmpTex, _T(""),FALSE);
			material.pbrMetallicRoughness.metallicRoughnessTexture.index = idx;

			tinygltf::Value::Object texIdx;
			if (pUberBmpOSLMap) {
				CreateTextureTransformBlockWithOSL(texIdx, pUberBmpOSLMap, FALSE);
			}
			else {
				if(pTex1)
					CreateTextureTransformBlock(texIdx, pTex1, FALSE);
				else
					CreateTextureTransformBlock(texIdx, pTex2, FALSE);
			}
			material.pbrMetallicRoughness.metallicRoughnessTexture.extensions = texIdx;
		}
	}

	if (pOcclusionMap == pBmpTex)
		material.occlusionTexture.index = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
	else if(pOcclusionMap)
		material.occlusionTexture.index = findTextureIndex(pOcclusionMap, _T(""),FALSE);
	//else
	//	material.occlusionTexture.index = findTextureIndex(pBmpTex, _T(""));

	//material.pbrMetallicRoughness.metallicRoughnessTexture.texcoord = 0;
	//material.pbrMetallicRoughness.metallicFactor = 1.0;
	//material.pbrMetallicRoughness.roughnessFactor = 1.0;

	return TRUE;
}

//======================================================================
//======================================================================
void glTFExporter_Core::CreateMaterialMap(BOOL exportSelected)
{
	m_MaterialMap.clear();
	m_TextureTable.clear();
	m_imagePathTable.clear();
	m_VariantMtlMap.clear();

	MtlBaseLib *mtlLib = GetCOREInterface()->GetSceneMtls();
	for (int i = 0; i < mtlLib->Count(); i++) {
		MtlBase *pMtl = *mtlLib->Addr(i);
		HSMMDepEnumProc dep(pMtl, exportSelected);
		pMtl->DoEnumDependents(&dep);
		if(dep.refTab.Count()>0) CreateMaterialMapRec(pMtl);
	}

	for (auto m : m_WireColorMtlMap) {
		CreateMaterialMapRec(m.second);
	}

	// Set XRef Material Table

	if (m_VariantMtlMap.size() > 0) m_MtlVariants_Used = TRUE;
}

//======================================================================
//======================================================================
void glTFExporter_Core::CreateMaterialMapRec(MtlBase* pOrgMtl, BOOL VariantPart)
{
	if (!pOrgMtl) return;

	MtlBase* pMtl = pOrgMtl;
	if (pOrgMtl->ClassID() == ShellMaterialID){
		pMtl = GetSubMtlFromShellMtl((Mtl*)pOrgMtl);
		if (!pMtl) return;
	}
	if (pOrgMtl->ClassID() == XREFMATERIAL_CLASS_ID) {
		IXRefMaterial* pXrefMtl = IXRefMaterial::GetInterface(*pOrgMtl);
		pMtl = pXrefMtl->GetSourceMaterial();
		if (!pMtl) return;
	}

	int idx = -1;
	tinygltf::Material material;
	material.name = WStringToString(pMtl->GetName().data());
	if (VariantPart) {
		int pos = material.name.find("__");
		if (pos > 0) {
			std::string n = material.name.substr(0, pos);
			material.name = n;
		}
	}

	LogOutput(tstring(_T("Mtl:")) + tstring(pMtl->GetName().data()));

	{
		tinygltf::Value::Object params;
		params.clear();
		ICustAttribContainer* pContainer = pMtl->GetCustAttribContainer();
		if (pContainer) {
			SetCustomAttribute(params, pContainer);
		}
		if(params.size()>0)
			material.extras = tinygltf::Value(params);
	}

	if (pMtl->ClassID() == PBRMetalMtlID) {
		PBRMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
		PBRSpecGlossMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
		m_MtlPbrSpcGls_Used = TRUE;
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
		OpenPBRMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		PhysicalMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}
	else  if (pMtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) {
		StdMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}
	else  if (pMtl->ClassID() == glTFMaterialID) {
		glTFMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}
	else  if (pMtl->ClassID() == USDMaterialID) {
		USDMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}
	else  if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		ArnoldMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}
	else  if (pMtl->ClassID() == VRayMaterialID) {
		VRayMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}
	else  if (pMtl->ClassID() == CoronaMaterialID) {
		CoronaMaterial(pMtl, material);
		m_model.materials.push_back(material);
		idx = m_model.materials.size() - 1;
	}


	if (idx>=0) {
		SetName(&m_model.materials[idx], tstring(pMtl->GetName()));
		m_MaterialMap.insert(std::make_pair(pOrgMtl, (UINT)idx));
	}

	int varCnt = 0;
	if (pMtl->ClassID() == multiClassID){
		for (int i = 0; i < ((Mtl*)pMtl)->NumSubMtls(); i++) {
			CreateMaterialMapRec(((Mtl*)pMtl)->GetSubMtl(i));
		}
	} else if (IsVariantMtl(pMtl, varCnt)) {
		std::vector<Mtl*> mtlTbl;
		if (pMtl->ClassID() == MaterialSwitcherClassID) {
			for (int i = 0; i < varCnt; i++) {
				Mtl *pSubMtl = ((Mtl*)pMtl)->GetSubMtl(i);
				if (pSubMtl) {
					CreateMaterialMapRec(pSubMtl, TRUE);
					mtlTbl.push_back(pSubMtl);
				}
			}
		}
		else if (pMtl->ClassID()== ArnoldSwitchShaderID) {
			IParamBlock2* pBlock = pMtl->GetParamBlock(1);
			for (int i = 0; i < 10; i++) {
				Mtl *pSubMtl;
				pBlock->GetValue(i + 3, m_time, pSubMtl, FOREVER);
				if (pSubMtl) {
					CreateMaterialMapRec(pSubMtl, TRUE);
					mtlTbl.push_back(pSubMtl);
				}
			}
		}
		else if (pMtl->ClassID() == CoronaSelectMtlID) {
			IParamBlock2* pBlock = pMtl->GetParamBlock(0);
			int num;
			pBlock->GetValue(311, m_time, num, FOREVER);
			for (int i = 0; i < num; i++) {
				Mtl *pSubMtl;
#if MAX_RELEASE>=24000
				pBlock->GetValue(312, m_time, pSubMtl, i);
#else
				pBlock->GetValue(312, m_time, pSubMtl, FOREVER, i);
#endif
				if (pSubMtl) {
					CreateMaterialMapRec(pSubMtl, TRUE);
					mtlTbl.push_back(pSubMtl);
				}
			}
		}
		else {
			int num = ((Mtl*)pMtl)->NumSubMtls();
			for (int i = 0; i < num; i++) {
				Mtl *pSubMtl = ((Mtl*)pMtl)->GetSubMtl(i);
				if(!pSubMtl) continue;
				CreateMaterialMapRec(pSubMtl, TRUE);
				mtlTbl.push_back(pSubMtl);
			}
		}
		m_VariantMtlMap.insert(std::make_pair(pMtl, mtlTbl));
	}
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::IsVariantMtl(MtlBase *pMtl, int &cnt)const
{
	if (!pMtl) return FALSE;
	Class_ID cc = pMtl->ClassID();
	if (cc == Class_ID(0x6769144b, 0x02c1017d)) return FALSE;

	if ((pMtl->ClassID() == glTFMtlSwitcherClassID) ||
		(pMtl->ClassID() == PBRMtlSwitcherClassID) ||
		(pMtl->ClassID() == USDMtlSwitcherClassID) ||
		(pMtl->ClassID() == StdMtlSwitcherClassID) ||
		(pMtl->ClassID() == PysicMtlSwitcherClassID)) return TRUE;

	cnt = ((Mtl*)pMtl)->NumSubMtls();
	if (pMtl->ClassID() == MaterialSwitcherClassID) return TRUE;

	if (pMtl->ClassID() == ArnoldSwitchShaderID) return TRUE;
	if (pMtl->ClassID() == CoronaSelectMtlID) return TRUE;

	return FALSE;
}

//======================================================================
//======================================================================
void glTFExporter_Core::CreateWireColorMtlMap(INode* pNode)
{
	if (!pNode) return;
//	if (pNode->SuperClassID() == BASENODE_CLASS_ID)
			
	if (IsGeometryObject(pNode) && !pNode->GetMtl()) {
		DWORD c = pNode->GetWireColor();
		if (m_WireColorMtlMap.find(c) == m_WireColorMtlMap.end()) {
			StdMat2* pSmat = (StdMat2*)NewDefaultStdMat();
			pSmat->SetDiffuse(Color(c), m_time);
			TSTR name;
			name.printf(_T("WireColorMtl%d"), c);
			pSmat->SetName(name);
			m_WireColorMtlMap[c] = pSmat;
		}
	}

	for (int i = 0; i < pNode->NumChildren(); i++) {
		CreateWireColorMtlMap(pNode->GetChildNode(i));
	}
}

//======================================================================
//======================================================================
BOOL GetCutOffValue(Texmap *pCutOffTex, Texmap* &pRetTex, float &val)
{
	pRetTex = NULL;
	val = 0.0f;
	if (!pCutOffTex) return FALSE;

	IParamBlock2 *pPBlock1 = pCutOffTex->GetParamBlock(1);
	pPBlock1->GetValue(0, 0, val, FOREVER);
	pPBlock1->GetValue(4, 0, pRetTex, FOREVER);
	
	if (!pRetTex) return FALSE;
	if (pRetTex->ClassID() == ColorCorrectTexID) {
		pRetTex = pRetTex->GetParamBlock(0)->GetTexmap(1);
	}
	if (pRetTex->ClassID() == RGBMultiTexID) {
		pRetTex = pRetTex->GetParamBlock(0)->GetTexmap(2);
		if (!pRetTex)
			pRetTex = pRetTex->GetParamBlock(0)->GetTexmap(3);
	}
	return TRUE;
}

//======================================================================
//======================================================================
void GetUV(UVGen *pUVGen, float &px, float &py, float &sx, float &sy)
{
	px = 0.0f;
	py = 0.0f;
	sx = 0.0f;
	sy = 0.0f;

	if (!pUVGen) return;

	IParamBlock *pBlock = (IParamBlock*)pUVGen->SubAnim(0);
	if (!pBlock) return;
	pBlock->GetValue(0, 0, px, FOREVER);
	pBlock->GetValue(1, 0, py, FOREVER);
	pBlock->GetValue(2, 0, sx, FOREVER);
	pBlock->GetValue(3, 0, sy, FOREVER);
}

//======================================================================
//======================================================================
Mtl* GetSubMtlFromShellMtl(Mtl* pMtl)
{
	if (!pMtl) return NULL;
	if (pMtl->ClassID() != ShellMaterialID) return NULL;

	IParamBlock2* pblock = pMtl->GetParamBlock(0);
	if (!pblock) return NULL;

	int vpOut = 0;
	pblock->GetValue(0, 0, vpOut, FOREVER);     // 0 = Origina

	return (vpOut == 0) ? pMtl->GetSubMtl(0) : pMtl->GetSubMtl(1);

}

