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

//----------------------------------------------------------
//----------------------------------------------------------
void glTFExporter_Core::PBRSpecGlossMaterial(MtlBase *pMtl, tinygltf::Material &material)
{
	BOOL animated = FALSE;

	tinygltf::Value::Object obj;

	IParamBlock2 *pBlock0 = pMtl->GetParamBlockByID(0);
	IParamBlock2 *pBlock1 = pMtl->GetParamBlockByID(1);

	Point4 c;
	pBlock1->GetValue(pbr_sg_base_color, m_time, c, FOREVER);
	if(c.x != 1.0 || c.y != 1.0 || c.z != 1.0 || c.w != 1.0){
		tinygltf::Value::Array col;
		col.push_back(tinygltf::Value(c.x));
		col.push_back(tinygltf::Value(c.y));
		col.push_back(tinygltf::Value(c.z));
		col.push_back(tinygltf::Value(1.0));
		obj.insert(std::make_pair("diffuseFactor", tinygltf::Value(col)));
	}
	pBlock1->GetValue(pbr_sg_specular, m_time, c, FOREVER);
	if (c.x || 1.0 && c.y || 1.0 && c.z || 1.0) {
		 tinygltf::Value::Array col;
		 col.push_back(tinygltf::Value(c.x));
		 col.push_back(tinygltf::Value(c.y));
		 col.push_back(tinygltf::Value(c.z));
		 obj.insert(std::make_pair("specularFactor", tinygltf::Value(col)));
	}
	float v;
	pBlock1->GetValue(pbr_sg_glossiness, m_time, v, FOREVER);
	if(v != 1.0){
		 obj.insert(std::make_pair("glossinessFactor", tinygltf::Value(v)));
	}

	Texmap *pTex = NULL;
	pBlock1->GetValue(pbr_sg_base_color_map, m_time, pTex, FOREVER);
	if (pTex) {
		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(findTextureIndex(pTex, _T(""),TRUE))));
		obj.insert(std::make_pair("diffuseTexture", tinygltf::Value(texIdx)));
	}
	pBlock1->GetValue(pbr_sg_specular_map, m_time, pTex, FOREVER);
	if (pTex) {
		tinygltf::Value::Object texIdx;
		texIdx.insert(std::make_pair("index", tinygltf::Value(findTextureIndex(pTex, _T(""),FALSE))));
		obj.insert(std::make_pair("specularGlossinessTexture", tinygltf::Value(texIdx)));
	}

	material.extensions.insert(std::make_pair("KHR_materials_pbrSpecularGlossiness", tinygltf::Value(obj)));



	float scale = 1.0f;
	pBlock1->GetValue(pbr_sg_bump_map_amt, m_time, scale, FOREVER);
	pBlock1->GetValue(pbr_sg_norm_map, m_time, pTex, FOREVER);
	if (pTex) {
		CreateNormalTexture(material, pTex, scale);
	}
	else {
		material.normalTexture.index = -1;
	}

	pTex = GetBitmapTextureRec(pBlock1->GetTexmap(pbr_sg_emit_color_map));
	if (pTex) {
		CreateEmitTexture(material, pTex);
	}
	else {
		material.emissiveTexture.index = -1;
		//material.emissiveTexture.texCoord = 0;
	}
	Point4 col;
	pBlock1->GetValue(pbr_emit_color, m_time, col, FOREVER);
	if(col.x>0.0f && col.y > 0.0f && col.z > 0.0f){
		material.emissiveFactor.push_back(col.x);
		material.emissiveFactor.push_back(col.y);
		material.emissiveFactor.push_back(col.z);
	}
	/*
	pBlock1->GetValue(pbr_ao_map, m_time, pTex, FOREVER);
	if (pTex) {
		CreateOcclusionTexture(material, pTex);
	}
	else {
		material.occlusionTexture.index = -1;
	}
	*/
	pBlock1->GetValue(pbr_sg_opacity_map, m_time, pTex, FOREVER);
	if (pTex) {
		if (pTex->ClassID() == OSLTex_CLASS_ID) {
			IParamBlock2* pPBlock1 = pTex->GetParamBlock(1);
			float cutoff = 1.0f;
			pPBlock1->GetValue(0, m_time, cutoff, FOREVER);
			material.alphaMode = "MASK";
			material.alphaCutoff = cutoff;
		}
		else {
			material.alphaMode = "BLEND";
		}
		//CreateOpacityTexture(material, pTex);
	}
	else {
	}

	{
		UnlitStruct str;
		if (SetUnlitParams(pMtl, str)) {
			CreateUnlitTexture(material, str, animated);
		}
	}

	{
		IridescenceStruct str;
		if (SetIridescenceParams(pMtl, str, animated)) {
			CreateIridescenceTexture(material, str, animated);
		}
	}

	{
		ClearCoatStruct str;
		if (SetClearCoatParams(pMtl, str, animated)) {
			CreateClearCoatTexture(material, str, animated);
		}
	}

	{
		SheenStruct str;
		if (SetSheenParams(pMtl, str, animated)) {
			CreateSheenTexture(material, str, animated);
		}
	}

	{
		EmissiveStrengthStruct str;
		if (SetEmissiveStrengthParams(pMtl, str, animated)) {
			CreateEmissiveStrengthTexture(material, str, animated);
		}
	}

	{
		DispersionStruct str;
		if (SetDispersionParams(pMtl, str, animated)) {
			CreateDispersionTexture(material, str, animated);
		}
	}

	{
		AnisotropyStruct str;
		if (SetAnisotropyParams(pMtl, str, animated)) {
			CreateAnisotropyTexture(material, str, animated);
		}
	}

	{
		DiffuseTransmissionStruct str;
		if (SetDiffuseTransmissionParams(pMtl, str, animated)) {
			CreateDiffuseTransmissionTexture(material, str, animated);
		}
	}

	//tinygltf::Value::Object extension;
	//extension.insert(std::make_pair("KHR_materials_pbrSpecularGlossiness", tinygltf::Value(obj)));

}