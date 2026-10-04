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

int CheckValType(const std::string& str);

//==========================================================
//==========================================================
IParamBlock2* glTFExporter_Core::GetCustAttrPBlock(ReferenceTarget * pRef, tstring& AttName)
{
	ICustAttribContainer* pContainer = pRef->GetCustAttribContainer();
	if (!pContainer) return NULL;

	BOOL found = FALSE;
	for (int i = 0; i < pContainer->GetNumCustAttribs(); i++) {
		CustAttrib* pAttr = pContainer->GetCustAttrib(i);
		if (GetCustomAttrName(pAttr) != TSTR(_T("Custom_Attributes"))) continue;

		IParamBlock2* pParamBlk = pAttr->GetParamBlockByID(0);
		if (pParamBlk == NULL) continue;
		if (pParamBlk->GetParameterType(0) != TYPE_STRING) continue;
		TCHAR* s = (TCHAR*)pParamBlk->GetStr(0, m_time);
		if (tstring(s) != AttName) continue;

		return pAttr->GetParamBlock(0);
	}

	return NULL;;
}

//==========================================================
//==========================================================
void glTFExporter_Core::SetCustomAttribute(tinygltf::Value::Object &params, ICustAttribContainer* pContainer)
{
	for (int i = 0; i < pContainer->GetNumCustAttribs(); i++) {
		CustAttrib* pAttr = pContainer->GetCustAttrib(i);
		if (GetCustomAttrName(pAttr) != TSTR(_T("Custom_Attributes"))) continue;

		int n = pAttr->NumParamBlocks();
		if (n == 1) {
			IParamBlock2* pParamBlk = pAttr->GetParamBlockByID(0);
			tstring s(pParamBlk->GetLocalName(0).data());
			if (s == _T("ExtensionName")) continue;
		}

		for (int j = 0; j < n; j++) {
			IParamBlock2* pParamBlk = pAttr->GetParamBlockByID(j);
			if (pParamBlk == NULL) continue;

			for (int x = 0; x < pParamBlk->NumParams(); x++) {
				try {	// GetParamDef()、GetLocalName() - This may result in an error.
					ParamID id = pParamBlk->IndextoID(x);
					ParamDef def = pParamBlk->GetParamDef(id);
					std::string str = WStringToString(pParamBlk->GetLocalName(id).data());

					ParamType2 a = pParamBlk->GetParameterType(id);
					if (pParamBlk->GetParameterType(id) == TYPE_BOOL) {
						int flag;
						pParamBlk->GetValue(id, m_time, flag, FOREVER);
						std::string val = flag ? "true": "false";
						params.insert(std::make_pair(str, val));
					}
					if (pParamBlk->GetParameterType(id) == TYPE_INT) {
						int val;
						pParamBlk->GetValue(id, m_time, val, FOREVER);
						params.insert(std::make_pair(str, val));
					}
					if (pParamBlk->GetParameterType(id) == TYPE_FLOAT) {
						float val;
						pParamBlk->GetValue(id, m_time, val, FOREVER);
						params.insert(std::make_pair(str, val));
					}
					if (pParamBlk->GetParameterType(id) == TYPE_RGBA) {
						Color val;
						pParamBlk->GetValue(id, m_time, val, FOREVER);
						tinygltf::Value::Array col;
						col.push_back(tinygltf::Value(val.r));
						col.push_back(tinygltf::Value(val.g));
						col.push_back(tinygltf::Value(val.b));
						params.insert(std::make_pair(str, col));
					}
					if (pParamBlk->GetParameterType(id) == TYPE_FRGBA) {
						Point4 val;
						pParamBlk->GetValue(id, m_time, val, FOREVER);
						tinygltf::Value::Array col;
						col.push_back(tinygltf::Value(val.x));
						col.push_back(tinygltf::Value(val.y));
						col.push_back(tinygltf::Value(val.z));
						col.push_back(tinygltf::Value(val.w));
						params.insert(std::make_pair(str, col));
					}
					if (pParamBlk->GetParameterType(id) == TYPE_STRING) {
						TCHAR* s = (TCHAR*)pParamBlk->GetStr(id, m_time);
						std::string val = WStringToString(s);
						params.insert(std::make_pair(str, val));
					}
				}
				catch (...) {}
			}

		}
	}
}

//==========================================================
//==========================================================
void glTFExporter_Core::SetUserPropString(tinygltf::Value::Object& params, TSTR &buffer)
{
	std::map<tstring, tstring> attrMap;
	AnalyzeAttrBuf(buffer, attrMap);
	for (auto it : attrMap) {
		std::string str = WStringToString(it.first);
		std::string val = WStringToString(it.second);
		switch (CheckValType(val)) {
		case TYPE_INT:
			params.insert(std::make_pair(str, atoi(val.c_str())));
			break;
		case TYPE_FLOAT:
			params.insert(std::make_pair(str, atof(val.c_str())));
			break;
		default:
			params.insert(std::make_pair(str, val));
		}
	}
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetIORParams(MtlBase* pMtl, IORStruct& str, BOOL &animated)
{
	animated = FALSE;
	str.ior = 1.5f;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("IOR")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("ior"), m_time, str.ior, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) animated = pC->IsAnimated();

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetUnlitParams(MtlBase* pMtl, UnlitStruct& str)
{

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Unlit")));
	if (!pBlock) return FALSE;

	int val;
	pBlock->GetValue(1, m_time, val, FOREVER);
	str.unlit = val;

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetIridescenceParams(MtlBase *pMtl, IridescenceStruct &str, BOOL& animated)
{
	str.factor = 0.0f;
	str.ior = 1.3f;
	str.minimum = 100.0f;
	str.maximum = 400.0f;
	str.texture = NULL;
	str.thicknessTexture = NULL;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Iridescence")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("iridescenceFactor"), m_time, str.factor, FOREVER, 0);
	pBlock->GetValueByName(_T("iridescenceIor"), m_time, str.ior, FOREVER, 0);
	pBlock->GetValueByName(_T("iridescenceThicknessMinimum"), m_time, str.minimum, FOREVER, 0);
	pBlock->GetValueByName(_T("iridescenceThicknessMaximum"), m_time, str.maximum, FOREVER, 0);

	if(str.texture)
		pBlock->GetValueByName(_T("iridescenceTexture"), m_time, str.texture, FOREVER, 0);
	if(str.thicknessTexture)
		pBlock->GetValueByName(_T("iridescenceThicknessTexture"), m_time, str.thicknessTexture, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) if (pC->IsAnimated()) animated = TRUE;
	pC = pBlock->GetControllerByIndex(3);
	if (pC) if (pC->IsAnimated()) animated = TRUE;
	pC = pBlock->GetControllerByIndex(4);
	if (pC) if (pC->IsAnimated()) animated = TRUE;
	pC = pBlock->GetControllerByIndex(5);
	if (pC) if (pC->IsAnimated()) animated = TRUE;

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetVolumeParams(MtlBase* pMtl, VolumeStruct& str, BOOL& animated)
{
	animated = FALSE;

	str.thickness = 0.0f;
	str.distance = 1.3f;
	str.color = AColor(1);
	str.pThicknessMap = NULL;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Volume")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("thicknessFactor"), m_time, str.thickness, FOREVER, 0);
	pBlock->GetValueByName(_T("attenuationDistance"), m_time, str.distance, FOREVER, 0);
	
	if(str.color) 
		pBlock->GetValueByName(_T("attenuationColor"), m_time, str.color, FOREVER, 0);

	if(str.pThicknessMap)
		pBlock->GetValueByName(_T("thicknessTexture"), m_time, str.pThicknessMap, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) if (pC->IsAnimated()) animated = TRUE;
	pC = pBlock->GetControllerByIndex(3);
	if (pC) if (pC->IsAnimated()) animated = TRUE;
	pC = pBlock->GetControllerByIndex(4);
	if (pC) if (pC->IsAnimated()) animated = TRUE;
	pC = pBlock->GetControllerByIndex(5);
	if (pC) if (pC->IsAnimated()) animated = TRUE;

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetTransmissionParams(MtlBase* pMtl, TransmissionStruct& str, BOOL& animated)
{
	animated = FALSE;

	str.factor = 0.0f;
	str.pTex = NULL;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Transmission")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("transmissionFactor"), m_time, str.factor, FOREVER, 0);
	if(str.pTex)
		pBlock->GetValueByName(_T("transmissionTexture"), m_time, str.pTex, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) if (pC->IsAnimated()) animated = TRUE;

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetSheenParams(MtlBase* pMtl, SheenStruct& str, BOOL& animated)
{
	animated = FALSE;

	str.color = Color(0.0f,0.0f,0.0f);
	str.roughness = 0.0f;
	str.pColMap = NULL;
	str.pRoughnessMap = NULL;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Sheen")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	if(str.color) 
		pBlock->GetValueByName(_T("sheenColorFactor"), m_time, str.color, FOREVER, 0);
	
	pBlock->GetValueByName(_T("sheenRoughnessFactor"), m_time, str.roughness, FOREVER, 0);	
	
	if(str.pColMap)
		pBlock->GetValueByName(_T("sheenColorTexture"), m_time, str.pColMap, FOREVER, 0);	
	
	if(str.pRoughnessMap)
		pBlock->GetValueByName(_T("sheenRoughnessTexture"), m_time, str.pRoughnessMap, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) if (pC->IsAnimated()) animated = TRUE;
	pC = pBlock->GetControllerByIndex(4);
	if (pC) if (pC->IsAnimated()) animated = TRUE;

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetClearCoatParams(MtlBase* pMtl, ClearCoatStruct& str, BOOL& animated)
{
	animated = FALSE;

	str.factor = 0.0f;
	str.normalValue = 0.0f;
	str.roughness = 0.0f;
	str.pMap = NULL;
	str.pRoughnessMap = NULL;
	str.pNormalMap = NULL;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Clearcoat")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("clearcoatFactor"), m_time, str.factor, FOREVER, 0);
	pBlock->GetValueByName(_T("clearcoatRoughnessFactor"), m_time, str.roughness, FOREVER, 0);
	
	if(str.pMap)
		pBlock->GetValueByName(_T("clearcoatTexture"), m_time, str.pMap, FOREVER, 0);
	if(str.pRoughnessMap)
		pBlock->GetValueByName(_T("clearcoatRoughnessTexture"), m_time, str.pRoughnessMap, FOREVER, 0);
	if(str.pRoughnessMap)
		pBlock->GetValueByName(_T("clearcoatNormalTexture"), m_time, str.pNormalMap, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) if (pC->IsAnimated()) animated = TRUE;
	pC = pBlock->GetControllerByIndex(4);
	if (pC) if (pC->IsAnimated()) animated = TRUE;

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetEmissiveStrengthParams(MtlBase* pMtl, EmissiveStrengthStruct& str, BOOL& animated)
{
	animated = FALSE;
	str.strength = 1.0f;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("EmissiveStrength")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("emissiveStrength"), m_time, str.strength, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) animated = pC->IsAnimated();

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetDispersionParams(MtlBase* pMtl, DispersionStruct& str, BOOL& animated)
{
	animated = FALSE;
	str.dispersion = 0.0f;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Dispersion")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("dispersion"), m_time, str.dispersion, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) animated = pC->IsAnimated();

	return TRUE;
}


//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetAnisotropyParams(MtlBase* pMtl, AnisotropyStruct& str, BOOL& animated)
{
	animated = FALSE;
	str.strength = 0.0f;
	str.rotation = 0.0f;
	str.texture = nullptr;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Anisotropy")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("anisotropyStrength"), m_time, str.strength, FOREVER, 0);
	pBlock->GetValueByName(_T("anisotropyRotation"), m_time, str.rotation, FOREVER, 0);

	// TODO: constant false condition ?
	if(str.texture)
		pBlock->GetValueByName(_T("anisotropyTexture"), m_time, str.texture, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) animated = pC->IsAnimated();
	pC = pBlock->GetControllerByIndex(3);
	if (pC && !animated) animated = pC->IsAnimated();

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetSpecularParams(MtlBase* pMtl, SpecularStruct& str, BOOL& animated)
{
	animated = FALSE;
	str.factor = 1.0f;
	str.color = Color(1.0f, 1.0f, 1.0f);
	str.pMap = nullptr;
	str.pColMap = nullptr;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Specular")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("specularFactor"), m_time, str.factor, FOREVER, 0);

	if(str.color )
		pBlock->GetValueByName(_T("specularColorFactor"), m_time, str.color, FOREVER, 0);

	// TODO: constant false condition ?
	if(str.pMap )
		pBlock->GetValueByName(_T("specularTexture"), m_time, str.pMap, FOREVER, 0);

	// TODO: constant false condition ?
	if(str.pColMap )
		pBlock->GetValueByName(_T("specularColorTexture"), m_time, str.pColMap, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) animated = pC->IsAnimated();
	pC = pBlock->GetControllerByIndex(3);
	if (pC && !animated) animated = pC->IsAnimated();

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetDiffuseTransmissionParams(MtlBase* pMtl, DiffuseTransmissionStruct& str, BOOL& animated)
{
	animated = FALSE;
	str.TransmissionFactor = 0.0f;
	str.TransmissionColor = Color(1.0f, 1.0f, 1.0f);
	str.TransmissionColorTexture = NULL;
	str.TransmissionTexture = NULL;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("DiffuseTransmission")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("diffuseTransmissionFactor"), m_time, str.TransmissionFactor, FOREVER, 0);

	if(str.TransmissionColor)
		pBlock->GetValueByName(_T("diffuseTransmissionClr"), m_time, str.TransmissionColor, FOREVER, 0);

	if(str.TransmissionColorTexture)
		pBlock->GetValueByName(_T("diffuseTransmissionColorTexture"), m_time, str.TransmissionColorTexture, FOREVER, 0);
	
	if(str.TransmissionTexture)
		pBlock->GetValueByName(_T("diffuseTransmissionTexture"), m_time, str.TransmissionTexture, FOREVER, 0);

	Control* pC = pBlock->GetControllerByIndex(2);
	if (pC) animated = pC->IsAnimated();
	pC = pBlock->GetControllerByIndex(3);
	if (pC && !animated) animated = pC->IsAnimated();

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetWebpTextureParams(MtlBase* pTex, WebpTextureStruct& str)
{
	str.originalPathStr = _T("");
	str.QualityFactor = 0.0f;
	str.LossLess = FALSE;

	IParamBlock2* pBlock = GetCustAttrPBlock(pTex, tstring(_T("Webp Encode")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("QualityFactor"), m_time, str.QualityFactor, FOREVER, 0);
	pBlock->GetValueByName(_T("LossLess"), m_time, str.LossLess, FOREVER, 0);
	str.originalPathStr = pBlock->GetStr(2, m_time);

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetKTX2TextureParams(MtlBase* pTex, KTX2TextureStruct& str)
{
	str.originalPathStr = _T("");
	str.compression = 4;
	str.quality = 128;
	str.mipmap = FALSE;

	IParamBlock2* pBlock = GetCustAttrPBlock(pTex, tstring(_T("KTX2 Encode")));
	if (!pBlock) return FALSE;
	if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("compression"), m_time, str.compression, FOREVER, 0);
	pBlock->GetValueByName(_T("quality"), m_time, str.quality, FOREVER, 0);
	pBlock->GetValueByName(_T("UASTC(On)/ETC1S(Off)"), m_time, str.useUASTC, FOREVER, 0);
	pBlock->GetValueByName(_T("mipmap"), m_time, str.mipmap, FOREVER, 0);
	str.originalPathStr = pBlock->GetStr(2, m_time);

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetVRayExtParams(MtlBase* pMtl, vrayExtStruct& str)
{
	str.roughness = 1.0f;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("VRay Extension")));
	if (!pBlock) return FALSE;

	pBlock->GetValueByName(_T("PBR roughness"), m_time, str.roughness, FOREVER, 0);

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetVisibilityParams(INode* pNode, VisibilityStruct& str)
{
	str.visible = TRUE;

	IParamBlock2* pBlock = GetCustAttrPBlock(pNode->GetObjectRef(), tstring(_T("Visibility")));
	if (!pBlock) return FALSE;
	//if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("visible"), m_time, str.visible, FOREVER, 0);

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetSelectabilityParams(INode* pNode, SelectabilityStruct& str)
{
	str.selectable = TRUE;

	IParamBlock2* pBlock = GetCustAttrPBlock(pNode->GetObjectRef(), tstring(_T("Selectability")));
	if (!pBlock) return FALSE;
	//if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("selectable"), m_time, str.selectable, FOREVER, 0);

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetHoverabilityParams(INode* pNode, HoverabilityStruct& str)
{
	str.hoverable = TRUE;

	IParamBlock2* pBlock = GetCustAttrPBlock(pNode->GetObjectRef(), tstring(_T("Hoverability")));
	if (!pBlock) return FALSE;
	//if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	pBlock->GetValueByName(_T("hoverable"), m_time, str.hoverable, FOREVER, 0);

	return TRUE;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::SetInteractivityParams(ReferenceTarget* pAnim, InteractivityStruct& str)
{
	str.id = 0;

	IParamBlock2* pBlock = GetCustAttrPBlock(pAnim, tstring(_T("Interactivity")));
	if (!pBlock) return FALSE;
	//if (pBlock->GetInt(1, m_time) == 0) return FALSE;

	float id;
	pBlock->GetValueByName(_T("id"), m_time, id, FOREVER, 0);
	str.id = (int)id;

	return TRUE;
}

//==========================================================
//==========================================================
std::string glTFExporter_Core::GetAlphaMode(MtlBase* pMtl, const std::string &defMode)
{
	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("AlphaMode")));
	if (!pBlock) return defMode;
	switch (pBlock->GetInt(1))
	{
	case 1: return "OPAQUE";
	case 2: return "MASK";
	case 3: return "BLEND";
	}

	return defMode;
}

//==========================================================
//==========================================================
BOOL glTFExporter_Core::CheckIfTextureIsUsed(MtlBase* pMtl)
{
	if (!pMtl) return FALSE;

	IParamBlock2* pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Transmission")));
	if (pBlock) {
		if (pBlock->GetInt(1)) {
			Texmap* pTex = pBlock->GetTexmap(3);
			if (pTex) return TRUE;
		}
	}
	pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Volume")));
	if (pBlock) {
		if (pBlock->GetInt(1)) {
			Texmap* pTex = pBlock->GetTexmap(5);
			if (pTex) return TRUE;
		}
	}
	pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Iridescence")));
	if (pBlock) {
		if (pBlock->GetInt(1)) {
			Texmap* pTex = pBlock->GetTexmap(6);
			if (pTex) return TRUE;
			pTex = pBlock->GetTexmap(7);
			if (pTex) return TRUE;
		}
	}
	pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Sheen")));
	if (pBlock) {
		if (pBlock->GetInt(1)) {
			Texmap* pTex = pBlock->GetTexmap(3);
			if (pTex) return TRUE;
			pTex = pBlock->GetTexmap(5);
			if (pTex) return TRUE;
		}
	}
	pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Clearcoat")));
	if (pBlock) {
		if (pBlock->GetInt(1)) {
			Texmap* pTex = pBlock->GetTexmap(3);
			if (pTex) return TRUE;
			pTex = pBlock->GetTexmap(5);
			if (pTex) return TRUE;
			pTex = pBlock->GetTexmap(6);
			if (pTex) return TRUE;

		}
	}
	pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Anisotropy")));
	if (pBlock) {
		if (pBlock->GetInt(1)) {
			Texmap* pTex = pBlock->GetTexmap(4);
			if (pTex) return TRUE;
		}
	}
	pBlock = GetCustAttrPBlock(pMtl, tstring(_T("DiffuseTransmission")));
	if (pBlock) {
		if (pBlock->GetInt(1)) {
			Texmap* pTex = pBlock->GetTexmap(5);
			if (pTex) return TRUE;
			pTex = pBlock->GetTexmap(4);
			if (pTex) return TRUE;
		}
	}
	pBlock = GetCustAttrPBlock(pMtl, tstring(_T("Specular")));
	if (pBlock) {
		if (pBlock->GetInt(1)) {
			Texmap* pTex = pBlock->GetTexmap(3);
			if (pTex) return TRUE;
			pTex = pBlock->GetTexmap(5);
			if (pTex) return TRUE;
		}
	}

	return FALSE;
}

//==========================================================
// Strung division
//==========================================================
std::vector<tstring> StripString(const tstring &s, TCHAR delim)
{
	std::vector<tstring> elems;
	tstring item;
	for (TCHAR ch : s) {
		if (ch == delim) {
			if (!item.empty())
				elems.push_back(item);
			item.clear();
		}
		else {
			item += ch;
		}
	}
	if (!item.empty())
		elems.push_back(item);

	return elems;
}

//----------------------------------------------------------
// Decompose multiple user attribute strings and expand them into a map.
//----------------------------------------------------------
int AnalyzeAttrBuf(TSTR &attrBuf, std::map<tstring, tstring> &attrMap)
{
	attrMap.clear();

	//int cnt = attrBuf.NumberOfLines();
	std::vector<tstring> tbl = StripString(attrBuf.data(), '\n');
	for (auto it : tbl) {
		TCHAR buf[MAX_PATH];
		memset(buf, 0, sizeof(buf));
		//_tcsncpy(buf, it.c_str(), it.size() - 1);
		_tcsncpy_s(buf, _countof(buf), it.c_str(), _TRUNCATE);
		//if (buf[_tcslen(buf) - 1] == 0xd) buf[_tcslen(buf) - 1] = NULL;
		TCHAR *p = _tcschr(buf, '=');
		if (p) {
			*p = NULL;
			if (*(p - 1) == ' ') *(p - 1) = NULL;
			if (*(p + 1) == ' ') p++;
			tstring attrKey(buf);
			tstring attrVal(p + 1);
			attrMap[attrKey] = attrVal;
		}
	}

	return (int)attrMap.size();
}
#include <cctype>
//==========================================================
//==========================================================
int CheckValType(const std::string& str)
{
	BOOL floatF = FALSE;
	for (char const& c : str) {
		if (c == '.') {
			floatF = TRUE;
			continue;
		}
		if (std::isdigit(c) == 0) return TYPE_STRING;
	}

	return floatF ? TYPE_FLOAT : TYPE_INT;
}