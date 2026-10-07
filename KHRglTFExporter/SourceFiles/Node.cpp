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
#include <ilayer.h>
#include <CS\BIPEXP.H>
#include <CATAPI\CATClassID.H>
#include <maxscript/maxscript.h>


//----------------------------------------------------------
//----------------------------------------------------------
Matrix3 GetOffsetTM(INode* pNode)
{
	Matrix3 offsetTM;
	Point3 pos = pNode->GetObjOffsetPos();
	offsetTM.PreTranslate(pos);
	Quat quat = pNode->GetObjOffsetRot();
	PreRotateMatrix(offsetTM, quat);
	ScaleValue scaleValue = pNode->GetObjOffsetScale();
	ApplyScaling(offsetTM, scaleValue);
	return (offsetTM);
}

//----------------------------------------------------------
//----------------------------------------------------------
BOOL glTFExporter_Core::MapUsed(Mtl *pMtl)
{
	if (!pMtl) return FALSE;
	if (CheckIfTextureIsUsed(pMtl)) return TRUE;

	int cnt = pMtl->NumSubTexmaps();
	for (int i = 0; i < cnt; i++) {
		if (pMtl->GetSubTexmap(i)) return TRUE;
	}
	if (pMtl->NumSubMtls() == 0) return FALSE;
	return TRUE;
/*
	if (!pMesh->mapSupport(mapCh)) return FALSE;
	MeshMap* pMap = &pMesh->Map(mapCh);

	int cnt = pMap->getNumVerts();
	if (cnt == 0) return FALSE;
	UVVert* pSrcUV = pMap->tv;
	for (int i = 0; i < cnt; i++) {
		if (pSrcUV->x != 0.0f || pSrcUV->y != 0.0f) return TRUE;
		pSrcUV++;
	}
*/
	return FALSE;
}

//----------------------------------------------------------
//----------------------------------------------------------
void glTFExporter_Core::CreateSceneData(tinygltf::Scene &scene, int XRefIdx, ILayer* pLayer)
{
	if (XRefIdx>=0) {
		INode* pRootNode = GetCOREInterface()->GetRootNode();
		INode* pXRefRootNode = pRootNode->GetXRefTree(XRefIdx);
		int numRootnode = pXRefRootNode->NumChildren();
		for (int i = 0; i < numRootnode; i++) {
			INode* pNode = pXRefRootNode->GetChildNode(i);
			if (pLayer) {
				if (pLayer != (ILayer*)pNode->GetReference(NODE_LAYER_REF)) continue;
			}
			if (std::find(m_morphTargetTable.begin(), m_morphTargetTable.end(), pNode) == m_morphTargetTable.end()) {
				tinygltf::Node node = CreateNodeDataRec(pNode);
				
				if(node.extensions_json_string != "EXT_mesh_gpu_instancing") {
					scene.nodes.push_back(m_model.nodes.size() - 1);
				}
			}
		}

		return;
	}

	if (exportSelected) {
		int numNode = GetCOREInterface()->GetSelNodeCount();
		for (int i = 0; i < numNode; i++) {
			INode* pNode = GetCOREInterface()->GetSelNode(i);
			if (std::find(m_morphTargetTable.begin(), m_morphTargetTable.end(), pNode) == m_morphTargetTable.end()) {
				tinygltf::Node node = CreateNodeDataRec(pNode, FALSE);
				
				if (node.extensions_json_string != "EXT_mesh_gpu_instancing") {
					scene.nodes.push_back(m_model.nodes.size() - 1);

					if (node.extensions_json_string == "KHR_physics_rigid_bodies") {
						CreateMeshDataNode(node);
					}
				}
			}
		}
	}
	else {
		int numRootnode = GetCOREInterface()->GetRootNode()->NumChildren();
		for (int i = 0; i < numRootnode; i++) {
			INode* pNode = GetCOREInterface()->GetRootNode()->GetChildNode(i);
			
			// skip nodes within the interactive layer.
			//if (m_pInteractiveGraphLayer == (ILayer*)pNode->GetReference(NODE_LAYER_REF)) continue;
			if (std::find(m_InteractiveLayerTable.begin(), m_InteractiveLayerTable.end(), (ILayer*)pNode->GetReference(NODE_LAYER_REF)) != m_InteractiveLayerTable.end()) 
				continue;

			if (pLayer) {
				if (pLayer != (ILayer*)pNode->GetReference(NODE_LAYER_REF)) continue;
			}
			if (std::find(m_morphTargetTable.begin(), m_morphTargetTable.end(), pNode) == m_morphTargetTable.end()) {
				tinygltf::Node node = CreateNodeDataRec(pNode);
				if (node.extensions_json_string == "EXT_mesh_gpu_instancing") {
				}
				else {
					scene.nodes.push_back(m_model.nodes.size() - 1);

					if (node.extensions_json_string == "KHR_physics_rigid_bodies") {
						CreateMeshDataNode(node);
					}
				}
			}
		}
		if(XRefIdx != Ignore_XRefScene){
			INode* pRootNode = GetCOREInterface()->GetRootNode();
			int numXref = pRootNode->GetXRefFileCount();
			for (int i = 0; i < numXref; i++) {
				if (pRootNode->GetXRefFlags(i) & XREF_HIDDEN) continue;

				INode* pXRefRootNode = pRootNode->GetXRefTree(i);
				for (int j = 0; j < pXRefRootNode->NumChildren(); j++) {
					INode* pNode = pXRefRootNode->GetChildNode(j);
					tinygltf::Node node = CreateNodeDataRec(pNode);
					scene.nodes.push_back(m_model.nodes.size() - 1);
					if (node.extensions_json_string == "KHR_physics_rigid_bodies") {
						CreateMeshDataNode(node);
					}
				}
			}
		}
	}

	if(m_GPUInstance)
		ExportGPUInstanceSection(scene);
}


//----------------------------------------------------------
//----------------------------------------------------------
tinygltf::Node glTFExporter_Core::CreateNodeDataRec(INode *pNode, BOOL recursive)
{
	BOOL gpuInstanceMode = 0;

	tinygltf::Node node;// = Create_glTFNode(pNode);

	LogOutput(tstring(_T("Node:")) + tstring(pNode->GetName()));

	SetName(&node, pNode->GetName());
	node.name = WStringToString(pNode->GetName());

	{
		VisibilityStruct str;
		if (SetVisibilityParams(pNode, str)) {
			CreateVisibilityNode(node, str, TRUE);
		}
	}
	{
		SelectabilityStruct str;
		if (SetSelectabilityParams(pNode, str)) {
			CreateSelectabilityNode(node, str, TRUE);
		}
	}
	{
		HoverabilityStruct str;
		if (SetHoverabilityParams(pNode, str)) {
			CreateHoverabilityNode(node, str, TRUE);
		}
	}


	Object *pObj = pNode->GetObjectRef();
	if (pObj->SuperClassID() == CAMERA_CLASS_ID) {
		CreateCamera(pNode);
		node.camera = m_model.cameras.size() - 1;
	}
	else if (pObj->SuperClassID() == LIGHT_CLASS_ID) {
		CreateLight(pNode);
		tinygltf::Value::Object obj;
		obj.insert(std::make_pair("light", tinygltf::Value((int)m_model.lights.size() - 1)));
		node.extensions.insert(std::make_pair("KHR_lights_punctual", obj));
	}
	else if ((pObj->ClassID() == BONE_OBJ_CLASSID) ||
		(pObj->ClassID() == SKELOBJ_CLASS_ID) ||
		(pObj->ClassID() == CATPARENT_CLASS_ID) ||
		(pObj->ClassID() == MUSCLESTRAND_CLASS_ID) ||
		(pObj->ClassID() == MUSCLEBONES_CLASS_ID)) {
	}
	else if(!pNode->IsNodeHidden()) {
		if (m_ResetXFormMod) {
			AffineParts af;
			decomp_affine(pNode->GetNodeTM(m_time), &af);
			if (af.f < 0) {
				GetCOREInterface()->SelectNode(pNode, 1);
#if MAX_RELEASE >= 24000
				ExecuteMAXScriptScript(_T("ResetXForm $"), MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
				ExecuteMAXScriptScript(_T("ResetXForm $"), TRUE);
#endif
			}
		}
		if ((m_MeshMap.find(pObj) != m_MeshMap.end()) && (m_Instancing)) {
			if (m_InstanceWithMtl) {
				node.mesh = CreateInstanceMeshWithMtl(pNode);
			}
			else {
				node.mesh = m_MeshMap[pObj];
			}
			if (m_GPUInstance) {
				auto m = m_GPUInstanceMap[pObj];
				m.push_back(pNode);
				m_GPUInstanceMap[pObj] = m;
				gpuInstanceMode = 2;
			}
		}
		else {
			BOOL deleteIt = FALSE;
			TriObject* pTri = GetTriObjectFromNode(pNode, m_time, deleteIt);
			BOOL MeshFound = (pTri != NULL);
			if (deleteIt) delete pTri;
			if (MeshFound) {// &&(pObj->SuperClassID()!= SHAPE_CLASS_ID))
				if (1) {
					ExCreateMeshData(pNode, node);
				}
				else {
					//CreateMeshData(pNode, node);
				}
				if (m_GPUInstance) {
					if (IsInstanced(pNode)) {
						std::vector<INode*> v;
						v.push_back(pNode);
						m_GPUInstanceMap.insert(std::make_pair(pObj,v));
						m_GPUInstanceNodeList.push_back(node);
						gpuInstanceMode = 1;
					}
				}
			}
		}
	}
/*
	//Matrix3 tm(pNode->GetNodeTM(m_time));
	Matrix3 objTM = pNode->GetObjectTM(m_time);
	{
		Matrix3 n = pNode->GetNodeTM(m_time);
		n.SetTranslate(Point3(0, 0, 0));
		objTM.SetTranslate(Point3(0, 0, 0));
		objTM = n * Inverse(objTM);
	}
*/
	Matrix3 tm(pNode->GetObjTMAfterWSM(m_time));
	//tm.SetTrans(tm.GetTrans() * m_scale);
	//tm = tm * Inverse(objTM);
	if (pNode->GetParentNode()->IsRootNode()) {
		if (m_ResetPivotTM)tm =  tm;
		else tm = Inverse(GetOffsetTM(pNode)) * tm;
		tm = tm * YupTM;
	}
	else {
		//Matrix3 ParentOffsetTM = GetOffsetTM(pNode->GetParentNode());
		//tm = tm * Inverse(pNode->GetParentTM(m_time) * ParentOffsetTM);
		if (m_ResetPivotTM)tm = tm;
		else tm = Inverse(GetOffsetTM(pNode)) * tm;
		tm = tm * Inverse(pNode->GetParentTM(m_time));
	}

	if (m_Mesh_quantization_Used) {
		QuantizationInfo quantInfo;
		GetQuatizationInfo(pNode, quantInfo);
		QuantizationInfo parentInfo;
		GetQuatizationInfo(pNode->GetParentNode(), parentInfo);

		AffineParts parts;
		decomp_affine(tm, &parts);
		node.rotation.push_back(parts.q.x);
		node.rotation.push_back(parts.q.y);
		node.rotation.push_back(parts.q.z);
		node.rotation.push_back(-parts.q.w);
		node.translation.push_back(parts.t.x   / parentInfo.meshScale);
		node.translation.push_back(parts.t.y   / parentInfo.meshScale);
		node.translation.push_back(parts.t.z   / parentInfo.meshScale);

		node.scale.push_back(parts.k.x * quantInfo.meshScale   / parentInfo.meshScale);
		node.scale.push_back(parts.k.y * quantInfo.meshScale   / parentInfo.meshScale);
		node.scale.push_back(parts.k.z * quantInfo.meshScale   / parentInfo.meshScale);
	}
	else {
		Control* pC = pNode->GetTMController();
		if (pC->IsAnimated() || m_FullFrame || m_ForceTRSMode) {
			AffineParts parts;
			decomp_affine(tm, &parts);
			node.translation.push_back(parts.t.x * m_scale);
			node.translation.push_back(parts.t.y * m_scale);
			node.translation.push_back(parts.t.z * m_scale);
			node.rotation.push_back(parts.q.x);
			node.rotation.push_back(parts.q.y);
			node.rotation.push_back(parts.q.z);
			node.rotation.push_back(-parts.q.w);
			node.scale.push_back(parts.k.x);
			node.scale.push_back(parts.k.y);
			node.scale.push_back(parts.k.z);
		}
		else {
			Matrix3ToFloat(tm, node.matrix, m_scale);
		}
	}

	{
		tinygltf::Value::Object params;
		params.clear();
		ICustAttribContainer* pContainer = pObj->GetCustAttribContainer();
		if (pContainer) {
			SetCustomAttribute(params, pContainer);
		}

		if (m_ExportUserProp) {
			TSTR buf(_T(""));
			pNode->GetUserPropBuffer(buf);
			SetUserPropString(params, buf);
		}

		if (params.size() > 0)
			node.extras = tinygltf::Value(params);
	}

	if (recursive) {
		int numCh = pNode->NumChildren();
		for (int i = 0; i < numCh; i++) {
			INode* pChNode = pNode->GetChildNode(i);
			tinygltf::Node chnode = CreateNodeDataRec(pChNode);
			node.children.push_back(findNodeIndex(pChNode));
			if (chnode.extensions_json_string == "KHR_physics_rigid_bodies") {
				CreateMeshDataNode(chnode);
			}
		}
	}

	if (m_Collision) {
		CreateCollisionShape(pNode, node);
	}

	if (gpuInstanceMode) {
		node.extensions_json_string = "EXT_mesh_gpu_instancing";
	}
	else {
		m_model.nodes.push_back(node);
		m_NodeMap.insert(std::make_pair(pNode, m_model.nodes.size() - 1));
	}


	return node;
}

//----------------------------------------------------------
//----------------------------------------------------------
void glTFExporter_Core::CreateMtlIDTable(Mesh *pMesh, Mtl *pMtl, std::map<int, std::vector<int> > &mtlIDMap)
{
	mtlIDMap.clear();
	int numf = pMesh->numFaces;

	if (!pMtl) {
	}
	else if (pMtl->ClassID() == multiClassID) {
		int mtlNum = pMtl->NumSubMtls();
		//IParamBlock2 *pBlock = pMtl->GetParamBlock(0);
		Face *pFace = pMesh->faces;
		for (int i = 0; i < numf; i++, pFace++) {
			int mtlID = pFace->getMatID() % mtlNum;
			auto tbl = mtlIDMap[mtlID];
			tbl.push_back(i);
			mtlIDMap[mtlID] = tbl;
		}
		return;
	}

	std::vector<int> tbl;
	for (int i = 0; i < numf; i++) {
		tbl.push_back(i);
	}
	mtlIDMap[-1] = tbl;
	return;
}

//----------------------------------------------------------
//----------------------------------------------------------
BOOL GetTangentTM(Mtl* pMtl, Matrix3& mtx)
{
	//mtx = Matrix3(Point3(1, 0, 0), Point3(0, 0, -1), Point3(0, 1, 0), Point3(0, 0, 0));
	mtx = Matrix3(Point3(1, 0, 0), Point3(0, 1, 0), Point3(0, 0, 1), Point3(0, 0, 0));//+++

	if (!pMtl) return FALSE;

	if (pMtl->ClassID() == glTFMaterialID) {
		return TRUE;
	}

	if (pMtl->ClassID() == PBRMetalMtlID) {
		int fred = pMtl->GetParamBlock(0)->GetInt(pbr_normal_flip_red, 0);
		int fgrn = pMtl->GetParamBlock(0)->GetInt(pbr_normal_flip_green, 0);
		if (fred && fgrn) {
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, -1, 0), Point3(0, 0, 1), Point3(0, 0, 0));
		}
		else if (fred && !fgrn) {
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 0, 1), Point3(0, -1, 0), Point3(0, 0, 0));
		}
		else if (!fred && fgrn) {
			/*
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 0, -1), Point3(0,  1, 0), Point3(0, 0, 0));
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 0, -1), Point3(0, -1, 0), Point3(0, 0, 0));
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 0,  1), Point3(0,  1, 0), Point3(0, 0, 0));
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 0,  1), Point3(0, -1, 0), Point3(0, 0, 0));

			mtx = Matrix3(Point3(-1, 0, 0), Point3(0, 0, -1), Point3(0,  1, 0), Point3(0, 0, 0));
			mtx = Matrix3(Point3(-1, 0, 0), Point3(0, 0, -1), Point3(0, -1, 0), Point3(0, 0, 0));
			mtx = Matrix3(Point3(-1, 0, 0), Point3(0, 0,  1), Point3(0,  1, 0), Point3(0, 0, 0));
			mtx = Matrix3(Point3(-1, 0, 0), Point3(0, 0,  1), Point3(0, -1, 0), Point3(0, 0, 0));

			mtx = Matrix3(Point3(-1, 0, 0), Point3(0, -1, 0), Point3(0, 0, -1), Point3(0, 0, 0));
			mtx = Matrix3(Point3(-1, 0, 0), Point3(0, -1, 0), Point3(0, 0,  1), Point3(0, 0, 0));			mtx = Matrix3(Point3(-1, 0, 0), Point3(0, 1, 0), Point3(0, 0, 1), Point3(0, 0, 0));
			mtx = Matrix3(Point3(-1, 0, 0), Point3(0,  1, 0), Point3(0, 0, -1), Point3(0, 0, 0));
			mtx = Matrix3(Point3(-1, 0, 0), Point3(0,  1, 0), Point3(0, 0,  1), Point3(0, 0, 0));//+++

			mtx = Matrix3(Point3(1, 0, 0), Point3(0,  1, 0), Point3(0, 0,  1), Point3(0, 0, 0));
			mtx = Matrix3(Point3(1, 0, 0), Point3(0,  1, 0), Point3(0, 0, -1), Point3(0, 0, 0));
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, -1, 0), Point3(0, 0, 1), Point3(0, 0, 0));//---
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, -1, 0), Point3(0, 0, -1), Point3(0, 0, 0));
			*/

			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 1, 0), Point3(0, 0, 1), Point3(0, 0, 0));//+++
		}
		else if (!fred && !fgrn) {
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 1, 0), Point3(0, 0, 1), Point3(0, 0, 0));
		}
		return TRUE;
	}

	Texmap* pNrmTex = NULL;
	if (pMtl->ClassID() == PHYSICALMATERIAL_CLASS_ID) {
		Texmap* pTex1 = pMtl->GetParamBlock(0)->GetTexmap(fm_bump_map);
		if (pTex1) {
			if (pTex1->ClassID() == NormalBumpMapClassID) pNrmTex = pTex1;
		}
	}
	else if (pMtl->ClassID() == ScanLineMtlID) {
		Texmap* pTex1 = pMtl->GetSubTexmap(ID_BU);
		if (pTex1) {
			if (pTex1->ClassID() == NormalBumpMapClassID) pNrmTex = pTex1;
		}
	}

	if (pNrmTex) {
		int fred = pNrmTex->GetParamBlock(0)->GetInt(7, 0);
		int fgrn = pNrmTex->GetParamBlock(0)->GetInt(8, 0);
		if (fred && fgrn) {
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, -1, 0), Point3(0, 0, 1), Point3(0, 0, 0));
		}
		else if (fred && !fgrn) {
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 0, 1), Point3(0, -1, 0), Point3(0, 0, 0));
		}
		else if (!fred && fgrn) {
			//mtx = Matrix3(Point3(1, 0, 0), Point3(0, 0, -1), Point3(0, 1, 0), Point3(0, 0, 0));
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 1, 0), Point3(0, 0, 1), Point3(0, 0, 0));//+++
		}
		else if (!fred && !fgrn) {
			mtx = Matrix3(Point3(1, 0, 0), Point3(0, 1, 0), Point3(0, 0, 1), Point3(0, 0, 0));
		}
		return TRUE;
	}

	return FALSE;
}
#if 0
//----------------------------------------------------------
//----------------------------------------------------------
void glTFExporter_Core::CreateMeshData(INode *pNode, tinygltf::Node &node)
{
	Mesh ShapeMesh;
	Mesh* pMesh = NULL;
	BezierShape* pShape = NULL;

	{
		ObjectState os = pNode->EvalWorldState(0);
		if (os.obj->SuperClassID() == SHAPE_CLASS_ID) {
			if (m_ExportShapeObj==0) return;
			else if (m_ExportShapeObj == 1) {
				CreateShapeData(pNode, node);
				return;
			}
			else {
				ShapeObject* pShapeObj = (ShapeObject*)os.obj;
				if (pShapeObj->GetRenderable()) {
					pShapeObj->GenerateMesh(m_time, GENMESH_RENDER, &ShapeMesh);
					pMesh = &ShapeMesh;
					pShape = &((SplineShape*)os.obj)->shape;
				}
				else return;
			}
		}
	}

	IGameNode* pGameNode = NULL;
	IGameObject* pGameObject = NULL;
	IGameMesh* pGameMesh = NULL;
	if (m_ExportTangent) {
		pGameNode = m_pIGameScene->GetIGameNode(pNode);
		pGameObject = pGameNode->GetIGameObject();
		if (pGameObject->GetIGameType() == IGameObject::IGAME_MESH) {
			pGameMesh = (IGameMesh*)pGameObject;
			pGameMesh->InitializeData();
			pGameMesh->InitializeBinormalData();
			Tab<int> mapTbl = pGameMesh->GetActiveMapChannelNum();
		}
	}

	BOOL SkinMod = FALSE;
	Modifier *pSkinMod = NULL;
	ISkin *pISkin = NULL;
	int SkinTopBoneID = 0;
	//ISkinImportData *pSkinImp = NULL;
	ISkinContextData *pSkinMC = NULL;
	if (FindModifier(pNode, SKIN_CLASSID, &pSkinMod) >= 0) {
		SkinMod = TRUE;
		//pSkinImp = (ISkinImportData *)pSkinMod->GetInterface(I_SKINIMPORTDATA);
		pISkin = (ISkin *)pSkinMod->GetInterface(I_SKIN);
		pSkinMC = pISkin->GetContextInterface(pNode);
		SkinTopBoneID = GetRootNodeBoneID(pISkin);

		m_skinNodeTable.insert(std::make_pair(pNode, pSkinMod));
		pSkinMod->DisableMod();
		pSkinMod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

	Modifier* pMorphMod = m_morphNodeTable[pNode];
	if (pMorphMod) {
		pMorphMod->DisableMod();
		pMorphMod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

	BOOL deleteIt = FALSE;
	TriObject* pTri = NULL;
	if (!pMesh) {
		pTri = GetTriObjectFromNode(pNode, m_time, deleteIt);
		if (!pTri) {
			node.mesh = -1;
			return;
		}
		pMesh = &pTri->mesh;
	}

	pMesh->SpecifyNormals();
	MeshNormalSpec* pNrmSpec = pMesh->GetSpecifiedNormals();
	pNrmSpec->BuildNormals();
	pNrmSpec->ComputeNormals();

	std::vector<std::map<int, Point3> > morphNormalMapList;
	if (pMorphMod && m_ExportMorphNrm) {
		CreateMorphVertMapTable(pMorphMod, pMesh, pNrmSpec, morphNormalMapList);
	}

	BOOL CVertMode = pNode->GetCVertMode();
	//pNode->SetShadeCVerts(TRUE);

	int mapCh = 1;


	//tinygltf::Material *material = 0;
	Mtl *pMtl = pNode->GetMtl();
	if (pMtl && pMtl->ClassID() == ShellMaterialID) pMtl = GetSubMtlFromShellMtl(pMtl);

	if (!pMtl && m_WireClrToMtl) {
		if(IsGeometryObject(pNode))
			pMtl = m_WireColorMtlMap[pNode->GetWireColor()];
	}
	BOOL mapCh1Used = MapUsed(pMtl) && pMesh->mapSupport(1);
	BOOL mapCh2Used = MapUsed(pMtl) && pMesh->mapSupport(2);
	if (mapCh1Used) { mapCh1Used = (pMesh->Map(1).tv != 0); }
	if (mapCh2Used) { mapCh2Used = (pMesh->Map(2).tv != 0); }

	Matrix3 TangentTM(1);
	GetTangentTM(pMtl, TangentTM);

	tinygltf::Mesh mesh;// = Create_glTFMesh(pNode->GetName());
	SetName(&mesh, pNode->GetName());
	mesh.name = WStringToString(pNode->GetName());

	Matrix3 TangentOffsetTM(1);
	{
		TangentOffsetTM = pNode->GetNodeTM(m_time);
		TangentOffsetTM.SetTrans(Point3(0,0,0));
		TangentOffsetTM = Inverse(TangentOffsetTM);
	}

	int VariantCount = 0;
	BOOL VariantMtl = IsVariantMtl(pMtl, VariantCount);

	//for (int varCnt = 0; varCnt < 1; varCnt++) {

		std::map<int, std::vector<int> > mtlIDMap;
		if (VariantMtl) {
			Mtl* pSubMtl = pMtl->GetSubMtl(0);
			CreateMtlIDTable(pMesh, pSubMtl, mtlIDMap);
		}
		else {
			CreateMtlIDTable(pMesh, pMtl, mtlIDMap);
		}
		int subMtlsCount = mtlIDMap.size();

		Face* pFace = pMesh->faces;
		int primitiveId = 0;
		for (auto mapTbl : mtlIDMap) {
			int AttributeIndex = 0;
			int mtlID = mapTbl.first;
			std::vector<int> faceIDTable = mapTbl.second;
			if (faceIDTable.size() == 0) continue;

			tinygltf::Primitive primitive;
			primitive.mode = TINYGLTF_MODE_TRIANGLES;

			int mtlIdx = -1;
			if (mtlIDMap.size() == 1) {
				if (pMtl) {
					if (pMtl->ClassID() == multiClassID) {
						pMtl = pMtl->GetSubMtl(mapTbl.first);
					}
				}
				mtlIdx = findMaterialIndex(pMtl);
			}
			else if (mtlIDMap.size() > 1) {
				if(VariantMtl)
					mtlIdx = findMaterialIndex(pMtl->GetSubMtl(0)->GetSubMtl(mtlID));
				else
					mtlIdx = findMaterialIndex(pMtl->GetSubMtl(mtlID));
			}
			primitive.material = mtlIdx;

			Matrix3 OffsetTM(1);
			if(!m_ResetPivotTM) OffsetTM = GetOffsetTM(pNode);

			std::vector<Point3> vertTable;
			//----------- Create Vertex table
			{
				vertTable.clear();
				for (auto id : faceIDTable) {
					DWORD* v = pFace[id].v;
					for (int pp = 0; pp < 3; pp++) {
						Point3 p = pMesh->verts[v[pp]];
						vertTable.push_back(p);
						//for (auto ptr : vertTable) {
						//	if (ptr == p) continue;
						//	break;
						//}
					}
				}
			}

			if (m_DracoCompress) {
				//CreateDracoMeshProp(primitive, pMesh, pNrmSpec, vertTable, faceIDTable, CVertMode, pMtl, pSkinMC, pMorphMod, pGameMesh, OffsetTM);
				//GetMeshInfoXX(pMesh);
			}
			else {
				//----------- Index
				{
					tinygltf::Accessor acc;// = Create_glTFAccessor();
					acc.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
					acc.type = TINYGLTF_TYPE_SCALAR;
					acc.count = faceIDTable.size() * 3;
					tinygltf::BufferView bfView;// = Create_glTFBufferView();
					bfView.buffer = 0;
					bfView.byteOffset = m_BufferByteOffset;
					bfView.byteLength = acc.count * sizeof(UINT);
					bfView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

					void* ptr = SecureMemory(bfView.byteLength);
					UINT* pShortIdx = (UINT*)((char*)ptr + bfView.byteOffset);

					Face* pFace = pMesh->faces;
					int faceIdx = 0;
					for (auto id : faceIDTable) {
						*pShortIdx++ = faceIdx++;
						*pShortIdx++ = faceIdx++;
						*pShortIdx++ = faceIdx++;
					}

					m_model.bufferViews.push_back(bfView);
					acc.bufferView = m_model.bufferViews.size() - 1;
					m_model.accessors.push_back(acc);
					primitive.indices = m_model.accessors.size() - 1;
				}

				//----------- Position
				{
					tinygltf::Accessor acc;// = Create_glTFAccessor();
					acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
					acc.type = TINYGLTF_TYPE_VEC3;
					acc.count = faceIDTable.size() * 3;
					tinygltf::BufferView bfView;// = Create_glTFBufferView();
					bfView.buffer = 0;
					bfView.byteOffset = m_BufferByteOffset;
					bfView.byteLength = acc.count * sizeof(float) * 3;
					bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

					//pFace = pMesh->faces;
					Point3 minPos = pMesh->verts[pFace[faceIDTable[0]].v[0]] * OffsetTM;
					Point3 maxPos = pMesh->verts[pFace[faceIDTable[0]].v[0]] * OffsetTM;

					void* ptr = SecureMemory(bfView.byteLength);
					float* pPos = (float*)((char*)ptr + bfView.byteOffset);
					for (auto id : faceIDTable) {
						DWORD* v = pFace[id].v;
						for (int pp = 0; pp < 3; pp++) {
							Point3 p = pMesh->verts[v[pp]];
							p = p * OffsetTM;
							if (p.x > maxPos.x)	maxPos.x = p.x;
							if (p.y > maxPos.y)	maxPos.y = p.y;
							if (p.z > maxPos.z)	maxPos.z = p.z;
							if (p.x < minPos.x)	minPos.x = p.x;
							if (p.y < minPos.y)	minPos.y = p.y;
							if (p.z < minPos.z)	minPos.z = p.z;

							*pPos++ = p.x;
							*pPos++ = p.y;
							*pPos++ = p.z;
						}
					}

					acc.maxValues.push_back(maxPos.x);
					acc.maxValues.push_back(maxPos.y);
					acc.maxValues.push_back(maxPos.z);
					acc.minValues.push_back(minPos.x);
					acc.minValues.push_back(minPos.y);
					acc.minValues.push_back(minPos.z);

					m_model.bufferViews.push_back(bfView);
					acc.bufferView = m_model.bufferViews.size() - 1;
					m_model.accessors.push_back(acc);
					primitive.attributes.insert(std::make_pair("POSITION", m_model.accessors.size() - 1));
				}

				//----------- Normal
				{
					tinygltf::Accessor acc;// = Create_glTFAccessor();
					acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
					acc.type = TINYGLTF_TYPE_VEC3;
					acc.count = faceIDTable.size() * 3;
					tinygltf::BufferView bfView;// = Create_glTFBufferView();

					bfView.buffer = 0;
					bfView.byteOffset = m_BufferByteOffset;
					bfView.byteLength = acc.count * sizeof(float) * 3;
					bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

					void* ptr = SecureMemory(bfView.byteLength);
					float* pNrm = (float*)((char*)ptr + bfView.byteOffset);
					for (auto id : faceIDTable) {
						MeshNormalFace& face = pNrmSpec->Face(id);
						//DWORD  *v = pFace->v;
						for (int j = 0; j < 3; j++) {
							//int nID = face.GetNormalID(j);
							//Point3 nn = pNrmSpec->Normal(nID);
							Point3& nrm = pNrmSpec->GetNormal(id, j);

							*pNrm++ = nrm.x;
							*pNrm++ = nrm.y;
							*pNrm++ = nrm.z;
						}
					}

					m_model.bufferViews.push_back(bfView);
					acc.bufferView = m_model.bufferViews.size() - 1;
					m_model.accessors.push_back(acc);
					primitive.attributes.insert(std::make_pair("NORMAL", m_model.accessors.size() - 1));
				}

				//----------- Tangent
				if (pGameMesh) {
					Matrix3 RotationTM = Matrix3(Point3(-1, 0, 0), Point3(0, 1, 0), Point3(0, 0, 1), Point3(0, 0, 0));
					mapCh = 1;

					tinygltf::Accessor acc;// = Create_glTFAccessor();
					acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
					acc.type = TINYGLTF_TYPE_VEC4;
					acc.count = faceIDTable.size() * 3;
					tinygltf::BufferView bfView;// = Create_glTFBufferView();

					bfView.buffer = 0;
					bfView.byteOffset = m_BufferByteOffset;
					bfView.byteLength = acc.count * sizeof(float) * 4;
					bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

					void* ptr = SecureMemory(bfView.byteLength);
					float* pTan = (float*)((char*)ptr + bfView.byteOffset);
					for (auto faceID : faceIDTable) {
						for (int pp = 0; pp < 3; pp++) {
							Point3 normal = pGameMesh->GetNormal(faceID, pp, TRUE);
							//normal = normal * Inverse(pNode->GetNodeTM(m_time));
							//normal = normal * TangentOffsetTM;
							//normal.FNormalize();

							int indexTangentBinormal = pGameMesh->GetFaceVertexTangentBinormal(faceID, pp, mapCh);
							Point3 tangent = pGameMesh->GetTangent(indexTangentBinormal, mapCh);
							tangent = tangent * TangentOffsetTM;
							tangent.FNormalize();

							Point3 bitangent = pGameMesh->GetBinormal(indexTangentBinormal, mapCh);
							bitangent = bitangent * TangentOffsetTM;
							bitangent.FNormalize();

							float w = GetW(normal, tangent, bitangent);

							*pTan++ = tangent.x;
							*pTan++ = tangent.y;
							*pTan++ = tangent.z;
							*pTan++ = w;
						}
					}

					m_model.bufferViews.push_back(bfView);
					acc.bufferView = m_model.bufferViews.size() - 1;
					m_model.accessors.push_back(acc);
					primitive.attributes.insert(std::make_pair("TANGENT", m_model.accessors.size() - 1));
				}

				//----------- UV1 map
				mapCh = 1;
				//if (pMesh->mapSupport(mapCh) && pMtl) {
				if (mapCh1Used) {
					tinygltf::Accessor acc;// = Create_glTFAccessor();
					acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
					acc.type = TINYGLTF_TYPE_VEC2;
					acc.count = faceIDTable.size() * 3;
					tinygltf::BufferView bfView;// = Create_glTFBufferView();

					bfView.buffer = 0;
					bfView.byteOffset = m_BufferByteOffset;
					bfView.byteLength = acc.count * sizeof(float) * 2;
					bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

					MeshMap* pMap = &pMesh->Map(mapCh);
					UVVert* pSrcUV = pMap->tv;
					TVFace* pTVFace = pMesh->mapFaces(mapCh);
					//Point3 minUV = *pSrcUV * Point3(1.0f, -1.0f, 0.0f);
					//Point3 maxUV = *pSrcUV * Point3(1.0f, -1.0f, 0.0f);
					Point3 minUV = pSrcUV[pTVFace[faceIDTable[0]].t[0]];
					minUV.y = 1.0f - minUV.y;
					Point3 maxUV = minUV;

					void* ptr = SecureMemory(bfView.byteLength);
					float* pTexUV = (float*)((char*)ptr + bfView.byteOffset);
					for (auto id : faceIDTable) {
						DWORD* v = pTVFace[id].t;
						for (int pp = 0; pp < 3; pp++) {
							UVVert p = pSrcUV[v[pp]];
							p.y = -p.y + 1.0f;
							if (p.x > maxUV.x)	maxUV.x = p.x;
							if (p.y > maxUV.y)	maxUV.y = p.y;
							if (p.x < minUV.x)	minUV.x = p.x;
							if (p.y < minUV.y)	minUV.y = p.y;

							*pTexUV++ = p.x;
							*pTexUV++ = p.y;
						}
					}

					acc.maxValues.push_back(maxUV.x);
					acc.maxValues.push_back(maxUV.y);
					acc.minValues.push_back(minUV.x);
					acc.minValues.push_back(minUV.y);

					m_model.bufferViews.push_back(bfView);
					acc.bufferView = m_model.bufferViews.size() - 1;
					m_model.accessors.push_back(acc);
					primitive.attributes.insert(std::make_pair("TEXCOORD_0", m_model.accessors.size() - 1));
				}

				//----------- UV2 map
				mapCh = 2;
				if (pMesh->mapSupport(mapCh) && pMtl) {
					tinygltf::Accessor acc;// = Create_glTFAccessor();
					acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
					acc.type = TINYGLTF_TYPE_VEC2;
					acc.count = faceIDTable.size() * 3;
					tinygltf::BufferView bfView;// = Create_glTFBufferView();

					bfView.buffer = 0;
					bfView.byteOffset = m_BufferByteOffset;
					bfView.byteLength = acc.count * sizeof(float) * 2;
					bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

					MeshMap* pMap = &pMesh->Map(mapCh);
					UVVert* pSrcUV = pMap->tv;
					TVFace* pTVFace = pMesh->mapFaces(mapCh);
					//Point3 minUV = *pSrcUV * Point3(1.0f, -1.0f, 0.0f);
					//Point3 maxUV = *pSrcUV * Point3(1.0f, -1.0f, 0.0f);
					Point3 minUV = pSrcUV[pTVFace[faceIDTable[0]].t[0]];
					minUV.y = 1.0f - minUV.y;
					Point3 maxUV = minUV;

					void* ptr = SecureMemory(bfView.byteLength);
					float* pTexUV = (float*)((char*)ptr + bfView.byteOffset);
					for (auto id : faceIDTable) {
						DWORD* v = pTVFace[id].t;
						for (int pp = 0; pp < 3; pp++) {
							UVVert p = pSrcUV[v[pp]];
							p.y = -p.y + 1.0f;
							if (p.x > maxUV.x)	maxUV.x = p.x;
							if (p.y > maxUV.y)	maxUV.y = p.y;
							if (p.x < minUV.x)	minUV.x = p.x;
							if (p.y < minUV.y)	minUV.y = p.y;

							*pTexUV++ = p.x;
							*pTexUV++ = p.y;
						}
					}

					acc.maxValues.push_back(maxUV.x);
					acc.maxValues.push_back(maxUV.y);
					acc.minValues.push_back(minUV.x);
					acc.minValues.push_back(minUV.y);

					m_model.bufferViews.push_back(bfView);
					acc.bufferView = m_model.bufferViews.size() - 1;
					m_model.accessors.push_back(acc);
					primitive.attributes.insert(std::make_pair("TEXCOORD_1", m_model.accessors.size() - 1));
				}


				//----------- Vertex Color
				if (CVertMode && pMesh->Map(0).tv) {
					tinygltf::Accessor acc;// = Create_glTFAccessor();
					acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
					acc.type = TINYGLTF_TYPE_VEC3;
					acc.count = faceIDTable.size() * 3;
					tinygltf::BufferView bfView;// = Create_glTFBufferView();

					bfView.buffer = 0;
					bfView.byteOffset = m_BufferByteOffset;
					bfView.byteLength = acc.count * sizeof(float) * 3;
					bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

					MeshMap* pMap = &pMesh->Map(0);
					UVVert* pSrcUV = pMap->tv;
					TVFace* pTVFace = pMesh->mapFaces(0);
					Point3 minUV = pSrcUV[pTVFace[faceIDTable[0]].t[0]];
					Point3 maxUV = minUV;

					void* ptr = SecureMemory(bfView.byteLength);
					float* pTexUV = (float*)((char*)ptr + bfView.byteOffset);
					for (auto id : faceIDTable) {
						DWORD* v = pTVFace[id].t;
						for (int pp = 0; pp < 3; pp++) {
							UVVert p = pSrcUV[v[pp]];
							if (p.x > maxUV.x)	maxUV.x = p.x;
							if (p.y > maxUV.y)	maxUV.y = p.y;
							if (p.z > maxUV.z)	maxUV.z = p.z;
							if (p.x < minUV.x)	minUV.x = p.x;
							if (p.y < minUV.y)	minUV.y = p.y;
							if (p.z < minUV.z)	minUV.z = p.z;

							*pTexUV++ = p.x;
							*pTexUV++ = p.y;
							*pTexUV++ = p.z;
						}
					}

					acc.maxValues.push_back(maxUV.x);
					acc.maxValues.push_back(maxUV.y);
					acc.maxValues.push_back(maxUV.z);
					acc.minValues.push_back(minUV.x);
					acc.minValues.push_back(minUV.y);
					acc.minValues.push_back(minUV.z);

					m_model.bufferViews.push_back(bfView);
					acc.bufferView = m_model.bufferViews.size() - 1;
					m_model.accessors.push_back(acc);
					primitive.attributes.insert(std::make_pair("COLOR_0", m_model.accessors.size() - 1));
				}

				//----------- Skin Joint/Weight
				if (SkinMod) {
					std::vector<std::array<float, 4> > wTable;
					std::vector<std::array<UINT, 4> > bTable;
					wTable.clear();
					bTable.clear();
					//wTable.reserve(faceIDTable.size() * 3);
					//bTable.reserve(faceIDTable.size() * 3);
					for (auto id : faceIDTable) {
						DWORD* v = pFace[id].v;
						for (int pp = 0; pp < 3; pp++) {
							std::array<float, 4> wa{ 0.0f, 0.0f, 0.0f, 0.0f };
							std::array<UINT, 4> ba{ 0, 0, 0, 0 };
							int numb = pSkinMC->GetNumAssignedBones(v[pp]);
							if (numb == 0) {
								wa[0] = 1.0f;
								ba[0] = SkinTopBoneID;
							}
							else {
								if (numb > 4) {
									numb = 4;
									m_IncorrectSkinDataFound = TRUE;
								}
								for (int j = 0; j < numb; j++) {
									int boneIdx = pSkinMC->GetAssignedBone(v[pp], j);
									float w = pSkinMC->GetBoneWeight(v[pp], j);
									wa[j] = w;
									//INode *pBone = pISkin->GetBone(boneIdx);
									ba[j] = boneIdx;
								}
							}
							wTable.push_back(wa);
							bTable.push_back(ba);
						}
					}

					//---------------------------------
					{
						tinygltf::Accessor acc;// = Create_glTFAccessor();
						acc.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
						acc.type = TINYGLTF_TYPE_VEC4;
						acc.count = bTable.size();
						tinygltf::BufferView bfView;// = Create_glTFBufferView();

						bfView.buffer = 0;
						bfView.byteOffset = m_BufferByteOffset;
						bfView.byteLength = acc.count * sizeof(USHORT) * 4;
						bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

						void* ptr = SecureMemory(bfView.byteLength);
						USHORT* pBone = (USHORT*)((char*)ptr + bfView.byteOffset);
						for (auto b : bTable) {
							*pBone++ = b[0];
							*pBone++ = b[1];
							*pBone++ = b[2];
							*pBone++ = b[3];
						}
						m_model.bufferViews.push_back(bfView);
						acc.bufferView = m_model.bufferViews.size() - 1;
						m_model.accessors.push_back(acc);
						primitive.attributes.insert(std::make_pair("JOINTS_0", m_model.accessors.size() - 1));
					}

					//---------------------------------
					{
						tinygltf::Accessor acc;// = Create_glTFAccessor();
						acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
						acc.type = TINYGLTF_TYPE_VEC4;
						acc.count = wTable.size();
						tinygltf::BufferView bfView;// = Create_glTFBufferView();

						bfView.buffer = 0;
						bfView.byteOffset = m_BufferByteOffset;
						bfView.byteLength = acc.count * sizeof(float) * 4;
						bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

						void* ptr = SecureMemory(bfView.byteLength);
						float* pWeight = (float*)((char*)ptr + bfView.byteOffset);
						for (auto w : wTable) {
							*pWeight++ = w[0];
							*pWeight++ = w[1];
							*pWeight++ = w[2];
							*pWeight++ = w[3];
						}
						m_model.bufferViews.push_back(bfView);
						acc.bufferView = m_model.bufferViews.size() - 1;
						m_model.accessors.push_back(acc);
						primitive.attributes.insert(std::make_pair("WEIGHTS_0", m_model.accessors.size() - 1));
					}
				}

				//-------- Morph
				if (pMorphMod) {
#if 0
					CreateMorphPrimiteve(primitive, pMesh, pMorphMod, faceIDTable);
#else
					MaxMorphModifier maxMorphModifier(pMorphMod);
					auto normalMap = morphNormalMapList.begin();

					primitive.targets.clear();
					int MorphTargetCount = GetMorphTargetNum(pMorphMod);
					for (int i = 0; i < MorphTargetCount; i++) {
						MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(i);
						if (!mc.IsActive())continue;
						if (!mc.GetMorphTarget()) continue;

						std::map<std::string, int> morphTargetParam;

						//GetMeshInfoXX(pMesh);
						//----------- Morph TargetPosition
						{
							std::vector<Point3> targetPtTbl;
							SetMorphTargetPositionTable(mc, faceIDTable, pMesh, targetPtTbl);

							tinygltf::Accessor acc;// = Create_glTFAccessor();
							acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
							acc.type = TINYGLTF_TYPE_VEC3;
							acc.count = faceIDTable.size() * 3;
							tinygltf::BufferView bfView;// = Create_glTFBufferView();
							bfView.buffer = 0;
							bfView.byteOffset = m_BufferByteOffset;
							bfView.byteLength = acc.count * sizeof(float) * 3;
							bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

							//pFace = pMesh->faces;
							Point3 minPos = targetPtTbl[0];// pTargetMesh->verts[pFace[faceIDTable[0]].v[0]];
							Point3 maxPos = targetPtTbl[0];//pTargetMesh->verts[pFace[faceIDTable[0]].v[0]];

							void* ptr = SecureMemory(bfView.byteLength);
							float* pPos = (float*)((char*)ptr + bfView.byteOffset);
							for (auto p : targetPtTbl) {
								if (p.x > maxPos.x)	maxPos.x = p.x;
								if (p.y > maxPos.y)	maxPos.y = p.y;
								if (p.z > maxPos.z)	maxPos.z = p.z;
								if (p.x < minPos.x)	minPos.x = p.x;
								if (p.y < minPos.y)	minPos.y = p.y;
								if (p.z < minPos.z)	minPos.z = p.z;
								*pPos++ = p.x;
								*pPos++ = p.y;
								*pPos++ = p.z;
							}

							acc.maxValues.push_back(maxPos.x);
							acc.maxValues.push_back(maxPos.y);
							acc.maxValues.push_back(maxPos.z);
							acc.minValues.push_back(minPos.x);
							acc.minValues.push_back(minPos.y);
							acc.minValues.push_back(minPos.z);
							m_model.bufferViews.push_back(bfView);
							acc.bufferView = m_model.bufferViews.size() - 1;
							m_model.accessors.push_back(acc);
							morphTargetParam.insert(std::make_pair("POSITION", m_model.accessors.size() - 1));
							//primitive.targets.push_back(w);
							//primitive.targets.(std::make_pair("POSITION", m_model.accessors.size() - 1));
						}

						//----------- Morph TargetNormal
						if (m_ExportMorphNrm) {

							std::vector<Point3> targetNrmTbl;
							SetMorphTargetNormalTable(faceIDTable, *normalMap, targetNrmTbl);

							tinygltf::Accessor acc;// = Create_glTFAccessor();
							acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
							acc.type = TINYGLTF_TYPE_VEC3;
							acc.count = faceIDTable.size() * 3;
							tinygltf::BufferView bfView;// = Create_glTFBufferView();
							bfView.buffer = 0;
							bfView.byteOffset = m_BufferByteOffset;
							bfView.byteLength = acc.count * sizeof(float) * 3;
							bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

							Point3 minNrm = targetNrmTbl[0];// pTargetMesh->verts[pFace[faceIDTable[0]].v[0]];
							Point3 maxNrm = targetNrmTbl[0];//pTargetMesh->verts[pFace[faceIDTable[0]].v[0]];

							void* ptr = SecureMemory(bfView.byteLength);
							float* pNrm = (float*)((char*)ptr + bfView.byteOffset);
							for (auto p : targetNrmTbl) {
								if (p.x > maxNrm.x)	maxNrm.x = p.x;
								if (p.y > maxNrm.y)	maxNrm.y = p.y;
								if (p.z > maxNrm.z)	maxNrm.z = p.z;
								if (p.x < minNrm.x)	minNrm.x = p.x;
								if (p.y < minNrm.y)	minNrm.y = p.y;
								if (p.z < minNrm.z)	minNrm.z = p.z;
								*pNrm++ = p.x;
								*pNrm++ = p.y;
								*pNrm++ = p.z;
							}

							acc.maxValues.push_back(maxNrm.x);
							acc.maxValues.push_back(maxNrm.y);
							acc.maxValues.push_back(maxNrm.z);
							acc.minValues.push_back(minNrm.x);
							acc.minValues.push_back(minNrm.y);
							acc.minValues.push_back(minNrm.z);
							m_model.bufferViews.push_back(bfView);
							acc.bufferView = m_model.bufferViews.size() - 1;
							m_model.accessors.push_back(acc);
							morphTargetParam.insert(std::make_pair("NORMAL", m_model.accessors.size() - 1));
							normalMap++;
						}
						primitive.targets.push_back(morphTargetParam);
					}
#endif
				}
			}

			//----------- Variant Material
			if (VariantMtl) {
				tinygltf::Value::Object mappings;
				tinygltf::Value::Array vArray;
				int num = pMtl->NumSubMtls();
				for (int i = 0; i < num; i++) {
					Mtl* pSubMtl = pMtl->GetSubMtl(i);
					if (!pSubMtl) continue;

					int m = -1;
					if (pSubMtl->ClassID() == multiClassID) {
						m = findMaterialIndex(pSubMtl->GetSubMtl(mesh.primitives.size()));
					}
					else {
						m = findMaterialIndex(pSubMtl);
					}
					primitive.material = m;

					tinygltf::Value::Object item;
					item.insert(std::make_pair("material", tinygltf::Value(m)));
					tinygltf::Value::Array v;
					v.push_back(tinygltf::Value(i));
					item.insert(std::make_pair("variants", v));
					vArray.push_back(tinygltf::Value(item));
				}
				mappings.insert(std::make_pair("mappings", tinygltf::Value(vArray)));
				primitive.extensions.insert(std::make_pair("KHR_materials_variants", mappings));
			}

			mesh.primitives.push_back(primitive);
		}


	if (pMorphMod) {
		SetMorphWeight(pMorphMod, mesh.weights);
		SetMorphTagetList(mesh, pMorphMod);
	}

	if (deleteIt) delete pTri;

	m_model.meshes.push_back(mesh);
	node.mesh = m_model.meshes.size() - 1;

	m_MeshMap[pNode->GetObjectRef()] = node.mesh;

	if (SkinMod) {
		pSkinMod->EnableMod();
	}
	if (pMorphMod) {
		pMorphMod->EnableMod();
	}

	if (pGameObject) {
		pGameNode->ReleaseIGameObject();
	}

}
#endif
//----------------------------------------------------------
//----------------------------------------------------------
UINT glTFExporter_Core::CreateInstanceMeshWithMtl(INode *pNode)
{
	Object* pObj = pNode->GetObjectRef();
	tinygltf::Mesh mesh(m_model.meshes[m_MeshMap[pObj]]);
	SetName(&mesh, pNode->GetName());
	mesh.name = WStringToString(pNode->GetName());

	Mtl* pMtl = pNode->GetMtl();
	if (!pMtl) {
		if(m_WireClrToMtl)
			pMtl = m_WireColorMtlMap[pNode->GetWireColor()];
		else
			mesh.primitives[0].material = -1;
	}

	if (pMtl) {
		if (pMtl->ClassID() == multiClassID) {
			for (int i = 0; i < mesh.primitives.size(); i++) {
				int mtlIdx = findMaterialIndex(pMtl->GetSubMtl(i));
				mesh.primitives[i].material = mtlIdx;
			}
		}
		else {
			int mtlIdx = findMaterialIndex(pMtl);
			mesh.primitives[0].material = mtlIdx;
		}
	}
	//int x = mesh.primitives[0].material;
	m_model.meshes.push_back(mesh);
	return m_model.meshes.size() - 1;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateVisibilityNode(tinygltf::Node& node, const VisibilityStruct& str, BOOL animated)
{
	tinygltf::Value::Object obj;
	bool b = str.visible;
	obj.insert(std::make_pair("visible", tinygltf::Value(b)));

	tinygltf::Value val(obj);
	node.extensions.insert(std::make_pair("KHR_node_visibility", val));

	m_Visibility_Used = TRUE;
	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateSelectabilityNode(tinygltf::Node& node, const SelectabilityStruct& str, BOOL animated)
{
	tinygltf::Value::Object obj;
	bool b = str.selectable;
	obj.insert(std::make_pair("selectable", tinygltf::Value(b)));

	tinygltf::Value val(obj);
	node.extensions.insert(std::make_pair("KHR_node_selectability", val));

	m_Selectability_Used = TRUE;
	return TRUE;
}
//======================================================================
//======================================================================
BOOL glTFExporter_Core::CreateHoverabilityNode(tinygltf::Node& node, const HoverabilityStruct& str, BOOL animated)
{
	tinygltf::Value::Object obj;
	bool b = str.hoverable;
	obj.insert(std::make_pair("hoverable", tinygltf::Value(b)));

	tinygltf::Value val(obj);
	node.extensions.insert(std::make_pair("KHR_node_hoverability", val));

	m_Hoverability_Used = TRUE;
	return TRUE;
}
//----------------------------------------------------------
//----------------------------------------------------------
int findVertTbl(int idx, Mesh *pMesh, std::vector<int> &idxTable, int mapCh)
{
	idxTable.clear();

	MeshMap *pMap = &pMesh->Map(mapCh);
	UVVert *pSrcUV = pMap->tv;
	Face *pFace = pMesh->faces;
	TVFace *pTVFace = pMesh->mapFaces(mapCh);
	for (int i = 0; i < pMesh->numFaces; i++) {
		if (pFace->v[0] == idx) idxTable.push_back(pTVFace->t[0]);
		if (pFace->v[1] == idx) idxTable.push_back(pTVFace->t[1]);
		if (pFace->v[2] == idx) idxTable.push_back(pTVFace->t[2]);
		pFace++;
		pTVFace++;
	}

	std::sort(idxTable.begin(), idxTable.end());
	idxTable.erase(std::unique(idxTable.begin(), idxTable.end()), idxTable.end());

	return idxTable.size();
}

//----------------------------------------------------------
//----------------------------------------------------------
void Matrix3ToFloat(Matrix3 &m, std::vector<double> &f, float scale)
{
	f.clear();

	if (m.IsIdentity()) return;

	Point3 r1 = m.GetRow(0);
	Point3 r2 = m.GetRow(1);
	Point3 r3 = m.GetRow(2);
	Point3 r4 = m.GetRow(3) * scale;

	f.push_back(r1.x);
	f.push_back(r1.y);
	f.push_back(r1.z);
	f.push_back(0.0f);
	f.push_back(r2.x);
	f.push_back(r2.y);
	f.push_back(r2.z);
	f.push_back(0.0f);
	f.push_back(r3.x);
	f.push_back(r3.y);
	f.push_back(r3.z);
	f.push_back(0.0f);
	f.push_back(r4.x);
	f.push_back(r4.y);
	f.push_back(r4.z);
	f.push_back(1.0f);
}

//=============================================================================
//=============================================================================
float GetW(const Point3 &normal, const Point3 &tangent, const Point3 &bitangent)
{
	if (bitangent.FLength()<=0.000001f)
	{
		return 1.0f;
	}

	// Cross product bitangent = w * normal ^ tangent
	// theorical bittangent
	Point3 cp = (normal ^ tangent).FNormalize();

	// Speaking in broadest terms, if the dot product of two non-zero vectors is positive, 
	// then the two vectors point in the same general direction, meaning less than 90 degrees. 
	// If the dot product is negative, then the two vectors point in opposite directions, 
	// or above 90 and less than or equal to 180 degrees.
	//float dot = MathUtilities.DotProduct(btx, bty, btz, x, y, z);
	float dot = DotProd(bitangent, cp);

	return dot < 0.0f ? -1.0f : 1.0f;
}

//=============================================================================
// Geommetry object?
//=============================================================================
BOOL IsGeometryObject(INode* pNode, TimeValue t)
{
	if (!pNode) return FALSE;
	if(pNode->IsRootNode()) return FALSE;

	Object* pObj = pNode->EvalWorldState(t).obj;
	if(!pObj) return FALSE;
	return (pObj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)));

}
//=============================================================================
//=============================================================================
static void GetMirroredNodeRec(INode* pNode, INodeTab &tab)
{
	if (!pNode) return;

	if (IsGeometryObject(pNode, 0)) {
		AffineParts af;
		decomp_affine(pNode->GetNodeTM(0), &af);
		if (af.f < 0) {
			tab.Append(1, &pNode);
		}
	}
	if (exportSelected) return;

	for (int i = 0; i < pNode->NumChildren(); i++) {
		GetMirroredNodeRec(pNode->GetChildNode(i), tab);
	}
}

int GetMirroredNode(INodeTab &tbl)
{
	tbl.ZeroCount();
	if (exportSelected) {
		int numNode = GetCOREInterface()->GetSelNodeCount();
		for (int i = 0; i < numNode; i++) {
			INode* pNode = GetCOREInterface()->GetSelNode(i);
			GetMirroredNodeRec(pNode, tbl);
		}
	}
	else {
		INode* pNode = GetCOREInterface()->GetRootNode();
		GetMirroredNodeRec(pNode, tbl);
	}

	return tbl.Count();
}

//=============================================================================
// Create TriObject for Node
//=============================================================================
TriObject* GetTriObjectFromNode(INode *pNode, TimeValue t, int &deleteIt)
{
	deleteIt = FALSE;
	// extract the object 
	Object *pObj = pNode->EvalWorldState(t).obj;
	// Check if we can convert to TriObject 
	if (pObj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) {
		TriObject *pTri = (TriObject *)pObj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
			
		 // if necessary, signal caller to delete the returned TriObject after use.
		if (pObj != pTri) deleteIt = TRUE;
		return pTri;
	}
	else {
		return NULL;
	}
}
#if 0
//=============================================================================
// Create SplineShape for Node
//=============================================================================
SplineShape* GetShapeObjectFromNode(INode* pNode, TimeValue t, int& deleteIt)
{
	deleteIt = FALSE;
	// extract the object 
	Object* pObj = pNode->EvalWorldState(t).obj;
	// Check if we can convert to SplineShape
	if (pObj->CanConvertToType(splineShapeClassID)) {
		SplineShape* pShape = (SplineShape*)pObj->ConvertToType(t, splineShapeClassID);
		
		// if necessary, signal caller to delete the returned SplineShape after use.
		if (pObj != pShape) deleteIt = TRUE;
		return pShape;
	}
	else {
		return NULL;
	}
}
#else
//=============================================================================
// Create SplineShape for Node
//=============================================================================
LinearShape* GetShapeObjectFromNode(INode* pNode, TimeValue t, int& deleteIt)
{
	deleteIt = FALSE;
	// extract the object 
	Object* pObj = pNode->EvalWorldState(t).obj;
	// Check if we can convert to SplineShape
	if (pObj->CanConvertToType(linearShapeClassID)) {
		LinearShape* pShape = (LinearShape*)pObj->ConvertToType(t, linearShapeClassID);

		// if necessary, signal caller to delete the returned SplineShape after use.
		if (pObj != pShape) deleteIt = TRUE;
		return pShape;
	}
	else {
		return NULL;
	}
}
#endif
void GetMeshInfoXX(Mesh* pMesh)
{
	std::vector<Point3> tbl;
	tbl.clear();
	for (int i = 0; i < pMesh->numFaces; i++) {
		Face face = pMesh->faces[i];
		for (int j = 0; j < 3; j++) {
			Point3 p = pMesh->verts[face.v[j]];
			tbl.push_back(p);
		}
	}
	int ii = 0;
}
