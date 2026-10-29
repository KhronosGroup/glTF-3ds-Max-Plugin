//======================================================================
//======================================================================

#include "KHRglTFExporter.h"


#pragma comment(lib, "Morpher.lib")

constexpr float ExportWeightScaleFactor = 0.01f;

//======================================================================
// Find the top node in the table
//======================================================================

//======================================================================
// Morph Assignment
//======================================================================
void glTFExporter_Core::CreateMorph(INode *pNode, Modifier *pMorphMod)
{
}

BOOL glTFExporter_Core::HasMorphModifier(INode* pNode)
{
	Modifier* pMod = NULL;
	return (FindModifier(pNode, MR3_CLASS_ID, &pMod) >= 0);
}

//======================================================================
//======================================================================
Control* glTFExporter_Core::GetMorphController(Modifier* pMod, int idx)
{
	MaxMorphModifier maxMorphModifier(pMod);
	Control *pCtrl = maxMorphModifier.GetMorphChannel(idx).GetMorphWeightController();

	return pCtrl;
}

//======================================================================
//======================================================================
void glTFExporter_Core::CreateMorphTableRec(INode *pNode)
{
	Modifier *pMod = NULL;
	if (FindModifier(pNode, MR3_CLASS_ID, &pMod) >= 0) {
		m_morphNodeTable.insert(std::make_pair(pNode, pMod));

		MaxMorphModifier maxMorphModifier(pMod);
		for (int i = 0; i < maxMorphModifier.NumMorphChannels(); i++) {
			MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(i);
			INode* pTargetNode = mc.GetMorphTarget();
			if(pTargetNode) m_morphTargetTable.push_back(pTargetNode);
		}
	}

	for (int i = 0; i < pNode->NumChildren(); i++) {
		CreateMorphTableRec(pNode->GetChildNode(i));
	}
}

//======================================================================
//======================================================================
void glTFExporter_Core::SetMorphTagetList(tinygltf::Mesh& mesh, Modifier* pMod)
{
//	Modifier* pMod = NULL;
//	if (FindModifier(pNode, MR3_CLASS_ID, &pMod) < 0) return;

	std::vector<tstring> tbl;
	MaxMorphModifier maxMorphModifier(pMod);
	for (int i = 0; i < maxMorphModifier.NumMorphChannels(); i++) {
		MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(i);
#if MAX_RELEASE>=24000
		if (mc.HasData() && mc.IsActive()) tbl.push_back(tstring(mc.GetName(TRUE)));
#else
		if (mc.HasData() && mc.IsActive()) tbl.push_back(tstring(mc.GetName()));
#endif
		//INode* pTargetNode = mc.GetMorphTarget();
		//if (pTargetNode) tbl.push_back(pTargetNode->GetName());
	}

	tinygltf::Value::Array n;
	for(auto name : tbl) {
		std::string str = WStringToString(name);
		n.push_back(tinygltf::Value(str));
	}
	tinygltf::Value::Object obj;
	obj.insert(std::make_pair("targetNames", tinygltf::Value(n)));

	mesh.extras = tinygltf::Value(obj);
}

//======================================================================
//======================================================================
void glTFExporter_Core::CreateMorphTable(void)
{
	m_morphTargetTable.clear();

	for (int i = 0; i < GetCOREInterface()->GetRootNode()->NumChildren(); i++) {
		CreateMorphTableRec(GetCOREInterface()->GetRootNode()->GetChildNode(i));
	}
}


//======================================================================
//======================================================================
void glTFExporter_Core::SetMorphWeight(Modifier* pMod, std::vector<double>& weights)
{
	weights.clear();

	MaxMorphModifier maxMorphModifier(pMod);
	for (int i = 0; i < maxMorphModifier.NumMorphChannels(); i++) {
		MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(i);
		if (mc.HasData()) weights.push_back(mc.GetMorphWeight(m_time) * ExportWeightScaleFactor);
	}
}

//======================================================================
//======================================================================
int glTFExporter_Core::GetMorphTargetNum(Modifier* pMod)
{
	int cnt = 0;

	MaxMorphModifier maxMorphModifier(pMod);
	for (int i = 0; i < maxMorphModifier.NumMorphChannels(); i++) {
		MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(i);
		if (mc.HasData()) cnt++;
	}

	return cnt;
}

//======================================================================
//======================================================================
/*
Mesh* glTFExporter_Core::GetMorphTargetMesh(Modifier* pMod, int idx)
{
	auto tbl = m_morphTargetNodeMapTable[pMod];
	INode* pNode = tbl.at(idx);

	BOOL deleteIt = FALSE;
	TriObject* pTri = GetTriObjectFromNode(pNode, m_time, deleteIt);
	Mesh* pMesh = &pTri->mesh;

	return pMesh;

#if 0
	MorphR3* pMorphMod = (MorphR3*)pMod;

	int cnt = 0;
	morphChannel* pmChan = NULL;
	for (int i = 0; i < MR3_NUM_CHANNELS; i++) {
		if (!(pmChan = &pMorphMod->chanBank[i])) continue;
		if (pmChan->mActive) {
			if (cnt == idx) {
				INode* pTargetNode = pmChan->mConnection;
				BOOL deleteIt = FALSE;
				TriObject* pTri = GetTriObjectFromNode(pTargetNode, m_time, deleteIt);
				Mesh* pMesh = &pTri->mesh;
				return pMesh;
			}
			cnt++;
		}
	}

	return NULL;
#endif
}
*/

//======================================================================
//======================================================================
void glTFExporter_Core::SetMorphTargetPositionTable(MaxMorphChannel& mc, const std::vector<int>& faceIDTable, Mesh* pMesh, std::vector<Point3>& targetPtTbl)
{
#if 0
	TSTR ComStr;
	FPValue ret;

	targetPtTbl.clear();
	Face* pFace = pMesh->faces;
	for (auto fID : faceIDTable) {
		for (int i = 0; i < 3; i++) {
			int vID = pFace[fID].v[i];
			Point3 BasePt = pMesh->verts[vID];
			ComStr.printf(_T("WM3_MC_GetMorphPoint $.Morpher %d %d"), chID + 1, vID);
			ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, 0, &ret);
			Point3 pt = *ret.p - BasePt;

			//Point3 pt = pTargetMesh->verts[vID];
			targetPtTbl.push_back(pt);
		}
	}

#else

	//	MaxMorphModifier maxMorphModifier(pMod);
	//	MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(chID);

	targetPtTbl.clear();
	for (auto fID : faceIDTable) {
		Face face = pMesh->faces[fID];
		for (int i = 0; i < 3; i++) {
			int vID = face.v[i];
			Point3 BasePt = pMesh->verts[vID];
			Point3 pt = mc.GetMorphPoint(vID) - BasePt;
			targetPtTbl.push_back(pt);
		}
	}

#endif
}

//======================================================================
//======================================================================
void glTFExporter_Core::SetMorphTargetNormalTable(const std::vector<int>& faceIDTable, std::map<int, Point3>& morphNormalMap, std::vector<Point3>& targetNrmTbl)
{
	targetNrmTbl.clear();
	for (auto fID : faceIDTable) {
		auto n0 = morphNormalMap[fID * 10 + 0];
		auto n1 = morphNormalMap[fID * 10 + 1];
		auto n2 = morphNormalMap[fID * 10 + 2];
		targetNrmTbl.push_back(n0);
		targetNrmTbl.push_back(n1);
		targetNrmTbl.push_back(n2);
	}
}

#define EPS 0.0001f

void glTFExporter_Core::CreateMorphVertMapTable(Modifier* pMorphMod, Mesh* pBaseMesh, MeshNormalSpec* pBaseNrmSpec, std::vector<std::map<int, Point3> >& morphNormalMapList)
{
	morphNormalMapList.clear();

	MaxMorphModifier maxMorphModifier(pMorphMod);
	for (int i = 0; i < maxMorphModifier.NumMorphChannels(); i++) {
		MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(i);
		if (!mc.HasData()) continue;
		if (!mc.IsActive()) continue;

		INode* pTargetNode = mc.GetMorphTarget();
		if (!pTargetNode) continue;
		BOOL deleteIt;
		TriObject* pTri = GetTriObjectFromNode(pTargetNode, 0, deleteIt);
		Mesh* pTargetMesh = &pTri->mesh;

		pTargetMesh->SpecifyNormals();
		MeshNormalSpec* pNrmSpec = pTargetMesh->GetSpecifiedNormals();
		pNrmSpec->BuildNormals();
		pNrmSpec->ComputeNormals();

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

					Point3& tNrm0 = pNrmSpec->GetNormal(tf, 0);
					Point3& tNrm1 = pNrmSpec->GetNormal(tf, 1);
					Point3& tNrm2 = pNrmSpec->GetNormal(tf, 2);
					normalMap[f * 10 + 0] = tNrm0 - nrm0;
					normalMap[f * 10 + 1] = tNrm1 - nrm1;
					normalMap[f * 10 + 2] = tNrm2 - nrm2;

					break;
				}
			}
		}

		if (deleteIt) delete pTri;

		morphNormalMapList.push_back(normalMap);
	}
}

/*
void glTFExporter_Core::SetMorphTargetNormalTable(MaxMorphChannel& mc, const std::vector<int>& faceIDTable, Mesh* pMesh, MeshNormalSpec* pNrmSpec, std::vector<Point3>& targetNrmTbl)
{
	//MaxMorphModifier maxMorphModifier(pMod);
	//MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(chID);

	targetNrmTbl.clear();
	for (auto fID : faceIDTable) {
		MeshNormalFace& face = pNrmSpec->Face(fID);
		for (int i = 0; i < 3; i++) {
			//int nID = face.GetNormalID(i);
			//Point3 BasePt = pMesh->verts[vID];
			Point3& nrm = pNrmSpec->GetNormal(fID, i);
			targetNrmTbl.push_back(nrm);
		}
	}
}
*/