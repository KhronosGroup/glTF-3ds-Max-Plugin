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

// TODO: move the Materials.cpp
static Texmap* UnwrapOSLMap(Texmap* pTex, TimeValue t)
{
	if(!pTex || pTex->ClassID() != OSLTex_CLASS_ID)
		return pTex;

	IParamBlock2* pb = pTex->GetParamBlock(1);
	Texmap* subMap = pb ? pb->GetTexmap(6, t, FOREVER) : nullptr;
	return subMap ? subMap : pTex;     // fall back to the OSL node itself
}


//----------------------------------------------------------
//----------------------------------------------------------
void glTFExporter_Core::glTFMaterial(MtlBase* pMtl, tinygltf::Material& material)
{
	DbgAssert(pMtl && pMtl->ClassID() == glTFMaterialID);

	// TODO: small Helper lambda, global refactor will place it into the header as inlined
	auto IsAnimated = [](IParamBlock2* pb, ParamID id, int tabIndex = 0) {
		if(!pb || pb->IDtoIndex(id) < 0)
			return false;
		Control* c = pb->GetControllerByID(id, tabIndex);
		return c && c->IsAnimated();
	};

	IParamBlock2* pBlock0 = pMtl->GetParamBlockByID(0);
	DbgAssert(pBlock0);
	if(!pBlock0) {
		LogOutput(std::wstring( L"Material '") + pMtl->GetName().data() + L"': missing param block 0, skipped");
		return;    // leave 'material' at its defaults
	}

	AColor baseCol = pBlock0->GetAColor(glTF_baseColor, m_time, FOREVER);

	// TODO: document the intention of this magic, why is alpha slightly reduced when an animation controller is attached to the base color ?
	if(baseCol == AColor(1.0f, 1.0f, 1.0f, 1.0f) && pBlock0->GetControllerByID(glTF_baseColor))
		baseCol.a -= 0.00000001f;

	// TODO: the current implementation ignores the active 3ds Max color management settings completely, 
	// this should be solved in a unified manner for all exported colors using 3ds Maxs color management APIs
	// Currently values are simply clamped and not converted to RGB correctly

	baseCol.ClampMinMax();

	material.doubleSided = pBlock0->GetInt(glTF_Doublesided, m_time, FOREVER);

	material.pbrMetallicRoughness.baseColorFactor = { baseCol.r, baseCol.g, baseCol.b, baseCol.a };

	material.pbrMetallicRoughness.roughnessFactor = truncateDecimal(pBlock0->GetFloat(glTF_roughness, m_time, FOREVER));
	material.pbrMetallicRoughness.metallicFactor = truncateDecimal(pBlock0->GetFloat(glTF_metalness, m_time, FOREVER));

	material.pbrMetallicRoughness.baseColorTexture.index = -1;
	if(Texmap* colTex = pBlock0->GetTexmap(glTF_baseColorMap, m_time, FOREVER)) {
		CreateBaseColorTexture(material, colTex);
	}

	float normalScale = pBlock0->GetFloat(glTF_normal, m_time, FOREVER);

	material.normalTexture.index = -1;
	if(Texmap* normTex = pBlock0->GetTexmap(glTF_normalMap, m_time, FOREVER)) {
		CreateNormalTexture(material, UnwrapOSLMap(normTex, m_time), normalScale);
	}

	material.emissiveTexture.index = -1;
	if(Texmap* emTex = pBlock0->GetTexmap(glTF_emissionMap, m_time, FOREVER)) {
		CreateEmitTexture(material, emTex);
	}

	AColor emColor = pBlock0->GetAColor(glTF_emissionColor, m_time, FOREVER);
	if(emColor.r != 0.0f || emColor.g != 0.0f || emColor.b != 0.0f) {
		emColor.ClampMinMax();
		material.emissiveFactor = { emColor.r, emColor.g, emColor.b };
	}

	material.alphaCutoff = truncateDecimal(pBlock0->GetFloat(glTF_alphaCutoff, m_time, FOREVER));
	switch(pBlock0->GetInt(glTF_alphaMode, m_time, FOREVER)) {
		case 1: material.alphaMode = "OPAQUE"; break;
		case 2: material.alphaMode = "MASK"; break;
		case 3: material.alphaMode = "BLEND"; break;
		default: material.alphaMode = "OPAQUE"; break;
	}

	Texmap* roughTex = pBlock0->GetTexmap(glTF_roughnessMap, m_time, FOREVER);
	Texmap* metalTex = pBlock0->GetTexmap(glTF_metalnessMap, m_time, FOREVER);
	Texmap* aoTex = pBlock0->GetTexmap(glTF_ambientOcclusionMap, m_time, FOREVER);

	if(roughTex || metalTex || aoTex) {
		int mapCh1 = 0;
		int mapCh2 = 1;
		CreateMetalRoughTexture(material, roughTex, metalTex, &mapCh1, aoTex, &mapCh2);
		if(roughTex || metalTex)
			material.pbrMetallicRoughness.baseColorTexture.texCoord = mapCh1;
		if(aoTex) {
			material.occlusionTexture.texCoord = mapCh2;
			CreateOcclusionTexture(material, aoTex, pBlock0->GetFloat(glTF_ambientOcclusion, m_time, FOREVER));
		}
	}

	if( IParamBlock2* pBlock1 = pMtl->GetParamBlockByID(1) )
	{
		if( pBlock1->GetInt(glTF_enableTransmission, m_time, FOREVER)) {
			TransmissionStruct transmission = {
				pBlock1->GetTexmap(glTF_transmissionMap, m_time, FOREVER),
				pBlock1->GetFloat(glTF_transmission, m_time, FOREVER)
			};
	
			CreateTransmissionTexture(material, transmission, IsAnimated(pBlock1, glTF_transmission));
		}

		if (pBlock1->GetInt(glTF_enableSpecular, m_time, FOREVER)) {
			SpecularStruct specular = {
				pBlock1->GetFloat(glTF_specular, m_time, FOREVER),
				pBlock1->GetTexmap(glTF_specularMap, m_time,  FOREVER),
				pBlock1->GetColor(glTF_specularcolor, m_time, FOREVER),
				pBlock1->GetTexmap(glTF_specularColorMap, m_time, FOREVER)
			};
			CreateSpecularTexture(material, specular, IsAnimated(pBlock1,glTF_specular) || IsAnimated(pBlock1,glTF_specularcolor) );
		}

		if( pBlock1->GetInt(glTF_enableClearcoat, m_time, FOREVER)) {
			ClearCoatStruct clearCoat = {
				pBlock1->GetFloat(glTF_clearcoat, m_time, FOREVER),
				pBlock1->GetTexmap(glTF_clearcoatMap, m_time, FOREVER),
				pBlock1->GetFloat(glTF_clearcoatRoughness, m_time,  FOREVER),
				pBlock1->GetTexmap(glTF_clearcoatRoughnessMap, m_time,  FOREVER),
				pBlock1->GetFloat(glTF_clearcoatNormal, m_time, FOREVER),
				pBlock1->GetTexmap(glTF_clearcoatNormalMap, m_time, FOREVER)
			};
			CreateClearCoatTexture(material, clearCoat, IsAnimated(pBlock1,glTF_clearcoat) || IsAnimated(pBlock1,glTF_clearcoatRoughness) );
		}

		if( pBlock1->GetInt(glTF_enableSheen, m_time, FOREVER)) {
			SheenStruct sheen = {
				pBlock1->GetColor(glTF_sheenColor, m_time, FOREVER),
				pBlock1->GetTexmap(glTF_sheenColorMap, m_time, FOREVER),
				pBlock1->GetFloat(glTF_sheenRoughness, m_time, FOREVER),
				pBlock1->GetTexmap(glTF_sheenRoughnessMap, m_time,  FOREVER)
			};
			CreateSheenTexture(material, sheen, IsAnimated(pBlock1,glTF_sheenColor) || IsAnimated(pBlock1,glTF_sheenRoughness) );
		}

		if (pBlock1->GetInt(glTF_enableVolume, m_time, FOREVER)) {
			VolumeStruct volume = {
				pBlock1->GetFloat(glTF_volumeThickness, m_time, FOREVER),
				pBlock1->GetTexmap(glTF_volumeThicknessMap, m_time, FOREVER),
				pBlock1->GetFloat(glTF_volumeDistance, m_time, FOREVER),
				pBlock1->GetColor(glTF_volumeColor, m_time, FOREVER)
			};
			CreateVolumeTexture(material, volume, IsAnimated(pBlock1,glTF_volumeThickness) || IsAnimated(pBlock1, glTF_volumeDistance) || IsAnimated(pBlock1, glTF_volumeColor));
		}

		if( pBlock1->GetInt(glTF_enableIndexOfRefraction, m_time, FOREVER)) {
			IORStruct ior = { pBlock1->GetFloat(glTF_indexOfRefraction, m_time,FOREVER) };
			CreateIORTexture(material, ior, IsAnimated(pBlock1, glTF_indexOfRefraction) );
		}

		if( pBlock1->GetInt(glTF_unlit, m_time, FOREVER)) {
			UnlitStruct unlit = { TRUE };
			CreateUnlitTexture(material, unlit, FALSE);
		}
	}

	// TODO: this pattern below has to be refactored/improved
	{
		IridescenceStruct str;
		BOOL isAnimated = false;
		if (SetIridescenceParams(pMtl, str, isAnimated)) {
			CreateIridescenceTexture(material, str, isAnimated);
		}
	}

	{
		EmissiveStrengthStruct str;
		BOOL isAnimated = false;
		if (SetEmissiveStrengthParams(pMtl, str, isAnimated)) {
			CreateEmissiveStrengthTexture(material, str, isAnimated);
		}
	}

	{
		DispersionStruct str;
		BOOL isAnimated = false;
		if (SetDispersionParams(pMtl, str, isAnimated)) {
			CreateDispersionTexture(material, str, isAnimated);
		}
	}

	{
		AnisotropyStruct str;
		BOOL isAnimated = false;
		if (SetAnisotropyParams(pMtl, str, isAnimated)) {
			CreateAnisotropyTexture(material, str, isAnimated);
		}
	}

	{
		DiffuseTransmissionStruct str;
		BOOL isAnimated = false;
		if (SetDiffuseTransmissionParams(pMtl, str, isAnimated)) {
			CreateDiffuseTransmissionTexture(material, str, isAnimated);
		}
	}
}
