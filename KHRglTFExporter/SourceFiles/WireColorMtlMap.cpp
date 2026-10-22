// TODO: This file appears to be redundant and not used anywhere and may be removable ?
#include "KHRglTFExporter.h"



void glTFExporter_Core::CreateWireColorMtlMap(INode *pNode)
{
	if (!pNode) return;

	if (!pNode->GetMtl()) {
		DWORD c = pNode->GetWireColor();
		if (m_WireColorMtlMap.find(c) == m_WireColorMtlMap.end()) {
			StdMat2* pSmat = (StdMat2*)NewDefaultStdMat();
			pSmat->SetDiffuse(Color(c), m_time);
			TSTR name;
			name.printf(_T("WireColorMtl%d"),c);
			pSmat->SetName(name);
			m_WireColorMtlMap[c] = pSmat;
		}
	}
		
	for (int i = 0; i < pNode->NumChildren(); i++) {
		CreateWireColorMtlMap(pNode->GetChildNode(i));
	}
}
