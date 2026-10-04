
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


//#include "maxscript/maxscript.h"
#include "simpobj.h"	// Required for GenSphere

#define CGLTF_IMPLEMENTATION

//#define USE_REFACTORED_SETSPARSEDATA

#include "KHRglTFImporter.h"
#include <IMaterialViewportShading.h>
#include <shlwapi.h>
#include <ilayermanager.h>
#include <ilayer.h>

#include <maxscript/util/listener.h>

//=================================================================
#define MAX_START_TIME 100000
//=================================================================
//=================================================================
static tstring s_TitleString;
static BOOL HH_DebugMode;
static BOOL HH_Animation;
static BOOL HH_MatchAnim;
static int HH_SceneMode;
static int HH_SceneChannel;
static int HH_AnimChannel;
static float HH_scale;
static int HH_MtlMode;
static BOOL HH_FlipNormalGrn;
static BOOL HH_FlipNormalRed;
static BOOL HH_CompositeMtl;
static BOOL HH_ViewVertexColor;
static BOOL HH_HideDummy;
//static BOOL HH_NameObject;
static BOOL HH_AvoidDupName;
static BOOL HH_UniqueTexture;
static BOOL HH_UseColorComposite;
static BOOL HH_CorrectGamma;
static int HH_GammaValue;
static BOOL HH_MapUnpackMode;
static BOOL HH_LaunchScript;
static BOOL HH_Instancing;
static BOOL HH_EnableBkColor;
static BOOL HH_ExtraToCustAttr;
static BOOL HH_ExtraToUserProp;
static BOOL HH_ScaleLightIntensity;
static BOOL HH_UseQuatCtrl;
static BOOL HH_ColorManagement;

static BOOL HH_LogOut;
static BOOL HH_FlatHierarchy;
static BOOL HH_ApplyScale;

static BOOL s_ArnoldMtlEnable = FALSE;
static BOOL s_USDMtlEnable = FALSE;
static BOOL s_gltfMtlEnable = FALSE;
static BOOL s_VRayMtlEnable = FALSE;
static BOOL s_CoronaMtlEnable = FALSE;
static BOOL s_OpenPBRMtlEnable = FALSE;
static tstring s_ScriptString;
static tstring s_ImageFileString;

static FPValue s_CurrentCustAttrObj;
void SetCurrentAttrObj(FPValue &pObj)
{
	s_CurrentCustAttrObj = pObj;
}

static INodeTab s_ImportedNodeTab;
int GetImportedNodeTab(INodeTab& tab)
{
	tab.ZeroCount();
	for (int i = 0; i < s_ImportedNodeTab.Count(); i++){
		INode* pNode = s_ImportedNodeTab[i];
		if(pNode->IsDeleted(pNode)) continue;
		tab.AppendNode(pNode);
	}

	return tab.Count();
}

static const TCHAR *pLicenseStr = _T(
	"glTF/glb Importer for 3dsmax Designed By Satoshi Hayashi\r\n\r\n \
jkuhlmann/cgltf is licensed under the MIT License.\r\n \
Draco is licensed under the Apache 2.0.\r\n \
V-Ray® and Chaos Corona are registered trademarks of Chaos Software Ltd.\r\n \
Pencil+® is registered trademarks of P SOFTHOUSE Co., Ltd..\r\n \
\r\n \
	=========================================\r\n \
	libwebp  LICENSE\r\n \
	Copyright(c) 2010, Google Inc.All rights reserved.\r\n \
	=========================================\r\n \
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\r\n \
	\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\r\n \
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\r\n \
	A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT\r\n \
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\r\n \
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT\r\n \
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE\r\n \
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\r\n \
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\r\n \
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\r\n \
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\r\n"
);


static glTFImporter_Core theImporterCore;

BOOL LaunchScript(tstring &script);
BOOL GetFileName(HWND hWnd, tstring &ret, FileType type);
int GetMtlType(void) {return HH_MtlMode;}

#if 0
class KHRglTF2Importer : public SceneImport
{
public:
	//Constructor/Destructor
	KHRglTF2Importer();
	virtual ~KHRglTF2Importer();

	virtual int			ExtCount();					// Number of extensions supported
	virtual const TCHAR* Ext(int n);					// Extension #n (i.e. "3DS")
	virtual const TCHAR* LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	virtual const TCHAR* ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	virtual const TCHAR* AuthorName();				// ASCII Author name
	virtual const TCHAR* CopyrightMessage();			// ASCII Copyright message
	virtual const TCHAR* OtherMessage1();			// Other message #1
	virtual const TCHAR* OtherMessage2();			// Other message #2
	virtual unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	virtual void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	virtual int				DoImport(const TCHAR* name, ImpInterface* i, Interface* gi, BOOL suppressPrompts = FALSE);	// Import file

};
class KHRglTF2ImporterClassDesc : public ClassDesc2
{
public:
	virtual int           IsPublic() override { return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) override { return new KHRglTF2Importer(); }
	virtual const TCHAR* ClassName() override { return GetString(IDS_CLASS_NAME2); }
	virtual SClass_ID     SuperClassID() override { return SCENE_IMPORT_CLASS_ID; }
	virtual Class_ID      ClassID() override { return KHRglTF2Importer_CLASS_ID; }
	virtual const TCHAR* Category() override { return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() override { return _T("KHRglTF2Importer"); } // Returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE     HInstance() override { return hInstance; } // Returns owning module handle
#if MAX_RELEASE>=24000
	const wchar_t* ClassDesc::NonLocalizedClassName(void) { return GetString(IDS_CLASS_NAME2); }
#endif
};
ClassDesc2* GetKHRglTF2ImporterDesc()
{
	static KHRglTF2ImporterClassDesc KHRglTF2ImporterDesc;
	return &KHRglTF2ImporterDesc;
}
#endif

//======================================================================
//======================================================================
BOOL IsLogOut(void) {	return HH_LogOut;}
void ClearListnerWnd(void)
{
	if (!HH_LogOut) return;

#if MAX_RELEASE >= 24000
	ExecuteMAXScriptScript(_T("setListenerSel #(0,-1)"), MAXScript::ScriptSource::NonEmbedded);
#else
	ExecuteMAXScriptScript(_T("setListenerSel #(0,-1)"));
#endif
	the_listener->edit_stream->puts(_T(""));
}
void LogOutput(const tstring &str, int pcs)
{
	if (!HH_LogOut) return;

	//the_listener->edit_stream->printf(_T("Hello"));
	the_listener->edit_stream->puts(str.c_str());
	the_listener->edit_stream->puts(_T("\n"));
	//wputs(str.c_str());
	//GetCOREInterface()->ProgressUpdate(pcs, TRUE, str.c_str());

}
void LogOutput(const std::string& str, int pcs)
{
	LogOutput(StringToWString(str.c_str()));
}
BOOL ISFlatHierarchy(void) {	return HH_FlatHierarchy;}

//======================================================================
// Define Importer class
//======================================================================
class KHRglTFImporter : public SceneImport
{
public:
	//Constructor/Destructor
	KHRglTFImporter();
	virtual ~KHRglTFImporter();

	virtual int				ExtCount();					// Number of extensions supported
	virtual const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	virtual const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	virtual const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	virtual const TCHAR *	AuthorName();				// ASCII Author name
	virtual const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	virtual const TCHAR *	OtherMessage1();			// Other message #1
	virtual const TCHAR *	OtherMessage2();			// Other message #2
	virtual unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	virtual void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	virtual int				DoImport(const TCHAR *name,ImpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE);	// Import file
	FPInterfaceDesc* GetDesc();
};
//======================================================================
// Plugin descriptor
//======================================================================
class KHRglTFImporterClassDesc : public ClassDesc2 
{
public:
	virtual int           IsPublic() override                       { return TRUE; }
	virtual void*         Create(BOOL /*loading = FALSE*/) override { return new KHRglTFImporter(); }
	virtual const TCHAR * ClassName() override                      { return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID     SuperClassID() override                   { return SCENE_IMPORT_CLASS_ID; }
	virtual Class_ID      ClassID() override                        { return KHRglTFImporter_CLASS_ID; }
	virtual const TCHAR*  Category() override                       { return GetString(IDS_CATEGORY); }

	virtual const TCHAR*  InternalName() override                   { return _T("KHRglTFImporter"); } // Returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE     HInstance() override                      { return hInstance; } // Returns owning module handle
#if MAX_RELEASE>=24000
	const wchar_t *ClassDesc::NonLocalizedClassName(void) { return GetString(IDS_CLASS_NAME); }
#endif
};
ClassDesc2* GetKHRglTFImporterDesc()
{
	static KHRglTFImporterClassDesc KHRglTFImporterDesc;
	return &KHRglTFImporterDesc;
}
//======================================================================
//======================================================================
enum {
	fnIdGetCurrentAnimFn,
	fnIdGetImportGeomArrayFn,
	fnIdSetDebugModeFn,
	fnSetUniqueIndexFn,
	fnGetUniqueIndexFn,
	fnGetObjByUniqueIDFn,
	fnRemoveUniqueIndexFn,
};
class KHRglTFImpExtentActions : public FPStaticInterface {
public:
	virtual FPValue GetCurrentAnimFn(void) = 0;
	virtual FPValue GetImportGeomArrayFn(void) = 0;
	virtual void SetDebugModeFn(int) = 0;
	virtual DWORD SetUniqueIndexFn(ReferenceTarget*) = 0;
	virtual BOOL GetUniqueIndexFn(ReferenceTarget*, DWORD& id) = 0;
	virtual ReferenceTarget* GetObjByUniqueIDFn(DWORD) = 0;
	virtual BOOL RemoveUniqueIndexFn(ReferenceTarget*) = 0;
};
class KHRglTFImpExtentActionsIMP : public KHRglTFImpExtentActions {
public:
	DECLARE_DESCRIPTOR(KHRglTFImpExtentActionsIMP)

	BEGIN_FUNCTION_MAP
	FN_0(fnIdGetCurrentAnimFn, TYPE_FPVALUE_BV, GetCurrentAnimFn);
	FN_0(fnIdGetImportGeomArrayFn, TYPE_FPVALUE_BV, GetImportGeomArrayFn);
	VFN_1(fnIdSetDebugModeFn, SetDebugModeFn, TYPE_INT);
	FN_1(fnSetUniqueIndexFn, TYPE_DWORD, SetUniqueIndexFn, TYPE_REFTARG);
	FN_2(fnGetUniqueIndexFn, TYPE_BOOL, GetUniqueIndexFn, TYPE_REFTARG, TYPE_DWORD_BR);
	FN_1(fnGetObjByUniqueIDFn, TYPE_REFTARG, GetObjByUniqueIDFn, TYPE_DWORD);
	FN_1(fnRemoveUniqueIndexFn, TYPE_BOOL, RemoveUniqueIndexFn, TYPE_REFTARG);
	END_FUNCTION_MAP

	FPValue GetCurrentAnimFn(void) {
		return s_CurrentCustAttrObj;
		//return theImporterCore.GetCurrentAnim();
	}
	FPValue GetImportGeomArrayFn(void) {
		return theImporterCore.GetImportGeomArray();
	}
	void SetDebugModeFn(int i) {
		TSTR profle;
		profle.printf(_T("%s\\%s"), GetCOREInterface()->GetDir(APP_PLUGCFG_DIR), _T("KHRglTFImporter.ini"));
		if(i)
			MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("DebugMode"), _T("1"), profle);
		else
			MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("DebugMode"), _T("0"), profle);
	}
	DWORD SetUniqueIndexFn(ReferenceTarget* pRef) {
		InteractivityStruct str;
		str.id = GetTickCount();
		return theImporterCore.CreateInteractivityAttr(pRef, str, TRUE);
	}
	BOOL GetUniqueIndexFn(ReferenceTarget *pRef, DWORD& id) {
		return theImporterCore.GetInteractivityPointerID(pRef, id);
	}
	ReferenceTarget* GetObjByUniqueIDFn(DWORD id) {
		return theImporterCore.GetAnimByUniqueID(id);

	}
	BOOL RemoveUniqueIndexFn(ReferenceTarget* pRef) {
		return theImporterCore.RemoveInteractivityAttr(pRef);
	}
};
static KHRglTFImpExtentActionsIMP KHRglTFImpExtentActionsFP(KHRGLTFIMP_INTERFACE_ID, _T("KHRglTFImp"), 0, GetKHRglTFImporterDesc(), 0,
	fnIdGetCurrentAnimFn,		_T("GetCurrentAnim"),		0, TYPE_FPVALUE_BV, 0, 0,
	fnIdGetImportGeomArrayFn,	_T("GetImportGeomArray"),	0, TYPE_FPVALUE_BV, 0, 0,
	fnIdSetDebugModeFn,			_T("SetDebugMode"),			0, TYPE_VOID,		0, 1, _T("param"), 0, TYPE_INT,
	fnSetUniqueIndexFn,			_T("SetUniqueIndex"),		0, TYPE_DWORD,		0, 1, _T("target"), 0, TYPE_REFTARG,
	fnGetUniqueIndexFn,			_T("GetUniqueIndex"),		0, TYPE_BOOL,		0, 2, _T("target"), 0, TYPE_REFTARG, _T("id"), 0, TYPE_DWORD_BR,
	fnGetObjByUniqueIDFn,		_T("GetObjByUniqueID"),		0, TYPE_REFTARG,	0, 1, _T("id"),		0, TYPE_DWORD,
	fnRemoveUniqueIndexFn,		_T("RemoveUniqueIndex"),	0, TYPE_BOOL,		0, 1, _T("target"), 0, TYPE_REFTARG,
	p_end
);
FPInterfaceDesc* KHRglTFImporter::GetDesc() { return &KHRglTFImpExtentActionsFP; }


//======================================================================
// Param setting dialog CallBack
//======================================================================
INT_PTR CALLBACK KHRglTFImporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam, LPARAM lParam) {

	static KHRglTFImporter* imp = nullptr;
	static ISpinnerControl *pSpin1;
	static ISpinnerControl* pSpin2;
	static ISpinnerControl* pSpin3;
	static IColorSwatch* pBkColor = NULL;

	TCHAR buf[MAX_PATH];

	switch(message) {
	case WM_INITDIALOG:
		SetWindowText(hWnd, s_TitleString.c_str());
		imp = (KHRglTFImporter *)lParam;
		pSpin1 = GetISpinner(::GetDlgItem(hWnd, IDC_SPIN_SCALE));
		pSpin1->SetLimits(0.0f, 1000.0f, TRUE);
		pSpin1->SetAutoScale(TRUE);
		pSpin1->SetScale(1.0f);
		pSpin1->LinkToEdit(::GetDlgItem(hWnd, IDC_EDIT_SCALE), EDITTYPE_FLOAT);
		pSpin1->SetValue(HH_scale, FALSE);
		pSpin2 = GetISpinner(::GetDlgItem(hWnd, IDC_SPIN_ANIMCH));
		pSpin2->SetLimits(0, 100, TRUE);
		pSpin2->SetAutoScale(TRUE);
		pSpin2->SetScale(1);
		pSpin2->LinkToEdit(::GetDlgItem(hWnd, IDC_EDIT_ANIMCH), EDITTYPE_INT);
		pSpin2->SetValue(HH_AnimChannel, FALSE);
		pSpin3 = GetISpinner(::GetDlgItem(hWnd, IDC_SPIN_SCNCH));
		pSpin3->SetLimits(0, 100, TRUE);
		pSpin3->SetAutoScale(TRUE);
		pSpin3->SetScale(1);
		pSpin3->LinkToEdit(::GetDlgItem(hWnd, IDC_EDIT_SCNCH), EDITTYPE_INT);
		pSpin3->SetValue(HH_SceneChannel, FALSE);

		SendMessage(GetDlgItem(hWnd, IDC_ROT_COMBO), CB_RESETCONTENT, 0, 0);
		SendMessage(GetDlgItem(hWnd, IDC_ROT_COMBO), CB_ADDSTRING, 0, (LPARAM)_T("Euler XYZ"));
		SendMessage(GetDlgItem(hWnd, IDC_ROT_COMBO), CB_ADDSTRING, 0, (LPARAM)_T("Linear Rotation"));
		SendMessage(GetDlgItem(hWnd, IDC_ROT_COMBO), CB_SETCURSEL, HH_UseQuatCtrl, (LPARAM)0);

		CheckDlgButton(hWnd, IDC_ANIM_CHK, HH_Animation);
		CheckDlgButton(hWnd, IDC_MATCHANIM_CHK, HH_MatchAnim);
		CheckDlgButton(hWnd, IDC_FLIPNRM_GRN_CHK, HH_FlipNormalGrn);
		CheckDlgButton(hWnd, IDC_FLIPNRM_RED_CHK, HH_FlipNormalRed);
		CheckDlgButton(hWnd, IDC_COMPOSITE_CHK, HH_CompositeMtl);
		CheckDlgButton(hWnd, IDC_VERTCOLOR_CHK, HH_ViewVertexColor);
		CheckDlgButton(hWnd, IDC_HIDEDMY_CHK, HH_HideDummy);
		CheckDlgButton(hWnd, IDC_NAMEOBJ_CHK, HH_AvoidDupName);
		CheckDlgButton(hWnd, IDC_INSTANCE_CHK, HH_Instancing);
		CheckDlgButton(hWnd, IDC_CUSTATTR_CHK, HH_ExtraToCustAttr);
		CheckDlgButton(hWnd, IDC_USERPROP_CHK, HH_ExtraToUserProp);
		CheckDlgButton(hWnd, IDC_UNIQUETEX_CHK, HH_UniqueTexture);
		CheckDlgButton(hWnd, IDC_CLRCORRECT_CHK, HH_UseColorComposite);
		CheckDlgButton(hWnd, IDC_GAMMA_CHK, HH_CorrectGamma);
		CheckDlgButton(hWnd, IDC_SCLINTS_CHK, HH_ScaleLightIntensity);
		CheckDlgButton(hWnd, IDC_APPLYSCL_CHK, HH_ApplyScale);
		
		CheckDlgButton(hWnd, IDC_RADIO1, HH_MapUnpackMode == 0);
		CheckDlgButton(hWnd, IDC_RADIO2, HH_MapUnpackMode == 1);
		CheckDlgButton(hWnd, IDC_RADIO3, HH_MapUnpackMode == 2);
		CheckDlgButton(hWnd, IDC_SCN_BTN1, HH_SceneMode == 0);
		CheckDlgButton(hWnd, IDC_SCN_BTN2, HH_SceneMode == 1);
		CheckDlgButton(hWnd, IDC_SCN_BTN3, HH_SceneMode == 2);

		CheckDlgButton(hWnd, IDC_ENABLEBKCOL_CHK, HH_EnableBkColor);
		//ShowWindow(GetDlgItem(hWnd, IDC_USEOSL_CHK), FALSE);

		CheckDlgButton(hWnd, IDC_SCRIPT_CHK, HH_LaunchScript);
		SetWindowText(GetDlgItem(hWnd, IDC_SCRIPTSTR), s_ScriptString.c_str());
		EnableWindow(GetDlgItem(hWnd, IDC_SCRIPTSTR), HH_LaunchScript);
		EnableWindow(GetDlgItem(hWnd, IDC_SETFILE_BTN), HH_LaunchScript);
		SetWindowText(GetDlgItem(hWnd, IDC_IMAGEFILESTR), s_ImageFileString.c_str());

		EnableWindow(GetDlgItem(hWnd, IDC_FLIPNRM_GRN_CHK), HH_MtlMode != 3);
		EnableWindow(GetDlgItem(hWnd, IDC_FLIPNRM_RED_CHK), HH_MtlMode != 3);

		EnableWindow(GetDlgItem(hWnd, IDC_MATCHANIM_CHK), HH_Animation);
		EnableWindow(GetDlgItem(hWnd, IDC_EDIT_ANIMCH), HH_Animation);
		EnableWindow(GetDlgItem(hWnd, IDC_SPIN_ANIMCH), HH_Animation);
		EnableWindow(GetDlgItem(hWnd, IDC_ROTCTL_STATIC), HH_Animation);
		EnableWindow(GetDlgItem(hWnd, IDC_ROT_COMBO), HH_Animation);

		EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SCNCH), HH_SceneMode == 1);
		EnableWindow(GetDlgItem(hWnd, IDC_SPIN_SCNCH), HH_SceneMode == 1);
		EnableWindow(GetDlgItem(hWnd, IDC_ANIM_STATIC), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));

		EnableWindow(GetDlgItem(hWnd, IDC_IMAGEFILESTR), HH_EnableBkColor);
		EnableWindow(GetDlgItem(hWnd, IDC_SETBMP_BTN), HH_EnableBkColor);

		EnableWindow(GetDlgItem(hWnd, IDC_MTL_RADIO5), s_ArnoldMtlEnable);
		if(!s_ArnoldMtlEnable) CheckDlgButton(hWnd, IDC_MTL_RADIO5, FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_MTL_RADIO6), s_USDMtlEnable);
		if (!s_USDMtlEnable) CheckDlgButton(hWnd, IDC_MTL_RADIO6, FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_MTL_RADIO7), s_VRayMtlEnable);
		if (!s_VRayMtlEnable) CheckDlgButton(hWnd, IDC_MTL_RADIO7, FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_MTL_RADIO8), s_CoronaMtlEnable);
		if (!s_CoronaMtlEnable) CheckDlgButton(hWnd, IDC_MTL_RADIO8, FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_MTL_RADIO9), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_MTL_RADIO10), s_OpenPBRMtlEnable);
		if (!s_OpenPBRMtlEnable) CheckDlgButton(hWnd, IDC_MTL_RADIO10, FALSE);

		if (!s_ArnoldMtlEnable && HH_MtlMode == 4) HH_MtlMode = 0;
		if (!s_USDMtlEnable && HH_MtlMode == 5) HH_MtlMode = 0;
		if (!s_VRayMtlEnable && HH_MtlMode == 6) HH_MtlMode = 0;
		if (!s_CoronaMtlEnable && HH_MtlMode == 7) HH_MtlMode = 0;
		if (!s_OpenPBRMtlEnable && HH_MtlMode == 9) HH_MtlMode = 0;

#if MAX_RELEASE <= 22000
		ShowWindow(GetDlgItem(hWnd, IDC_MTL_RADIO3), FALSE);
		ShowWindow(GetDlgItem(hWnd, IDC_MTL_RADIO5), FALSE);
		ShowWindow(GetDlgItem(hWnd, IDC_USEOSL_CHK), FALSE);
		if (HH_MtlMode == 2 || HH_MtlMode == 4)HH_MtlMode = 0;
#endif
#if MAX_RELEASE <= 23000
		ShowWindow(GetDlgItem(hWnd, IDC_MTL_RADIO6), FALSE);
		ShowWindow(GetDlgItem(hWnd, IDC_MTL_RADIO7), FALSE);
		ShowWindow(GetDlgItem(hWnd, IDC_MTL_RADIO8), FALSE);
		ShowWindow(GetDlgItem(hWnd, IDC_MTL_RADIO9), FALSE);
		if (HH_MtlMode == 5 || HH_MtlMode == 6 || HH_MtlMode == 7|| HH_MtlMode == 8) HH_MtlMode = 0;

#endif
#if MAX_RELEASE <= 24000
		ShowWindow(GetDlgItem(hWnd, IDC_MTL_RADIO4), FALSE);
		if (HH_MtlMode == 3)HH_MtlMode = 0;

#endif
#if MAX_RELEASE >= 26000
		CheckDlgButton(hWnd, IDC_CM_CHECK, HH_ColorManagement);
#else
		ShowWindow(GetDlgItem(hWnd, IDC_CM_CHECK), FALSE);
#endif
#if MAX_RELEASE >= 27000
		ShowWindow(GetDlgItem(hWnd, IDC_COMPOSITE_CHK), FALSE);
#endif
		ShowWindow(GetDlgItem(hWnd, IDC_LICENSE_BTN), FALSE);

		CheckRadioButton(hWnd, IDC_MTL_RADIO1, IDC_MTL_RADIO10, IDC_MTL_RADIO1+HH_MtlMode);

		ShowWindow(GetDlgItem(hWnd, IDC_LOG_CHK), HH_DebugMode);
		ShowWindow(GetDlgItem(hWnd, IDC_FLAT_HIR_CHK), HH_DebugMode);
		ShowWindow(GetDlgItem(hWnd, IDC_APPLYSCL_CHK), HH_DebugMode);

		SetWindowText(GetDlgItem(hWnd, IDC_LICENSE_EDIT), pLicenseStr);
		CenterWindow(hWnd,GetParent(hWnd));
		return TRUE;

	case WM_CLOSE:
		ReleaseISpinner(pSpin1);
		ReleaseISpinner(pSpin2);
		ReleaseISpinner(pSpin3);
		EndDialog(hWnd, 0);
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			HH_scale = pSpin1->GetFVal();
			HH_AnimChannel = pSpin2->GetIVal();
			HH_SceneChannel = pSpin3->GetIVal();
			//HH_DebugMode = IsDlgButtonChecked(hWnd, IDC_CHECK1);
			HH_Animation = IsDlgButtonChecked(hWnd, IDC_ANIM_CHK);
			HH_MatchAnim = IsDlgButtonChecked(hWnd, IDC_MATCHANIM_CHK);
			HH_FlipNormalGrn = IsDlgButtonChecked(hWnd, IDC_FLIPNRM_GRN_CHK);
			HH_FlipNormalRed = IsDlgButtonChecked(hWnd, IDC_FLIPNRM_RED_CHK);
			HH_CompositeMtl = IsDlgButtonChecked(hWnd, IDC_COMPOSITE_CHK);
			HH_ViewVertexColor = IsDlgButtonChecked(hWnd, IDC_VERTCOLOR_CHK);
			HH_HideDummy = IsDlgButtonChecked(hWnd, IDC_HIDEDMY_CHK);
			HH_AvoidDupName = IsDlgButtonChecked(hWnd, IDC_NAMEOBJ_CHK);
			HH_ExtraToCustAttr = IsDlgButtonChecked(hWnd, IDC_CUSTATTR_CHK);
			HH_ExtraToUserProp = IsDlgButtonChecked(hWnd, IDC_USERPROP_CHK);
			HH_UniqueTexture = IsDlgButtonChecked(hWnd, IDC_UNIQUETEX_CHK);
			HH_UseColorComposite = IsDlgButtonChecked(hWnd, IDC_CLRCORRECT_CHK);
			HH_CorrectGamma = IsDlgButtonChecked(hWnd, IDC_GAMMA_CHK);
			HH_ScaleLightIntensity = IsDlgButtonChecked(hWnd, IDC_SCLINTS_CHK);
			HH_ApplyScale = IsDlgButtonChecked(hWnd, IDC_APPLYSCL_CHK);
			if (IsDlgButtonChecked(hWnd, IDC_RADIO1)) HH_MapUnpackMode = 0;
			if (IsDlgButtonChecked(hWnd, IDC_RADIO2)) HH_MapUnpackMode = 1;
			if (IsDlgButtonChecked(hWnd, IDC_RADIO3)) HH_MapUnpackMode = 2;

			if (IsDlgButtonChecked(hWnd, IDC_SCN_BTN1)) HH_SceneMode = 0;
			if (IsDlgButtonChecked(hWnd, IDC_SCN_BTN2)) HH_SceneMode = 1;
			if (IsDlgButtonChecked(hWnd, IDC_SCN_BTN3)) HH_SceneMode = 2;

			HH_LaunchScript = IsDlgButtonChecked(hWnd, IDC_SCRIPT_CHK);
			HH_Instancing = IsDlgButtonChecked(hWnd, IDC_INSTANCE_CHK);
			GetWindowText(GetDlgItem(hWnd, IDC_SCRIPTSTR), buf, MAX_PATH);
			s_ScriptString = buf;
			HH_EnableBkColor = IsDlgButtonChecked(hWnd, IDC_ENABLEBKCOL_CHK);
			if (HH_EnableBkColor) {
				GetWindowText(GetDlgItem(hWnd, IDC_IMAGEFILESTR), buf, MAX_PATH);
				s_ImageFileString = buf;
			}

			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO1)) HH_MtlMode = 0;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO2)) HH_MtlMode = 1;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO3)) HH_MtlMode = 2;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO4)) HH_MtlMode = 3;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO5)) HH_MtlMode = 4;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO6)) HH_MtlMode = 5;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO7)) HH_MtlMode = 6;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO8)) HH_MtlMode = 7;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO9)) HH_MtlMode = 8;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO10)) HH_MtlMode = 9;
			if (IsDlgButtonChecked(hWnd, IDC_MTL_RADIO11)) HH_MtlMode = 10;
			HH_UseQuatCtrl = (BOOL)SendMessage(GetDlgItem(hWnd, IDC_ROT_COMBO), CB_GETCURSEL, 0, (LPARAM)0);
			HH_ColorManagement = IsDlgButtonChecked(hWnd, IDC_CM_CHECK);

			HH_LogOut = IsDlgButtonChecked(hWnd, IDC_LOG_CHK);
			HH_FlatHierarchy = IsDlgButtonChecked(hWnd, IDC_FLAT_HIR_CHK);

			::EndDialog(hWnd, 1);
			break;
		case IDCANCEL:
			::EndDialog(hWnd, 0);
			break;
		case IDC_SCRIPT_CHK:
			EnableWindow(GetDlgItem(hWnd, IDC_SCRIPTSTR), IsDlgButtonChecked(hWnd, IDC_SCRIPT_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_SETFILE_BTN), IsDlgButtonChecked(hWnd, IDC_SCRIPT_CHK));
			break;
		case IDC_SETFILE_BTN:
			if (GetFileName(hWnd, s_ScriptString, FileType::SCRIPT)) {
				SetWindowText(GetDlgItem(hWnd, IDC_SCRIPTSTR), s_ScriptString.c_str());
			}
			break;
		case IDC_SETBMP_BTN:
			{
				BitmapInfo bi;
				if (TheManager->SelectFileInput(&bi, hWnd, _T("Select BackGround Image")))
					SetWindowText(GetDlgItem(hWnd, IDC_IMAGEFILESTR), bi.Name());
			}
			break;
		case IDC_ANIM_CHK:
			EnableWindow(GetDlgItem(hWnd, IDC_MATCHANIM_CHK), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_EDIT_ANIMCH), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_SPIN_ANIMCH), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_ANIM_STATIC), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_ROTCTL_STATIC), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_ROT_COMBO), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			break;
		case IDC_ENABLEBKCOL_CHK:
			EnableWindow(GetDlgItem(hWnd, IDC_IMAGEFILESTR), IsDlgButtonChecked(hWnd, IDC_ENABLEBKCOL_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_SETBMP_BTN), IsDlgButtonChecked(hWnd, IDC_ENABLEBKCOL_CHK));
			break;
		case IDC_SCN_BTN1:
		case IDC_SCN_BTN2:
		case IDC_SCN_BTN3:
			//EnableWindow(GetDlgItem(hWnd, IDC_SCENE_STATIC), !IsDlgButtonChecked(hWnd, IDC_SCENE_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_EDIT_SCNCH), IsDlgButtonChecked(hWnd, IDC_SCN_BTN2));
			EnableWindow(GetDlgItem(hWnd, IDC_SPIN_SCNCH), IsDlgButtonChecked(hWnd, IDC_SCN_BTN2));
			break;

		case IDC_MTL_RADIO1:
		case IDC_MTL_RADIO2:
		case IDC_MTL_RADIO3:
		case IDC_MTL_RADIO4:
		case IDC_MTL_RADIO5:
		case IDC_MTL_RADIO6:
		case IDC_MTL_RADIO7:
		case IDC_MTL_RADIO8:
		case IDC_MTL_RADIO9:
		case IDC_MTL_RADIO10:
			EnableWindow(GetDlgItem(hWnd, IDC_FLIPNRM_GRN_CHK), !IsDlgButtonChecked(hWnd, IDC_MTL_RADIO4));
			EnableWindow(GetDlgItem(hWnd, IDC_FLIPNRM_RED_CHK), !IsDlgButtonChecked(hWnd, IDC_MTL_RADIO4));
			break;

		}
		break;
	}
	return 0;
}

//--- KHRglTFImporter -------------------------------------------------------
KHRglTFImporter::KHRglTFImporter()
{
}

KHRglTFImporter::~KHRglTFImporter() 
{
}

int KHRglTFImporter::ExtCount()
{
#ifdef _DEBUG
	return 3;
#endif
	return 2;
}

const TCHAR *KHRglTFImporter::Ext(int i)
{		
	switch(i) {
	case 0:	return _T("glTF");
	case 1:	return _T("glb");
	case 2:	return _T("glTFX");
	}
	return _T("glTF");
}

const TCHAR *KHRglTFImporter::LongDesc()
{
	return _T("glTF 2.0 Importer for 3ds Max");
}
	
const TCHAR *KHRglTFImporter::ShortDesc() 
{			
	return _T("glTF 2.0 (KHRglTFImporter)");
}

const TCHAR *KHRglTFImporter::AuthorName()
{			
	return _T("Satoshi Hayashi");
}

const TCHAR *KHRglTFImporter::CopyrightMessage() 
{	
	return _T("Satoshi Hayashi");
}

const TCHAR *KHRglTFImporter::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *KHRglTFImporter::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int KHRglTFImporter::Version()
{				
	return 100;
}

void KHRglTFImporter::ShowAbout(HWND /*hWnd*/)
{			
	// Optional
}
#if 0
//--- KHRglTFImporter -------------------------------------------------------
KHRglTF2Importer::KHRglTF2Importer()
{
}

KHRglTF2Importer::~KHRglTF2Importer()
{
}

int KHRglTF2Importer::ExtCount()
{
	return 2;
}

const TCHAR *KHRglTF2Importer::Ext(int i)
{
	switch (i) {
	case 0:	return _T("glTF");
	case 1:	return _T("glb");
	}
	return _T("glTF");
}

const TCHAR *KHRglTF2Importer::LongDesc()
{
	return _T("glTF 2.0 Importer for 3ds Max");
}

const TCHAR *KHRglTF2Importer::ShortDesc()
{
	return _T("glTF 2.0 (KHRglTFImporter)");
}

const TCHAR *KHRglTF2Importer::AuthorName()
{
	return _T("Satoshi Hayashi");
}

const TCHAR *KHRglTF2Importer::CopyrightMessage()
{
	return _T("(C) Satoshi Hayashi");
}

const TCHAR *KHRglTF2Importer::OtherMessage1()
{
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *KHRglTF2Importer::OtherMessage2()
{
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int KHRglTF2Importer::Version()
{
	return 100;
}

void KHRglTF2Importer::ShowAbout(HWND /*hWnd*/)
{
	// Optional
}
int KHRglTF2Importer::DoImport(const TCHAR* filename, ImpInterface* importerInt, Interface* ip, BOOL suppressPrompts)
{
	return theImporterCore.ImportPreProcess(filename, suppressPrompts, 2);
}
#endif
//======================================================================
// Execute import
//======================================================================
int KHRglTFImporter::DoImport(const TCHAR* filename, ImpInterface* importerInt, Interface* ip, BOOL suppressPrompts)
{
	return theImporterCore.ImportPreProcess(filename, suppressPrompts, 1);
}

//======================================================================
// Importer body
//======================================================================
BOOL glTFImporter_Core::ImportPreProcess(const TCHAR* filename, BOOL suppressPrompts, int ver)
{
	TCHAR buf[MAX_PATH];


	s_TitleString = _T("Khronos glTF importer for 3dsmax ") + tstring(KHR_GLTF_IMPORTER_VER);

	SubClassList* subList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(MATERIAL_CLASS_ID);
	int idx = subList->FindClass(Arnold_StandardSufaceID);
	if (idx >= 0) s_ArnoldMtlEnable = TRUE;
	idx = subList->FindClass(VRayMaterialID);
	if (idx >= 0) s_VRayMtlEnable = TRUE;
	idx = subList->FindClass(CoronaMaterialID);
	if (idx >= 0) s_CoronaMtlEnable = TRUE;
	idx = subList->FindClass(USDMaterialID);
	if (idx >= 0) s_USDMtlEnable = TRUE;
	//idx = subList->FindClass(Pencil4MaterialID);
	//if (idx >= 0) s_PencilMtlEnable = TRUE;
	idx = subList->FindClass(OpenPBRMaterialID);
	if (idx >= 0) s_OpenPBRMtlEnable = TRUE;


	TSTR profle;
	profle.printf(_T("%s\\%s"), GetCOREInterface()->GetDir(APP_PLUGCFG_DIR), _T("KHRglTFImporter.ini"));

	HH_DebugMode = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("DebugMode"), 0, profle);

	int x = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("scale"), 1000, profle);
	HH_scale = x / 1000.0f;
	HH_MtlMode = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("MtlMode"), 3, profle);
#if MAX_RELEASE <= 23000
	if (HH_MtlMode >= 2 && HH_MtlMode!=6) HH_MtlMode = 0;
#endif
	HH_Animation = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("Animation"), 1, profle);
	HH_MatchAnim = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("MatchAnim"), 1, profle);
	HH_AnimChannel = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("AnimChannel"), 1, profle);
	HH_SceneMode = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("UseDefSceneCh"), 0, profle);
	HH_SceneChannel = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("SceneChannel"), 0, profle);
	HH_FlipNormalGrn = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("FlipNormalGrn"), 1, profle);
	HH_FlipNormalRed = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("FlipNormalRed"), 1, profle);
	HH_CompositeMtl = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("CompositeMtl"), 1, profle);
	HH_ViewVertexColor = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("VertexColor"), 0, profle);
	HH_HideDummy = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("HideDummy"), 0, profle);
	HH_AvoidDupName = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("AvoidDupName"), 1, profle);
	HH_UniqueTexture = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("UniqueTexture"), 1, profle);
	HH_UseColorComposite = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("UseColorCorrectTex"), 1, profle);
	HH_CorrectGamma = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("CorrectGamma"), 1, profle);
	HH_ScaleLightIntensity = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("ScaleLightIntensity"), 1, profle);
	HH_LaunchScript = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("LaunchScript"), 0, profle);
	MaxSDK::Util::GetPrivateProfileString(_T("ImpSettings"), _T("ScriptString"), _T(""), buf, MAX_PATH, profle);
	s_ScriptString = buf;
	HH_GammaValue =	MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("GammaValue"), 100, profle);
	HH_MapUnpackMode = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("UseOSLTex"), 0, profle);
	HH_Instancing = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("Instancing"), 1, profle);
	HH_EnableBkColor = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("EnableBkColor"), 0, profle);
	MaxSDK::Util::GetPrivateProfileString(_T("ImpSettings"), _T("ImageFileString"), _T(""), buf, MAX_PATH, profle);
	s_ImageFileString = buf;
	HH_ExtraToCustAttr = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("ExtraToCustAttr"), 0, profle);
	HH_ExtraToUserProp = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("ExtraToUserProp"), 0, profle);
	HH_UseQuatCtrl = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("UseQuatCtrl"), 0, profle);
	HH_ColorManagement = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("ColorManagement"), 0, profle);
	HH_ApplyScale = MaxSDK::Util::GetPrivateProfileInt(_T("ImpSettings"), _T("ApplyScale"), 1, profle);

	if (!suppressPrompts) {
		if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PANEL), GetActiveWindow(), KHRglTFImporterOptionsDlgProc, (LPARAM)this) == 0) return TRUE;

		_stprintf_s(buf, MAX_PATH, _T("%d"), (int)(HH_scale*1000.0f));
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("scale"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_Animation ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("Animation"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_MtlMode);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("MtlMode"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_MatchAnim ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("MatchAnim"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_SceneChannel);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("SceneChannel"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_SceneMode);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("UseDefSceneCh"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_AnimChannel);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("AnimChannel"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_FlipNormalGrn ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("FlipNormalGrn"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_FlipNormalRed ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("FlipNormalRed"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_CompositeMtl ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("CompositeMtl"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ViewVertexColor ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("VertexColor"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_HideDummy ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("HideDummy"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_AvoidDupName ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("AvoidDupName"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_UniqueTexture ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("UniqueTexture"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_UseColorComposite ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("UseColorCorrectTex"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_CorrectGamma ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("CorrectGamma"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ScaleLightIntensity ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("ScaleLightIntensity"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_MapUnpackMode);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("UseOSLTex"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_LaunchScript ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("LaunchScript"), buf, profle);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("ScriptString"), s_ScriptString.c_str(), profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_GammaValue);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("GammaValue"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_Instancing ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("Instancing"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_EnableBkColor ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("EnableBkColor"), buf, profle);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("ImageFileString"), s_ImageFileString.c_str(), profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ExtraToCustAttr ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("ExtraToCustAttr"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ExtraToUserProp ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("ExtraToUserProp"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_UseQuatCtrl ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("UseQuatCtrl"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ColorManagement);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("ColorManagement"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ApplyScale);
		MaxSDK::Util::WritePrivateProfileString(_T("ImpSettings"), _T("ApplyScale"), buf, profle);
	}

	//scale = 39.37f;
	m_scale = HH_scale;
	if (HH_ApplyScale) {
		float scale = 1.0;
		int type = 0;
#if MAX_RELEASE < 24000
		GetMasterUnitInfo(&type, &scale);
#else
		GetSystemUnitInfo(&type, &scale);
#endif
		//scale /= scale;
		switch (type)
		{
		case UNITS_INCHES:		m_scale /= 0.0254f;		break;
		case UNITS_FEET:		m_scale /= 0.3048f;		break;
		case UNITS_MILES:		m_scale /= 1609.34f;	break;
		case UNITS_MILLIMETERS:	m_scale /= 0.001f;		break;
		case UNITS_CENTIMETERS:	m_scale /= 0.01f;		break;
		case UNITS_METERS:		m_scale /= 1.0f;		break;
		case UNITS_KILOMETERS:	m_scale /= 1000.0f;		break;
		default:				m_scale /= 1.0f;		break;
		}
	}

	m_time = 0;
	m_TimeScale = (float)(GetTicksPerFrame() * GetFrameRate());
	m_SceneMode = HH_SceneMode;
	m_SceneChannel = HH_SceneChannel;
	m_AnimChannel = HH_AnimChannel;
	m_MatchAnim = HH_MatchAnim;
	m_FlipNormalGrn = HH_FlipNormalGrn;
	m_FlipNormalRed = HH_FlipNormalRed;
#if MAX_RELEASE < 27000
	m_CompositeMtl = HH_CompositeMtl;
#else
	m_CompositeMtl = 1;
#endif
	m_ViewVertexColor = HH_ViewVertexColor;
	m_HideDummy = HH_HideDummy;
	m_AvoidDupName = HH_AvoidDupName;
	m_Instancing = HH_Instancing;
	m_UniqueTexture = HH_UniqueTexture;
	m_UseColorComposite = HH_UseColorComposite;
	m_CorrectGamma = FALSE;// HH_CorrectGamma;
	m_GammaValue = HH_GammaValue / 100.0f;
	m_MapUnpackMode = HH_MapUnpackMode;
	m_LaunchScript = HH_LaunchScript;
	m_EnableBkColor = HH_EnableBkColor;
	m_ExtraToCustAttr = HH_ExtraToCustAttr;
	m_ExtraToUserProp = HH_ExtraToUserProp;
	m_ScaleLightIntensity = HH_ScaleLightIntensity;
	if(m_ScaleLightIntensity)
		m_LiteIntensityScale = m_scale * m_scale;
	else
		m_LiteIntensityScale = 1.0f;

	m_UseQuatCtrl = HH_UseQuatCtrl;

#if MAX_RELEASE >= 26000
	{
		MaxSDK::ColorManagement::IColorPipelineMgr* pColMgr = (MaxSDK::ColorManagement::IColorPipelineMgr*)GetCOREInterface(COLORPIPELINEMGR_INTERFACE);
/*
		//MaxSDK::ColorManagement::ColorPipelineMode pp = pColMgr->GetColorPipelineMode();
		if (HH_ColorManagement == 1) {
			pColMgr->SetColorPipelineMode(MaxSDK::ColorManagement::ColorPipelineMode::kGAMMA);
			GetCOREInterface()->ForceCompleteRedraw();
		}
*/
		if (HH_ColorManagement) {
			pColMgr->SetColorPipelineMode(MaxSDK::ColorManagement::ColorPipelineMode::kOCIO_CUSTOM);
			MaxSDK::ColorManagement::IModeSettings* pModeSetting = pColMgr->Settings();
			TSTR oicoConfigFile;
			oicoConfigFile.printf(_T("%sColorManagement\\ocio_configs\\glTF_PBR_Neutral_Tone_Mapper\\%s"), GetCOREInterface()->GetDir(APP_MAX_SYS_ROOT_DIR), _T("glTF_PBR_Neutral_Tone_Mapper.ocio"));
			auto ret = pModeSetting->SetOCIOConfigFilePath(oicoConfigFile);
		}
	}
#endif


	std::filesystem::path p(filename);
	if (p.extension() == _T(".gltfx")) {
		gltfx_reference(filename);
		return TRUE;
	}


	m_fullpath = std::wstring(filename);
	m_SourceImageFolder = m_fullpath.parent_path();
	m_SourceImageFolder += tstring(_T("\\"));
	//m_WorkImageFolder = tstring(m_fullpath.parent_path()) + tstring(_T("\\Images\\"));
	TSTR imgPath = GetCOREInterface()->GetDir(APP_IMAGE_DIR);
	 m_WorkImageFolder = tstring(imgPath.data()) + _T("\\") + tstring(m_fullpath.stem()) + tstring(_T("_Images\\"));
	std::filesystem::create_directory(m_WorkImageFolder);

	ClassEntry* ce = GetCOREInterface()->GetDllDirectory()->ClassDir().FindClassEntry(BMM_IO_CLASS_ID, Class_ID(0x6be260fb, 0));
	ClassDesc* cd = ce->FullCD();
	m_pPNG_BmpIO = (IBitmapIO_Png*)cd->GetInterface(BMPIO_INTERFACE);

	if (ImportScene(suppressPrompts)) {
		if (m_EnableBkColor) {
			SetEnvironmentMap(s_ImageFileString);
			//GetCOREInterface()->SetBackGround(m_time, m_BkColor);
		}
		if (m_LaunchScript) {
			TSTR scriptStr;
			LaunchScript(s_ScriptString);
		}
	}
	else {
		return TRUE;
	}

	s_ImportedNodeTab.ZeroCount();
	for (auto n : m_NodeMap) {
		if(n.second->SuperClassID()==BASENODE_CLASS_ID)	s_ImportedNodeTab.AppendNode(n.second);
	}


	return TRUE;
}

//======================================================================
// Read file→scene object
//======================================================================
BOOL glTFImporter_Core::ImportScene(BOOL suppressPrompts)
{
	setlocale(LC_NUMERIC, "en_US");

	ClearListnerWnd();

	UINT CodePage = CP_ACP;
	cgltf_options options = { cgltf_file_type_invalid };
	m_glTF_data = NULL;
	cgltf_result result = cgltf_parse_file(&options, WStringToString(m_fullpath, CodePage).c_str(), &m_glTF_data);
	if (result != cgltf_result_success) return FALSE;

	result = cgltf_load_buffers(&options, m_glTF_data, WStringToString(m_fullpath, CodePage).c_str());
	if (result != cgltf_result_success) return FALSE;
	if(m_glTF_data->scenes_count < m_SceneChannel) return FALSE;

	LogOutput(_T("Import:") + tstring(m_fullpath));

	m_Quantization = FALSE;
	if (m_glTF_data->extensions_used_count>0) {
		for (int i = 0; i < m_glTF_data->extensions_used_count;i++) {
			char *ptr = m_glTF_data->extensions_used[i];
			if (!_stricmp(ptr, "EXT_meshopt_compression")) {
				if(!suppressPrompts) MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("WARNING:EXT_meshopt_compression is not supported."), _T("File Import Failed"), MB_OK | MB_ICONWARNING);
				cgltf_free(m_glTF_data);
				return FALSE;
			}
			else if(_stricmp(ptr, "KHR_mesh_quantization")==0)	m_Quantization = TRUE;
		}
	}


	m_NodeMap.clear();
	m_MeshNodeMap.clear();
	m_MaterialMap.clear();
	m_TextureMap.clear();
	m_TextureViewMap.clear();
	m_MorphTable.clear();
	m_VariantTable.clear();
	m_CustAttrMap.clear();
	m_CameraMap.clear();
	m_LightMap.clear();
	m_CollisionTable.clear();
	
	//InitWireConnectTable();

	if (m_glTF_data->variants_count > 0) {
		cgltf_material_variant* variants = m_glTF_data->variants;
		for (int i = 0; i < m_glTF_data->variants_count;i++) {
			m_VariantTable.push_back(variants->name);
			variants++;
		}
	}

	OpenProgreessDlg(m_glTF_data);

	CreateTextureTable();
	if (m_EmbedFormat) {
		m_SourceImageFolder = m_WorkImageFolder;
	}

#ifdef _DEBUG
	for (int i = 0; i < m_glTF_data->buffer_views_count; i++) {
		cgltf_buffer_view bv = m_glTF_data->buffer_views[i];
		int ia = 10;
	}
#endif

	// Create a dummy PBR material so the viewport renderer can support metalness
	Mtl* pDummyMtl = (Mtl*)GetCOREInterface()->CreateInstance(MATERIAL_CLASS_ID, PBRMetalMtlID);

	SetMtlImportStatus(0);

#if MAX_RELEASE <= 22000
	switch (HH_MtlMode) {
	case 1:	CreatePhysicalMaterial();	break;
	case 6: CreateVRayMaterial();		break;
	default:CreateScanlineMaterial();	break;
	}
#else
	switch (HH_MtlMode) {
	case 1:	CreatePhysicalMaterial();	break;
	case 2:	CreatePBRMetalMaterial();	break;
	case 3:	CreateglTFMaterial();		break;
	case 4:	CreateArnoldMaterial();		break;
	case 5: CreateUSDMaterial();		break;
	case 6: CreateVRayMaterial();		break;
	case 7: CreateCoronaMaterial();		break;
	case 9: CreateOpenPBRMaterial();	break;
	default:CreateScanlineMaterial();	break;
	}
#endif

	for (auto m : m_MaterialMap) {
		cgltf_size size;
		cgltf_result ret = cgltf_copy_extras_json(m_glTF_data, &m.first->extras, NULL, &size);
		if (size > 0) {
			std::vector<custAttrParam> attrTbl;
			CreateParamTableFromExtras(m.first->extras, size, attrTbl);

			/// TODO: validate if this should check m_ExtraToCustAttr before
			AttachCustAttr(m.second, attrTbl);
		}
	}

	SetMtlImportStatus(-1);

	SetNodeImportStatus(0);
	GetCOREInterface()->SetCommandPanelTaskMode(TASK_MODE_MODIFY);
	if (m_glTF_data->scenes_count>1 && m_SceneMode==2) {
		ILayerManager* pLayerMan = GetCOREInterface13()->GetLayerManager();
		tstring defaultSceneName;
		for (int i = 0; i < m_glTF_data->scenes_count; i++) {
			cgltf_scene* pScene = &m_glTF_data->scenes[i];
			tstring sceneName;
			if (!pScene->name)
				sceneName = _T("scene") + to_tstring(i);
			else
				sceneName = StringToWString(pScene->name);

			ILayer* pLayer = pLayerMan->GetLayer(sceneName.c_str());
			if (!pLayer) {
				pLayer = pLayerMan->CreateLayer(sceneName.c_str());
				pLayerMan->AddLayer(pLayer);
			}
			pLayerMan->SetCurrentLayer(sceneName.c_str());
			if (pScene == m_glTF_data->scene)
				defaultSceneName = sceneName;
			else
				pLayer->Hide(TRUE);

			for (int j = 0; j < pScene->nodes_count; j++) {
				CreateNodeInfosRec(pScene->nodes[j]);
			}
		}
		pLayerMan->SetCurrentLayer(defaultSceneName.c_str());
	} else{
		if (m_SceneChannel >= m_glTF_data->scenes_count) m_SceneChannel = static_cast<int>(m_glTF_data->scenes_count - 1);
		cgltf_scene* pScene = m_SceneMode==0 ? m_glTF_data->scene : &m_glTF_data->scenes[m_SceneChannel];
		if (!pScene) pScene = m_glTF_data->scenes;
		if (!pScene) return FALSE;

		for (int j = 0; j < pScene->nodes_count; j++) {
			CreateNodeInfosRec(pScene->nodes[j]);
		}
	}
	SetNodeImportStatus(-1);

#if MAX_RELEASE > MAX_RELEASE_R25

	//ExecuteMAXScriptScript(_T("actionMan.executeAction 0 \"63547\""), MAXScript::ScriptSource::NonEmbedded);

	for(auto mtl:m_MaterialMap) {
		Mtl *p = mtl.second;
		IMaterialViewportShading *pVS = (IMaterialViewportShading*)p->GetInterface(IID_MATERIAL_VIEWPORT_SHADING);
		if (pVS->IsShadingModelSupported(*p, IMaterialViewportShading::Hardware)) {
			pVS->SetCurrentShadingModel(*p, IMaterialViewportShading::Hardware);
		}
		//p->SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS);
		//p->SetMtlFlag(MTL_HW_MAT_ENABLED);
		p->SetMtlFlag(MTL_HW_TEX_ENABLED);

	}
/*
	for (auto tex : m_TextureMap) {
		Texmap *p = tex.second;
		p->SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS);
	}
	*/
	//GetCOREInterface()->ForceCompleteRedraw();

#endif

	// Attach Skin
	SetSkinImportStatus(0);
	for (auto n : m_NodeMap) {
		if (n.first->skin) {
			SetSkin(n.first);
		}
	}
	SetSkinImportStatus(-1);

	SetMorph();

	// Animation
	if(HH_Animation && (m_glTF_data->animations_count > 0)) {
		SetAnimImportStatus(0);

		m_StartTime = MAX_START_TIME;
		m_LastTime = 0;

#ifdef ANIMATION_LAYER_
		//ClassDesc *pPosCtrlClassDesc = GetDefaultController(CTRL_POSITION_CLASS_ID);
		//ClassDesc *pRotCtrlClassDesc = GetDefaultController(CTRL_ROTATION_CLASS_ID);
		//Control *pRotC = (Control*)GetCOREInterface()->CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(EULER_CONTROL_CLASS_ID, 0x0));
		//SetDefaultController(CTRL_POSITION_CLASS_ID, pPosCtrlClassDesc);
		//SetDefaultController(CTRL_ROTATION_CLASS_ID, pRotCtrlClassDesc);


		IAnimLayerControlManager *pAnimLayerMgr = NULL;
		if (m_glTF_data->animations_count > 1) {
			pAnimLayerMgr = static_cast<IAnimLayerControlManager*>(GetCOREInterface(IANIMLAYERCONTROLMANAGER_INTERFACE));
			GetAnimatedNodeTable();
			pAnimLayerMgr->EnableAnimLayers(m_AnimationNodeTab, IAnimLayerControlManager::Filter::ePos | IAnimLayerControlManager::Filter::eRot | IAnimLayerControlManager::Filter::eScale);
			pAnimLayerMgr->SetLayerMute(0, TRUE);

			int cnt = m_glTF_data->animations_count;
			for (int animID = 0; animID < cnt; animID++) {
				TSTR AnimName(_T(""));
				cgltf_animation *animation = &m_glTF_data->animations[animID];
				AnimName = TSTR(StringToWString(animation->name).c_str());

				m_AnimationNodeTab.ZeroCount();
				INode *pRootNode = GetCOREInterface()->GetRootNode();
				for (int i = 0; i < pRootNode->NumChildren(); i++) {
					GetAnimatedNodeTableRec(pRootNode->GetChildNode(i), animID);
				}
				int AnimLayerIndex = -1;
				if (m_AnimationNodeTab.Count() > 0) {
					pAnimLayerMgr->AddLayer(AnimName, m_AnimationNodeTab, TRUE);
					AnimLayerIndex = pAnimLayerMgr->GetLayerCount() - 1;
					pAnimLayerMgr->SetLayerActiveNodes(AnimLayerIndex, m_AnimationNodeTab);
				}
			}
		}


		SuspendAnimate();
		AnimateOn();

		INode *pRootNode = GetCOREInterface()->GetRootNode();
		int cnt = m_glTF_data->animations_count;
		for (int animID = 0; animID < cnt; animID++) {
			TSTR AnimName(_T(""));
			cgltf_animation *animation = &m_glTF_data->animations[animID];
			AnimName = TSTR(StringToWString(animation->name).c_str());

			if (pAnimLayerMgr) {
				m_AnimationNodeTab.ZeroCount();
				for (int i = 0; i < pRootNode->NumChildren(); i++) {
					GetAnimatedNodeTableRec(pRootNode->GetChildNode(i), animID);
				}

				int AnimLayerIndex = -1;
				if (m_AnimationNodeTab.Count() > 0) {
					//GetCOREInterface()->SelectNodeTab(m_AnimationNodeTab, TRUE);
					//pAnimLayerMgr->AddLayer(AnimName, m_AnimationNodeTab, TRUE);
					//AnimLayerIndex = pAnimLayerMgr->GetLayerCount() - 1;
					//pAnimLayerMgr->SetLayerWeight(index, m_time, 1.0f);
					//pAnimLayerMgr->SetLayerActive(AnimLayerIndex);
					//pAnimLayerMgr->SetLayerActiveNodes(AnimLayerIndex, m_AnimationNodeTab);
				}
				pAnimLayerMgr->SetLayerActive(animID+1);


				for (int i = 0; i < pRootNode->NumChildren(); i++) {
					SetAnimationRec(pRootNode->GetChildNode(i), animID, pAnimLayerMgr, AnimName);
				}

				pAnimLayerMgr->SetLayerMute(AnimLayerIndex, TRUE);
			}

			if (m_AnimationNodeTab.Count() > 0) {
			//	pAnimLayerMgr->AddLayer(AnimName, m_AnimationNodeTab, TRUE);
			//GetCOREInterface()->SelectNodeTab(m_AnimationNodeTab,TRUE);
				//ExecuteMAXScriptScript(_T("AnimLayerManager.enableLayers $"), MAXScript::ScriptSource::NonEmbedded);
			}
		}

		if (pAnimLayerMgr) {
			pAnimLayerMgr->SetLayerMute(0, FALSE);
		}

		AnimateOff();
		ResumeAnimate();


#else
		SuspendAnimate();
		AnimateOn();

		if (m_AnimChannel == 0) {
			size_t cnt = m_glTF_data->animations_count;
			for (int anim = 0; anim < cnt; anim++) {
				INode *pRootNode = GetCOREInterface()->GetRootNode();
				for (int i = 0; i < pRootNode->NumChildren(); i++) {
					SetAnimationRec(pRootNode->GetChildNode(i), anim);
				}
				SetAnimationPointer(anim);
			}
		}
		else {
			INode *pRootNode = GetCOREInterface()->GetRootNode();
			for (int i = 0; i < pRootNode->NumChildren(); i++) {
				SetAnimationRec(pRootNode->GetChildNode(i), m_AnimChannel-1);
			}
			SetAnimationPointer(m_AnimChannel - 1);
		}

		AnimateOff();
		ResumeAnimate();
#endif

		SetAnimImportStatus(-1);
	}

	{
		SetPhysicImportStatus(0);
		SetRigidModefiers();
		SetPhysicImportStatus(-1);
	}

	//CreateWireConnect();

	SetSceneProperties();
	SetSceneInfos();

	if (HH_Animation && m_MatchAnim && (m_LastTime>0 || m_StartTime != 100000)) {
		if(m_LastTime > m_StartTime)GetCOREInterface()->SetAnimRange(Interval(m_StartTime, m_LastTime));
	}

	cgltf_free(m_glTF_data);

	CloseProgreessDlg();

	LogOutput(_T("Finish."));

	GetCOREInterface()->ForceCompleteRedraw();

	return TRUE;
}

//======================================================================
// Create data list specified by the accessor
// =====================================================================

#ifdef USE_REFACTORED_SETSPARSEDATA

//======================================================================
// Gemini AI refactored version of the original SetSparseData ( see below )
// Does not crash with Khronos meshopt samples like the original, but fails to import any meshdata
// =====================================================================
void glTFImporter_Core::SetSparseData(std::vector<float>& retVal, cgltf_accessor* acc)
{
	if (!acc) return;

	cgltf_type type = acc->type;
	size_t count = acc->count;
	cgltf_component_type componentType = acc->component_type;
	cgltf_buffer_view* bufferView = acc->buffer_view;

	// 1. Explicitly track component byte size & validate enum definitions
	size_t size = 0;
	switch (componentType) {
		case cgltf_component_type_r_8:   size = sizeof(char);           break;
		case cgltf_component_type_r_8u:  size = sizeof(unsigned char);  break;
		case cgltf_component_type_r_16:  size = sizeof(short);          break;
		case cgltf_component_type_r_16u: size = sizeof(unsigned short); break;
		case cgltf_component_type_r_32u: size = sizeof(unsigned int);   break;
		case cgltf_component_type_r_32f: size = sizeof(float);          break;
		default:
			// Safety fallback: Unhandled or malicious component type enum
			return;
	}

	// 2. Safely translate type dimensions and catch unhandled enums
	size_t dataLen = 0;
	switch (type) {
		case cgltf_type_scalar: dataLen = 1;  break;
		case cgltf_type_vec2:   dataLen = 2;  break;
		case cgltf_type_vec3:   dataLen = 3;  break;
		case cgltf_type_vec4:   dataLen = 4;  break;
		case cgltf_type_mat2:   dataLen = 4;  break;
		case cgltf_type_mat3:   dataLen = 9;  break;
		case cgltf_type_mat4:   dataLen = 16; break;
		default:
			// Safety fallback: Unhandled or invalid structural dimension type
			return;
	}

	// 3. Fallback tracking when no buffer backing is explicitly present
	cgltf_buffer* buffer = nullptr;
	if (bufferView) {
		buffer = bufferView->buffer;
	}
	else {
		// Prevent 64-bit multiplication overflows during allocation sizes
		if (count > 0 && dataLen > (std::numeric_limits<size_t>::max() / count)) {
			return; 
		}

		// Optimally allocate space directly on the vector to avoid re-allocation loops
		size_t totalElements = count * dataLen;
		try {
			retVal.insert(retVal.end(), totalElements, 0.0f);
		} catch (const std::bad_alloc&) {
			// Guard against system out-of-memory states if count is exceptionally large
			return;
		}
		return;
	}

	// Validate that the underlying buffer container is actually populated
	if (!buffer || !buffer->data) {
		return;
	}

	// 4. Calculate stride step safely and verify internal element fit
	size_t bs = bufferView->stride;
	size_t step = (bs > 0) ? bs : dataLen * size;

	// Ensure our element data fits cleanly within the designated step/stride span
	if (dataLen * size > step) {
		return;
	}

	// 5. Defend against integer multiplication overflow before processing loop
	if (count > 0 && step > (std::numeric_limits<size_t>::max() / count)) {
		return;
	}

	// Calculate structural block placement and boundaries across 64-bit constraints
	size_t baseOffset = bufferView->offset + acc->offset;
	size_t totalRequiredBytes = baseOffset + (count * step);

	// Final buffer boundary validation checking the file definition vs real memory size
	if (totalRequiredBytes > buffer->size) {
		return;
	}

	// Base pointer address mapping across the complete validated byte span
	const char* const basePtr = static_cast<const char*>(buffer->data) + baseOffset;

	for (size_t i = 0; i < count; i++) {
		const char* const elementPtr = basePtr + (i * step);

		for (size_t j = 0; j < dataLen; j++) {
			const char* const componentPtr = elementPtr + (j * size);

			switch (componentType) {
				case cgltf_component_type_r_8: {
					char v;
					std::memcpy(&v, componentPtr, sizeof(v));
					retVal.push_back(static_cast<float>(v));
					break;
				}
				case cgltf_component_type_r_8u: {
					unsigned char v;
					std::memcpy(&v, componentPtr, sizeof(v));
					retVal.push_back(static_cast<float>(v));
					break;
				}
				case cgltf_component_type_r_16: {
					short v;
					std::memcpy(&v, componentPtr, sizeof(v));
					retVal.push_back(static_cast<float>(v));
					break;
				}
				case cgltf_component_type_r_16u: {
					unsigned short v;
					std::memcpy(&v, componentPtr, sizeof(v));
					retVal.push_back(static_cast<float>(v));
					break;
				}
				case cgltf_component_type_r_32u: {
					unsigned int v;
					std::memcpy(&v, componentPtr, sizeof(v));
					retVal.push_back(static_cast<float>(v));
					break;
				}
				case cgltf_component_type_r_32f: {
					float v;
					std::memcpy(&v, componentPtr, sizeof(v));
					retVal.push_back(v);
					break;
				}
			}
		}
	}
}

#else
/// TODO: throws exception when importing Khronos gltf-meshopt samples
/// !!! This function crashes at memcpy when importing Khronos meshopt samples 
/// BrainStem\glTF-Meshopt\BrainStem.gltf
/// glTF-Meshopt\DragonAttenuation.gltf

void glTFImporter_Core::SetSparseData(std::vector<float>& retVal, cgltf_accessor* acc)
{
	if (!acc) return;

	cgltf_type type = acc->type;
	size_t count = acc->count;
	cgltf_component_type componentType = acc->component_type;
	cgltf_buffer_view *bufferView = acc->buffer_view;

	size_t size = 1;
	switch (componentType) {
	case cgltf_component_type_r_8:		size = 1;	break;
	case cgltf_component_type_r_8u:	size = 1;	break;
	case cgltf_component_type_r_16:	size = 2;	break;
	case cgltf_component_type_r_16u:	size = 2;	break;
	case cgltf_component_type_r_32u:	size = 4;	break;
	case cgltf_component_type_r_32f:	size = 4;	break;
	}
	size_t dataLen = 1;
	switch (type) {
	case cgltf_type_scalar:	dataLen = 1; break;
	case cgltf_type_vec2:	dataLen = 2; break;
	case cgltf_type_vec3:	dataLen = 3; break;
	case cgltf_type_vec4:	dataLen = 4; break;
	case cgltf_type_mat2:	dataLen = 4; break;
	case cgltf_type_mat3:	dataLen = 9; break;
	case cgltf_type_mat4:	dataLen = 16; break;
	}

	cgltf_buffer* buffer = NULL;
	if (bufferView) {
		buffer = bufferView->buffer;
	}
	else {
		for (auto i = 0; i < count; i++) {
			for (size_t j = 0; j < dataLen; j++) {
				retVal.push_back(0.0f);
			}
		}
		return;
	}

	size_t bs = bufferView->stride;
	size_t step = (bs > 0) ? bs : dataLen * size;

	UINT index = 0;
	for (UINT i = 0; i < count; i++) {
		char *ptr = (char*)(buffer->data) + bufferView->offset + acc->offset + i * step;

		for (size_t j = 0; j < dataLen; j++) {

			switch (componentType) {
			case cgltf_component_type_r_8:
			{
				char v;
				memcpy(&v, (ptr + j * size), sizeof(char));
				retVal.push_back(v);
			}
			break;
			case cgltf_component_type_r_8u:
			{
				unsigned char v;
				memcpy(&v, (ptr + j * size), sizeof(char));
				retVal.push_back(v);
			}
			break;
			case cgltf_component_type_r_16:
			{
				short v;
				memcpy(&v, (ptr + j * size), sizeof(short));
				retVal.push_back(v);
			}
			break;
			case cgltf_component_type_r_16u:
			{
				unsigned short v;
				memcpy(&v, (ptr + j * size), sizeof(short));
				retVal.push_back(v);
			}
			break;
			case cgltf_component_type_r_32u:
			{
				unsigned int v;
				memcpy(&v, (ptr + j * size), sizeof(int));
				retVal.push_back(static_cast<float>(v));
			}
			break;
			case cgltf_component_type_r_32f:
			{
				float v;
				memcpy(&v, (ptr + j * size), sizeof(float));
				retVal.push_back(v);
			}
			break;
			}
			index++;
		}
	}
}
#endif USE_REFACTORED_SETSPARSEDATA
//======================================================================
// Chane data size specified by the sparse
//======================================================================
BOOL glTFImporter_Core::GetDataList(std::vector<float>& retVal, cgltf_accessor* acc)
{
	if (!acc) return TRUE;

	SetSparseData(retVal, acc);
	if (!acc->is_sparse) return TRUE;

	cgltf_accessor_sparse* sparse = &acc->sparse;
	cgltf_type type = acc->type;
	cgltf_component_type componentType = acc->component_type;

	size_t IdxCount = sparse->count;
	cgltf_component_type IdxComponentType = sparse->indices_component_type;
	cgltf_buffer_view *IdxBufferView = sparse->indices_buffer_view;
	cgltf_buffer *IdxBuffer = IdxBufferView->buffer;

	//======================================
	//  Index Table
	//======================================
	std::vector<int> idx;
	char* IdxPtr = (char*)(IdxBuffer->data) + IdxBufferView->offset + sparse->indices_byte_offset;
	switch (IdxComponentType) {
	case cgltf_component_type_r_8u:
		for (UINT i = 0; i < IdxCount; i++) {
			unsigned char data;
			memcpy(&data, IdxPtr, sizeof(char));
			idx.push_back(data);
			IdxPtr += sizeof(char);
		}
		break;
	case cgltf_component_type_r_16u:
		for (UINT i = 0; i < IdxCount; i++){
			unsigned short data;
			memcpy(&data, IdxPtr, sizeof(short));
			idx.push_back(data);
			IdxPtr += sizeof(short);
		}
		break;
	case cgltf_component_type_r_32u:
		for (UINT i = 0; i < IdxCount; i++) {
			unsigned int data;
			memcpy(&data, IdxPtr, sizeof(int));
			idx.push_back(data);
			IdxPtr += sizeof(int);
		}
	}

	//======================================
	//======================================
	size_t size = 1;
	switch (componentType) {
	case cgltf_component_type_r_8:		size = 1;	break;
	case cgltf_component_type_r_8u:	size = 1;	break;
	case cgltf_component_type_r_16:	size = 2;	break;
	case cgltf_component_type_r_16u:	size = 2;	break;
	case cgltf_component_type_r_32u:	size = 4;	break;
	case cgltf_component_type_r_32f:	size = 4;	break;
	}
	size_t dataLen = 1;
	switch (type) {
	case cgltf_type_scalar:	dataLen = 1; break;
	case cgltf_type_vec2:	dataLen = 2; break;
	case cgltf_type_vec3:	dataLen = 3; break;
	case cgltf_type_vec4:	dataLen = 4; break;
	case cgltf_type_mat2:	dataLen = 4; break;
	case cgltf_type_mat3:	dataLen = 9; break;
	case cgltf_type_mat4:	dataLen = 16; break;
	}

	size_t step = dataLen * size;
	cgltf_buffer_view *valBufferView = sparse->values_buffer_view;
	cgltf_buffer *valBuffer = valBufferView->buffer;

	//char* ptr = (char*)(valBuffer->data) + valBufferView->offset + sparse->values_byte_offset;
	for (int i = 0; i < idx.size(); i++) {
		auto index = idx[i];
		char* ptr = (char*)(valBuffer->data) + valBufferView->offset + sparse->values_byte_offset;
		ptr += i * step;
		for (size_t j = 0; j < dataLen; j++) {
			switch (componentType) {
			case cgltf_component_type_r_8:
			{
				char v;
				memcpy(&v, (ptr + j * size), sizeof(char));
				retVal[index * dataLen +j] = v;
			}
			break;
			case cgltf_component_type_r_8u:
			{
				unsigned char v;
				memcpy(&v, (ptr + j * size), sizeof(char));
				retVal[index * dataLen + j] = v;
			}
			break;
			case cgltf_component_type_r_16:
			{
				short v;
				memcpy(&v, (ptr + j * size), sizeof(short));
				retVal[index * dataLen + j] = v;
			}
			break;
			case cgltf_component_type_r_16u:
			{
				unsigned short v;
				memcpy(&v, (ptr + j * size), sizeof(short));
				retVal[index * dataLen + j] = v;
			}
			break;
			case cgltf_component_type_r_32u:
			{
				unsigned int v;
				memcpy(&v, (ptr + j * size), sizeof(int));
				retVal[index * dataLen + j] = static_cast<float>(v);
			}
			break;
			case cgltf_component_type_r_32f:
			{
				float v;
				memcpy(&v, (ptr + j * size), sizeof(float));
				retVal[index * dataLen + j] = v;
			}
			break;
			}
		}
	}

	return TRUE;
}

//======================================================================
// Attach the modfier
// =====================================================================
Modifier *AddModifier(INode* pNode, const Class_ID &CID)
{
	IDerivedObject *pDobj = NULL;
	Object *pObj = pNode->GetObjectRef();
	if (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID) {
		pDobj = (IDerivedObject*)pObj;
	}
	else {
		pDobj = CreateDerivedObject(pObj);
		pNode->SetObjectRef(pDobj);
	}

	Modifier *pMod = (Modifier*)CreateInstance(OSM_CLASS_ID, CID);
	pDobj->AddModifier(pMod);

	pMod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

	return pMod;
}
//======================================================================
// Attach the modfier
//======================================================================
void AddModifier(INode* pNode, Modifier* pMod)
{
	IDerivedObject* pDobj = NULL;
	Object* pObj = pNode->GetObjectRef();
	if (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID) {
		pDobj = (IDerivedObject*)pObj;
	}
	else {
		pDobj = CreateDerivedObject(pObj);
		pNode->SetObjectRef(pDobj);
	}

	pDobj->AddModifier(pMod);

	pMod->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);

}
//======================================================================
// Get specified modifier
//======================================================================
int FindModifier(INode* pNode, const Class_ID &CID, Modifier **pMod)
{
	*pMod = NULL;
	Object* pObj = pNode->GetObjectRef();
	if (!pObj) return NULL;

	if (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID) {
		IDerivedObject *pDerivedObject = static_cast<IDerivedObject*>(pObj);
		for (int i = 0; i < pDerivedObject->NumModifiers(); i++) {
			*pMod = pDerivedObject->GetModifier(i);
			if ((*pMod)->ClassID() == CID) return i;
		}
	}
	*pMod = NULL;
	return -1;
}

//======================================================================
// TODO: check this function's implementation and need for custom 
// unicode escape handling. if possible make use of possibly existing 
// API's in one of the json libs already used by the project
// 
//   Char. number range  |        UTF-8 octet sequence
//      (hexadecimal)    |              (binary)
//   --------------------+---------------------------------------------
//   0000 0000-0000 007F | 0xxxxxxx
//   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
//   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
//   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//======================================================================
bool decode_unicode_escape_to_utf8(const std::string& src, std::string& dst) {
	std::string result("");
	result.resize(src.size());

	for (size_t i = 0; i < result.size(); ++i) {
		result[i] = '\0';
	}

	size_t result_index = 0;
	bool in_surrogate_pair = false;
	size_t surrogate_buffer = 0;
	for (size_t i = 0; i < src.size(); ++i) {
		if (i + 1 < src.size() && src[i] == '\\' && src[i + 1] == 'u') {
			size_t octet = 0;
			{  // calculate octet
				const char* const hex = &src[i + 2];
				for (int j = 0; j < 4; ++j) {
					octet *= 16;
					if ('0' <= hex[j] && hex[j] <= '9') {
						octet += static_cast<int>(hex[j] - '0');
					}
					else if ('a' <= hex[j] && hex[j] <= 'f') {
						octet += static_cast<int>(hex[j] - 'a' + 10);
					}
					else if ('A' <= hex[j] && hex[j] <= 'F') {
						octet += static_cast<int>(hex[j] - 'A' + 10);
					}
					else {
						return false;
					}
				}
			}
			{  // fill up sequence
				char* const sequence = &result[result_index];
				if (in_surrogate_pair) {
					if (0xdc00 <= octet && octet <= 0xdfff) {
						// low surrogate pair
						const size_t joined = surrogate_buffer + (octet & 0x03ff) + 0x10000;
						sequence[0] = (static_cast<char>(joined >> 18) & 0x3) | 0xf0;
						sequence[1] = (static_cast<char>(joined >> 12) & 0x3f) | 0x80;
						sequence[2] = (static_cast<char>(joined >> 6) & 0x3f) | 0x80;
						sequence[3] = (static_cast<char>(joined & 0xff) & 0x3f) | 0x80;
						result_index += 4;
						in_surrogate_pair = false;
					}
					else {
						return false;
					}
				}
				else if (octet < 0x7f) {
					sequence[0] = static_cast<char>(octet) & 0x7f;
					result_index += 1;
				}
				else if (octet < 0x7ff && octet <= 0xdbff) {
					sequence[0] = (static_cast<char>(octet >> 6) & 0xdf) | 0xc0;
					sequence[1] = (static_cast<char>(octet) & 0x3f) | 0x80;
					result_index += 2;
				}
				else if (0xdbff < octet) {
					// high surrogate pair
					in_surrogate_pair = true;
					surrogate_buffer = (octet & 0x03ff) * 0x400;
				}
				else {
					sequence[0] = (static_cast<char>(octet >> 12) & 0x0f) | 0xe0;
					sequence[1] = (static_cast<char>(octet >> 6) & 0x3f) | 0x80;
					sequence[2] = (static_cast<char>(octet) & 0x3f) | 0x80;
					result_index += 3;
				}
			}
			i += 5;  // \\uXXXX is 6 bytes, so + 5 here, and + 1 in next loop
		}
		else {  // not unicode
			if (in_surrogate_pair) {
				return false;
			}
			result[result_index] = src[i];
			result_index += 1;
		}
		// next char
	}
	result.resize(result_index);
	dst.swap(result);
	return true;
}

//======================================================================
//	wstring to string
//======================================================================
std::string WStringToString(std::wstring oWString, int code)
{
	int iBufferSize = WideCharToMultiByte(code, 0, oWString.c_str(), -1, (char *)NULL, 0, NULL, NULL);
	CHAR* cpMultiByte = new CHAR[iBufferSize];

	// wstring -> UTF8
	WideCharToMultiByte(code, 0, oWString.c_str(), -1, cpMultiByte, iBufferSize, NULL, NULL);

	// Construct the string
	std::string oRet(cpMultiByte, cpMultiByte + iBufferSize - 1);

	delete[] cpMultiByte;

	return(oRet);
}

//======================================================================
//	UTF8toSjis
//======================================================================
std::string UTF8toSjis(std::string srcUTF8)
{
	// get string for Unicode conversion length
	size_t lenghtUnicode = MultiByteToWideChar(CP_UTF8, 0, srcUTF8.c_str(), (int)(srcUTF8.size() + 1), NULL, 0);

	// Keep buffer for Unicode string
	wchar_t* bufUnicode = new wchar_t[lenghtUnicode];

	//UTF8 -> Unicode
	MultiByteToWideChar(CP_UTF8, 0, srcUTF8.c_str(), (int)(srcUTF8.size() + 1), bufUnicode, (int)lenghtUnicode);

	int lengthSJis = WideCharToMultiByte(CP_ACP, 0, bufUnicode, -1, NULL, 0, NULL, NULL);

	char* bufShiftJis = new char[lengthSJis];

	WideCharToMultiByte(CP_ACP, 0, bufUnicode, (int)(lenghtUnicode + 1), bufShiftJis, lengthSJis, NULL, NULL);

	std::string strSJis(bufShiftJis);

	delete [] bufUnicode;
	delete [] bufShiftJis;

	return strSJis;
}

//======================================================================
//	string to wstring
// TODO: check this function's implementation and need for custom 
// unicode escape handling. if possible make use of possibly existing 
// API's in one of the json libs already used by the project
//======================================================================
std::wstring StringToWString(const char *oStringOrg, int code)
{
	if (!oStringOrg) return std::wstring(_T(""));

	const char* oString = oStringOrg;

	std::string str;
	if (strlen(oString) > 3) {
		if (oString[0] == '\\' && oString[1] == 'u') {
			decode_unicode_escape_to_utf8(oString, str);
			oString = str.c_str();
		}
	}

	int iBufferSize = MultiByteToWideChar(code, 0, oString, -1, (wchar_t*)NULL, 0);
	if (iBufferSize == 0) return _T("");

	wchar_t* cpUCS2 = new wchar_t[iBufferSize];

	// UTF8 → wstring
	MultiByteToWideChar(code, 0, oString, -1, cpUCS2, iBufferSize);

	// string generation
	std::wstring oRet(cpUCS2, cpUCS2 + iBufferSize - 1);

	delete[] cpUCS2;

	return(oRet);
}

//---------------------------------------------------------
// Get TriObject form Node
//---------------------------------------------------------
TriObject* GetTriObjectFromNode(INode *pNode, TimeValue t, int &deleteIt)
{
	deleteIt = FALSE;
	Object *pObj = pNode->EvalWorldState(t).obj;
	if (pObj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) {
		TriObject *pTri = (TriObject *)pObj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
		if (pObj != pTri) deleteIt = TRUE;
		return pTri;
	}
	else {
		return NULL;
	}
}

//======================================================================
// Execute script
//=====================================================================
BOOL LaunchScript(tstring &script)
{
	std::filesystem::path fullpath = script;

	TSTR comStr;
	comStr.printf(_T("fileIn @\"%s\""), script.c_str());
#if MAX_RELEASE >= 24000
	ExecuteMAXScriptScript(comStr, MAXScript::ScriptSource::NonEmbedded);
#else
	ExecuteMAXScriptScript(comStr);
#endif

	return TRUE;
}

//======================================================================
// Get idx-th IParamBlock from pAnim
//======================================================================
IParamBlock* GetParamBlock(Animatable* pAnim, int idx)
{
	int num = pAnim->NumSubs();
	if (idx > num - 1) return NULL;

	int cnt = 0;
	for (int i = 0; i < num; i++) {
		Animatable* p = pAnim->SubAnim(i);
		if (p->SuperClassID() == PARAMETER_BLOCK_CLASS_ID) {
			if (idx == cnt) return (IParamBlock*)p;
			cnt++;
		}
	}
	return NULL;
}

//================================================================
// Set Environment Map
//================================================================
void SetEnvironmentMap(const tstring& mapName)
{
	if (!GetCOREInterface()->CanImportBitmap(mapName.c_str())) return;

	BitmapTex* pTex = NewDefaultBitmapTex();
	pTex->SetName(mapName.c_str());
	pTex->GetUVGen()->SetCoordMapping(UVMAP_SCREEN_ENV);
	pTex->GetUVGen()->SetTextureTiling(U_WRAP | V_WRAP);
	pTex->GetUVGen()->InitSlotType(MAPSLOT_TEXTURE);
	pTex->SetMapName(mapName.c_str());
	pTex->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED, TRUE);

	GetCOREInterface()->SetUseEnvironmentMap(TRUE);
	GetCOREInterface()->SetEnvironmentMap(pTex);
}

//================================================================
// File open dialog
//================================================================
BOOL GetFileName(HWND hWnd, tstring &ret, FileType type)
{
	OPENFILENAME	OpenInfo;
	TCHAR			FileFullPath[MAX_PATH];
	TCHAR			FileTitle[MAX_PATH];

	memset(FileFullPath, 0, sizeof(FileFullPath));
	_tcscpy_s(FileFullPath, MAX_PATH, _T(""));

	OpenInfo.lStructSize = sizeof(OPENFILENAME);
	OpenInfo.hwndOwner = hWnd;
	OpenInfo.hInstance = NULL;
	if(type== FileType::SCRIPT)
		OpenInfo.lpstrFilter = _T("Script File(*.ms)\0*.ms\0All Files(*.*)\0*.*\0\0");
	else if (type == FileType::IMAGE)
		OpenInfo.lpstrFilter = _T("JPEG File(*.jpg)\0*.jpg\0All Files(*.*)\0*.*\0\0");

	OpenInfo.nFilterIndex = 0;
	OpenInfo.lpstrCustomFilter = 0;
	OpenInfo.nMaxCustFilter = 256;
	OpenInfo.lpstrFile = FileFullPath;
	OpenInfo.nMaxFile = sizeof(FileFullPath);
	OpenInfo.lpstrFileTitle = FileTitle;
	OpenInfo.nMaxFileTitle = sizeof(FileTitle);
	OpenInfo.lpstrInitialDir = ret.c_str();
	OpenInfo.lpstrTitle = _T("Select File.");
	OpenInfo.Flags = OFN_HIDEREADONLY;
	OpenInfo.lpstrDefExt = NULL;
	OpenInfo.lCustData = 0;
	OpenInfo.lpfnHook = NULL;
	OpenInfo.lpTemplateName = NULL;

	//  If cancelled, do nothing and exit
	if (GetOpenFileName(&OpenInfo) == 0) return FALSE;

	ret = FileFullPath;

	return TRUE;
}

//================================================================
// HexToChar
//================================================================
TCHAR HexToChar(TCHAR first, TCHAR second)
{
	TCHAR ret;

	// Convert hex character to numeric value
	if (first >= 'A') {
		ret = first - 'A' + 10;
	}
	else {
		ret = first - '0';
	}

	// Shift the earlier character into the high bits
	ret = ret << 4;

	// Convert the later character and add it
	if (second >= 'A') {
		ret += second - 'A' + 10;
	}
	else {
		ret += second - '0';
	}

	return ret;
}

//================================================================
// Decod the URL string
//================================================================
tstring urlDecode(tstring str)
{
	tstring retStr = _T("");
	tstring::size_type length = str.size();
	TCHAR tmpChar[2];

	// Repeat for each character
	for (tstring::size_type i = 0; i < length; i++) {
		// Convert '+' to space
		if (str[i] == '+') {
			retStr += ' ';
			//Converts characters with % prefixes
		}
		else if (str[i] == '%' && (i + 2) < length) {
			tmpChar[0] = str[i + 1];
			tmpChar[1] = str[i + 2];
			// Check if characters are hexadecimal
			if (isxdigit(tmpChar[0]) && isxdigit(tmpChar[1])) {
				i += 2;

				// Call conversion function and append the result
				retStr += HexToChar(tmpChar[0], tmpChar[1]);

				// If not hex digits, append '%'
			}
			else {
				retStr += '%';
			}
			// Otherwise append the character as-is
		}
		else {
			retStr += str[i];
		}
	}

	return retStr;
}
