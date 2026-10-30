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
#include <istdplug.h>
#include "define.h"

#define IPOS_CONTROL_CLASS_ID		Class_ID(0x118f7e02,0xffee238a)

static float GetSceneScale(void);

Control* ConvertFloatToColorController(Control* pSrcC, UINT ch);

//======================================================================
// Create (TRS) animation channel list of specified node
//======================================================================
BOOL glTFImporter_Core::FindAnimationChannels(cgltf_node *node, cgltf_animation *animation, std::vector<size_t> &ChannelList)
{
	if (!animation) return FALSE;

	size_t ChannelCnt = animation->channels_count;
	for (size_t i = 0; i < ChannelCnt; i++) {
		cgltf_animation_channel *ch = &animation->channels[i];
		//if (!ch)continue;
		if (node == ch->target_node) {
			ChannelList.push_back(i);
		}
	}
	return (ChannelList.size() != 0);
}

//======================================================================
// Create animation channel list of specified material
//======================================================================
BOOL glTFImporter_Core::FindMtlAnimationChannels(cgltf_material* mtl, cgltf_animation* animation, std::vector<size_t>& ChannelList)
{
	if (!animation) return FALSE;

	size_t ChannelCnt = animation->channels_count;

	for (size_t i = 0; i < ChannelCnt; i++) {
		cgltf_animation_channel* ch = &animation->channels[i];
		if (!ch)continue;
		if (ch->target_path != cgltf_animation_path_type_pointer) continue;
		if (ch->extensions_count == 0) continue;

		cgltf_extension* extensions = ch->extensions;
		if (strcmp(extensions->name, "pointer")) continue;

		char type[MAX_PATH], dmy[MAX_PATH];
		int target;
		if (sscanf(extensions->data, "/%255s/%d/%255s", type, &target, dmy) != 3) continue;
		if (strcmp(type, "materials")) continue;

		if (mtl == &m_glTF_data->materials[target]) {
			ChannelList.push_back(i);
		}
	}

	return (ChannelList.size() != 0);
}

//======================================================================
// Create position controler key frame list
//======================================================================
void glTFImporter_Core::GetPosAnimKeyFrameList(cgltf_animation_sampler *sampler, std::map<TimeValue, AnimKeyInfo> &PosKeyList)
{
	std::vector<float> KeyFrames;
	GetDataList(KeyFrames, sampler->input);

	std::vector<float> AnimationList;
	GetDataList(AnimationList, sampler->output);

	AnimKeyInfo keyInfo;
	PosKeyList.clear();
	if (sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float ix = *(p + 0);
			float iy = *(p + 1);
			float iz = *(p + 2);
			float x = *(p + 3);
			float y = *(p + 4);
			float z = *(p + 5);
			float ox = *(p + 6);
			float oy = *(p + 7);
			float oz = *(p + 8);

			keyInfo.pos = Point3(x, y, z)*m_scale;
			keyInfo.inTan = Point3(ix, iy, iz);
			keyInfo.outTan = Point3(ox, oy, oz);
			PosKeyList.insert(std::make_pair((TimeValue)(key*m_TimeScale), keyInfo));
			p += 9;
		}
	}
	else {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float x = *p;
			float y = *(p + 1);
			float z = *(p + 2);
			keyInfo.pos = Point3(x, y, z)*m_scale;
			PosKeyList.insert(std::make_pair((TimeValue)(key*m_TimeScale), keyInfo));
			p += 3;
		}
	}
}
//======================================================================
// Create rotation controler key frame list
//======================================================================
void glTFImporter_Core::GetRotAnimKeyFrameList(cgltf_animation_sampler *sampler, std::map<TimeValue, AnimKeyInfo> &RotKeyList)
{
	std::vector<float> KeyFrames;
	GetDataList(KeyFrames, sampler->input);

	std::vector<float> AnimationList;
	GetDataList(AnimationList, sampler->output);

	AnimKeyInfo keyInfo;
	RotKeyList.clear();
	if (sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
		// TODO: CubicSpline InTan/OutTan not yet implemented(Rotate part)
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float ix = *(p + 0);
			float iy = *(p + 1);
			float iz = *(p + 2);
			float iw = *(p + 3);
			float x = *(p + 4);
			float y = *(p + 5);
			float z = *(p + 6);
			float w = *(p + 7);
			float ox = *(p + 8);
			float oy = *(p + 9);
			float oz = *(p + 10);
			float ow = *(p + 11);
			keyInfo.rot = Quat(-x, -y, -z, w);
			keyInfo.inTanQ = Quat(ix, iy, iz,iw);
			keyInfo.outTanQ = Quat(ox, oy, oz, ow);
			RotKeyList.insert(std::make_pair((TimeValue)(key*m_TimeScale), keyInfo));
			p += 12;
		}
	}
	else {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float x = *p;
			float y = *(p + 1);
			float z = *(p + 2);
			float w = *(p + 3);
			keyInfo.rot = Quat(-x, -y, -z, w);
			RotKeyList.insert(std::make_pair((TimeValue)(key*m_TimeScale), keyInfo));
			p += 4;
		}
	}
}
//======================================================================
// Create color controler key frame list
//======================================================================
void glTFImporter_Core::GetClr3AnimKeyFrameList(cgltf_animation_sampler* sampler, std::map<TimeValue, AnimKeyInfo>& PosKeyList)
{
	std::vector<float> KeyFrames;
	GetDataList(KeyFrames, sampler->input);

	std::vector<float> AnimationList;
	GetDataList(AnimationList, sampler->output);

	AnimKeyInfo keyInfo;
	PosKeyList.clear();
	if (sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
		// TODO: CubicSpline InTan/OutTan not yet implemented(Scale part)
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float ix = *(p + 0);
			float iy = *(p + 1);
			float iz = *(p + 2);
			float x = *(p + 3);
			float y = *(p + 4);
			float z = *(p + 5);
			float ox = *(p + 6);
			float oy = *(p + 7);
			float oz = *(p + 8);

			keyInfo.pos = Point3(x, y, z);
			keyInfo.inTan = Point3(ix, iy, iz);
			keyInfo.outTan = Point3(ox, oy, oz);
			PosKeyList.insert(std::make_pair((TimeValue)(key * m_TimeScale), keyInfo));
			p += 9;
		}
	}
	else {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float x = *p;
			float y = *(p + 1);
			float z = *(p + 2);
			keyInfo.pos = Point3(x, y, z);
			PosKeyList.insert(std::make_pair((TimeValue)(key * m_TimeScale), keyInfo));
			p += 3;
		}
	}
}

//======================================================================
// Create clor+alpha controler key frame list
//======================================================================
void glTFImporter_Core::GetClr4AnimKeyFrameList(cgltf_animation_sampler* sampler, std::map<TimeValue, AnimKeyInfo>& PosKeyList)
{
	std::vector<float> KeyFrames;
	GetDataList(KeyFrames, sampler->input);

	std::vector<float> AnimationList;
	GetDataList(AnimationList, sampler->output);

	AnimKeyInfo keyInfo;
	PosKeyList.clear();
	if (sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
		// CubicSpline[InTan/OutTan] - not yet implemented
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float ix = *(p + 0);
			float iy = *(p + 1);
			float iz = *(p + 2);
			float iw = *(p + 3);
			float x = *(p + 4);
			float y = *(p + 5);
			float z = *(p + 6);
			float w = *(p + 7);
			float ox = *(p + 8);
			float oy = *(p + 9);
			float oz = *(p + 10);
			float ow = *(p + 11);

			keyInfo.clr4 = Point4(x, y, z, w);
			keyInfo.inTan4 = Point4(ix, iy, iz, iw);
			keyInfo.outTan4 = Point4(ox, oy, oz, ow);
			PosKeyList.insert(std::make_pair((TimeValue)(key * m_TimeScale), keyInfo));
			p += 12;
		}
	}
	else {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float x = *p;
			float y = *(p + 1);
			float z = *(p + 2);
			float w = *(p + 3);
			keyInfo.clr4 = Point4(x, y, z, w);
			PosKeyList.insert(std::make_pair((TimeValue)(key * m_TimeScale), keyInfo));
			p += 4;
		}
	}
}

//======================================================================
// Create float controler key frame list
//======================================================================
void glTFImporter_Core::GetFloatAnimKeyFrameList(cgltf_animation_sampler* sampler, std::map<TimeValue, AnimKeyInfo>& FloatKeyList)
{
	std::vector<float> KeyFrames;
	GetDataList(KeyFrames, sampler->input);

	std::vector<float> AnimationList;
	GetDataList(AnimationList, sampler->output);

	AnimKeyInfo keyInfo;
	FloatKeyList.clear();
	if (sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
		// CubicSpline[InTan/OutTan] - not yet implemented
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float i = *(p + 0);
			float f = *(p + 1);
			float o = *(p + 2);

			keyInfo.f = f;
			//keyInfo.inTan = Point3(i, iy, iz, iw);
			//keyInfo.outTan = Point3(o, oy, oz, ow);
			FloatKeyList.insert(std::make_pair((TimeValue)(key * m_TimeScale), keyInfo));
			p += 3;
		}
	}
	else {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float f = *p;
			keyInfo.f = f;
			FloatKeyList.insert(std::make_pair((TimeValue)(key * m_TimeScale), keyInfo));
			p ++;
		}
	}
}

//======================================================================
// Create time & key info list from Key Frame List 
//======================================================================
void glTFImporter_Core::GetPoint2AnimKeyFrameList(cgltf_animation_sampler* sampler, std::map<TimeValue, AnimKeyInfo>& Point2KeyList)
{
	std::vector<float> KeyFrames;
	GetDataList(KeyFrames, sampler->input);

	std::vector<float> AnimationList;
	GetDataList(AnimationList, sampler->output);

	AnimKeyInfo keyInfo;
	Point2KeyList.clear();
	if (sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
		// CubicSpline[InTan/OutTan] - not yet implemented
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float ix = *(p + 0);
			float iy = *(p + 1);
			float x = *(p + 2);
			float y = *(p + 3);
			float ox = *(p + 4);
			float oy = *(p + 5);

			keyInfo.pos = Point3(x, y, 0.0f);
			keyInfo.inTan = Point3(ix, iy, 0.0f);
			keyInfo.outTan = Point3(ox, oy, 0.0f);
			Point2KeyList.insert(std::make_pair((TimeValue)(key * m_TimeScale), keyInfo));
			p += 6;
		}
	}
	else {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float x = *p;
			float y = *(p + 1);
			keyInfo.pos = Point3(x, y, 0.0f);
			Point2KeyList.insert(std::make_pair((TimeValue)(key * m_TimeScale), keyInfo));
			p += 2;
		}
	}
}

//======================================================================
// Create float controler from Key Frame List
//======================================================================
Control* glTFImporter_Core::CreateFloatController(const std::map<TimeValue, AnimKeyInfo> &KeyList, float scale)
{
	Control* pFloatC = (Control*)GetCOREInterface()->CreateInstance(CTRL_FLOAT_CLASS_ID, Class_ID(0x2007, 0x0));
	for (const auto &key : KeyList) {
		TimeValue t = key.first;
		float f = key.second.f * scale;
		pFloatC->SetValue(t, &f);
		if (m_StartTime > t) m_StartTime = t;
		if (m_LastTime < t) m_LastTime = t;
	}

	return pFloatC;
}
//======================================================================
// Create color controler from Key Frame List
//======================================================================
Control* glTFImporter_Core::CreateColorController(const std::map<TimeValue, AnimKeyInfo>& KeyList, cgltf_type type, Control* pOriginalC)
{
	Control* pClrC = pOriginalC;
	if (type == cgltf_type_vec3) {
		pClrC = (Control*)GetCOREInterface()->CreateInstance(CTRL_POINT3_CLASS_ID, Class_ID(0x2011, 0x0));
		for (const auto &key : KeyList) {
			TimeValue t = key.first;
			Point3 clr = key.second.pos;
			pClrC->SetValue(t, &clr);
			if (m_StartTime > t) m_StartTime = t;
			if (m_LastTime < t) m_LastTime = t;
		}
	}
	else {
		pClrC = (Control*)GetCOREInterface()->CreateInstance(CTRL_POINT4_CLASS_ID, Class_ID(0x2013, 0x0));
		for (const auto &key : KeyList) {
			TimeValue t = key.first;
			Point4 clr = key.second.clr4;
			pClrC->SetValue(t, &clr);
			if (m_StartTime > t) m_StartTime = t;
			if (m_LastTime < t) m_LastTime = t;
		}
	}

	return pClrC;
}

//======================================================================
// Create XYZ controler from Key Frame List
//======================================================================
void glTFImporter_Core::SetXYZController(Control *pCtrl, cgltf_interpolation_type InterpType, TimeValue start, const std::map<TimeValue, AnimKeyInfo>& KeyList, float scale)
{
	{
		IKeyControl* pIkeyXCrl = GetKeyControlInterface(pCtrl->GetXController());
		if (!pIkeyXCrl) return;

		for (int i = 0; i < pIkeyXCrl->GetNumKeys(); i++) {
			IBezFloatKey Key;
			pIkeyXCrl->GetKey(i, &Key);

			if (InterpType == cgltf_interpolation_type_cubic_spline) {
				TimeValue t = Key.time;
				if (KeyList.find(t) == KeyList.end()) continue;
				const auto& info = KeyList.at(t);

				Key.inLength = 0.0f;
				Key.outLength = 0.0f;
				Key.intan = 0.0f;
				Key.outtan = 0.0f;

				// Apply actual InTan/OutTan values.
				// Scale by dt to convert from "per-second rate" to "per-interval delta."

				if (i > 0) {
					Key.inLength = 0.333333f;
					IBezFloatKey KeyIn;
					pIkeyXCrl->GetKey(i - 1, &KeyIn);
					Key.intan = info.inTan.x / (t - KeyIn.time) * 3.0f * -scale;
				}
				if (i < pIkeyXCrl->GetNumKeys() - 1) {
					Key.outLength = 0.333333f;
					IBezFloatKey KeyOut;
					pIkeyXCrl->GetKey(i + 1, &KeyOut);
					Key.outtan = info.outTan.x / (KeyOut.time - t) * 3.0f * scale;
				}

				SetInTanType(Key.flags, BEZKEY_USER);
				SetOutTanType(Key.flags, BEZKEY_USER);
			}

			else if (InterpType == cgltf_interpolation_type_linear) {
				SetInTanType(Key.flags, BEZKEY_LINEAR);
				SetOutTanType(Key.flags, BEZKEY_LINEAR);
			}
			else if (InterpType == cgltf_interpolation_type_step) {
				SetInTanType(Key.flags, BEZKEY_STEP);
				SetOutTanType(Key.flags, BEZKEY_STEP);
			}
			pIkeyXCrl->SetKey(i, &Key);
		}
	}

	{
		IKeyControl* pIkeyZCrl = GetKeyControlInterface(pCtrl->GetZController());
		if (!pIkeyZCrl) return;

		for (int i = 0; i < pIkeyZCrl->GetNumKeys(); i++) {
			IBezFloatKey Key;
			pIkeyZCrl->GetKey(i, &Key);

			if (InterpType == cgltf_interpolation_type_cubic_spline) {
				TimeValue t = Key.time;
				if (KeyList.find(t) == KeyList.end()) continue;
				const auto& info = KeyList.at(t);

				// Handle length（Default = 1/3）
				Key.inLength = 0.0f;
				Key.outLength = 0.0f;
				Key.intan = 0.0f;
				Key.outtan = 0.0f;

				if (i > 0) {
					Key.inLength = 0.333333f;
					IBezFloatKey KeyIn;
					pIkeyZCrl->GetKey(i - 1, &KeyIn);
					Key.intan = info.inTan.y / (t - KeyIn.time) * 3.0f * -scale;
				}
				if (i < pIkeyZCrl->GetNumKeys() - 1) {
					Key.outLength = 0.333333f;
					IBezFloatKey KeyOut;
					pIkeyZCrl->GetKey(i + 1, &KeyOut);
					Key.outtan = info.outTan.y / (KeyOut.time - t) * 3.0f * scale;
				}

				SetInTanType(Key.flags, BEZKEY_USER);
				SetOutTanType(Key.flags, BEZKEY_USER);
			}
			else if (InterpType == cgltf_interpolation_type_linear) {
				SetInTanType(Key.flags, BEZKEY_LINEAR);
				SetOutTanType(Key.flags, BEZKEY_LINEAR);
			}
			else if (InterpType == cgltf_interpolation_type_step) {
				SetInTanType(Key.flags, BEZKEY_STEP);
				SetOutTanType(Key.flags, BEZKEY_STEP);
			}
			pIkeyZCrl->SetKey(i, &Key);
		}
	}

	{
		IKeyControl* pIkeyYCrl = GetKeyControlInterface(pCtrl->GetYController());
		if (!pIkeyYCrl) return;

		for (int i = 0; i < pIkeyYCrl->GetNumKeys(); i++) {
			IBezFloatKey Key;
			pIkeyYCrl->GetKey(i, &Key);

			if (InterpType == cgltf_interpolation_type_cubic_spline) {
				TimeValue t = Key.time;
				if (KeyList.find(t) == KeyList.end()) continue;
				const auto& info = KeyList.at(t);

				Key.inLength = 0.0f;
				Key.intan = 0.0f;
				Key.intan = 0.0f;
				Key.outtan = 0.0f;

				if (i > 0) {
					Key.inLength = 0.333333f;
					IBezFloatKey KeyIn;
					pIkeyYCrl->GetKey(i - 1, &KeyIn);
					Key.intan = -info.inTan.z / (t - KeyIn.time) * 3.0f * -scale;
				}
				if (i < pIkeyYCrl->GetNumKeys() - 1) {
					Key.outLength = 0.333333f;
					IBezFloatKey KeyOut;
					pIkeyYCrl->GetKey(i + 1, &KeyOut);
					Key.outtan = -info.outTan.z / (KeyOut.time - t) * 3.0f * scale;
				}

				SetInTanType(Key.flags, BEZKEY_USER);
				SetOutTanType(Key.flags, BEZKEY_USER);
			}
			else if (InterpType == cgltf_interpolation_type_linear) {
				SetInTanType(Key.flags, BEZKEY_LINEAR);
				SetOutTanType(Key.flags, BEZKEY_LINEAR);
			}
			else if (InterpType == cgltf_interpolation_type_step) {
				SetInTanType(Key.flags, BEZKEY_STEP);
				SetOutTanType(Key.flags, BEZKEY_STEP);
			}
			pIkeyYCrl->SetKey(i, &Key);
		}
	}

	// Cleanup: Remove auto-generated key at frame 0 when the actual animation starts later.
	if (start != 0) {
		pCtrl->DeleteKeyAtTime(0);
	}
}

//======================================================================
// Set Base color controller
//======================================================================
void glTFImporter_Core::SetBaseColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
		for (int i = 0; i < pMtl->NumSubs(); i++) {
			Animatable* p = pMtl->SubAnim(i);
			if (p->SuperClassID() == SHADER_CLASS_ID) {
				IParamBlock2* pBlock = p->GetParamBlock(0);
				pBlock->SetControllerByID(1, 0, pCtrl);
				break;
			}
		}
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(0, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(pbr_base_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(pbr_sg_base_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_base_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_base_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(vr_diffuse, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_baseColor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_diffuseColor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(0);
		pBlock->SetControllerByID(opbr_base_color, 0, pCtrl);
	}
}
//======================================================================
// Set Normal scale controller
//======================================================================
void glTFImporter_Core::SetNrmScaleController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
	}
	if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(glTF_normal, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(pbr_bump_map_amt, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(pbr_sg_bump_map_amt, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_bump_map_amt, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		Texmap *pTex = pMtl->GetParamBlock(1)->GetTexmap(an_sf_normal_shader);
		if (pTex) {
			if (pTex->ClassID() == ArnoldNormalMapID) {
				IParamBlock2* pBlock = pTex->GetParamBlock(1);
				pBlock->SetControllerByID(15, 0, pCtrl);
			}
		}
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}
}

//======================================================================
// Set vplume thivknress controller
//======================================================================
void glTFImporter_Core::SetVolumeThicknessController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(2, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_volumeThickness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(2, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(2, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(vr_translucency_thickness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}
}

//======================================================================
// Set vplume distance controller
//======================================================================
void glTFImporter_Core::SetVolumeDistanceController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(3, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_volumeDistance, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(3, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(3, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(vr_translucency_fbCoeff, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}
}

//======================================================================
// Set vplume color controller
//======================================================================
void glTFImporter_Core::SetVolumeColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(4, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_volumeColor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(4, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("Volume")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(4, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_transmission_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(vr_translucency_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(0);
		pBlock->SetControllerByID(opbr_transmission_color, 0, pCtrl);
	}
}

//======================================================================
// Set transmission controller
//======================================================================
void glTFImporter_Core::SetTransmissionController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_transmission, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
#define solidcolor 0
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		Texmap* pTex = pBlock->GetTexmap(pbr_opacity_map);
		if (!pTex) return;
		if (pTex->ClassID() != ColorMapTexID)return;
		pTex->GetParamBlock(0)->SetControllerByID(solidcolor, 0, pCtrl);

	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_transparency, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_transmission, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(3);
		pBlock->SetControllerByID(100, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_opacity, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(0);
		pBlock->SetControllerByID(opbr_transmission_weight, 0, pCtrl);
	}
}

//======================================================================
// Set metal scale controller
//======================================================================
void glTFImporter_Core::SetMetalScaleController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(glTF_metalness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(pbr_metalness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_metalness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_metalness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(vr_reflection_metalness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_metallic, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(0);
		pBlock->SetControllerByID(opbr_base_metalness, 0, pCtrl);
	}
}
//======================================================================
// Set metal roughness controller
//======================================================================
void glTFImporter_Core::SetRoughScaleController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(glTF_roughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(pbr_roughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_roughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_specular_roughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("VRay Extension")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(1, 0, pCtrl);
		}
		//IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		//pBlock->SetControllerByID(vr_diffuse_roughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_baseRoughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_roughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(0);
		pBlock->SetControllerByID(opbr_specular_roughness, 0, pCtrl);
	}
}

//======================================================================
// Set occulusion strength controller
//======================================================================
void glTFImporter_Core::SetOccStrengthController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(glTF_ambientOcclusion, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_occlusion, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}
}

//======================================================================
// Set alpha cutoff controller
//======================================================================
void glTFImporter_Core::SetAlphaCutOffController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
		Texmap *pTex = pMtl->GetSubTexmap(ID_OP);
		if (GetOSLMapType(pTex) != OSL_CutOff) return;

		IParamBlock2* pBlock = pTex->GetParamBlock(1);
		pBlock->SetControllerByID(0, 0, pCtrl);
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(glTF_alphaCutoff, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		Texmap* pTex = pBlock->GetTexmap(pbr_opacity_map, 0);
		if (GetOSLMapType(pTex) != OSL_CutOff) return;

		pBlock = pTex->GetParamBlock(1);
		pBlock->SetControllerByID(0, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		Texmap* pTex = pBlock->GetTexmap(pbr_sg_opacity_map, 0);
		if (GetOSLMapType(pTex) != OSL_CutOff) return;

		pBlock = pTex->GetParamBlock(1);
		pBlock->SetControllerByID(0, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock0 = pMtl->GetParamBlock(0);
		Texmap* pTex = pBlock0->GetTexmap(fm_cutout_map, 0);
		if (GetOSLMapType(pTex) != OSL_CutOff) return;

		IParamBlock2* pBlock = pTex->GetParamBlock(1);
		pBlock->SetControllerByID(0, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock1 = pMtl->GetParamBlock(1);
		Texmap* pTex = pBlock1->GetTexmap(an_sf_opacity_shader, 0);
		if (GetOSLMapType(pTex) != OSL_CutOff) return;

		IParamBlock2* pBlock = pTex->GetParamBlock(1);
		pBlock->SetControllerByID(0, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(3);
		Texmap* pTex = pBlock->GetTexmap(vr_texmap_opacity, 0);
		if (GetOSLMapType(pTex) != OSL_CutOff) return;

		IParamBlock2* pBlock1 = pTex->GetParamBlock(1);
		pBlock1->SetControllerByID(0, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		Texmap* pTex = pBlock->GetTexmap(crn_opacityTexmap, 0);
		if (GetOSLMapType(pTex) != OSL_CutOff) return;

		IParamBlock2* pBlock1 = pTex->GetParamBlock(1);
		pBlock1->SetControllerByID(0, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock1 = pMtl->GetParamBlock(1);
		pBlock1->SetControllerByID(usd_opacityThreshold, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}
}

//======================================================================
// Set emissive color controller
//======================================================================
void glTFImporter_Core::SetEmissiveColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		//Control* pC = ConvertFloatToColorController(pCtrl, 0);
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(glTF_emissionColor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(pbr_emit_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_emit_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_emission_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(vr_selfIllumination, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_selfIllumColor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_emissiveColor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(0);
		pBlock->SetControllerByID(opbr_emission_color, 0, pCtrl);
	}
}

//======================================================================
// Set emissive strength controller
//======================================================================
void glTFImporter_Core::SetEmissiveStrengthController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("EmissiveStrength")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(2, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_emission, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_emission, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_selfIllumLevel, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}
}

//======================================================================
// Set IOR controller
//======================================================================
void glTFImporter_Core::SetIORController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("IOR")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(2, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_indexOfRefraction, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = NULL;
		GetCustAttrPBlock(pMtl, tstring(_T("IOR")), pBlock);
		if (pBlock) {
			pBlock->SetControllerByID(2, 0, pCtrl);
		}
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
#if MAX_RELEASE>=25000
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_trans_ior, 0, pCtrl);
#endif
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_specular_IOR, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_baseIor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_ior, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}
}

//======================================================================
// Set Iridescence Factor controller
//======================================================================
void glTFImporter_Core::SetIridescenceFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_thin_film_weight, 0, pCtrl);
		return;
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {

	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}


	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Iridescence")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(2, 0, pCtrl);
		return;
	}
}

//======================================================================
// Set Iridescence IOR controller
//======================================================================
void glTFImporter_Core::SetIridescenceIorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_thin_film_ior, 0, pCtrl);
		return;
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_thin_film_IOR, 0, pCtrl);
		return;
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(vr_thinfilm_ior, 0, pCtrl);
		return;
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Iridescence")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(3, 0, pCtrl);
	}
}

//======================================================================
// Set Iridescence max controller
//======================================================================
void glTFImporter_Core::SetIridescenceMaxController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_thin_film_thickness, 0, pCtrl);
		return;
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_thin_film_thickness, 0, pCtrl);
		return;
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(vr_thinfilm_thickness_max, 0, pCtrl);
		return;
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}


	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Iridescence")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(5, 0, pCtrl);
	}
}

//======================================================================
// Set Iridescence min controller
//======================================================================
void glTFImporter_Core::SetIridescenceMinController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Iridescence")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(4, 0, pCtrl);
	}
	return;

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(vr_thinfilm_thickness_min, 0, pCtrl);
		return;
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}
//======================================================================
//======================================================================
void glTFImporter_Core::SetClearcoatFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Clearcoat")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(2, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_clearcoat, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_coat, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_clearcoatAmount, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_clearcoat, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}

//======================================================================
//======================================================================
void glTFImporter_Core::SetClearcoatRoughFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Clearcoat")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(4, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_clearcoatRoughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(an_sf_coat_roughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_clearcoatRoughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(usd_clearcoatRoughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}

//======================================================================
//======================================================================
void glTFImporter_Core::SetSheenColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Sheen")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(2, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_sheenColor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		//pBlock->SetControllerByID(vr_sheen_color, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_sheenColor, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}

//======================================================================
//======================================================================
void glTFImporter_Core::SetSheenRoughFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Sheen")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(4, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		pBlock->SetControllerByID(glTF_sheenRoughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		//pBlock->SetControllerByID(vr_texmap_sheen_glossiness_multiplier, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(crn_sheenRoughness, 0, pCtrl);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}

//======================================================================
//======================================================================
void glTFImporter_Core::SetDispersionController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Dispersion")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(2, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_dispersion, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}
}

//======================================================================
//======================================================================
void glTFImporter_Core::SetAnisotropyStrengthController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Anisotropy")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(2, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_anisotropy, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(4);
		pBlock->SetControllerByID(vr_texmap_anisotropy_multiplier, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}

//======================================================================
//======================================================================
void glTFImporter_Core::SetAnisotropyRotationController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Anisotropy")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(3, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		pBlock->SetControllerByID(fm_anisoangle, 0, pCtrl);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(4);
		pBlock->SetControllerByID(vr_texmap_anisotropy_rotation_multiplier, 0, pCtrl);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}

//======================================================================
//======================================================================
void glTFImporter_Core::SetDiffTransFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("DiffuseTransmission")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(2, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}

//======================================================================
//======================================================================
void glTFImporter_Core::SetDiffTransColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("DiffuseTransmission")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(3, 0, pCtrl);
		return;
	}

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
	}
	else if (pMtl->ClassID() == USDMaterialID) {
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}

}

//======================================================================
//======================================================================
void glTFImporter_Core::SetSpecularFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Specular")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(2, 0, pCtrl);
		return;
	}
}
//======================================================================
//======================================================================
void glTFImporter_Core::SetSpecularColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start)
{
	IParamBlock2* pBlock = NULL;
	GetCustAttrPBlock(pMtl, tstring(_T("Specular")), pBlock);
	if (pBlock) {
		pBlock->SetControllerByID(3, 0, pCtrl);
		return;
	}
}

//======================================================================
//======================================================================
void glTFImporter_Core::SetUVScaleController(Mtl* pMtl, Control* pUC, Control* pVC, cgltf_interpolation_type InterpType, TimeValue start, TargetTex target)
{
	Texmap* pTex = NULL;

	if (pMtl->ClassID() == StandardMtlID) {
		if (target == TargetTex::BaseColorMap)
			pTex = pMtl->GetSubTexmap(ID_DI);
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(glTF_baseColorMap);
		if (target == TargetTex::EmissiveMap)
			pTex = pBlock->GetTexmap(glTF_emissionMap);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(pbr_base_color_map);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(pbr_sg_base_color_map);
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(fm_base_color_map);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(an_sf_base_color_shader);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(3);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(100);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(crn_baseTexmap);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(usd_diffuseColor_map);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}


	if (!pTex) return;

	if (pTex->ClassID() == CompositeTexClassID) {
		pTex = pTex->GetParamBlock(0)->GetTexmap(9);
	}

	StdUVGen* pUVGen = GetUVGen(pTex);
	if (!pUVGen) return;

	IParamBlock* pBlock = GetParamBlock(pUVGen, 0);
	if(pUC)
		pBlock->SetController(2, pUC);
	if (pVC)
		pBlock->SetController(3, pVC);

}
//======================================================================
//======================================================================
void glTFImporter_Core::SetUVOffsetController(Mtl* pMtl, Control* pUC, Control* pVC, cgltf_interpolation_type InterpType, TimeValue start, TargetTex target)
{
	Texmap* pTex = NULL;

	if (pMtl->ClassID() == StandardMtlID) {
		if(target== TargetTex::BaseColorMap)
			pTex = pMtl->GetSubTexmap(ID_DI);
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(glTF_baseColorMap);
		if (target == TargetTex::EmissiveMap)
			pTex = pBlock->GetTexmap(glTF_emissionMap);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(pbr_base_color_map);
		if (target == TargetTex::EmissiveMap)
			pTex = pBlock->GetTexmap(pbr_emit_color_map);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(pbr_sg_base_color_map);
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(fm_base_color_map);
		if (target == TargetTex::EmissiveMap)
			pTex = pBlock->GetTexmap(fm_emit_color_map);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(an_sf_base_color_shader);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(4);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(100);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(crn_baseTexmap);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(usd_diffuseColor_map);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}


	if (!pTex) return;

	if (pTex->ClassID() == CompositeTexClassID) {
		pTex = pTex->GetParamBlock(0)->GetTexmap(9);
	}

	StdUVGen* pUVGen = GetUVGen(pTex);
	if (!pUVGen) return;

	IParamBlock* pBlock = GetParamBlock(pUVGen, 0);

	IKeyControl* pIkeyCrl = GetKeyControlInterface(pUC);
	if (!pIkeyCrl) return;

	for (int i = 0; i < pIkeyCrl->GetNumKeys(); i++) {
		IBezFloatKey key;
		pIkeyCrl->GetKey(i, &key);
		TimeValue t = key.time;
		float sclU = pUVGen->GetUScl(t);
		float sclV = pUVGen->GetVScl(t);
		float rot = pUVGen->GetWAng(t);

		Point2 offset(0.0f, 0.0f);
		pUC->GetValue(t, &offset.x, FOREVER);
		pVC->GetValue(t, &offset.y, FOREVER);
		//if (offset.x > 1.0f)offset.x -= int(offset.x);
		//if (offset.x < -1.0f)offset.x += int(offset.x);
		//if (offset.y > 1.0f)offset.y -= int(offset.y);
		//if (offset.y < -1.0f)offset.y += int(offset.y);

		float localoffsetU = -0.5f * cos(rot) + 0.5f * sin(rot) + 0.5f;
		float localoffsetV = -0.5f * sin(rot) - 0.5f * cos(rot) + 0.5f;
		//localoffsetU +=  (1.0f + int(1.0f / sclU) - (1.0f / sclU)) / 2.0f;
		//localoffsetV +=  (1.0f + int(1.0f / sclV) - (1.0f / sclV)) / 2.0f;
		if (sclU == 0.0f) {
		}else if (sclU >= 1.0f) {
			localoffsetU += (1.0f - (1.0f / sclU)) / 2.0f;
			offset.x = -offset.x - localoffsetU;
		}
		else if (sclU < 1.0f) {
			localoffsetU += (1.0f - (1.0f / sclU)) / 2.0f;
			//			offset.x = (1.0f / sclU -1.0f) * 0.5f +(1.0f- offset.x)/sclU;
			offset.x = (1.0f / sclU -1.0f) * 0.5f +(1.0f- offset.x)/sclU;
		}

		if (sclV == 0.0f) {
		} else if (sclV >= 1.0f) {
			localoffsetV += (1.0f - (1.0f / sclV)) / 2.0f;
			offset.y = offset.y + localoffsetV;
		}
		else if (sclV < 1.0f) {
			offset.y = (offset.y) / sclV - (1.0f / sclV - 1.0f) / 2.0f;
		}


		pUVGen->SetUOffs(offset.x, t);
		pUVGen->SetVOffs(offset.y, t);
	}
/*
	if (pUC)
		pBlock->SetController(0, pUC);
	if (pVC)
		pBlock->SetController(1, pVC);
	*/
}

//======================================================================
//======================================================================
void glTFImporter_Core::SetUVRotateController(Mtl* pMtl, Control* pRotWC, cgltf_interpolation_type InterpType, TimeValue start, TargetTex target)
{
	Texmap* pTex = NULL;

	if (pMtl->ClassID() == StandardMtlID) {
		if (target == TargetTex::BaseColorMap)
			pTex = pMtl->GetSubTexmap(ID_DI);
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(glTF_baseColorMap);
		if (target == TargetTex::EmissiveMap)
			pTex = pBlock->GetTexmap(glTF_emissionMap);
		if (target == TargetTex::NormalMap)
			pTex = pBlock->GetTexmap(glTF_normalMap);
		if (target == TargetTex::VolumeThicknessMap)
			pTex = pBlock->GetTexmap(glTF_volumeThicknessMap);
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(pbr_base_color_map);
		if (target == TargetTex::NormalMap)
			pTex = pBlock->GetTexmap(pbr_norm_map);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(pbr_sg_base_color_map);
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(fm_base_color_map);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(an_sf_base_color_shader);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(3);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(100);
	}
	else if (pMtl->ClassID() == CoronaMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(0);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(crn_baseTexmap);
	}
	else if (pMtl->ClassID() == USDMaterialID) {
		IParamBlock2* pBlock = pMtl->GetParamBlock(1);
		if (target == TargetTex::BaseColorMap)
			pTex = pBlock->GetTexmap(usd_diffuseColor_map);
	}
	else if (pMtl->ClassID() == OpenPBRMaterialID) {
	}


	if (!pTex) return;

	if (pTex->ClassID() == CompositeTexClassID) {
		pTex = pTex->GetParamBlock(0)->GetTexmap(9);
	}

	StdUVGen* pUVGen = GetUVGen(pTex);
	if (!pUVGen) return;

	IParamBlock* pBlock = GetParamBlock(pUVGen, 0);
	if (pRotWC)
		pBlock->SetController(6, pRotWC);

}

//======================================================================
// Create Key Frame List of scale controller
//======================================================================
void glTFImporter_Core::GetSclAnimKeyFrameList(cgltf_animation_sampler *sampler, std::map<TimeValue, AnimKeyInfo> &SclKeyList)
{
	std::vector<float> KeyFrames;
	GetDataList(KeyFrames, sampler->input);

	std::vector<float> AnimationList;
	GetDataList(AnimationList, sampler->output);

	AnimKeyInfo keyInfo;
	SclKeyList.clear();
	if (sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
		// CubicSpline[InTan/OutTan] - not yet implemented
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float ix = *(p + 0);
			float iy = *(p + 1);
			float iz = *(p + 2);
			float x = *(p + 3);
			float y = *(p + 4);
			float z = *(p + 5);
			float ox = *(p + 6);
			float oy = *(p + 7);
			float oz = *(p + 8);
			keyInfo.pos = Point3(x, y, z);
			keyInfo.inTan = Point3(ix, iy, iz);
			keyInfo.outTan = Point3(ox, oy, oz);
			SclKeyList.insert(std::make_pair((TimeValue)(key*m_TimeScale), keyInfo));
			p += 9;
		}
	}
	else {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			float x = *p;
			float y = *(p + 1);
			float z = *(p + 2);
			keyInfo.pos = Point3(x, y, z);
			SclKeyList.insert(std::make_pair((TimeValue)(key*m_TimeScale), keyInfo));
			p += 3;
		}
	}
}

//======================================================================
// Assign animation
// Set animation from parent hierachey
//======================================================================
void glTFImporter_Core::SetAnimationRec(INode *pNode, int animIdx)
{
	std::map<TimeValue, AnimKeyInfo> PosKeyList;
	std::map<TimeValue, AnimKeyInfo> RotKeyList;
	std::map<TimeValue, AnimKeyInfo> SclKeyList;
	std::map<TimeValue, std::vector<float> > WeightKeyList;

	cgltf_interpolation_type ScaleInterpType = cgltf_interpolation_type_linear;
	cgltf_interpolation_type RotInterpType = cgltf_interpolation_type_linear;
	cgltf_interpolation_type TransInterpType = cgltf_interpolation_type_linear;

	cgltf_node *node = FindNodeByINode(pNode);
	std::vector<size_t> ChannelList;
	cgltf_animation* animation = NULL;
	if(m_glTF_data->animations_count > animIdx)
		animation = &m_glTF_data->animations[animIdx];

	FindAnimationChannels(node, animation, ChannelList);
	if (ChannelList.size() > 0) {
		for (auto chIdx : ChannelList) {
			cgltf_animation_channel *ch = &animation->channels[chIdx];
			cgltf_animation_sampler *sampler = ch->sampler;
			if (ch->target_path == cgltf_animation_path_type_translation) {
				TransInterpType = sampler->interpolation;
				GetPosAnimKeyFrameList(sampler, PosKeyList);
			} else if (ch->target_path == cgltf_animation_path_type_rotation) {
				RotInterpType = sampler->interpolation;
				GetRotAnimKeyFrameList(sampler, RotKeyList);
			} else if (ch->target_path == cgltf_animation_path_type_scale) {
				ScaleInterpType = sampler->interpolation;
				GetSclAnimKeyFrameList(sampler, SclKeyList);
			} else if (ch->target_path == cgltf_animation_path_type_weights) {
				GetWeightAnimKeyFrameList(sampler, WeightKeyList, (int)node->mesh->weights_count);
			}

			SetAnimImportStatus(1);
		}
	}
#ifdef MAX_RELEASE_R24
	Matrix3 mtx;
#else
	Matrix3 mtx(1);
#endif 

	if (pNode->GetParentNode()->IsRootNode())
		mtx = YupTM;

	if (SclKeyList.size() > 0) {
		Control *pSclC = (Control*)GetCOREInterface()->CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID(0x118f7c01, 0xfeee238b));
		pNode->GetTMController()->SetScaleController(pSclC);
		//Control *pSclC = pNode->GetTMController()->GetScaleController();
		for (const auto &key : SclKeyList) {
			TimeValue t = key.first;
			Point3 scl = key.second.pos;// *mtx;
			pSclC->SetValue(t, &scl);
			if (m_StartTime > t) m_StartTime = t;
			if (m_LastTime < t) m_LastTime = t;
		}
		SetXYZController(pSclC, ScaleInterpType, SclKeyList.begin()->first, SclKeyList);
	}

	if (RotKeyList.size() > 0) {
		if (m_UseQuatCtrl) {
			Control* pRotC = (Control*)GetCOREInterface()->CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(LININTERP_ROTATION_CLASS_ID, 0x0));
			pNode->GetTMController()->SetRotationController(pRotC);
			for (const auto& key : RotKeyList) {
				TimeValue t = key.first;
				Quat rot = key.second.rot * mtx;
				pRotC->SetValue(t, &rot);
				if (m_StartTime > t) m_StartTime = t;
				if (m_LastTime < t) m_LastTime = t;
			}
		}
		else {
			Control* pRotC = (Control*)GetCOREInterface()->CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(EULER_CONTROL_CLASS_ID, 0x0));
			pNode->GetTMController()->SetRotationController(pRotC);
			for (const auto& key : RotKeyList) {
				TimeValue t = key.first;
				Quat rot = key.second.rot * mtx;
				pRotC->SetValue(t, &rot);
				if (m_StartTime > t) m_StartTime = t;
				if (m_LastTime < t) m_LastTime = t;
			}
			SetXYZController(pRotC, RotInterpType, RotKeyList.begin()->first, RotKeyList);
		}
	}

	if (PosKeyList.size() > 0) {
		Control *pPosC = (Control*)GetCOREInterface()->CreateInstance(CTRL_POSITION_CLASS_ID, IPOS_CONTROL_CLASS_ID);
		pNode->GetTMController()->SetPositionController(pPosC);
		for (const auto &key : PosKeyList) {
			TimeValue t = key.first;
			Point3 pos = key.second.pos * mtx;
			pPosC->SetValue(t, &pos);
			if (m_StartTime > t) m_StartTime = t;
			if (m_LastTime < t) m_LastTime = t;
		}
		float scale = GetSceneScale();
		SetXYZController(pPosC, TransInterpType, PosKeyList.begin()->first, PosKeyList, scale);
	}

	if (WeightKeyList.size() > 0) {
		SetMorphWeightAnimation(pNode, WeightKeyList);
	}


	for (int i = 0; i < pNode->NumChildren(); i++) {
		SetAnimationRec(pNode->GetChildNode(i), animIdx);
	}
}

//======================================================================
//======================================================================
void glTFImporter_Core::GetAnimatedNodeTableRec(INode *pNode, int animID)
{
	cgltf_node *node = FindNodeByINode(pNode);
	std::vector<size_t> ChannelList;
	cgltf_animation *animation = &m_glTF_data->animations[animID];
	FindAnimationChannels(node, animation, ChannelList);
	if (ChannelList.size() > 0) {
		Control *pPosC = (Control*)GetCOREInterface()->CreateInstance(CTRL_POSITION_CLASS_ID, Class_ID(0x118f7e02, 0xffee238a));
		Control *pRotC = (Control*)GetCOREInterface()->CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(EULER_CONTROL_CLASS_ID, 0x0));
		Control *pSclC = (Control*)GetCOREInterface()->CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID(0x118f7c01, 0xfeee238b));
		pNode->GetTMController()->SetPositionController(pPosC);
		pNode->GetTMController()->SetRotationController(pRotC);
		pNode->GetTMController()->SetScaleController(pSclC);
		m_AnimationNodeTab.AppendNode(pNode);
	}
	for (int i = 0; i < pNode->NumChildren(); i++) {
		GetAnimatedNodeTableRec(pNode->GetChildNode(i), animID);
	}
}

void glTFImporter_Core::GetAnimatedNodeTable(void)
{
	m_AnimationNodeTab.ZeroCount();

	INode *pRootNode = GetCOREInterface()->GetRootNode();

	size_t cnt = m_glTF_data->animations_count;
	for (int animID = 0; animID < cnt; animID++) {
		for (int i = 0; i < pRootNode->NumChildren(); i++) {
			GetAnimatedNodeTableRec(pRootNode->GetChildNode(i), animID);
		}
	}
}

//======================================================================
// float controller to color cohntroller
//======================================================================
Control* ConvertFloatToColorController(Control* pSrcC, UINT ch)
{
	if (!pSrcC) return NULL;

	Control* pDstC = (Control*)GetCOREInterface()->CreateInstance(CTRL_POINT4_CLASS_ID, Class_ID(0x2013, 0x0));
	IKeyControl* ikeys = GetKeyControlInterface(pSrcC);
	if (!ikeys) return NULL;

	for (int i = 0; i < ikeys->GetNumKeys(); i++) {
		IBezFloatKey key;
		ikeys->GetKey(i, &key);
		TimeValue t = key.time;
		Point4 val(key.val, key.val, key.val,1.0f);
		pDstC->SetValue(key.time, &val.x);
	}
	return pDstC;
}
//======================================================================
//======================================================================
// TODO: move this into some utility namespace
float GetSceneScale(void)
{
	float scale = 1.0f;

	int type = 0;
#if MAX_RELEASE < 24000
	GetMasterUnitInfo(&type, &scale);
#else
	GetSystemUnitInfo(&type, &scale);
#endif
	switch (type)
	{
	case UNITS_INCHES:		scale /= 0.0254f;	break;
	case UNITS_FEET:		scale /= 0.3048f;	break;
	case UNITS_MILES:		scale /= 1609.34f;	break;
	case UNITS_MILLIMETERS:	scale /= 0.001f;	break;
	case UNITS_CENTIMETERS:	scale /= 0.01f;		break;
	case UNITS_METERS:		scale /= 1.0f;		break;
	case UNITS_KILOMETERS:	scale /= 1000.0f;	break;
	default:				scale /= 1.0f;		break;
	}

	return scale;
}



#include <ILayerControl.h>

void CreateAnimLayerCtrl(INode *pNode)
{

}
