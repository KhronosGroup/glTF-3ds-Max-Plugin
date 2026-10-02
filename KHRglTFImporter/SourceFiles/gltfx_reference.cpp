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
#include <AssetManagement/iassetmanager.h>
//#include <AssetManagement/AssetUser.h>
#include <fstream>
#include <json/json.h>

#pragma comment(lib, "jsoncpp_static.lib")

extern  int GetImportedNodeTab(INodeTab &tab);

struct AssetStr {
	tstring uri;
	int environment;
};

static Quat s_Xaxis90;

std::vector<MaxSDK::AssetManagement::AssetUser> XRefAssetTable;

BOOL CreateAssetTable(const TCHAR* filename, std::vector<AssetStr>& tbl);
BOOL CreateReferenceBase(const TCHAR* filename, float scale);
INode *CreateAssetRec(cgltf_node* node, INode* pParent, float scale);

//======================================================================
//======================================================================
BOOL glTFImporter_Core::gltfx_reference(const TCHAR* filename)
{
	std::vector<AssetStr> assetTbl;
	if (!CreateAssetTable(filename, assetTbl)) return FALSE;

	float f = DegToRad(90.0f) / 2.0f;
	s_Xaxis90 = Quat(sin(f), 0.0f, 0.0f, cos(f));

	TSTR profle;
	profle.printf(_T("%s\\%s"), GetCOREInterface()->GetDir(APP_PLUGCFG_DIR), _T("KHRglTFImporter.ini"));
	MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("scale"), _T("1000"), profle);

	HWND hDlg = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_GLTFX_DLG), ::GetActiveWindow(), (DLGPROC)NULL, NULL);
	HWND hProgressBar = GetDlgItem(hDlg, IDC_PROGRESS1);
	CenterWindow(hDlg, GetParent(hDlg));

	INode* pRootNode = GetCOREInterface()->GetRootNode();

	Class_ID cid = KHRglTFImporter_CLASS_ID;
	std::filesystem::path p(filename);
	tstring p_path = p.parent_path();
#if MAX_RELEASE > 26000
	tstring m_path = GetCOREInterface()->GetDir(APP_SCENE_DIR).data();
#else
	tstring m_path = GetCOREInterface()->GetDir(APP_SCENE_DIR);
#endif

	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, assetTbl.size()));
	SendMessage(hProgressBar, PBM_SETSTEP, (WPARAM)1, 0);
	int pos = 0;
	XRefAssetTable.clear();

	for (auto as : assetTbl) {
		SendMessage(hProgressBar, PBM_SETPOS, ++pos, 0);
		SetWindowText(GetDlgItem(hDlg, IDC_GLTFX_STATIC),as.uri.c_str());
		std::filesystem::path p(as.uri);
		tstring gltf_file = p_path + _T("\\") + as.uri;
		tstring max_file = m_path + _T("\\") + p.stem().c_str() + tstring(_T(".max"));
		GetCOREInterface()->ImportFromFile(gltf_file.c_str(), TRUE, &cid);
		INodeTab tt;
		GetImportedNodeTab(tt);
		if (tt.Count() > 0) {
			GetCOREInterface()->SelectNodeTab(tt, TRUE);
			GetCOREInterface()->FileSaveSelected(max_file.c_str());
			GetCOREInterface14()->DeleteNodes(tt);

			MaxSDK::AssetManagement::AssetUser asset = MaxSDK::AssetManagement::IAssetManager::GetInstance()->GetAsset(TSTR(max_file.c_str()), MaxSDK::AssetManagement::AssetType::kXRefAsset);
			//pRootNode->AddNewXRefFile(asset);
			XRefAssetTable.push_back(asset);
		}
	}

	CreateReferenceBase(filename, m_scale);

	::EndDialog(hDlg, 0);

	TCHAR buf[MAX_PATH];
	_stprintf_s(buf, MAX_PATH, _T("%d"), (int)(m_scale * 1000.0f));
	MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("scale"), buf, profle);

	return TRUE;
}

//======================================================================
//======================================================================
BOOL CreateAssetTable(const TCHAR* filename, std::vector<AssetStr> &tbl)
{
	tbl.clear();

	// Open File Stream
	std::ifstream ifs(filename);
	if (!ifs.is_open()) return FALSE;

	// JSON parser preparation
	Json::CharReaderBuilder builder;
	builder["collectComments"] = false;
	Json::Value root;
	std::string errs;

	// File parsing
	if (!Json::parseFromStream(builder, ifs, &root, &errs)) return FALSE;

	// Retrieving and processing the "assets" array
	const Json::Value& assets = root["assets"];
	if (!assets.isArray()) return FALSE;

	/// Process each asset
	for (const auto& asset : assets) {
		AssetStr a;
		a.environment = -1;
		if (asset.isMember("uri")) {
			a.uri = StringToWString(asset["uri"].asString().c_str());
		}
		else if (asset.isMember("environment")) {
			a.environment = asset["environment"].asInt();
		}
		tbl.push_back(a);
	}

	return TRUE;
}

//======================================================================
//======================================================================
BOOL CreateReferenceBase(const TCHAR* filename, float scale)
{
	setlocale(LC_NUMERIC, "en_US");
	UINT CodePage = CP_ACP;

	cgltf_options options = { cgltf_file_type_invalid };
	cgltf_data* glTF_data = NULL;
	cgltf_result result = cgltf_parse_file(&options, WStringToString(filename, CodePage).c_str(), &glTF_data);
	if (result != cgltf_result_success) return FALSE;

	result = cgltf_load_buffers(&options, glTF_data, WStringToString(filename, CodePage).c_str());
	if (result != cgltf_result_success) return FALSE;

	for (int i = 0; i < glTF_data->scenes_count; i++) {
		cgltf_scene* pScene = &glTF_data->scenes[i];
		tstring sceneName;
		if (!pScene->name)
			sceneName = _T("scene") + to_tstring(i);
		else
			sceneName = StringToWString(pScene->name);

		for (int j = 0; j < pScene->nodes_count; j++) {
			CreateAssetRec(pScene->nodes[j], NULL, scale);
		}
	}

	cgltf_free(glTF_data);

	return TRUE;
}

//======================================================================
//======================================================================
INode* CreateAssetRec(cgltf_node* node, INode* pParent, float scale)
{
	INode* pRootNode = GetCOREInterface()->GetRootNode();

	DummyObject* pObj = (DummyObject*)GetCOREInterface()->CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0));
	pObj->SetBox(Box3(Point3(-2, -2, -2), Point3(2, 2, 2)));
	INode* pNode = GetCOREInterface()->CreateObjectNode(pObj);
	pNode->Hide(TRUE);

	tstring name = StringToWString(node->name);
	if (name.size() > 0) {
		pNode->SetName(name.c_str());
	}

	if (node->asset_index >= 0) {
		auto asset = XRefAssetTable.at(node->asset_index);
		pRootNode->AddNewXRefFile(asset);
		int index = pRootNode->GetXRefFileCount() - 1;

		INode* pXRefNode = pRootNode->GetXRefTree(index);
		pRootNode->SetXRefParent(index, pNode);
	}

#ifdef MAX_RELEASE_R24
	Matrix3 tm;
#else
	Matrix3 tm(1);
#endif

	if (node->has_matrix) {
		float* mtx = node->matrix;
		tm = Matrix3(
			Point3(mtx[0], mtx[1], mtx[2]),
			Point3(mtx[4], mtx[5], mtx[6]),
			Point3(mtx[8], mtx[9], mtx[10]),
			Point3(mtx[12], -mtx[14], mtx[13]) * scale);
	}
	else {
		float scl[] = { 1.0, 1.0 ,1.0 };
		if (node->has_scale) {
			scl[0] = node->scale[0];
			scl[1] = node->scale[1];
			scl[2] = node->scale[2];
		}
		Quat q;
		if (node->has_rotation) {
			float rot[] = { 0.0, 0.0 ,0.0, 0.0 };
			rot[0] = node->rotation[0];
			rot[1] = node->rotation[1];
			rot[2] = node->rotation[2];
			rot[3] = node->rotation[3];
			Quat q2(-rot[0], -rot[1], -rot[2], rot[3]);
			q = s_Xaxis90 + q2 * YupTM;
			//q = q2 * (YupTM);

		}
		float pos[] = { 0.0, 0.0 , 0.0 };
		if (node->has_translation) {
			pos[0] = node->translation[0];
			pos[1] = node->translation[1];
			pos[2] = node->translation[2];
		}

		tm.IdentityMatrix();
		tm.SetRotate(q);
		tm.PreScale(Point3(scl[0], scl[1], scl[2]));
		tm.SetTrans(Point3(pos[0], -pos[2], pos[1]) * scale);
	}

	if (pParent) {
		pNode->SetNodeTM(0, tm * pParent->GetNodeTM(0));
		pParent->AttachChild(pNode);
	}
	else
		pNode->SetNodeTM(0, tm);

	for (int j = 0; j < node->children_count; j++) {
		CreateAssetRec(node->children[j], pNode, scale);
	}

	return pNode;
}