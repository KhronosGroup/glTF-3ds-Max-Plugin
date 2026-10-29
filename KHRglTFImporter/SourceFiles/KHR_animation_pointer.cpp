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
#include "define.h"

//Control* CheckIfAlreadyExist(Mtl* pMtl);

//======================================================================
//======================================================================
BOOL GetTargetPath(char* jsonStr, std::vector<std::string>& retPath)
{
	retPath.clear();

	char buf[MAX_PATH];
	char* ptr1 = jsonStr;
	char* ptr2 = buf;
	while (*ptr1) {
		if (*ptr1 == '\n' || *ptr1 == '\"' || *ptr1 == '\r' || *ptr1 == '\t') {
			ptr1++;
			continue;
		}
		if (*ptr1 == ' ') {
			ptr1++;
			continue;
		}
		*ptr2++ = *ptr1++;
		if ((ptr2 - buf) >= MAX_PATH) break;
	}
	*--ptr2 = 0;

	ptr1 = strchr(buf, ':');
	if (ptr1)	ptr1 += 2;
	else		ptr1 = buf;

	char* p = strchr(ptr1, '/');
	while (p) {
		*p = 0;
		retPath.push_back(std::string(ptr1));
		ptr1 = p + 1;
		p = strchr(ptr1, '/');
	}
	retPath.push_back(std::string(ptr1));

	return TRUE;
}
//======================================================================
//======================================================================
void SplitPoint3ChannelList(std::map<TimeValue, AnimKeyInfo>& KeyList, std::map<TimeValue, AnimKeyInfo>& XKeyList, std::map<TimeValue, AnimKeyInfo>& YKeyList, std::map<TimeValue, AnimKeyInfo>& ZKeyList)
{
	XKeyList.clear();
	YKeyList.clear();
	ZKeyList.clear();
	for (auto key : KeyList) {
		AnimKeyInfo keyInfo;

		TimeValue t = key.first;

		keyInfo.f = key.second.pos.x;
		XKeyList.insert(std::make_pair(t, keyInfo));

		keyInfo.f = key.second.pos.y;
		YKeyList.insert(std::make_pair(t, keyInfo));

		keyInfo.f = key.second.pos.z;
		ZKeyList.insert(std::make_pair(t, keyInfo));
	}

}
//======================================================================
//======================================================================
void glTFImporter_Core::SetAnimationPointer(int animID)
{
	if (m_glTF_data->animations_count <= animID) return;

	cgltf_animation* animation = &m_glTF_data->animations[animID];
	if (!animation) return;


	struct offsetCtrl {
		Control* pUC;
		Control* pVC;
	};
	std::map<Mtl*, offsetCtrl> offsetBaseColorTexList;
	std::map<Mtl*, offsetCtrl> offsetEmissiveTexList;
	offsetBaseColorTexList.clear();
	offsetEmissiveTexList.clear();

	size_t ChannelCnt = animation->channels_count;
	for (size_t i = 0; i < ChannelCnt; i++) {
		cgltf_animation_channel* ch = &animation->channels[i];
		if (!ch)continue;
		if (ch->target_path != cgltf_animation_path_type_pointer) continue;
		if (ch->extensions_count == 0) continue;

		cgltf_animation_sampler* sampler = ch->sampler;
		cgltf_type type = sampler->output->type;
		cgltf_extension* extension = ch->extensions;
		for (int j = 0; j < ch->extensions_count; j++, extension++) {
			if (strcmp(extension->name, "KHR_animation_pointer")) continue;

			std::vector<std::string> retPath;
			GetTargetPath(extension->data, retPath);

			if (retPath.size() < 1) continue;


			if (retPath[0] == "nodes") {
				cgltf_interpolation_type ScaleInterpType = cgltf_interpolation_type_linear;
				cgltf_interpolation_type RotInterpType = cgltf_interpolation_type_linear;
				cgltf_interpolation_type TransInterpType = cgltf_interpolation_type_linear;

				//std::vector<size_t> ChannelList;
				if (retPath.size() < 2) continue;
				int idx = atoi(retPath[1].c_str());
				if (idx < 0 || (size_t)idx >= m_glTF_data->nodes_count) continue;
				cgltf_node* node = &m_glTF_data->nodes[idx];
				INode* pNode = m_NodeMap[node];
				if (!pNode) continue;

#ifdef MAX_RELEASE_R24
				Matrix3 mtx;
#else
				Matrix3 mtx(1);
#endif 
				if (pNode->GetParentNode()->IsRootNode())
					mtx = YupTM;

				if (retPath.size() < 3) continue;
				if (retPath[2] == "translation") {
					std::map<TimeValue, AnimKeyInfo> PosKeyList;
					TransInterpType = sampler->interpolation;
					GetPosAnimKeyFrameList(sampler, PosKeyList);

					Control* pPosC = (Control*)GetCOREInterface()->CreateInstance(CTRL_POSITION_CLASS_ID, Class_ID(0x118f7e02, 0xffee238a));
					pNode->GetTMController()->SetPositionController(pPosC);
					for (auto& const key: PosKeyList) {
						TimeValue t = key.first;
						Point3 pos = key.second.pos * mtx;
						pPosC->SetValue(t, &pos);
						if (m_StartTime > t) m_StartTime = t;
						if (m_LastTime < t) m_LastTime = t;
					}
					if (!PosKeyList.empty())
						SetXYZController(pPosC, TransInterpType, PosKeyList.begin()->first, PosKeyList);
					//m_AnimationNodeTab.AppendNode(pNode);
				}
				else if (retPath[2] == "rotation") {
					std::map<TimeValue, AnimKeyInfo> RotKeyList;
					RotInterpType = sampler->interpolation;
					GetRotAnimKeyFrameList(sampler, RotKeyList);

					Control* pRotC = (Control*)GetCOREInterface()->CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(EULER_CONTROL_CLASS_ID, 0x0));
					pNode->GetTMController()->SetRotationController(pRotC);
					for (auto& const key : RotKeyList) {
						TimeValue t = key.first;
						Quat rot = key.second.rot * mtx;
						pRotC->SetValue(t, &rot);
						if (m_StartTime > t) m_StartTime = t;
						if (m_LastTime < t) m_LastTime = t;
					}
					if (!RotKeyList.empty())
						SetXYZController(pRotC, RotInterpType, RotKeyList.begin()->first, RotKeyList);

				}
				else if (retPath[2] == "scale") {
					std::map<TimeValue, AnimKeyInfo> SclKeyList;
					ScaleInterpType = sampler->interpolation;
					GetSclAnimKeyFrameList(sampler, SclKeyList);

					Control* pSclC = (Control*)GetCOREInterface()->CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID(0x118f7c01, 0xfeee238b));
					pNode->GetTMController()->SetScaleController(pSclC);
					for (auto& const key: SclKeyList) {
						TimeValue t = key.first;
						Point3 scl = key.second.pos * mtx;
						pSclC->SetValue(t, &scl);
						if (m_StartTime > t) m_StartTime = t;
						if (m_LastTime < t) m_LastTime = t;
					}
					if (!SclKeyList.empty())
						SetXYZController(pSclC, ScaleInterpType, SclKeyList.begin()->first, SclKeyList);
				}
				else if (retPath[2] == "weights") {
					std::map<TimeValue, std::vector<float> > WeightKeyList;
					if (node->mesh)
						GetWeightAnimKeyFrameList(sampler, WeightKeyList, (int)node->mesh->weights_count);
					SetMorphWeightAnimation(pNode, WeightKeyList);
				}
			}

			else if (retPath[0] == "materials") {
				if (retPath.size() < 2) continue;
				int idx = atoi(retPath[1].c_str());
				if (idx < 0 || (size_t)idx >= m_glTF_data->materials_count) continue;
				cgltf_material* mtl = &m_glTF_data->materials[idx];
				Mtl* pMtl = m_MaterialMap[mtl];
				if (!pMtl) continue;

				if (retPath.size() < 3) continue;
				if (retPath[2] == "emissiveFactor") {
					std::map<TimeValue, AnimKeyInfo> ColorKeyList;
					GetClr3AnimKeyFrameList(sampler, ColorKeyList);

					Control* pClrC = CreateColorController(ColorKeyList, type);
					SetEmissiveColorController(pMtl, pClrC, sampler->interpolation, 0);
				}
				else if (retPath[2] == "occlusionTexture") {
					if (retPath.size() < 4) continue;
					if (retPath[3] == "strength") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);
						Control* pFloatC = CreateFloatController(FloatKeyList);
						SetOccStrengthController(pMtl, pFloatC, sampler->interpolation, 0);
					}
				}
				else if (retPath[2] == "alphaCutoff") {
					std::map<TimeValue, AnimKeyInfo> FloatKeyList;
					GetFloatAnimKeyFrameList(sampler, FloatKeyList);

					Control* pFloatC = CreateFloatController(FloatKeyList);
					SetAlphaCutOffController(pMtl, pFloatC, sampler->interpolation, 0);
				}
				else if (retPath[2] == "normalTexture") {
					if (retPath.size() < 4) continue;
					if (retPath[3] == "scale") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList);
						SetNrmScaleController(pMtl, pFloatC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "extensions") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "KHR_texture_transform") {
							if (retPath.size() < 6) continue;
							if (retPath[5] == "offset") {
								std::map<TimeValue, AnimKeyInfo> Point2KeyList;
								GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

								std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
								std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
								std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
								SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

								offsetCtrl oc;
								oc.pUC = CreateFloatController(FloatXKeyList);
								oc.pVC = CreateFloatController(FloatYKeyList);
								offsetBaseColorTexList.insert(std::make_pair(pMtl, oc));
								//SetUVOffsetController(pMtl, pUC, pVC, sampler->interpolation, 0);
							}
							else if (retPath[5] == "rotation") {
								std::map<TimeValue, AnimKeyInfo> FloatKeyList;
								GetFloatAnimKeyFrameList(sampler, FloatKeyList);

								Control* pFloatC = CreateFloatController(FloatKeyList);
								SetUVRotateController(pMtl, pFloatC, sampler->interpolation, 0, TargetTex::NormalMap);
							}
							else if (retPath[5] == "scale") {
								std::map<TimeValue, AnimKeyInfo> Point2KeyList;
								GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

								std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
								std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
								std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
								SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

								Control* pUC = CreateFloatController(FloatXKeyList);
								Control* pVC = CreateFloatController(FloatYKeyList);
								SetUVScaleController(pMtl, pUC, pVC, sampler->interpolation, 0, TargetTex::NormalMap);
							}
						}
					}
				}
				else if (retPath[2] == "pbrMetallicRoughness") {
					if (retPath.size() < 4) continue;
					if (retPath[3] == "baseColorFactor") {
						std::map<TimeValue, AnimKeyInfo> ColorKeyList;
						//TransInterpType = sampler->interpolation;
						if (type == cgltf_type_vec3) {
							GetClr3AnimKeyFrameList(sampler, ColorKeyList);
						}
						else if (type == cgltf_type_vec4) {
							GetClr4AnimKeyFrameList(sampler, ColorKeyList);
						}
						//Control* pOriginalClr = CheckIfAlreadyExist(pMtl);
						Control* pClrC = CreateColorController(ColorKeyList, type);
						SetBaseColorController(pMtl, pClrC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "roughnessFactor") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);
						Control* pFloatC = CreateFloatController(FloatKeyList);
						SetRoughScaleController(pMtl, pFloatC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "metallicFactor") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList);
						SetMetalScaleController(pMtl, pFloatC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "baseColorTexture") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "extensions") {
							if (retPath.size() < 6) continue;
							if (retPath[5] == "KHR_texture_transform") {
								if (retPath.size() < 7) continue;
								if (retPath[6] == "scale") {
									std::map<TimeValue, AnimKeyInfo> Point2KeyList;
									GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

									std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
									std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
									std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
									SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

									Control* pUC = CreateFloatController(FloatXKeyList);
									Control* pVC = CreateFloatController(FloatYKeyList);
									SetUVScaleController(pMtl, pUC, pVC, sampler->interpolation, 0, TargetTex::BaseColorMap);
								}
								else if (retPath[6] == "offset") {
									std::map<TimeValue, AnimKeyInfo> Point2KeyList;
									GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

									std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
									std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
									std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
									SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

									offsetCtrl oc;
									oc.pUC = CreateFloatController(FloatXKeyList);
									oc.pVC = CreateFloatController(FloatYKeyList);
									offsetBaseColorTexList.insert(std::make_pair(pMtl, oc));
									//SetUVOffsetController(pMtl, pUC, pVC, sampler->interpolation, 0);
								}
							}
						}
					}
				}
				else if (retPath[2] == "emissiveTexture") {
				if (retPath.size() < 4) continue;
				if (retPath[3] == "extensions") {
					if (retPath.size() < 5) continue;
					if (retPath[4] == "KHR_texture_transform") {
						if (retPath.size() < 6) continue;
						if (retPath[5] == "scale") {
							std::map<TimeValue, AnimKeyInfo> Point2KeyList;
							GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

							std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
							std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
							std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
							SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

							Control* pUC = CreateFloatController(FloatXKeyList);
							Control* pVC = CreateFloatController(FloatYKeyList);
							SetUVScaleController(pMtl, pUC, pVC, sampler->interpolation, 0, TargetTex::EmissiveMap);
						}
						else if (retPath[5] == "offset") {
							std::map<TimeValue, AnimKeyInfo> Point2KeyList;
							GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

							std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
							std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
							std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
							SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

							offsetCtrl oc;
							oc.pUC = CreateFloatController(FloatXKeyList);
							oc.pVC = CreateFloatController(FloatYKeyList);
							offsetEmissiveTexList.insert(std::make_pair(pMtl, oc));
							//SetUVOffsetController(pMtl, pUC, pVC, sampler->interpolation, 0);
						}
					}
				}
		}

				else if (retPath[2] == "extensions") {
					if (retPath.size() < 4) continue;
					if (retPath[3] == "KHR_materials_volume") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "thicknessFactor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetVolumeThicknessController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "attenuationDistance") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetVolumeDistanceController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "attenuationColor") {
							std::map<TimeValue, AnimKeyInfo> ColorKeyList;
							if (type == cgltf_type_vec3) {
								GetClr3AnimKeyFrameList(sampler, ColorKeyList);
							}
							else if (type == cgltf_type_vec4) {
								GetClr4AnimKeyFrameList(sampler, ColorKeyList);
							}
							Control* pClrC = CreateColorController(ColorKeyList, type);
							SetVolumeColorController(pMtl, pClrC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "thicknessTexture") {
							if (retPath.size() < 6) continue;
							if (retPath[5] == "extensions") {
								if (retPath.size() < 7) continue;
								if (retPath[6] == "KHR_texture_transform") {
									if (retPath.size() < 8) continue;
									if (retPath[7] == "offset") {
										std::map<TimeValue, AnimKeyInfo> Point2KeyList;
										GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

										std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
										std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
										std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
										SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

										offsetCtrl oc;
										oc.pUC = CreateFloatController(FloatXKeyList);
										oc.pVC = CreateFloatController(FloatYKeyList);
										offsetBaseColorTexList.insert(std::make_pair(pMtl, oc));
										//SetUVOffsetController(pMtl, pUC, pVC, sampler->interpolation, 0);
									}
									else if (retPath[7] == "rotation") {
										std::map<TimeValue, AnimKeyInfo> FloatKeyList;
										GetFloatAnimKeyFrameList(sampler, FloatKeyList);

										Control* pFloatC = CreateFloatController(FloatKeyList);
										SetUVRotateController(pMtl, pFloatC, sampler->interpolation, 0, TargetTex::VolumeThicknessMap);
									}
									else if (retPath[7] == "scale") {
										std::map<TimeValue, AnimKeyInfo> Point2KeyList;
										GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

										std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
										std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
										std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
										SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

										Control* pUC = CreateFloatController(FloatXKeyList);
										Control* pVC = CreateFloatController(FloatYKeyList);
										SetUVScaleController(pMtl, pUC, pVC, sampler->interpolation, 0, TargetTex::VolumeThicknessMap);
									}
								}
							}
						}
					}

					else if (retPath[3] == "KHR_materials_transmission") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "transmissionFactor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetTransmissionController(pMtl, pFloatC, sampler->interpolation, 0);
						}
					}

					else if (retPath[3] == "KHR_materials_emissive_strength") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "emissiveStrength") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);
							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetEmissiveStrengthController(pMtl, pFloatC, sampler->interpolation, 0);
						}
					}

					else if (retPath[3] == "KHR_materials_ior") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "ior") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetIORController(pMtl, pFloatC, sampler->interpolation, 0);
						}
					}

					else if (retPath[3] == "KHR_materials_iridescence") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "iridescenceFactor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetIridescenceFactorController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "iridescenceIor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetIridescenceIorController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "iridescenceThicknessMaximum") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetIridescenceMaxController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "iridescenceThicknessMinimum") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetIridescenceMinController(pMtl, pFloatC, sampler->interpolation, 0);
						}
					}
					else if (retPath[3] == "KHR_materials_clearcoat") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "clearcoatFactor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetClearcoatFactorController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "clearcoatRoughnessFactor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetClearcoatRoughFactorController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "clearcoatTexture") {
							if (retPath.size() < 6) continue;
							if (retPath[5] == "extensions") {
								if (retPath.size() < 7) continue;
								if (retPath[6] == "KHR_texture_transform") {
									if (retPath.size() < 8) continue;
									if (retPath[7] == "offset") {
									std::map<TimeValue, AnimKeyInfo> Point2KeyList;
									GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

									std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
									std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
									std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
									SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

									offsetCtrl oc;
									oc.pUC = CreateFloatController(FloatXKeyList);
									oc.pVC = CreateFloatController(FloatYKeyList);
									offsetBaseColorTexList.insert(std::make_pair(pMtl, oc));
									//SetUVOffsetController(pMtl, pUC, pVC, sampler->interpolation, 0);
									}
									else if (retPath[7] == "rotation") {
									}
									else if (retPath[7] == "scale") {
										std::map<TimeValue, AnimKeyInfo> Point2KeyList;
										GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

										std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
										std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
										std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
										SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

										Control* pUC = CreateFloatController(FloatXKeyList);
										Control* pVC = CreateFloatController(FloatYKeyList);
										SetUVScaleController(pMtl, pUC, pVC, sampler->interpolation, 0, TargetTex::ClearcoatMap);
									}
								}
							}
						}
					}
					else if (retPath[3] == "KHR_materials_sheen") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "sheenColorFactor") {
							std::map<TimeValue, AnimKeyInfo> ColorKeyList;
							GetClr3AnimKeyFrameList(sampler, ColorKeyList);

							Control* pColorC = CreateFloatController(ColorKeyList);
							SetSheenColorController(pMtl, pColorC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "sheenRoughnessFactor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetSheenRoughFactorController(pMtl, pFloatC, sampler->interpolation, 0);
						}
					}
					else if (retPath[3] == "KHR_materials_specular") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "specularFactor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetSpecularFactorController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "specularColorFactor") {
							std::map<TimeValue, AnimKeyInfo> ColorKeyList;
							GetClr3AnimKeyFrameList(sampler, ColorKeyList);

							Control* pColorC = CreateFloatController(ColorKeyList);
							SetSpecularColorController(pMtl, pColorC, sampler->interpolation, 0);
						}
					}
					else if (retPath[3] == "KHR_materials_dispersion") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "dispersion") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetDispersionController(pMtl, pFloatC, sampler->interpolation, 0);
						}
					}
					else if (retPath[3] == "KHR_materials_anisotropy") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "anisotropyStrength") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetAnisotropyStrengthController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "anisotropyRotation") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetAnisotropyRotationController(pMtl, pFloatC, sampler->interpolation, 0);
						}
					}
					else if (retPath[3] == "KHR_materials_diffuse_transmission") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "diffuseTransmissionFactor") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList);
							SetDiffTransFactorController(pMtl, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "diffuseTransmissionColorFactor") {
							std::map<TimeValue, AnimKeyInfo> ColorKeyList;
							GetClr3AnimKeyFrameList(sampler, ColorKeyList);

							Control* pColorC = CreateFloatController(ColorKeyList);
							SetDiffTransColorController(pMtl, pColorC, sampler->interpolation, 0);
						}
					}
					else if (retPath[3] == "KHR_materials_pbrSpecularGlossiness") {
						if (retPath.size() < 5) continue;
						if (retPath[4] == "diffuseFactor") {
						}
						else if (retPath[4] == "specularFactor") {
						}
						else if (retPath[4] == "glossinessFactor") {
						}
					}

				}
			}

			else if (retPath[0] == "cameras") {
				if (retPath.size() < 2) continue;
				int idx = atoi(retPath[1].c_str());
				if (idx < 0 || (size_t)idx >= m_glTF_data->cameras_count) continue;
				cgltf_camera* camera = &m_glTF_data->cameras[idx];
				GenCamera* pCamera = m_CameraMap[camera];
				if (!pCamera) continue;

				if (retPath.size() < 3) continue;
				if (retPath[2] == "perspective") {
					if (retPath.size() < 4) continue;
					if (retPath[3] == "znear") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList, m_scale);
						SetCamPZnearController(pCamera, pFloatC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "zfar") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList, m_scale);
						SetCamPZfarController(pCamera, pFloatC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "yfov") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList);
						SetCamPYfovController(pCamera, pFloatC, sampler->interpolation, 0);
					}
				}
				else if (retPath[2] == "orthographic") {
					if (retPath.size() < 4) continue;
					if (retPath[3] == "ymag") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList);
						SetCamOYmagController(pCamera, pFloatC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "xmag") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList);
						SetCamOXmagController(pCamera, pFloatC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "znear") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList, m_scale);
						SetCamOZnearController(pCamera, pFloatC, sampler->interpolation, 0);
					}
					else if (retPath[3] == "zfar") {
						std::map<TimeValue, AnimKeyInfo> FloatKeyList;
						GetFloatAnimKeyFrameList(sampler, FloatKeyList);

						Control* pFloatC = CreateFloatController(FloatKeyList, m_scale);
						SetCamOZfarController(pCamera, pFloatC, sampler->interpolation, 0);
					}
				}
			}

			else if (retPath[0] == "extensions") {
				if (retPath.size() < 2) continue;
				if (retPath[1] == "KHR_lights_punctual") {
					if (retPath.size() < 3) continue;
					if (retPath[2] == "lights") {
						if (retPath.size() < 5) continue;
						int idx = atoi(retPath[3].c_str());
						if (idx < 0 || (size_t)idx >= m_glTF_data->lights_count) continue;
						cgltf_light* light = &m_glTF_data->lights[idx];
						GenLight* pLight = m_LightMap[light];
						if (!pLight) continue;
						int lightType = pLight->Type();

						if (retPath[4] == "intensity") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);
							float iscale = 1.0f;
							if (lightType == TSPOT_LIGHT || lightType == FSPOT_LIGHT || lightType == OMNI_LIGHT) iscale = m_LiteIntensityScale;
							Control* pFloatC = CreateFloatController(FloatKeyList, iscale);
							SetLightIntensController(pLight, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "range") {
							std::map<TimeValue, AnimKeyInfo> FloatKeyList;
							GetFloatAnimKeyFrameList(sampler, FloatKeyList);

							Control* pFloatC = CreateFloatController(FloatKeyList, m_scale);
							SetLightRangeController(pLight, pFloatC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "color") {
							std::map<TimeValue, AnimKeyInfo> ColorKeyList;
							if (type == cgltf_type_vec3) {
								GetClr3AnimKeyFrameList(sampler, ColorKeyList);
							}
							else if (type == cgltf_type_vec4) {
								GetClr4AnimKeyFrameList(sampler, ColorKeyList);
							}

							Control* pClrC = CreateColorController(ColorKeyList, type);
							SetLightColorController(pLight, pClrC, sampler->interpolation, 0);
						}
						else if (retPath[4] == "spot") {
							if (retPath.size() < 6) continue;
							if (retPath[5] == "outerConeAngle") {
								std::map<TimeValue, AnimKeyInfo> FloatKeyList;
								GetFloatAnimKeyFrameList(sampler, FloatKeyList);

								Control* pFloatC = CreateFloatController(FloatKeyList, 180.0f / PI);
								SetLightOutAngleController(pLight, pFloatC, sampler->interpolation, 0);
							}
							else if (retPath[5] == "innerConeAngle") {
								std::map<TimeValue, AnimKeyInfo> FloatKeyList;
								GetFloatAnimKeyFrameList(sampler, FloatKeyList);

								Control* pFloatC = CreateFloatController(FloatKeyList, 180.0f / PI);
								SetLightInAngleController(pLight, pFloatC, sampler->interpolation, 0);
							}
						}
					}
				}
			}

			//int xxx = 1;
			/*
						if (type == "nodes") {
							cgltf_node* node = &m_glTF_data->nodes[target];
							INode* pNode = m_NodeMap[node];
						}
						if (type == "materials") {
							cgltf_material* mtl = &m_glTF_data->materials[target];
							Mtl* pMtl = m_MaterialMap[mtl];
						}
						if (type == "cameras") {
						}
			*/

		}

	}

	for (auto om : offsetBaseColorTexList) {
		SetUVOffsetController(om.first, om.second.pUC, om.second.pVC, cgltf_interpolation_type_linear, 0, TargetTex::BaseColorMap);
	}
	for (auto om : offsetEmissiveTexList) {
		SetUVOffsetController(om.first, om.second.pUC, om.second.pVC, cgltf_interpolation_type_linear, 0, TargetTex::EmissiveMap);
	}
}

void glTFImporter_Core::SetUVAnimation(Mtl *pMtl, cgltf_animation_sampler* sampler, TargetTex target)
{
	std::map<TimeValue, AnimKeyInfo> Point2KeyList;
	GetPoint2AnimKeyFrameList(sampler, Point2KeyList);

	std::map<TimeValue, AnimKeyInfo> FloatXKeyList;
	std::map<TimeValue, AnimKeyInfo> FloatYKeyList;
	std::map<TimeValue, AnimKeyInfo> FloatZKeyList;
	SplitPoint3ChannelList(Point2KeyList, FloatXKeyList, FloatYKeyList, FloatZKeyList);

	Control* pUC = CreateFloatController(FloatXKeyList);
	Control* pVC = CreateFloatController(FloatYKeyList);
	SetUVScaleController(pMtl, pUC, pVC, sampler->interpolation, 0, target);
}
#if 0
//===============================================================
//===============================================================
Control* CheckIfAlreadyExist(Mtl* pMtl)
{
	if (!pMtl) return NULL;

	if (pMtl->ClassID() == StandardMtlID) {
	}
	else if (pMtl->ClassID() == glTFMaterialID) {
	}
	else if (pMtl->ClassID() == PBRMetalMtlID) {
		IParamBlock2* pBlock1 = pMtl->GetParamBlockByID(1);
		return pBlock1->GetControllerByID(pbr_base_color);
	}
	else if (pMtl->ClassID() == PBRSpecGlossMtlID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(1);
		pBlock->GetControllerByID(pbr_sg_base_color);
	}
	else if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(0);
		return pBlock->GetControllerByID(fm_base_color);
	}
	else if (pMtl->ClassID() == Arnold_StandardSufaceID) {
		IParamBlock2* pBlock = pMtl->GetParamBlockByID(1);
		pBlock->GetControllerByID(an_sf_base_color);
	}
	else if (pMtl->ClassID() == VRayMaterialID) {
	}

	return NULL;
}
#endif