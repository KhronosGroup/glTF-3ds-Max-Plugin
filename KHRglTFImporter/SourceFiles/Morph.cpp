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

#pragma comment(lib, "Morpher.lib")

#include "KHRglTFImporter.h"
#include <iEditNormals.h>
#include <MeshNormalSpec.h>
#include <ilayermanager.h>
#include <ilayer.h>
#include <include\MorpherApi.h>
#include <maxscript\maxscript.h>
#include "wM3.h"


#define MR3_CLASS_ID		Class_ID(0x17bb6854, 0xa5cba2a3)


constexpr float ImportWeightScaleFactor = 100;

void CreateMorphVertMapTable(Modifier* pMorphMod, Mesh* pBaseMesh, MeshNormalSpec* pBaseNrmSpec, std::vector<std::vector<Point3> >& morphNormalMapList);


//======================================================================
//======================================================================
cgltf_accessor* FindTargetAttrAccessor(cgltf_morph_target *pr, const char *str)
{
	cgltf_attribute *attr = pr->attributes;
	for (int i = 0; i < pr->attributes_count; i++, attr++) {
		if (!_stricmp(str, attr->name)) return attr->data;
	}
	return NULL;
}
//======================================================================
// Create Key Frame List of Weight controller
//=====================================================================
void glTFImporter_Core::GetWeightAnimKeyFrameList(cgltf_animation_sampler *sampler, std::map<TimeValue, std::vector<float> > &WeightKeyList, int weightCount)
{
	std::vector<float> KeyFrames;
	GetDataList(KeyFrames, sampler->input);

	std::vector<float> AnimationList;
	GetDataList(AnimationList, sampler->output);

	WeightKeyList.clear();
	std::vector<float> weightTab;
	if (sampler->interpolation == cgltf_interpolation_type_cubic_spline) {
		//CubicSpline is not yet implemented.
	}
	else {
		std::vector<float>::iterator p = AnimationList.begin();
		for (auto key : KeyFrames) {
			weightTab.clear();
			for (int wc = 0; wc < weightCount; wc++) {
				weightTab.push_back(*p++);
			}
			WeightKeyList.insert(std::make_pair((TimeValue)(key*m_TimeScale), weightTab));
		}
	}
}
//======================================================================
//======================================================================
void glTFImporter_Core::SetMorphWeightAnimation(INode *pNode, std::map<TimeValue, std::vector<float> > &WeightKeyList)
{
	Modifier *pMod;
	FindModifier(pNode, MR3_CLASS_ID, &pMod);
	MorphR3* pMorph = (MorphR3*)pMod;
	if (!pMorph) return;

	std::vector<IParamBlock*> pblockList;
	for (int i = 0; i < pMorph->NumRefs(); i++) {
		RefTargetHandle hr = ((ReferenceMaker*)pMorph)->GetReference(i);
		if (!hr) continue;
		if (hr->SuperClassID() == PARAMETER_BLOCK_CLASS_ID) {
			IParamBlock *pParamBlk = (IParamBlock*)hr;
			pblockList.push_back(pParamBlk);
		}
	}

	for (auto key : WeightKeyList) {
		TimeValue t = key.first;
		int pIdx = 1;
		for (auto f : key.second) {
			IParamBlock *pblock = pblockList.at(pIdx);
			pblock->SetValue(0, t, f*ImportWeightScaleFactor);
			pIdx++;
		}
		if (m_StartTime > t) m_StartTime = t;
		if (m_LastTime < t) m_LastTime = t;
	}
}

//======================================================================
//======================================================================
void glTFImporter_Core::SetMorph(void)
{
	if (m_MorphTable.size() == 0)return;

	ILayerManager *pLayerMan = GetCOREInterface13()->GetLayerManager();
	ILayer *pLayer = pLayerMan->GetLayer(_T("HSMorphTargetLayer"));
	if (!pLayer) {
		pLayer = pLayerMan->CreateLayer(_T("HSMorphTargetLayer"));
		pLayerMan->AddLayer(pLayer);
	}
	pLayer->Hide(TRUE);

	for (auto node : m_MorphTable) {
		std::vector<INode*> morphTargetNode;
		INode *pNode = m_NodeMap[node];
		//int deleteIt;
		//TriObject *pObj = GetTriObjectFromNode(pNode, m_time, deleteIt);
		//Mesh *pOrgMesh = &pObj->mesh;
		Mesh* pOrgMesh = &((TriObject*)(pNode->GetObjectRef()))->mesh;

		std::vector<tstring> morphTargetTbl;
		SetMorphChannelNameTable(node->mesh, morphTargetTbl);

		std::vector<std::vector<Point3> > morphNormalMapList;
		size_t targetNum = node->mesh->primitives[0].targets_count;
		for (int i = 0; i < targetNum; i++) {
			Mesh targetMesh(*pOrgMesh);

			Point3 *pOrgPt = pOrgMesh->verts;
			MeshNormalSpec* pOrgNrmSpec = pOrgMesh->GetSpecifiedNormals();
			MeshNormalSpec* pNrmSpec = targetMesh.GetSpecifiedNormals();

			std::vector<Point3> VertNormalTable;

			int VertOffset = 0;
			int NormalOffset = 0;

			cgltf_mesh *mesh = node->mesh;
			for (int j = 0; j < mesh->primitives_count; j++) {
				cgltf_primitive *pr = &mesh->primitives[j];
				cgltf_morph_target *target = &pr->targets[i];
				cgltf_draco_mesh_compression *mc = NULL;
				// Will Target not be Draco?
				if (pr->has_draco_mesh_compression) {
					//mc = &pr->draco_mesh_compression;
				}

				//cgltf_attribute_type att_type;
				std::vector<float> VertIdList;
				if (mc) {
					DracoDecodeProc(mc->buffer_view, pr, VertIdList, DracoDecodeType::POSITION);
				}
				else {
					GetDataList(VertIdList, FindTargetAttrAccessor(target, "POSITION"));
				}

				size_t VertNum = VertIdList.size() / 3;
				UINT vIdx = VertOffset;
				for (std::vector<float>::iterator v = VertIdList.begin(); v != VertIdList.end(); v += 3, vIdx++) {
					Point3 op = pOrgPt[vIdx];// *Inverse(pNode->GetNodeTM(m_time));
					Point3 p(*v, *(v + 1), *(v + 2));
					targetMesh.setVert(vIdx, (op + p* m_scale));
				}
				VertOffset += static_cast<int>(VertNum);

				// Setting vertex normals
				std::vector<float> NormalList;
				if (mc) {
					DracoDecodeProc(mc->buffer_view, pr, NormalList, DracoDecodeType::NORMAL);
				}
				else {
					GetDataList(NormalList, FindTargetAttrAccessor(target, "NORMAL"));
				}
				size_t normalNum = NormalList.size() / 3;
				if (normalNum > 0) {
					for (std::vector<float>::iterator v = NormalList.begin(); v != NormalList.end(); v += 3) {
						Point3 n(*v, *(v + 1), *(v + 2));
						VertNormalTable.push_back(n);
					}
				}
				NormalOffset += static_cast<int>(normalNum);
			}

			morphNormalMapList.push_back(VertNormalTable);

			TriObject *pTri = CreateNewTriObject();
			pTri->mesh = targetMesh;
			INode *pTargetNode = GetCOREInterface()->CreateObjectNode(pTri);
			pTargetNode->SetNodeTM(m_time, pNode->GetNodeTM(m_time));
			morphTargetNode.push_back(pTargetNode);
			pLayer->AddToLayer(pTargetNode);
		}

		MorphR3* pMorph = (MorphR3*)AddModifier(pNode, MR3_CLASS_ID);

		GetCOREInterface()->SelectNode(pNode);
		for (int i = 0; i < targetNum; i++) {
			INode *pTargetNode = morphTargetNode.at(i);
			TSTR ComStr;
			ComStr.printf(_T("WM3_MC_BuildFromNode $.Morpher %d $%s"), i + 1, pTargetNode->GetName());
			TSTR ComStr2;
			TSTR n = pTargetNode->GetName();
			if (i < morphTargetTbl.size()) n = TSTR(morphTargetTbl[i].c_str());
			ComStr2.printf(_T("WM3_MC_SetName $.Morpher %d \"%s\""), i + 1, n.data());
#if MAX_RELEASE >= 24000
			ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded);
			ExecuteMAXScriptScript(ComStr2, MAXScript::ScriptSource::NonEmbedded);
#else
			ExecuteMAXScriptScript(ComStr);
#endif;
		}

		size_t wc = node->mesh->weights_count;
		for (int i = 0; i < wc; i++) {
			float w = node->mesh->weights[i] * ImportWeightScaleFactor;
			TSTR ComStr;
			ComStr.printf(_T("WM3_MC_SetValue $.Morpher %d %f"), i + 1, w);
#if MAX_RELEASE >= 24000
			ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded);
#else
			ExecuteMAXScriptScript(ComStr);
#endif;
		}
		// Set the normal vector of the target object
		{
			pOrgMesh->SpecifyNormals();
			MeshNormalSpec* pBaseNrmSpec = pOrgMesh->GetSpecifiedNormals();
			pBaseNrmSpec->BuildNormals();
			pBaseNrmSpec->ComputeNormals();
			CreateMorphVertMapTable(pMorph, pOrgMesh, pBaseNrmSpec, morphNormalMapList);
		}

		for (int i = 0; i < targetNum; i++) {
			INode* pTargetNode = morphTargetNode.at(i);
			if(i<morphTargetTbl.size())pTargetNode->SetName(morphTargetTbl[i].c_str());
		}
	}
}


//====================================================================
// Find the target position matches the base mesh shape and then set the normal of base target mesh
//====================================================================
#define EPS 0.0001f
void CreateMorphVertMapTable(Modifier* pMorphMod, Mesh* pBaseMesh, MeshNormalSpec* pBaseNrmSpec, std::vector<std::vector<Point3> >& morphNormalMapList)
{
	if (!pMorphMod) return;

	size_t morphCnt = morphNormalMapList.size();
	auto VertNormalTable = morphNormalMapList.begin();

	MaxMorphModifier maxMorphModifier(pMorphMod);
	for (int i = 0; i < morphCnt; i++) {
		MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(i);
		//if (!mc.HasData()) continue;
		//if (!mc.IsActive()) continue;
		if (VertNormalTable->size() == 0) {
			VertNormalTable++;
			continue;
		}

		INode* pTargetNode = mc.GetMorphTarget();
		Mesh* pTargetMesh = &((TriObject*)(pTargetNode->GetObjectRef()))->mesh;

		std::map<int, Point3> normalMap;
		normalMap.clear();

		Face* pFace = pBaseMesh->faces;
		for (int f = 0; f < pBaseMesh->numFaces; f++, pFace++) {
			DWORD v0 = pFace->v[0];
			DWORD v1 = pFace->v[1];
			DWORD v2 = pFace->v[2];

			Point3 pt0 = mc.GetMorphPoint(v0);
			Point3 pt1 = mc.GetMorphPoint(v1);
			Point3 pt2 = mc.GetMorphPoint(v2);

			Face* pTargetFace = pTargetMesh->faces;
			for (int tf = 0; tf < pTargetMesh->numFaces; tf++, pTargetFace++) {
				DWORD tv0 = pTargetFace->v[0];
				DWORD tv1 = pTargetFace->v[1];
				DWORD tv2 = pTargetFace->v[2];

				Point3 p0 = pTargetMesh->getVert(tv0);
				Point3 p1 = pTargetMesh->getVert(tv1);
				Point3 p2 = pTargetMesh->getVert(tv2);

				Point3 pp = (pt0 - p0) + (pt1 - p1) + (pt2 - p2);
				if (pp.LengthSquared() < EPS) {
					Point3& nrm0 = pBaseNrmSpec->GetNormal(f, 0);
					Point3& nrm1 = pBaseNrmSpec->GetNormal(f, 1);
					Point3& nrm2 = pBaseNrmSpec->GetNormal(f, 2);

					Point3& tNrm0 = VertNormalTable->at(tv0);
					Point3& tNrm1 = VertNormalTable->at(tv1);
					Point3& tNrm2 = VertNormalTable->at(tv2);
					normalMap[f * 10 + 0] = tNrm0 + nrm0;
					normalMap[f * 10 + 1] = tNrm1 + nrm1;
					normalMap[f * 10 + 2] = tNrm2 + nrm2;

					break;
				}
			}
		}

		//----------------------------------------
		pTargetMesh->SpecifyNormals();
		MeshNormalSpec* pTargetNrmSpec = pTargetMesh->GetSpecifiedNormals();
		pTargetNrmSpec->SetNumFaces(pTargetMesh->numFaces);
		pTargetNrmSpec->SetNumNormals((int)VertNormalTable->size());

		for (auto item : normalMap) {
			int faceID = (int)(item.first / 10);
			int v = item.first - (faceID * 10);
			pTargetNrmSpec->SetNormal(faceID, v, item.second);
		}
		pTargetNrmSpec->SetAllExplicit(TRUE);

		VertNormalTable++;

		//if (deleteIt) delete pTri;
	}
}
//======================================================================
//======================================================================
int glTFImporter_Core::SetMorphChannelNameTable(cgltf_mesh* mesh, std::vector<tstring> &morphTargetTbl)
{
	morphTargetTbl.clear();

	if (!mesh) return 0;

	cgltf_size size;
	cgltf_result ret = cgltf_copy_extras_json(m_glTF_data, &mesh->extras, NULL, &size);
	if (size == 0) return 0;

	CreateTargetListFromExtras(mesh->extras, size, morphTargetTbl);

	return 0;
}

