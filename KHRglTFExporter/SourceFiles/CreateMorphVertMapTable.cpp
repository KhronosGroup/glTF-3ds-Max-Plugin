// TODO: This file appears to be redundant and not used anywhere and may be removable ?
#include "KHRglTFExporter.h"

#define EPS 0.0001f

void glTFExporter_Core::CreateMorphVertMapTable(Modifier *pMorphMod, Mesh *pBaseMesh, MeshNormalSpec* pBaseNrmSpec, std::vector<std::map<int, Point3> >& morphNormalMapList)
{
	morphNormalMapList.clear();

	MaxMorphModifier maxMorphModifier(pMorphMod);
	for (int i = 0; i < maxMorphModifier.NumMorphChannels(); i++) {
		MaxMorphChannel mc = maxMorphModifier.GetMorphChannel(i);
		if (!mc.HasData()) continue;
		if (!mc.IsActive()) continue;

		INode* pTargetNode = mc.GetMorphTarget();
		BOOL deleteIt;
		TriObject *pTri = GetTriObjectFromNode(pTargetNode, 0, deleteIt);
		Mesh *pTargetMesh = &pTri->mesh;

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

