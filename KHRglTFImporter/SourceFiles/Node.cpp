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
#include <iEditNormals.h>
#include <MeshNormalSpec.h>
#include <iMaterialViewportShading.h>
#include <shape.h>
#include <splshape.h>	// Required  for SplineShape
/*
BOOL CheckBufferSize(cgltf_accessor *acc)
{
	if (!acc) return FALSE;

	int dataSize = sizeof(float);
	switch (acc->component_type) {
	case cgltf_component_type_r_8:	dataSize = 1; break;	// BYTE
	case cgltf_component_type_r_8u:	dataSize = 1; break;	// UNSIGNED_BYTE
	case cgltf_component_type_r_16:	 dataSize = 2; break;  // SHORT
	case cgltf_component_type_r_16u: dataSize = 2; break;	// UNSIGNED_SHORT
	case cgltf_component_type_r_32u: dataSize = 4; break;  // UNSIGNED_INT
	case cgltf_component_type_r_32f: dataSize = 4; break;  // FLOAT
	}

	int typeSize = 1;
	switch (acc->type) {
	case cgltf_type_invalid: typeSize = 1; break;
	case cgltf_type_scalar: typeSize = 1; break;
	case cgltf_type_vec2: typeSize = 2; break;
	case cgltf_type_vec3: typeSize = 3; break;
	case cgltf_type_vec4: typeSize = 4; break;
	case cgltf_type_mat2: typeSize = 4; break;
	case cgltf_type_mat3: typeSize = 9; break;
	case cgltf_type_mat4: typeSize = 16; break;
	}

	cgltf_buffer_view* view = acc->buffer_view;
	int bufferSize = view->size;
	int stride = view->stride;

	int size = 0;
	if (stride > 0)
		size = stride * acc->count - acc->offset;
	else 
		size = typeSize * dataSize * acc->count - acc->offset;

	return size <= bufferSize;
}
*/

//======================================================================
//======================================================================
void SetNormal(Mesh *pMesh, const std::vector<Point3> &VertNormalTable)
{
	pMesh->SpecifyNormals();
	MeshNormalSpec *pNrmSpec = pMesh->GetSpecifiedNormals();
	//pNrmSpec->SetParent(pMesh);
	pNrmSpec->SetNumFaces(pMesh->numFaces);
	pNrmSpec->SetNumNormals((int)VertNormalTable.size());

	Face *pFace = pMesh->faces;
	for (int i = 0; i < pMesh->numFaces; i++, pFace++) {
		DWORD  *v = pFace->v;
		for (int j = 0; j < 3; j++, v++) {
			Point3 nrm(1.0f,0.0f,0.0f);
			if ((*v) < VertNormalTable.size()) {
				nrm = VertNormalTable[*v];
			}
			pNrmSpec->SetNormal(i, j, nrm);
		}
	}
	pNrmSpec->SetAllExplicit(TRUE);
}

//======================================================================
//======================================================================
cgltf_accessor* findAttrAccessor(cgltf_primitive *pr, const char *str)
{
	cgltf_attribute *attr = pr->attributes;
	for (int i = 0; i < pr->attributes_count; i++, attr++) {
		if (!_stricmp(str, attr->name)) return attr->data;
	}
	return NULL;
}

//======================================================================
//======================================================================
float GetQuantScale(cgltf_accessor *acc)
{
	if (!acc) return 1.0f;
	if (acc->normalized) {
		switch(acc->component_type) {
		case cgltf_component_type_r_8: /* BYTE */
			return 1.0f / 127.0f; break;
		case cgltf_component_type_r_8u: /* UNSIGNED_BYTE */
			return 1.0f / 255.0f; break;
		case cgltf_component_type_r_16: /* SHORT */
			return 1.0f / 32767.0f; break;
		case cgltf_component_type_r_16u: /* UNSIGNED_SHORT */
			return 1.0f / 65535.0f; break;

		}
	}

	return 1.0f;

}

// Helpers to classify primitive types.
enum class PrimCategory
{
	Unknown = 0,
	Points,
	Lines,
	Triangles,
};


PrimCategory GetPrimCategory(int primType)
{
	switch (primType)
	{
		case cgltf_primitive_type_points:
			return PrimCategory::Points;
		case cgltf_primitive_type_triangles:
		case cgltf_primitive_type_triangle_strip:
		case cgltf_primitive_type_triangle_fan:
			return PrimCategory::Triangles;

		case cgltf_primitive_type_lines:
		case cgltf_primitive_type_line_loop:
		case cgltf_primitive_type_line_strip:
			return PrimCategory::Lines;

		default:
			return PrimCategory::Unknown;
	}
}
// end of helpers

// ======================================================================
// Create the actual node object
// ======================================================================
INode* glTFImporter_Core::CreateMaxNode(cgltf_node* node, INode* pParent)
{
	tstring name;
	const char* pname = node->name;
	if (pname) name = StringToWString(pname);
	//DebugPrint(StringToWString(name.C_Str()).c_str());
	std::vector<Mtl*> mtlIdTable;

	Mesh NewMesh;
	BezierShape NewShape;
	//NewMesh.setSmoothFlags(1);

	cgltf_mesh* mesh = node->mesh;
	if (!pname) name = StringToWString(mesh->name);

	if (m_Instancing) {
		INode* pInstanceNode = m_MeshNodeMap[mesh];
		if (pInstanceNode) {
			Object* pObj = pInstanceNode->GetObjectRef();
			INode* pNode = GetCOREInterface()->CreateObjectNode(pObj);
			pNode->SetName(name.c_str()); //StringToWString(name.C_Str()).c_str());
			pNode->SetMtl(pInstanceNode->GetMtl());
			return pNode;
		}
	}

	int VertOffset = 0;
	int FaceOffset = 0;
	int Tex1Offset = 0;
	int Tex2Offset = 0;
	int vClrOffset = 0;
	int NormalOffset = 0;
	BOOL ViewVertColor = FALSE;
	
	PrimCategory ObjectType = PrimCategory::Unknown;

	std::vector<Point3> VertNormalTable;
	VertNormalTable.clear();

	//DWORD smGroupBit = 0x1;
	//UINT primId = 0;
	StdMat* pCompositeMtl = NULL;

#ifdef MAX_RELEASE_R24
	Matrix3 parentTM;
#else
	Matrix3 parentTM(1);
#endif
	if (pParent) {
		parentTM = pParent->GetNodeTM(0);
		parentTM.SetTrans(Point3(0, 0, 0));
	}

	if (mesh->primitives_count > 0) {
		cgltf_primitive* firstPrim = &mesh->primitives[0];
		
		ObjectType = GetPrimCategory(firstPrim->type);

		if (firstPrim->targets_count > 0)
			m_MorphTable.push_back(node);
		if (firstPrim->mappings_count > 0 && m_CompositeMtl) {
			switch (GetMtlType()) {
			case 0:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, MaterialSwitcherClassID); ;
				if(!pCompositeMtl)
					pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, StdMtlSwitcherClassID);
				break;
			case 1:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, MaterialSwitcherClassID); ;
				if (!pCompositeMtl)
					pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, PysicMtlSwitcherClassID);
				break;
			case 2:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, MaterialSwitcherClassID); break;
				if (!pCompositeMtl)
					pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, PBRMtlSwitcherClassID);
				break;
			case 3:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, MaterialSwitcherClassID); break;
				if (!pCompositeMtl)
					pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, glTFMtlSwitcherClassID);
				break;
			case 4:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, ArnoldSwitchShaderID);	break;
			case 5:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, MaterialSwitcherClassID); break;
				if (!pCompositeMtl)
					pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, USDMtlSwitcherClassID);
				break;
			case 6:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, MaterialSwitcherClassID); break;
				break;
			case 7:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, CoronaSelectMtlID);	break;
				break;
			case 9:
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, MaterialSwitcherClassID); break;
				break;
			default:
				pCompositeMtl = NULL;
				break;
			}

			if (!pCompositeMtl)
				pCompositeMtl = (StdMat*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, CompositeMtlClassID);
		}

		// There is a possibility that primitives with maps and primitives without maps will be mixed together.
		for (int i = 0; i < mesh->primitives_count; i++) {
			cgltf_primitive* tmpPrim = &mesh->primitives[i];

			if (findAttrAccessor(tmpPrim, "COLOR_0")) {
				NewMesh.setMapSupport(0, TRUE);
				if (m_ViewVertexColor) ViewVertColor = TRUE;
			}

			int maxMap = 1;
			if (findAttrAccessor(tmpPrim, "TEXCOORD_0")) maxMap = 2;
			if (findAttrAccessor(tmpPrim, "TEXCOORD_1")) maxMap = 3;
			NewMesh.setNumMaps(maxMap, FALSE);
			if (maxMap == 2) {
				NewMesh.setMapSupport(1, TRUE);
			}
			else if (maxMap == 3) {
				NewMesh.setMapSupport(1, TRUE);
				NewMesh.setMapSupport(2, TRUE);
			}
		}
	}

	for (int i = 0; i < mesh->primitives_count; i++) {
		cgltf_primitive* pr = &mesh->primitives[i];
		cgltf_draco_mesh_compression* mc = NULL;
		if (pr->has_draco_mesh_compression) {
			mc = &pr->draco_mesh_compression;
		}
		float scale_u = 1.0f;
		float scale_v = 1.0f;
		float offset_u = 0.0f;
		float offset_v = 0.0f;
		if (m_Quantization && pr->material) {
			if (pr->material->has_pbr_metallic_roughness) {
				if (pr->material->pbr_metallic_roughness.base_color_texture.has_transform) {
					scale_u = pr->material->pbr_metallic_roughness.base_color_texture.transform.scale[0];
					scale_v = pr->material->pbr_metallic_roughness.base_color_texture.transform.scale[1];
					offset_u = pr->material->pbr_metallic_roughness.base_color_texture.transform.offset[0];
					offset_v = pr->material->pbr_metallic_roughness.base_color_texture.transform.offset[1];
				}
			}
		}

		std::vector<Point3> texCoords;
		std::vector<Point3> tex2Coords;
		std::vector<UINT> nrmId;

		int mId = 0;
		cgltf_material* material = pr->material;
		if (material) {
			Mtl* pMtl = m_MaterialMap[material];
			std::vector<Mtl*>::iterator itr = std::find(mtlIdTable.begin(), mtlIdTable.end(), pMtl);
			if (itr == mtlIdTable.end()) {
				mtlIdTable.push_back(pMtl);
				mId = static_cast<int>(mtlIdTable.size());
			}
			else {
				mId = static_cast<int>(std::distance(mtlIdTable.begin(), itr) + 1);
			}
		}

		// Set vertex positions
		std::vector<float> flatCoordsBuf;

		if (mc) {
			DracoDecodeProc(mc->buffer_view, pr, flatCoordsBuf, DracoDecodeType::POSITION);
		}
		else {
			GetDataList(flatCoordsBuf, findAttrAccessor(pr, "POSITION"));
		}

		size_t VertNum = 0;
		std::vector<Point3> pointBuf;

		PrimCategory primCat = GetPrimCategory(pr->type);

		float quantScale = m_Quantization?GetQuantScale(findAttrAccessor(pr, "POSITION")):1.0F;

		if(primCat == PrimCategory::Points) {
			VertNum = flatCoordsBuf.size() / 3;
			NewMesh.setNumVerts((int)(VertNum + VertOffset), TRUE);
			NewMesh.setNumFaces(0);
			UINT vIdx = VertOffset;
			for (std::vector<float>::iterator v = flatCoordsBuf.begin(); v != flatCoordsBuf.end(); v += 3, vIdx++) {
				Point3 p(*v, *(v + 1), *(v + 2));
				NewMesh.setVert(vIdx, p * quantScale * m_scale);
			}
		}
		else if (primCat == PrimCategory::Lines) {
			pointBuf.clear();
			VertNum = flatCoordsBuf.size();
			uint32_t vIdx = VertOffset;
			for (std::vector<float>::iterator v = flatCoordsBuf.begin(); v != flatCoordsBuf.end(); v += 3, vIdx++) {
				Point3 p(*v, *(v + 1), *(v + 2));
				pointBuf.push_back(p * quantScale * m_scale);
			}
		}
		else if (primCat == PrimCategory::Triangles) {
			VertNum = flatCoordsBuf.size() / 3;
			NewMesh.setNumVerts((int)(VertNum + VertOffset), TRUE);
			UINT vIdx = VertOffset;
			for (std::vector<float>::iterator v = flatCoordsBuf.begin(); v != flatCoordsBuf.end(); v += 3, vIdx++) {
				Point3 p(*v, *(v + 1), *(v + 2));
				NewMesh.setVert(vIdx, p * quantScale * m_scale);
			}
		}

		// Face settings
		size_t FaceNum = 0;
		if ( primCat == PrimCategory::Triangles) {
			std::vector<uint32_t> FaceIdList;
			if (mc) {
				GetDracoMeshIndexList(mc->buffer_view, FaceIdList);
			}
			else {
				if (pr->indices) {
					std::vector<float> floatIndices;
					GetDataList(floatIndices, pr->indices);
					FaceIdList = FloatToUInt32IndexBuffer(floatIndices);
				}
				else {
					int numf = NewMesh.numVerts / 3;
					for (int i = 0; i < numf; i++) {
						FaceIdList.push_back(i * 3 + 0 + VertOffset);
						FaceIdList.push_back(i * 3 + 1 + VertOffset);
						FaceIdList.push_back(i * 3 + 2 + VertOffset);
					}
				}
			}

			if (pr->type == cgltf_primitive_type_triangles) {
				FaceNum = FaceIdList.size() / 3;
				NewMesh.setNumFaces((int)(FaceNum + FaceOffset), TRUE);
				UINT fIdx = FaceOffset;
				for (std::vector<uint32_t>::iterator f = FaceIdList.begin(); f != FaceIdList.end(); f += 3, fIdx++) {
					NewMesh.faces[fIdx].v[0] = *(f + 0) + VertOffset;
					NewMesh.faces[fIdx].v[1] = *(f + 1) + VertOffset;
					NewMesh.faces[fIdx].v[2] = *(f + 2) + VertOffset;
					NewMesh.faces[fIdx].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
					NewMesh.faces[fIdx].setMatID(mId);
					//NewMesh.faces[fIdx].setSmGroup(smGroupBit);
				}
			}
			else if (pr->type == cgltf_primitive_type_triangle_strip) {
				FaceNum = FaceIdList.size() - 2;
				NewMesh.setNumFaces((int)(FaceNum + FaceOffset), TRUE);
				UINT fIdx = FaceOffset;
				auto f = FaceIdList.begin();
				UINT vIdx1 = (UINT) * (f++) + VertOffset;
				UINT vIdx2 = (UINT) * (f++) + VertOffset;
				UINT vIdx3 = (UINT) * (f++) + VertOffset;
				NewMesh.faces[fIdx].v[0] = vIdx1;
				NewMesh.faces[fIdx].v[1] = vIdx2;
				NewMesh.faces[fIdx].v[2] = vIdx3;
				NewMesh.faces[fIdx].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
				NewMesh.faces[fIdx].setMatID(mId);
				fIdx++;
				BOOL CCW = FALSE;
				for (; f != FaceIdList.end(); f++, fIdx++) {
					vIdx1 = vIdx2;
					vIdx2 = vIdx3;
					vIdx3 = (UINT) * (f)+VertOffset;
					NewMesh.faces[fIdx].v[0] = vIdx1;
					NewMesh.faces[fIdx].v[1] = CCW ? vIdx2 : vIdx3;
					NewMesh.faces[fIdx].v[2] = CCW ? vIdx3 : vIdx2;
					NewMesh.faces[fIdx].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
					NewMesh.faces[fIdx].setMatID(mId);
					CCW = !CCW;
				}
			}
			else if (pr->type == cgltf_primitive_type_triangle_fan) {
				FaceNum = FaceIdList.size() - 1;
				NewMesh.setNumFaces((int)(FaceNum + FaceOffset), TRUE);
				UINT fIdx = FaceOffset;
				auto f = FaceIdList.begin();
				UINT vIdx1 = (UINT) * (f++) + VertOffset;
				UINT vIdx2 = (UINT) * (f++) + VertOffset;
				for (; f != FaceIdList.end(); f++, fIdx++) {
					UINT vIdx3 = (UINT) * (f) + VertOffset;
					NewMesh.faces[fIdx].v[0] = vIdx1;
					NewMesh.faces[fIdx].v[1] = vIdx2;
					NewMesh.faces[fIdx].v[2] = vIdx3;
					NewMesh.faces[fIdx].setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
					NewMesh.faces[fIdx].setMatID(mId);
					vIdx2 = vIdx3;
				}
			}
		}
		/*
		if (pr->type == cgltf_primitive_type_triangle_strip) {
			std::vector<float> FaceIdList;
			if (mc) {
				GetDracoMeshIndexList(mc->buffer_view, FaceIdList);
			}
			else {
				if (pr->indices) {
					GetDataList(FaceIdList, pr->indices);
				}
			}
		}
		*/

		// Setting ShapeLine
		if (primCat == PrimCategory::Lines ) {
			std::vector<uint32_t> KnotIdList;
			bool isClosed = false;
			if (mc) {
				GetDracoMeshIndexList(mc->buffer_view, KnotIdList);
			}
			else {
				if (pr->indices) {
					std::vector<float> floatIndices;
					GetDataList(floatIndices, pr->indices);
					KnotIdList = FloatToUInt32IndexBuffer( floatIndices);
				}
				else {
					for (uint32_t vIdx = 0; vIdx < flatCoordsBuf.size()/3; vIdx++) {
						KnotIdList.push_back(vIdx);
					}
				}
			}
			uint32_t vIdx = VertOffset;

			auto plyShape = PolyShape();
			auto plyLine = plyShape.NewLine();
			plyLine->SetNumPts(static_cast<int>(KnotIdList.size()), FALSE);

			switch(pr->type) {
				case cgltf_primitive_type_lines:
					for(int idx = 0; idx < KnotIdList.size(); idx += 2) {
						(*plyLine)[idx] = PolyPt(pointBuf[KnotIdList[idx]]);
						(*plyLine)[idx+1] = PolyPt(pointBuf[KnotIdList[idx+1]]);						
					}
					break;
				case cgltf_primitive_type_line_loop:
						plyLine->Close();
						[[fallthrough]];
				case cgltf_primitive_type_line_strip:
					for(int idx = 0; idx < KnotIdList.size(); idx++) {
						auto pt = PolyPt(pointBuf[idx]);
						(*plyLine)[idx] = pt;
					}
					if (KnotIdList[0] == KnotIdList[KnotIdList.size() - 1])	
						plyLine->Close();
					break;
				default:
					break;
			}

			// assign to our final Shape node
			NewShape = plyShape;
		}

		// Set the Vertex normals
		std::vector<float> NormalList;
		if (mc) {
			DracoDecodeProc(mc->buffer_view, pr, NormalList, DracoDecodeType::NORMAL);
		}
		else {
			GetDataList(NormalList, findAttrAccessor(pr, "NORMAL"));
		}
		size_t normalNum = NormalList.size() / 3;
		if (normalNum > 0) {
			float quantScale = m_Quantization ? GetQuantScale(findAttrAccessor(pr, "POSITION")) : 1.0F;
			int vIdx = NormalOffset;
			for (std::vector<float>::iterator v = NormalList.begin(); v != NormalList.end(); v += 3, vIdx++) {
				Point3 p( *v, *(v+1), *(v+2));
				VertNormalTable.push_back(p* quantScale);
			}
			//if (normalNum != VertNormalTable.size())VertNormalTable.clear();
		}

		// Set Vertex Ccolor
		std::vector<float> vClrList;
		cgltf_type val_type= cgltf_type_vec4;
		float vcScale = 255.0f;
		if (mc) {
			DracoDecodeProc(mc->buffer_view, pr, vClrList, DracoDecodeType::COLOR);
			cgltf_accessor* acc = findAttrAccessor(pr, "COLOR_0");
			if (acc) {
				if (acc->component_type == cgltf_component_type::cgltf_component_type_r_32f) vcScale = 1.0f;
				val_type = acc->type;
			}
		}
		else {
			cgltf_accessor *acc = findAttrAccessor(pr, "COLOR_0");
			GetDataList(vClrList, acc);
			if (acc) {
				if (acc->component_type == cgltf_component_type::cgltf_component_type_r_32f) vcScale = 1.0f;
				val_type = acc->type;
			}
		}
		size_t data_size = (val_type == cgltf_type_vec3)? 3:4;
		size_t vClrNum = vClrList.size() / data_size;
		if (vClrNum > 0) {
			NewMesh.setMapSupport(0, TRUE);
			NewMesh.setNumMapVerts(0, (int)(VertNum + VertOffset), TRUE);
			NewMesh.setNumMapFaces(0, (int)(FaceNum + FaceOffset), TRUE);

			if (NewMesh.mapFaces(0)) {
				MeshMap *pMap = &NewMesh.Map(0);
				TVFace *pTVFace = &NewMesh.mapFaces(0)[FaceOffset];

				Face *pFace = &NewMesh.faces[FaceOffset];
				for (UINT f = FaceOffset; f < FaceOffset + FaceNum; f++) {
					pTVFace->t[0] = pFace->v[0];
					pTVFace->t[1] = pFace->v[1];
					pTVFace->t[2] = pFace->v[2];
					pTVFace++;
					pFace++;
				}
				UINT vclrIdx = vClrOffset;
				UVVert *pDstUV = &(pMap->tv[vclrIdx]);
				for (std::vector<float>::iterator v = vClrList.begin(); v != vClrList.end(); v += data_size, vclrIdx++) {
					float r = *v / vcScale;
					float g = *(v + 1) / vcScale;
					float b = *(v + 2) / vcScale;
					float alpha = 0.0f;
					if(val_type == cgltf_type_vec4) alpha = *(v + 3);
					*pDstUV++ = UVVert(r, g, b);
				}
			}
		}

		//Set UV1
		std::vector<float> texCoord1List;
		quantScale = 1.0f;
		if (mc) {
			DracoDecodeProc(mc->buffer_view, pr, texCoord1List, DracoDecodeType::TEX_COORD);
		}
		else {
			cgltf_accessor* acc = findAttrAccessor(pr, "TEXCOORD_0");
			//if(CheckBufferSize(acc))
			GetDataList(texCoord1List, acc);
			quantScale = m_Quantization ? GetQuantScale(acc) : 1.0F;
		}

		size_t tex1Num = texCoord1List.size() / 2;
		if (NewMesh.mapSupport(1)&& tex1Num==0) {
			NewMesh.setNumMapVerts(1, VertOffset, TRUE);
			NewMesh.setNumMapFaces(1, FaceOffset, TRUE);

			//tex1Num=VertNum;
			/*
			NewMesh.setNumMapVerts(1, VertNum + VertOffset, TRUE);
			NewMesh.setNumMapFaces(1, FaceNum + FaceOffset, TRUE);
			TVFace *pTVFace = &NewMesh.mapFaces(1)[FaceOffset];
			Face *pFace = &NewMesh.faces[FaceOffset];
			for (UINT f = FaceOffset; f < FaceOffset + FaceNum; f++) {
				pTVFace->t[0] = pFace->v[0];
				pTVFace->t[1] = pFace->v[1];
				pTVFace->t[2] = pFace->v[2];
				pTVFace++;
				pFace++;
			}

			tex1Num = FaceNum * 3;
			UINT vtex1Idx = Tex1Offset;
			MeshMap *pMap = &NewMesh.Map(1);
			UVVert *pDstUV = &(pMap->tv[vtex1Idx]);
			for (int vi = 0; vi< tex1Num; vi++) {
				*pDstUV++ = UVVert(0.0f, 0.0f, 0.0f);
			}
			*/
		}
		else if (tex1Num > 0) {
			NewMesh.setMapSupport(1, TRUE);
			NewMesh.setNumMapVerts(1, (int)(VertNum + VertOffset), TRUE);
			NewMesh.setNumMapFaces(1, (int)(FaceNum + FaceOffset), TRUE);
			TVFace *pTVFace = &NewMesh.mapFaces(1)[FaceOffset];
			Face *pFace = &NewMesh.faces[FaceOffset];
			for (UINT f = FaceOffset; f < FaceOffset + FaceNum; f++) {
				pTVFace->t[0] = pFace->v[0];
				pTVFace->t[1] = pFace->v[1];
				pTVFace->t[2] = pFace->v[2];
				pTVFace++;
				pFace++;
			}
			Box2D RectUV;
			RectUV.max = Point2(0.0f, 0.0f);
			RectUV.min = Point2(1.0f, 1.0f);
			UINT vtex1Idx = Tex1Offset;
			MeshMap *pMap = &NewMesh.Map(1);
			UVVert *pDstUV = &(pMap->tv[vtex1Idx]);
			for (std::vector<float>::iterator v = texCoord1List.begin(); v != texCoord1List.end(); v += 2, vtex1Idx++) {
				float x = *v * quantScale * scale_u + offset_u;
				float y = -(*(v + 1) * quantScale * scale_v + offset_v) + 1.0f;
				if (x > RectUV.max.x) RectUV.max.x = x;
				if (x < RectUV.min.x) RectUV.min.x = x;
				if (y > RectUV.max.y) RectUV.max.y = y;
				if (y < RectUV.min.y) RectUV.min.y = y;
				*pDstUV++ = UVVert(x, y, 0.0f);
			}
			//Point2 UV1Size(max_U - min_U, max_V - min_V);
			if(mId>0) RescaleUVOffset(mtlIdTable[mId-1], RectUV);
		}

		// Vertex UV2 settings
		std::vector<float> texCoord2List;
		quantScale = 1.0f;
		if (mc) {
			DracoDecodeProc(mc->buffer_view, pr, texCoord2List, DracoDecodeType::TEX_COORD, 1);
		}
		else {
			cgltf_accessor* acc = findAttrAccessor(pr, "TEXCOORD_1");
			//if (CheckBufferSize(acc))
			GetDataList(texCoord2List, acc);
			quantScale = m_Quantization ? GetQuantScale(acc) : 1.0F;
		}
		size_t tex2Num = texCoord2List.size() / 2;
		if (NewMesh.mapSupport(2) && tex2Num == 0) {
			NewMesh.setNumMapVerts(2, VertOffset, TRUE);
			NewMesh.setNumMapFaces(2, FaceOffset, TRUE);
		}
		else if (tex2Num > 0) {
			NewMesh.setMapSupport(2, TRUE);
			NewMesh.setNumMapVerts(2, (int)(VertNum + VertOffset), TRUE);
			NewMesh.setNumMapFaces(2, (int)(FaceNum + FaceOffset), TRUE);
			MeshMap *pMap = &NewMesh.Map(2);
			TVFace *pTVFace = &NewMesh.mapFaces(2)[FaceOffset];
			Face *pFace = &NewMesh.faces[FaceOffset];
			for (UINT f = FaceOffset; f < FaceOffset + FaceNum; f++) {
				pTVFace->t[0] = pFace->v[0];
				pTVFace->t[1] = pFace->v[1];
				pTVFace->t[2] = pFace->v[2];
				pTVFace++;
				pFace++;
			}
			UINT vtex2Idx = Tex2Offset;
			UVVert *pDstUV = &(pMap->tv[vtex2Idx]);
			for (std::vector<float>::iterator v = texCoord2List.begin(); v != texCoord2List.end(); v += 2, vtex2Idx++) {
				float x = *v* quantScale;
				float y = *(v + 1)* quantScale;
				*pDstUV++ = UVVert(x, -y+1.0f, 0.0f);
			}
		}

		VertOffset += (int)VertNum;
		FaceOffset += (int)FaceNum;
		NormalOffset += (int)normalNum;
		vClrOffset += (int)vClrNum;
		Tex1Offset += (int)tex1Num;
		Tex2Offset += (int)tex2Num;
	}
	//primId++;
	//smGroupBit = smGroupBit << 1;
	//m_UsingMtlIdxTab.push_back(pr.material);

	if (VertNormalTable.size() > 0) {
		SetNormal(&NewMesh, VertNormalTable);
	}

	INode* pNode = NULL;
	if(ObjectType == PrimCategory::Points) {
		TriObject* pTri = CreateNewTriObject();
		NewMesh.buildBoundingBox();
		pTri->mesh = NewMesh;
		pNode = GetCOREInterface()->CreateObjectNode(pTri);

		if (!pNode) return NULL;
		
		// Maybe 3ds Max will gain a dedicated points object in the future ?
		pNode->VertTicks(TRUE); // enable VertexTicks, otherwise there would be no visual in the viewport
	} 
	else if (ObjectType == PrimCategory::Lines) {
		NewShape.UpdateSels();
		NewShape.InvalidateGeomCache();
		SplineShape* pSpline = (SplineShape*)GetCOREInterface()->CreateInstance(SHAPE_CLASS_ID, splineShapeClassID);
		pSpline->shape = NewShape;
		pNode = GetCOREInterface()->CreateObjectNode(pSpline);
		pNode->SetWireColor(RGB(255, 255, 255));
	}
	else if( ObjectType == PrimCategory::Triangles) {
		TriObject* pTri = CreateNewTriObject();
		pTri->mesh = NewMesh;
		pNode = GetCOREInterface()->CreateObjectNode(pTri);
	} 

	if (!pNode) return NULL;


	pNode->SetName(name.c_str()); //StringToWString(name.C_Str()).c_str());
	//m_MeshNodeMap.insert(std::make_pair(mesh,pNode));
	m_MeshNodeMap[mesh] = pNode;

	if (mtlIdTable.size() >= 1) {
		if (pCompositeMtl) {
			//IMaterialViewportShading *p = (IMaterialViewportShading*)pCompositeMtl->GetInterface(IID_MATERIAL_VIEWPORT_SHADING);
			if (pCompositeMtl->ClassID()== MaterialSwitcherClassID) {
				size_t num = mesh->primitives[0].mappings_count;
				IParamBlock2* pBlock = pCompositeMtl->GetParamBlock(0);
				pBlock->SetCount(0, static_cast<int>(num));
				pBlock->SetCount(1, static_cast<int>(num));
				pBlock->SetValue(2, m_time, 1);
				pBlock->SetValue(3, m_time, static_cast<int>(num));
			}

			cgltf_material_mapping *mappings = mesh->primitives[0].mappings;
			for (int i = 0; i < mesh->primitives[0].mappings_count; i++) {
				cgltf_material *material = mappings[i].material;
				Mtl *pMtl = m_MaterialMap[material];
				tstring str = StringToWString(m_VariantTable[i].c_str());
				TSTR name;
				name.printf(_T("%s__%s"), pMtl->GetName().data(), str.c_str());
				pMtl->SetName(name);
				pMtl->ActivateTexDisplay(FALSE);
				if (pCompositeMtl->ClassID() == MaterialSwitcherClassID) {//(pCompositeMtl->ClassID() == glTFMtlSwitcherClassID) {
					IParamBlock2* pBlock = pCompositeMtl->GetParamBlock(0);
					pBlock->SetValue(0, m_time, pMtl, i);
					pBlock->SetValue(1, m_time, str.c_str(), i);
				}
				else if (pCompositeMtl->ClassID() == ArnoldSwitchShaderID) {//(pCompositeMtl->ClassID() == glTFMtlSwitcherClassID) {
					IParamBlock2* pBlock = pCompositeMtl->GetParamBlock(1);
					pBlock->SetValue(i+3, m_time, pMtl);
				}
				else if(pCompositeMtl->ClassID() == CoronaSelectMtlID) {
					IParamBlock2* pBlock = pCompositeMtl->GetParamBlock(0);
					pBlock->SetValue(312, m_time, pMtl, i);
				}
				else {
					pCompositeMtl->SetSubMtl(i, pMtl);
				}
			}
			pNode->SetMtl(pCompositeMtl);
			if ((pCompositeMtl->ClassID() == glTFMtlSwitcherClassID) ||
				(pCompositeMtl->ClassID() == PBRMtlSwitcherClassID) ||
				(pCompositeMtl->ClassID() == PBRMtlSwitcherClassID) ||
				(pCompositeMtl->ClassID() == USDMtlSwitcherClassID) ||
				(pCompositeMtl->ClassID() == PysicMtlSwitcherClassID) ){
				GetCOREInterface()->SelectNode(pNode);
#if MAX_RELEASE >= 24000
				ExecuteMAXScriptScript(_T("global mm = $.material"), MAXScript::ScriptSource::NonEmbedded);
				ExecuteMAXScriptScript(_T("mm.Init()"), MAXScript::ScriptSource::NonEmbedded);
#else
				ExecuteMAXScriptScript(_T("global mm = $.material"));
				ExecuteMAXScriptScript(_T("mm.Init()"));
#endif
			}
			else if (pCompositeMtl->ClassID() == CoronaSelectMtlID) {
				IParamBlock2* pBlock = pCompositeMtl->GetParamBlock(0);
				pBlock->SetValue(311, m_time, (int)(mesh->primitives[0].mappings_count));
			}
			else {
			}
			//pCompositeMtl->EnableMap(0, TRUE);
			//m_MaterialMap[mappings[0].material]->ActivateTexDisplay(TRUE);
			//pCompositeMtl->SetActiveMB(m_MaterialMap[0]);
		}
		else {
			pNode->SetMtl(mtlIdTable[0]);
		}

		if (mtlIdTable.size() > 1 && mesh->primitives[0].mappings_count==0) {
			MultiMtl* pNewMtl = NewDefaultMultiMtl();
			pNewMtl->SetNumSubMtls(0);
			int idx = 1;
			for (auto m : mtlIdTable) {
				pNewMtl->SetSubMtlAndName(idx++, m, m->GetName());
			}
			pNewMtl->RemoveMtl(0);	// The first one is unnecessary
			pNode->SetMtl(pNewMtl);
		}
	}

	AttachNodeExtensions(pNode, node);

	if (ViewVertColor) {
		pNode->SetCVertMode(TRUE);
		pNode->SetShadeCVerts(TRUE);
	}


	//m_NodeUVSizeMap.insert(std::make_pair(pNode, UV1Size));

	return pNode;
}

//======================================================================
// Create Node Object
//======================================================================
void glTFImporter_Core::CreateNodeInfosRec(cgltf_node *node, INode *targetParent)
{
#ifdef MAX_RELEASE_R24
	Matrix3 tm;
#else
	Matrix3 tm(1);
#endif

	if (node->has_matrix) {
		float *mtx = node->matrix;
		tm = Matrix3(
			Point3(mtx[0], mtx[1], mtx[2]),
			Point3(mtx[4], mtx[5], mtx[6]),
			Point3(mtx[8], mtx[9], mtx[10]),
			Point3(mtx[12], mtx[13], mtx[14])*m_scale);
	}
	else {
		float scl[] = { 1.0, 1.0 ,1.0 };
		if (node->has_scale) {
			scl[0] = node->scale[0];
			scl[1] = node->scale[1];
			scl[2] = node->scale[2];
		}
		float rot[] = { 0.0, 0.0 ,0.0, 0.0 };
		if (node->has_rotation) {
			rot[0] = node->rotation[0];
			rot[1] = node->rotation[1];
			rot[2] = node->rotation[2];
			rot[3] = node->rotation[3];
		}
		Quat q(-rot[0], -rot[1], -rot[2], rot[3]);
		float pos[] = { 0.0, 0.0 , 0.0 };
		if (node->has_translation) {
			pos[0] = node->translation[0];
			pos[1] = node->translation[1];
			pos[2] = node->translation[2];
		}

		tm.IdentityMatrix();
		tm.SetRotate(q);
		tm.PreScale(Point3(scl[0], scl[1], scl[2]));
		tm.SetTrans(Point3(pos[0], pos[1], pos[2])*m_scale);
	}

	INode *pNewObject = NULL;
	TSTR baseName;

	if(node->camera) {
		baseName = _T("Camera");
		pNewObject = CreateCamera(node);
		
		if (!pNewObject) return;
	}
	else if(node->light) {
		baseName = _T("Light");
		pNewObject = CreateLight(node);
		
		if (!pNewObject) return;
	}
	else if(!node->mesh) {
		baseName = _T("Dummy");

		DummyObject* pObj =  static_cast<DummyObject*>(GetCOREInterface()->CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0)));
		pObj->SetBox(Box3(Point3(-10, -10, -10), Point3(10, 10, 10)));
		pNewObject = GetCOREInterface()->CreateObjectNode(pObj);

		if (m_HideDummy) pNewObject->Hide(TRUE);
	}

	else {
		baseName = _T("Object");
		pNewObject = CreateMaxNode(node, targetParent);
		
		if (!pNewObject) return;
		
		if (!pNewObject->GetMtl() && pNewObject->GetObjectRef()->SuperClassID()== GEOMOBJECT_CLASS_ID) {
			pNewObject->SetWireColor(RGB(128,128,128));
		}
	}

	if (m_AvoidDupName) {
		TSTR name = pNewObject->GetName();
		if(_tcslen(name) == 0) {
			name = baseName;
		}
		GetCOREInterface()->MakeNameUnique(name);
		pNewObject->SetName(name);
	}

	// featch and attach extras as Custom Attributes/User Props
	cgltf_size size;
	cgltf_result ret = cgltf_copy_extras_json(m_glTF_data, &node->extras, NULL, &size);
	if (size > 0) {
		std::vector<custAttrParam> attrTbl;
		CreateParamTableFromExtras(node->extras, size, attrTbl);
		//AttachCustAttr(pNewObject->GetObjectRef(), attrTbl);
		if (m_ExtraToUserProp)
			SetUserPropParam(pNewObject, attrTbl);
		if(m_ExtraToCustAttr)
			AttachCustAttr(pNewObject, attrTbl);
	}

	m_NodeMap.insert(std::make_pair(node, pNewObject));
	SetNodeImportStatus(1);

	if (targetParent) {
		pNewObject->SetNodeTM(0, tm*targetParent->GetNodeTM(m_time));
		if(!ISFlatHierarchy())targetParent->AttachChild(pNewObject);
	}
	else
		pNewObject->SetNodeTM(0, tm*YupTM);

	if(IsLogOut())
	{
		AffineParts parts;
		float xyz[3];

		decomp_affine(tm, &parts);
		tstring posStr1 = _T("T:[") + to_tstring(parts.t.x) + _T(",");
		posStr1 += to_tstring(parts.t.y) + _T(",");
		posStr1 += to_tstring(parts.t.z) + _T("]");
		QuatToEuler(parts.q, xyz);
		tstring rotStr1 = _T("R:[") + to_tstring(RadToDeg(xyz[0])) + _T(",");
		rotStr1 += to_tstring(RadToDeg(xyz[1])) + _T(",");
		rotStr1 += to_tstring(RadToDeg(xyz[2])) + _T("]");
		tstring sclStr1 = _T("S:[") + to_tstring(parts.k.x) + _T(",");
		sclStr1 += to_tstring(parts.k.y) + _T(",");
		sclStr1 += to_tstring(parts.k.z) + _T("]");

		Matrix3 postTM = pNewObject->GetNodeTM(0);
		decomp_affine(postTM, &parts);
		tstring posStr2 = _T("[") + to_tstring(parts.t.x) + _T(",");
		posStr2 += to_tstring(parts.t.y) + _T(",");
		posStr2 += to_tstring(parts.t.z) + _T("]");
		QuatToEuler(parts.q, xyz);
		tstring rotStr2 = _T("[") + to_tstring(RadToDeg(xyz[0])) + _T(",");
		rotStr2 += to_tstring(RadToDeg(xyz[1])) + _T(",");
		rotStr2 += to_tstring(RadToDeg(xyz[2])) + _T("]");
		tstring sclStr2 = _T("[") + to_tstring(parts.k.x) + _T(",");
		sclStr2 += to_tstring(parts.k.y) + _T(",");
		sclStr2 += to_tstring(parts.k.z) + _T("]");

		LogOutput(_T("--------------"));
		LogOutput(StringToWString(node->name));
		LogOutput(posStr1 + _T("->") + posStr2);
		LogOutput(rotStr1 + _T("->") + rotStr2);
		LogOutput(sclStr1 + _T("->") + sclStr2);
	}

	if (node->has_mesh_gpu_instancing) {
		std::vector<Point3> posList;
		std::vector<Quat> rotList;
		std::vector<Point3> sclList;
		cgltf_mesh_gpu_instancing* inst = &node->mesh_gpu_instancing;
		for (int xx = 0; xx < inst->attributes_count; xx++) {
			cgltf_attribute* attr = &inst->attributes[xx];
			if (!_stricmp(attr->name, "TRANSLATION")) {
				cgltf_accessor* acc = attr->data;
				std::vector<float> tList;
				GetDataList(tList, acc);
				for (auto x = tList.begin(); x < tList.end(); x+=3) {
					Point3 p(*x, *(x+ 1), *(x + 2));
					posList.push_back(p);
				}
			}
			if (!_stricmp(attr->name, "ROTATION")) {
				cgltf_accessor* acc = attr->data;
				std::vector<float> rList;
				GetDataList(rList, acc);
				for (auto x = rList.begin(); x < rList.end(); x += 4) {
					Quat p(*x, *(x + 1), *(x + 2), *(x + 3));
					rotList.push_back(p);
				}
			}
			if (!_stricmp(attr->name, "SCALE")) {
				cgltf_accessor* acc = attr->data;
				std::vector<float> sList;
				GetDataList(sList, acc);
				for (auto x = sList.begin(); x < sList.end(); x += 3) {
					Point3 p(*x, *(x + 1), *(x + 2));
					sclList.push_back(p);
				}
			}
		}

		for (UINT xx = 0; xx < posList.size(); xx++) {
			Matrix3 tm;
			tm.IdentityMatrix();
			if (xx < rotList.size()) tm.SetRotate(rotList.at(xx));
			if (xx < sclList.size()) tm.PreScale(sclList.at(xx));
			tm.SetTrans(posList.at(xx) * m_scale);
			if (targetParent)
				tm = tm * targetParent->GetNodeTM(m_time);
			else
				tm = tm * YupTM;

			if (xx == 0) {
				pNewObject->SetNodeTM(m_time, tm);
			}
			else {
				INode *pNode = GetCOREInterface()->CreateObjectNode(pNewObject->GetObjectRef());
				pNode->SetNodeTM(m_time, tm);

				if (pNewObject->GetMtl())
					pNode->SetMtl(pNewObject->GetMtl());
				else
					pNode->SetWireColor(RGB(128, 128, 128));

			}
		}
	}
	//Matrix3 tmm = tm * YupTM;

	cgltf_extension* ext = node->extensions;
	for (int cnt = 0; cnt < node->extensions_count; cnt++, ext++) {
		char* name = ext->name;
		char* data = ext->data;
		if (!_stricmp(name, "KHR_physics_rigid_bodies"))
			CreateRigidTable(pNewObject, data);
	}

	for (int ch = 0; ch < node->children_count; ch++) {
		CreateNodeInfosRec(node->children[ch], pNewObject);
	}
}

//======================================================================
// Attach Extension params 
//======================================================================
void glTFImporter_Core::AttachNodeExtensions(INode* pNode, cgltf_node* node)
{
	if (!pNode || !node) return;

	if (node->has_node_visibility) {
		VisibilityStruct str;
		str.visible = node->visibility.visible;
		CreateVisibilityAttr(pNode, str, TRUE);
	}

	if (node->has_node_hoverability) {
		HoverabilityStruct str;
		str.hoverable = node->hoverabilty.hoverable;
		CreateHoverabilityAttr(pNode, str, TRUE);
	}

	if (node->has_node_selectability) {
		SelectabilityStruct str;
		str.selectable = node->selectability.selectable;
		CreateSelectabilityAttr(pNode, str, TRUE);
	}

}
