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

#include <unordered_set>

extern Matrix3 GetOffsetTM(INode* pNode);


void ExSetMorphTargetPositionTable(MaxMorphChannel& mc, const std::vector<VertexProp>& VertPropTable, Mesh* pMesh, std::vector<Point3>& targetPtTbl);
void ExSetMorphTargetNormalTable(const std::vector<VertexProp>& VertPropTable, std::map<int, Point3>& morphNormalMap, std::vector<Point3>& targetNrmTbl);

//
int GetVertNum(Mesh *pMesh, std::vector<int> &faceIDTable)
{
	std::unordered_set<int> temp;
	for (auto f: faceIDTable) {
		auto *p = pMesh->faces[f].v;
		for(int i=0;i<3;i++) temp.insert(*p++);
	}

	return temp.size();
}
/*
std::map<int, int> sameVertMap;
std::vector<int> diffVertMap;

// Create a map of vertices that are considered the same vertex
// Vertices are considered the same if their coordinates and normal vectors are equal

void testFunc(Mesh *pMesh, std::vector<int> faceIDTabl, MeshNormalSpec* pNrmSpec)
{
	sameVertMap.clear();
	diffVertMap.clear();

	for (auto fid1 : faceIDTabl) {
		Face face1 = pMesh->faces[fid1];
		int mtlID1 = face1.getMatID();
		int smsGrp1 = face1.getSmGroup();
		for (int j = 0; j < 3; j++) {
			int vertID = face1.v[j];
			Point3 nrm = pNrmSpec->GetNormal(fid1, j);
			for (auto fid2 : faceIDTabl) {
				if (fid1 == fid2) continue;
				Face face2 = pMesh->faces[fid2];
				int mtlID2 = face2.getMatID();
				if (mtlID1 != mtlID2) continue;
				int smsGrp2 = face2.getSmGroup();
				if ((smsGrp1 & smsGrp2) ==0) continue;
				for (int j = 0; j < 3; j++) {
					if (vertID == face2.v[j]) {
						Point3 nrm2 = pNrmSpec->GetNormal(fid2, j);
						if (nrm == nrm2) sameVertMap.insert(std::make_pair(fid2 * 10 + j, vertID));
						else diffVertMap.push_back(fid2 * 10 + j);
					}
				}
			}
		}
	}
}
*/

std::vector<VertexProp> VertPropTable;
std::map<int, int> vertPropMap;

//==========================================================
// Returns the index of the first matching VertexProp entry in the Attribute Information Table
// If there is no matching entry, one is added to the and of the table and its index returned
//==========================================================
int SetVertPropMap(VertexProp &str)
{
	int idx = 0;
	for (auto i : VertPropTable) {
		if (i.originalIdx == str.originalIdx &&
			FLength(i.normal-str.normal)<0.01f &&
			i.uv1 == str.uv1 &&
			i.uv2 == str.uv2 &&
			i.vc == str.vc) {
			return idx;
		}
		idx++;
	}
	VertPropTable.push_back(str);

	return VertPropTable.size()-1;
}

//==========================================================
// Create an Attribute Information Table for each vertex
// VertPropTable: Vertex table with attributes
// vertPropMap: Mapping table indicating which VertPropTable each vertex in a face belongs to
//==========================================================
void BuildVertexPropTable(Mesh *pMesh, std::vector<int> &faceIDTabl, MeshNormalSpec* pNrmSpec, const vertPropFlag &flag)
{
	VertPropTable.clear();
	vertPropMap.clear();

	TVFace* pTVFace1 = NULL;
	if(flag.mapCh1Used) pTVFace1 = pMesh->mapFaces(1);
	TVFace* pTVFace2 = NULL;
	if (flag.mapCh2Used) pTVFace2 = pMesh->mapFaces(2);
	TVFace* pTVFaceVC = NULL;
	if (flag.VColorUsed) pTVFaceVC = pMesh->mapFaces(0);

	for (auto fid1 : faceIDTabl) {
		Face face1 = pMesh->faces[fid1];
		MeshNormalFace nface1 = pNrmSpec->Face(fid1);
		for (int j = 0; j < 3; j++) {
			VertexProp str;
			str.faceID = fid1;
			str.corner = j;
			str.originalIdx = face1.v[j];
			str.nrmID = nface1.GetNormalID(j);
			str.normal = pNrmSpec->GetNormalArray()[str.nrmID];
			str.uv1 = (pTVFace1) ? pTVFace1[fid1].t[j] : -1;
			str.uv2 = (pTVFace2) ? pTVFace2[fid1].t[j] : -1;
			str.vc = (pTVFaceVC) ? pTVFaceVC[fid1].t[j] : -1;
			int idx = SetVertPropMap(str);
			vertPropMap.insert(std::make_pair(fid1*10+j, idx));
		}
	}
}

//==========================================================
//==========================================================
void glTFExporter_Core::ExCreateMeshData(INode* pNode, tinygltf::Node& node)
{
	Mesh ShapeMesh;
	Mesh* pMesh = NULL;
	BezierShape* pShape = NULL;

	vertPropFlag propFlag;
	//------------------------------------------
	//------------------------------------------
	{
		ObjectState os = pNode->EvalWorldState(0);
		if (os.obj->SuperClassID() == SHAPE_CLASS_ID) {
			if (m_ExportShapeObj == 0) return;
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
		propFlag.pGameMesh = NULL;
		pGameNode = m_pIGameScene->GetIGameNode(pNode);
		pGameObject = pGameNode->GetIGameObject();
		if (pGameObject->GetIGameType() == IGameObject::IGAME_MESH) {
			pGameMesh = (IGameMesh*)pGameObject;
			pGameMesh->InitializeData();
			pGameMesh->InitializeBinormalData();
			Tab<int> mapTbl = pGameMesh->GetActiveMapChannelNum();
		}
	}
	propFlag.pGameMesh = pGameMesh;

	//------------------------------------------
	//------------------------------------------
	BOOL SkinMod = FALSE;
	Modifier* pSkinMod = NULL;
	ISkin* pISkin = NULL;
	int SkinTopBoneID = 0;
	ISkinContextData* pSkinMC = NULL;
	if (FindModifier(pNode, SKIN_CLASSID, &pSkinMod) >= 0) {
		SkinMod = TRUE;
		//pSkinImp = (ISkinImportData *)pSkinMod->GetInterface(I_SKINIMPORTDATA);
		pISkin = (ISkin*)pSkinMod->GetInterface(I_SKIN);
		pSkinMC = pISkin->GetContextInterface(pNode);
		SkinTopBoneID = GetRootNodeBoneID(pISkin);

		m_skinNodeTable.insert(std::make_pair(pNode, pSkinMod));
		pSkinMod->DisableMod();
		pSkinMod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

	//------------------------------------------
	//------------------------------------------
	Modifier* pMorphMod = m_morphNodeTable[pNode];
	if (pMorphMod) {
		pMorphMod->DisableMod();
		pMorphMod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

	//------------------------------------------
	//------------------------------------------
	//float quantizationScale = 1.0f;
	//Point3 quantizationOfset(0, 0, 0);
	QuantizationInfo quatInfo;

	BOOL deleteIt = FALSE;
	TriObject* pTri = NULL;
	if (!pMesh) {
		pTri = GetTriObjectFromNode(pNode, m_time, deleteIt);
		if (!pTri) {
			node.mesh = -1;
			return;
		}
		pMesh = &pTri->mesh;

		if (m_Mesh_quantization_Used) {
			GetQuatizationInfo(pNode, quatInfo);
			/*
			Box3 box;
			pTri->GetDeformBBox(0, box);
			float lenX = box.pmax.x - box.pmin.x;
			float lenY = box.pmax.y - box.pmin.y;
			float lenZ = box.pmax.z - box.pmin.z;
			quantizationScale = std::max({ lenX, lenY, lenZ }) / 65535.0f * m_scale;
			quantizationOfset = -box.pmin;
			*/
		}
	}

	//------------------------------------------
	//------------------------------------------
	pMesh->SpecifyNormals();
	MeshNormalSpec* pNrmSpec = pMesh->GetSpecifiedNormals();
	pNrmSpec->BuildNormals();
	pNrmSpec->ComputeNormals();
	int numnrm = pNrmSpec->GetNumNormals();

	std::vector<std::map<int, Point3> > morphNormalMapList;
	if (pMorphMod && m_ExportMorphNrm) {
		CreateMorphVertMapTable(pMorphMod, pMesh, pNrmSpec, morphNormalMapList);
	}

	int mapCh = 1;

	//------------------------------------------
	//------------------------------------------
	Mtl* pMtl = pNode->GetMtl();
	if (pMtl && pMtl->ClassID() == ShellMaterialID) pMtl = GetSubMtlFromShellMtl(pMtl);

	if (!pMtl && m_WireClrToMtl) {
		if (IsGeometryObject(pNode))
			pMtl = m_WireColorMtlMap[pNode->GetWireColor()];
	}
	propFlag.mapCh1Used = MapUsed(pMtl) && pMesh->mapSupport(1);
	propFlag.mapCh2Used = MapUsed(pMtl) && pMesh->mapSupport(2);

	propFlag.VColorUsed = pNode->GetCVertMode() && pMesh->Map(0).tv;

	Matrix3 TangentTM;
	GetTangentTM(pMtl, TangentTM);

	tinygltf::Mesh mesh;// = Create_glTFMesh(pNode->GetName());
	SetName(&mesh, pNode->GetName());
	mesh.name = WStringToString(pNode->GetName());

	//------------------------------------------
	//------------------------------------------
	Matrix3 TangentOffsetTM;
	{
		TangentOffsetTM = pNode->GetNodeTM(m_time);
		TangentOffsetTM.SetTrans(Point3(0, 0, 0));
		TangentOffsetTM = Inverse(TangentOffsetTM);
	}

	int VariantCount = 0;
	BOOL VariantMtl = IsVariantMtl(pMtl, VariantCount);

	//------------------------------------------
	//------------------------------------------
	std::map<int, std::vector<int> > mtlIDMap;
	if (VariantMtl) {
		Mtl* pSubMtl = pMtl->GetSubMtl(0);
		CreateMtlIDTable(pMesh, pSubMtl, mtlIDMap);
	}
	else {
		CreateMtlIDTable(pMesh, pMtl, mtlIDMap);
	}
	int subMtlsCount = mtlIDMap.size();

	//------------------------------------------
	//------------------------------------------
	Matrix3 OffsetTM;
	if (!m_ResetPivotTM) OffsetTM = GetOffsetTM(pNode);

	//------------------------------------------
	//------------------------------------------
	Face* pFace = pMesh->faces;
	int primitiveId = 0;
	for (auto mapTbl : mtlIDMap) {
		int AttributeIndex = 0;
		int mtlID = mapTbl.first;
		std::vector<int> faceIDTable = mapTbl.second;
		if (faceIDTable.size() == 0) continue;


		tinygltf::Primitive primitive;
		primitive.mode = TINYGLTF_MODE_TRIANGLES;

		//------------------------------------------
		//------------------------------------------
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
			if (VariantMtl)
				mtlIdx = findMaterialIndex(pMtl->GetSubMtl(0)->GetSubMtl(mtlID));
			else
				mtlIdx = findMaterialIndex(pMtl->GetSubMtl(mtlID));
		}
		primitive.material = mtlIdx;

		//------------------------------------------
		//------------------------------------------
		BuildVertexPropTable(pMesh, faceIDTable, pNrmSpec, propFlag);
		int vertNum = VertPropTable.size();

		//----------- Create Vertex table
		if (m_DracoCompress) {
			CreateDracoMeshProp(primitive, pMesh, pNrmSpec, faceIDTable, VertPropTable, vertPropMap, propFlag, pMtl, pSkinMC, pMorphMod, OffsetTM);
			//GetMeshInfoXX(pMesh);
		}
		else {
			//----------- Index
			{
				size_t dataSize = faceIDTable.size() * 3;
				tinygltf::Accessor acc;// = Create_glTFAccessor();
				acc.componentType = dataSize < 65535 ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT : TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
				acc.type = TINYGLTF_TYPE_SCALAR;
				acc.count = dataSize;
				tinygltf::BufferView bfView;// = Create_glTFBufferView();
				bfView.buffer = 0;
				bfView.byteOffset = m_BufferByteOffset;
				bfView.byteLength = acc.count * (dataSize < 65535 ? sizeof(USHORT) : sizeof(UINT));
				bfView.byteLength += (bfView.byteLength % 4) == 0 ? 0 : (4 - (bfView.byteLength % 4));
				bfView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

				void* ptr = SecureMemory(bfView.byteLength);
				USHORT* pShortIdx = (USHORT*)((char*)ptr + bfView.byteOffset);
				UINT* pIntIdx = (UINT*)((char*)ptr + bfView.byteOffset);
				for (auto f : faceIDTable) {
					for (int i = 0; i < 3; i++) {
						auto idx = vertPropMap[f * 10 + i];
						if (dataSize < 65535)	*pShortIdx++ = idx;
						else 					*pIntIdx++ = idx;
					}
				}

				m_model.bufferViews.push_back(bfView);
				acc.bufferView = m_model.bufferViews.size() - 1;
				m_model.accessors.push_back(acc);
				primitive.indices = m_model.accessors.size() - 1;
			}

			//----------- Position
			{
				tinygltf::Accessor acc;
				acc.componentType = m_Mesh_quantization_Used ? TINYGLTF_COMPONENT_TYPE_SHORT : TINYGLTF_COMPONENT_TYPE_FLOAT;
				acc.type = TINYGLTF_TYPE_VEC3;
				acc.count = vertNum;
				if (m_Mesh_quantization_Used) {
					acc.normalized = true; // 0.0 ~ 1.0 
				}
				tinygltf::BufferView bfView;
				bfView.buffer = 0;
				bfView.byteOffset = m_BufferByteOffset;
				if (m_Mesh_quantization_Used) {
					bfView.byteLength = acc.count * 8;
					bfView.byteStride = 8;
				}
				else {
					bfView.byteLength = acc.count * sizeof(float) * 3;
				}
				bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

				Point3 minPos = pMesh->verts[pFace[faceIDTable[0]].v[0]] * m_scale * OffsetTM;
				Point3 maxPos = minPos;

				void* ptr = SecureMemory(bfView.byteLength);
				float* pPos = (float*)((char*)ptr + bfView.byteOffset);
				SHORT* pPosQ = (SHORT*)((char*)ptr + bfView.byteOffset);
				float quantScale = 32767.0f / quatInfo.meshScale / m_scale;

				for (auto v : VertPropTable) {
					Point3 pp = pMesh->verts[v.originalIdx] * m_scale;
					Point3 p = pp * OffsetTM;
					if (m_Mesh_quantization_Used) {
						if (p.x > quatInfo.meshScale)	p.x = quatInfo.meshScale;
						if (p.y > quatInfo.meshScale)	p.y = quatInfo.meshScale;
						if (p.z > quatInfo.meshScale)	p.z = quatInfo.meshScale;
					}
					if (p.x > maxPos.x)	maxPos.x = p.x;
					if (p.y > maxPos.y)	maxPos.y = p.y;
					if (p.z > maxPos.z)	maxPos.z = p.z;
					if (p.x < minPos.x)	minPos.x = p.x;
					if (p.y < minPos.y)	minPos.y = p.y;
					if (p.z < minPos.z)	minPos.z = p.z;

					if (m_Mesh_quantization_Used) {
						*pPosQ++ = round(p.x * quantScale);
						*pPosQ++ = round(p.y * quantScale);
						*pPosQ++ = round(p.z * quantScale);
						*pPosQ++ = 0;
					}
					else {
						*pPos++ = p.x;
						*pPos++ = p.y;
						*pPos++ = p.z;
					}
				}

				if (m_Mesh_quantization_Used) {
					acc.maxValues.push_back(round(maxPos.x * quantScale));
					acc.maxValues.push_back(round(maxPos.y * quantScale));
					acc.maxValues.push_back(round(maxPos.z * quantScale));
					acc.minValues.push_back(round(minPos.x * quantScale));
					acc.minValues.push_back(round(minPos.y  * quantScale));
					acc.minValues.push_back(round(minPos.z  * quantScale));
				}
				else {
					acc.maxValues.push_back(maxPos.x);
					acc.maxValues.push_back(maxPos.y);
					acc.maxValues.push_back(maxPos.z);
					acc.minValues.push_back(minPos.x);
					acc.minValues.push_back(minPos.y);
					acc.minValues.push_back(minPos.z);
				}

				m_model.bufferViews.push_back(bfView);
				acc.bufferView = m_model.bufferViews.size() - 1;
				m_model.accessors.push_back(acc);
				primitive.attributes.insert(std::make_pair("POSITION", m_model.accessors.size() - 1));
			}

			//----------- Normal
			{
				tinygltf::Accessor acc;// = Create_glTFAccessor();
				acc.componentType = m_Mesh_quantization_Used ? TINYGLTF_COMPONENT_TYPE_BYTE : TINYGLTF_COMPONENT_TYPE_FLOAT;
				acc.type = TINYGLTF_TYPE_VEC3;
				acc.count = vertNum;
				if (m_Mesh_quantization_Used)acc.normalized = true;

				tinygltf::BufferView bfView;// = Create_glTFBufferView();
				bfView.buffer = 0;
				bfView.byteOffset = m_BufferByteOffset;
				bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
				if (m_Mesh_quantization_Used) {
				bfView.byteLength = acc.count * 4;
					bfView.byteStride = 4;
				}
				else {
					bfView.byteLength = acc.count * sizeof(float) * 3;
				}

				void* ptr = SecureMemory(bfView.byteLength);
				if (m_Mesh_quantization_Used) {
					char* pNrmQ = (char*)((char*)ptr + bfView.byteOffset);
					for (auto v : VertPropTable) {
						*pNrmQ++ = round(v.normal.x * 127.0f);
						*pNrmQ++ = round(v.normal.y * 127.0f);
						*pNrmQ++ = round(v.normal.z * 127.0f);
						*pNrmQ++ = 0;
					}
				}
				else {
					float* pNrm = (float*)((char*)ptr + bfView.byteOffset);
					for (auto v : VertPropTable) {
						//Point3 nrm = pNrmSpec->GetNormalArray()[v.normal];
						Point3 nrm = v.normal;
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
				acc.componentType = m_Mesh_quantization_Used ? TINYGLTF_COMPONENT_TYPE_BYTE : TINYGLTF_COMPONENT_TYPE_FLOAT;
				acc.type = TINYGLTF_TYPE_VEC4;
				acc.count = vertNum;
				tinygltf::BufferView bfView;// = Create_glTFBufferView();

				bfView.buffer = 0;
				bfView.byteOffset = m_BufferByteOffset;
				bfView.byteLength = acc.count * sizeof(float) * 4;
				bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
				if (m_Mesh_quantization_Used) {
					bfView.byteLength = acc.count * 4;
					bfView.byteStride = 4;
				}
				else {
					bfView.byteLength = acc.count * sizeof(float) * 3;
				}
				void* ptr = SecureMemory(bfView.byteLength);
				float* pTan = (float*)((char*)ptr + bfView.byteOffset);
				char* pTanQ = (char*)((char*)ptr + bfView.byteOffset);

				for (auto v : VertPropTable) {
					Point3 normal = pGameMesh->GetNormal(v.faceID, v.corner, TRUE);
					int indexTangentBinormal = pGameMesh->GetFaceVertexTangentBinormal(v.faceID, v.corner, mapCh);
					Point3 tangent = pGameMesh->GetTangent(indexTangentBinormal, mapCh);
					tangent = tangent * TangentOffsetTM;
					tangent.FNormalize();

					Point3 bitangent = pGameMesh->GetBinormal(indexTangentBinormal, mapCh);
					bitangent = bitangent * TangentOffsetTM;
					bitangent.FNormalize();

					float w = GetW(normal, tangent, bitangent);
					if (m_Mesh_quantization_Used) {
						*pTanQ++ = round(tangent.x * 127.0f);
						*pTanQ++ = round(tangent.y * 127.0f);
						*pTanQ++ = round(tangent.z * 127.0f);
						*pTanQ++ = round(w * 127.0f);
					}
					else {
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
			if (propFlag.mapCh1Used) {
				tinygltf::Accessor acc;// = Create_glTFAccessor();
				acc.componentType = m_Mesh_quantization_Used ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT : TINYGLTF_COMPONENT_TYPE_FLOAT;
				acc.type = TINYGLTF_TYPE_VEC2;
				acc.count = vertNum;
				tinygltf::BufferView bfView;// = Create_glTFBufferView();

				bfView.buffer = 0;
				bfView.byteOffset = m_BufferByteOffset;
				if (m_Mesh_quantization_Used) {
					bfView.byteLength = acc.count * 4;
					bfView.byteStride = 4;
				}
				else {
					bfView.byteLength = acc.count * sizeof(float) * 2;
				}
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
				USHORT* pTexUVQ = (USHORT*)((char*)ptr + bfView.byteOffset);
				for (auto v : VertPropTable) {
					UVVert p = pMesh->mapVerts(mapCh)[v.uv1];
					p.y = -p.y + 1.0f;
					if (p.x > maxUV.x)	maxUV.x = p.x;
					if (p.y > maxUV.y)	maxUV.y = p.y;
					if (p.x < minUV.x)	minUV.x = p.x;
					if (p.y < minUV.y)	minUV.y = p.y;

					if (m_Mesh_quantization_Used) {
						*pTexUVQ++ = (USHORT)round((p.x + quatInfo.uvmap1Offset.x) / quatInfo.uvmap1Scale);
						*pTexUVQ++ = (USHORT)round((p.y + quatInfo.uvmap1Offset.y) / quatInfo.uvmap1Scale);
					}
					else{
						*pTexUV++ = p.x;
						*pTexUV++ = p.y;
					}
				}

				if (m_Mesh_quantization_Used) {
					acc.maxValues.push_back(round((maxUV.x + quatInfo.uvmap1Offset.x) / quatInfo.uvmap1Scale));
					acc.maxValues.push_back(round((maxUV.y + quatInfo.uvmap1Offset.y) / quatInfo.uvmap1Scale));
					acc.minValues.push_back(round((minUV.x + quatInfo.uvmap1Offset.x) / quatInfo.uvmap1Scale));
					acc.minValues.push_back(round((minUV.y + quatInfo.uvmap1Offset.y) / quatInfo.uvmap1Scale));
				}
				else {
					acc.maxValues.push_back(maxUV.x);
					acc.maxValues.push_back(maxUV.y);
					acc.minValues.push_back(minUV.x);
					acc.minValues.push_back(minUV.y);
				}

				m_model.bufferViews.push_back(bfView);
				acc.bufferView = m_model.bufferViews.size() - 1;
				m_model.accessors.push_back(acc);
				primitive.attributes.insert(std::make_pair("TEXCOORD_0", m_model.accessors.size() - 1));
			}

			//----------- UV2 map
			mapCh = 2;
			if (propFlag.mapCh2Used) {
				tinygltf::Accessor acc;// = Create_glTFAccessor();
				acc.componentType = m_Mesh_quantization_Used ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT : TINYGLTF_COMPONENT_TYPE_FLOAT;
				acc.type = TINYGLTF_TYPE_VEC2;
				acc.count = vertNum;
				tinygltf::BufferView bfView;// = Create_glTFBufferView();

				bfView.buffer = 0;
				bfView.byteOffset = m_BufferByteOffset;
				if (m_Mesh_quantization_Used) {
					bfView.byteLength = acc.count * 4;
					bfView.byteStride = 4;
				}
				else {
					bfView.byteLength = acc.count * sizeof(float) * 2;
				}
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
				USHORT* pTexUVQ = (USHORT*)((char*)ptr + bfView.byteOffset);

				for (auto v : VertPropTable) {
					UVVert p = pMesh->mapVerts(mapCh)[v.uv2];
					p.y = -p.y + 1.0f;
					if (p.x > maxUV.x)	maxUV.x = p.x;
					if (p.y > maxUV.y)	maxUV.y = p.y;
					if (p.x < minUV.x)	minUV.x = p.x;
					if (p.y < minUV.y)	minUV.y = p.y;

					if (m_Mesh_quantization_Used) {
						*pTexUVQ++ = (USHORT)round((p.x + quatInfo.uvmap2Offset.x) / quatInfo.uvmap2Scale);
						*pTexUVQ++ = (USHORT)round((p.y + quatInfo.uvmap2Offset.y) / quatInfo.uvmap2Scale);
					}
					else{
						*pTexUV++ = p.x;
						*pTexUV++ = p.y;
					}
				}

				if (m_Mesh_quantization_Used) {
					acc.maxValues.push_back(round((maxUV.x + quatInfo.uvmap2Offset.x) / quatInfo.uvmap2Scale));
					acc.maxValues.push_back(round((maxUV.y + quatInfo.uvmap2Offset.y) / quatInfo.uvmap2Scale));
					acc.minValues.push_back(round((minUV.x + quatInfo.uvmap2Offset.x) / quatInfo.uvmap2Scale));
					acc.minValues.push_back(round((minUV.y + quatInfo.uvmap2Offset.y) / quatInfo.uvmap2Scale));
				}
				else {
					acc.maxValues.push_back(maxUV.x);
					acc.maxValues.push_back(maxUV.y);
					acc.minValues.push_back(minUV.x);
					acc.minValues.push_back(minUV.y);
				}

				m_model.bufferViews.push_back(bfView);
				acc.bufferView = m_model.bufferViews.size() - 1;
				m_model.accessors.push_back(acc);
				primitive.attributes.insert(std::make_pair("TEXCOORD_1", m_model.accessors.size() - 1));
			}

			//----------- Vertex Color
			mapCh = 0;
			if (propFlag.VColorUsed) {
				tinygltf::Accessor acc;// = Create_glTFAccessor();
				acc.componentType = m_Mesh_quantization_Used ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE : TINYGLTF_COMPONENT_TYPE_FLOAT;
				acc.type = TINYGLTF_TYPE_VEC3;
				acc.count = vertNum;
				tinygltf::BufferView bfView;// = Create_glTFBufferView();

				bfView.buffer = 0;
				bfView.byteOffset = m_BufferByteOffset;
				if (m_Mesh_quantization_Used) {
					bfView.byteLength = acc.count * 4;
					bfView.byteStride = 4;
				}
				else {
					bfView.byteLength = acc.count * sizeof(float) * 3;
				}
				bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

				MeshMap* pMap = &pMesh->Map(0);
				UVVert* pSrcUV = pMap->tv;
				TVFace* pTVFace = pMesh->mapFaces(0);
				Point3 minUV = pSrcUV[pTVFace[faceIDTable[0]].t[0]];
				Point3 maxUV = minUV;

				void* ptr = SecureMemory(bfView.byteLength);
				float* pTexUV = (float*)((char*)ptr + bfView.byteOffset);
				char* pTexUVQ = (char*)((char*)ptr + bfView.byteOffset);

				for (auto v : VertPropTable) {
					UVVert p = pMesh->mapVerts(0)[v.vc];
					if (p.x > maxUV.x)	maxUV.x = p.x;
					if (p.y > maxUV.y)	maxUV.y = p.y;
					if (p.z > maxUV.z)	maxUV.z = p.z;
					if (p.x < minUV.x)	minUV.x = p.x;
					if (p.y < minUV.y)	minUV.y = p.y;
					if (p.z < minUV.z)	minUV.z = p.z;

					if (m_Mesh_quantization_Used) {
						*pTexUVQ++ = (unsigned char)round(p.x * 255.0);
						*pTexUVQ++ = (unsigned char)round(p.y * 255.0);
						*pTexUVQ++ = (unsigned char)round(p.z * 255.0);
						*pTexUVQ++ = 0;
					}
					else {
						*pTexUV++ = p.x;
						*pTexUV++ = p.y;
						*pTexUV++ = p.z;
					}
				}

				if (m_Mesh_quantization_Used) {
					acc.maxValues.push_back(round(maxUV.x * 255.0f));
					acc.maxValues.push_back(round(maxUV.y * 255.0f));
					acc.maxValues.push_back(round(maxUV.z * 255.0f));
					acc.minValues.push_back(round(minUV.x * 255.0f));
					acc.minValues.push_back(round(minUV.y * 255.0f));
					acc.minValues.push_back(round(minUV.z * 255.0f));
				}
				else{
					acc.maxValues.push_back(maxUV.x);
					acc.maxValues.push_back(maxUV.y);
					acc.maxValues.push_back(maxUV.z);
					acc.minValues.push_back(minUV.x);
					acc.minValues.push_back(minUV.y);
					acc.minValues.push_back(minUV.z);
				}

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

				for (auto v : VertPropTable) {
					std::array<float, 4> wa{ 0.0f, 0.0f, 0.0f, 0.0f };
					std::array<UINT, 4> ba{ 0, 0, 0, 0 };
					int numb = pSkinMC->GetNumAssignedBones(v.originalIdx);
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
							int boneIdx = pSkinMC->GetAssignedBone(v.originalIdx, j);
							float w = pSkinMC->GetBoneWeight(v.originalIdx, j);
							wa[j] = w;
							ba[j] = boneIdx;
						}
					}
					wTable.push_back(wa);
					bTable.push_back(ba);
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

				//------ Weight ---------------------------
				{
					tinygltf::Accessor acc;// = Create_glTFAccessor();
					acc.componentType = m_Mesh_quantization_Used ? TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT : TINYGLTF_COMPONENT_TYPE_FLOAT;
					acc.type = TINYGLTF_TYPE_VEC4;
					acc.count = wTable.size();
					if (m_Mesh_quantization_Used) {
						acc.normalized = true; // 0.0 ~ 1.0 
					}

					tinygltf::BufferView bfView;// = Create_glTFBufferView();
					bfView.buffer = 0;
					bfView.byteOffset = m_BufferByteOffset;
					if (m_Mesh_quantization_Used) {
						bfView.byteLength = acc.count * sizeof(USHORT) * 4;
					}
					else {
						bfView.byteLength = acc.count * sizeof(float) * 4;
					}
					bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
					void* ptr = SecureMemory(bfView.byteLength);
					if (m_Mesh_quantization_Used) {
						USHORT* pWeightQ = (USHORT*)((char*)ptr + bfView.byteOffset);
						for (auto w : wTable) {
							*pWeightQ++ = (USHORT)round(w[0] * 65535.0f);
							*pWeightQ++ = (USHORT)round(w[1] * 65535.0f);
							*pWeightQ++ = (USHORT)round(w[2] * 65535.0f);
							*pWeightQ++ = (USHORT)round(w[3] * 65535.0f);
						}
					}
					else {
						float* pWeight = (float*)((char*)ptr + bfView.byteOffset);
						for (auto w : wTable) {
							*pWeight++ = w[0];
							*pWeight++ = w[1];
							*pWeight++ = w[2];
							*pWeight++ = w[3];
						}
					}

					m_model.bufferViews.push_back(bfView);
					acc.bufferView = m_model.bufferViews.size() - 1;
					m_model.accessors.push_back(acc);
					primitive.attributes.insert(std::make_pair("WEIGHTS_0", m_model.accessors.size() - 1));
				}
			}

			//-------- Morph
			if (pMorphMod) {

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
						ExSetMorphTargetPositionTable(mc, VertPropTable, pMesh, targetPtTbl);

						tinygltf::Accessor acc;// = Create_glTFAccessor();
						acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
						acc.type = TINYGLTF_TYPE_VEC3;
						acc.count = vertNum;
						tinygltf::BufferView bfView;// = Create_glTFBufferView();
						bfView.buffer = 0;
						bfView.byteOffset = m_BufferByteOffset;
						bfView.byteLength = acc.count * sizeof(float) * 3;
						bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

						//pFace = pMesh->faces;
						Point3 minPos = targetPtTbl[0] * m_scale;// pTargetMesh->verts[pFace[faceIDTable[0]].v[0]];
						Point3 maxPos = targetPtTbl[0] * m_scale;//pTargetMesh->verts[pFace[faceIDTable[0]].v[0]];

						void* ptr = SecureMemory(bfView.byteLength);
						float* pPos = (float*)((char*)ptr + bfView.byteOffset);
						for (auto p : targetPtTbl) {
							p *= m_scale;
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
						ExSetMorphTargetNormalTable(VertPropTable, *normalMap, targetNrmTbl);

						tinygltf::Accessor acc;// = Create_glTFAccessor();
						acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
						acc.type = TINYGLTF_TYPE_VEC3;
						acc.count = vertNum;
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

	if (mesh.primitives.size() > 0) {
		m_model.meshes.push_back(mesh);

		node.mesh = m_model.meshes.size() - 1;

		m_MeshMap[pNode->GetObjectRef()] = node.mesh;
	}

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

//======================================================================
//======================================================================
void ExSetMorphTargetPositionTable(MaxMorphChannel& mc, const std::vector<VertexProp>& VertPropTable, Mesh* pMesh, std::vector<Point3>& targetPtTbl)
{
	targetPtTbl.clear();
	for (auto v : VertPropTable) {
		int vID = v.originalIdx;
		Point3 BasePt = pMesh->verts[vID];
		Point3 pt = mc.GetMorphPoint(vID) - BasePt;
		targetPtTbl.push_back(pt);
	}
}

//======================================================================
//======================================================================
void ExSetMorphTargetNormalTable(const std::vector<VertexProp>& VertPropTable, std::map<int, Point3>& morphNormalMap, std::vector<Point3>& targetNrmTbl)
{
	targetNrmTbl.clear();
	for (auto v : VertPropTable) {
		auto n0 = morphNormalMap[v.faceID * 10 + v.corner];
		targetNrmTbl.push_back(n0);
	}
}
