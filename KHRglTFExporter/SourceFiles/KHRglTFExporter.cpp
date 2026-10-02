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

 //#pragma warning( disable : 4819 )
//#pragma warning( disable : 4828 )
//#pragma warning( disable : 4267 )
// Define these only in *one* .cc file.

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "KHRglTFExporter.h"
#include <maxscript/maxscript.h>
#include <AssetManagement/iassetmanager.h>
#include <AssetManagement/AssetUser.h>
#include <winutil.h>
#include <shellapi.h>

BOOL exportSelected;

static tstring s_TitleString;

static float HH_scale;
static BOOL HH_CopyImage;
static int HH_ExportFileType;
static BOOL HH_DracoCompress;
static int HH_ExportShapeObj;
static BOOL HH_Instancing;
static int HH_EncodeSpeed;
static BOOL HH_ExportAnimation;
static BOOL HH_FullFrame;
static BOOL HH_ExportTangent;
static BOOL HH_ExportMorphNrm;
static BOOL HH_AnimPointer;
static BOOL HH_ViewAnimPointer;
static BOOL HH_WireClrToMtl;
static BOOL HH_ExportUserProp;
static int HH_MROMapExportMode;
static int HH_MROImageSize;
static int HH_MROImageType;
static BOOL HH_GPUInstance;
static int HH_MultiScene;
static BOOL HH_Collision;
static BOOL HH_AttachRigidInfo;
static BOOL HH_InstanceWithMtl;
static UINT HH_DefSceneIdx;
static BOOL HH_ResetXFormMod;
static BOOL HH_PostProcess;
static BOOL HH_ApplyScaling;
static BOOL HH_ReferenceFileMode;
static BOOL HH_Interactivity;
static int HH_InteractiveGraphID;
static BOOL HH_CubicSplineT;
static BOOL HH_Quantization;
static int HH_glTFFileVer;

static BOOL Open_InstanceWithMtl;


static const TCHAR *pLicenseStr = _T(
"glTF/glb Exporter for 3dsmax Designed By Satoshi Hayashi\r\n \
tiny-glTF2.0 is licensed under the MIT License.\r\n \
Draco is licensed under the Apache 2.0.\r\n \
\r\n \
===================================================== \r\n \
libwebp  LICENSE\r\n \
Copyright(c) 2010, Google Inc.All rights reserved.\r\n \
===================================================== \r\n \
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

glTFExporter_Core theExporterCore;

static BOOL LogExport = TRUE;
static tstring LogFileName;

std::vector<tstring> glTF_File_Ver = {_T("2.0"),_T("2.1")};
//======================================================================
//======================================================================
void LogInit(const tstring &output)
{
	std::filesystem::path fname = output;
	LogFileName = tstring(fname.parent_path()) + tstring(_T("\\")) + tstring(fname.stem()) + tstring(_T(".log"));
	if (PathFileExists(LogFileName.c_str())) {
		DeleteFile(LogFileName.c_str());
	}
}
void LogOutput(const std::wstring& str, int pcs)
{
	if (!LogExport) return;

	GetCOREInterface()->ProgressUpdate(pcs, TRUE, str.c_str());

#ifdef _DEBUG
#else
	return;
#endif

	std::wstring fname;
	//fname = GetCOREInterface()->GetDir(APP_PLUGINS_DIR);

	FILE* fp = nullptr;
	errno_t err = _tfopen_s(&fp, LogFileName.c_str(), _T("a+"));
	if (err) return;

	_ftprintf(fp, _T("%s\n"), str.c_str());

	//TSTR log;
	//log.printf(_T("%s\n"), str.c_str());
	//_ftprintf(fp, log);

	fclose(fp);
}

//======================================================================
//======================================================================
inline tstring TextureTableCountStr(void)
{
	return to_tstring(theExporterCore.TextureTableCount());
/*
#ifdef UNICODE
	return std::to_wstring(theExporterCore.TextureTableCount());
#else
	return std::to_string(theExporterCore.TextureTableCount());
#endif
*/
}
//======================================================================
//======================================================================
inline const tstring ExportFolder(void){return theExporterCore.ExportFolder();}

//======================================================================
//======================================================================
inline IPoint2 GetBitmapSize(void){	return theExporterCore.m_CreateBitmapSize;}

//======================================================================
// Exportee class definition
//======================================================================
class KHRglTFExporter : public SceneExport
{
public:
	//Constructor/Destructor
	KHRglTFExporter();
	virtual ~KHRglTFExporter();

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
	virtual int				DoExport(const TCHAR *name, ExpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE, DWORD options = 0);	// Export file
	BOOL					SupportsOptions(int ext, DWORD options) {return(options == SCENE_EXPORT_SELECTED) ? TRUE : FALSE;}
};
#if 0
class KHRglTF2Exporter : public SceneExport
{
public:
	//Constructor/Destructor
	KHRglTF2Exporter();
	virtual ~KHRglTF2Exporter();

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
	virtual int				DoExport(const TCHAR *name, ExpInterface *i, Interface *gi, BOOL suppressPrompts = FALSE, DWORD  options = 0);	// Export file

};
class KHRglTF2ExporterClassDesc : public ClassDesc2
{
public:
	virtual int           IsPublic() override { return TRUE; }
	virtual void*         Create(BOOL /*loading = FALSE*/) override { return new KHRglTF2Exporter(); }
	virtual const TCHAR* ClassName() override { return GetString(IDS_CLASS_NAME2); }
	virtual SClass_ID     SuperClassID() override { return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID      ClassID() override { return KHRglTF2Exporter_CLASS_ID; }
	virtual const TCHAR* Category() override { return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() override { return _T("KHRglTF2Exporter"); } // Returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE     HInstance() override { return hInstance; } // Returns owning module handle
#if MAX_RELEASE>=24000
	const wchar_t* ClassDesc::NonLocalizedClassName(void) { return GetString(IDS_CLASS_NAME2); }
#endif
};
ClassDesc2* GetKHRglTF2ExporterDesc()
{
	static KHRglTF2ExporterClassDesc KHRglTF2ExporterDesc;
	return &KHRglTF2ExporterDesc;
}
//--- KHRglTFImporter -------------------------------------------------------
KHRglTF2Exporter::KHRglTF2Exporter()
{
}

KHRglTF2Exporter::~KHRglTF2Exporter()
{
}

int KHRglTF2Exporter::ExtCount()
{
	return 2;
}

const TCHAR* KHRglTF2Exporter::Ext(int i)
{
	switch (i) {
	case 0:	return _T("glTF");
	case 1:	return _T("glb");
	}
}

const TCHAR* KHRglTF2Exporter::LongDesc()
{
	return _T("glTF 2.0 Exporter for 3ds Max");
}

const TCHAR* KHRglTF2Exporter::ShortDesc()
{
	return _T("glTF 2.0 (KHRglTFExporter)");
}

const TCHAR* KHRglTF2Exporter::AuthorName()
{
	return _T("Satoshi Hayashi");
}

const TCHAR* KHRglTF2Exporter::CopyrightMessage()
{
	return _T("(C) Satoshi Hayashi");
}

const TCHAR* KHRglTF2Exporter::OtherMessage1()
{
	return _T("");
}

const TCHAR* KHRglTF2Exporter::OtherMessage2()
{
	return _T("");
}

unsigned int KHRglTF2Exporter::Version()
{
	return 100;
}

void KHRglTF2Exporter::ShowAbout(HWND /*hWnd*/)
{
	// Optional
}
int KHRglTF2Exporter::DoExport(const TCHAR* filename, ExpInterface* exporterInt, Interface* ip, BOOL suppressPrompts, DWORD options)
{
	/*
	if (!IsValid()) {
		MessageBox(GetCOREInterface()->GetMAXHWnd(), TEXT("License Expired."), TEXT("License Expired"), MB_ICONINFORMATION);;
		return TRUE;
	}
	*/

	return theExporterCore.ExportPreProcess(filename, suppressPrompts, 2);
}
#endif
//======================================================================
// Plugin description
//======================================================================
class KHRglTFExporterClassDesc : public ClassDesc2 
{
public:
	virtual int           IsPublic() override                       { return TRUE; }
	virtual void*         Create(BOOL /*loading = FALSE*/) override { return new KHRglTFExporter(); }
	virtual const TCHAR * ClassName() override                      { return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID     SuperClassID() override                   { return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID      ClassID() override                        { return KHRglTFExporter_CLASS_ID; }
	virtual const TCHAR*  Category() override                       { return GetString(IDS_CATEGORY); }

	virtual const TCHAR*  InternalName() override                   { return _T("KHRglTFImporter"); } // Returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE     HInstance() override                      { return hInstance; } // Returns owning module handle
#if MAX_RELEASE>=24000
	const wchar_t *ClassDesc::NonLocalizedClassName(void) { return GetString(IDS_CLASS_NAME); }
#endif
};
ClassDesc2* GetKHRglTFExporterDesc()
{
	static KHRglTFExporterClassDesc KHRglTFExporterDesc;
	return &KHRglTFExporterDesc;
}

//======================================================================
//======================================================================
DWORD WINAPI StatusBarFn(LPVOID arg)
{
	return(0);
}
//======================================================================
// Parameter Setting Dialog CallBack
//======================================================================
INT_PTR CALLBACK KHRglTFRapidCompOptionsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	static KHRglTFExporter* exp = nullptr;

	switch (message) {
	case WM_INITDIALOG:
	{
		CheckDlgButton(hWnd, IDC_CHECK1, HH_PostProcess);

		// Set hyperlink color to blue
		HWND hLink = GetDlgItem(hWnd, IDC_HYPERLINK_STATIC);
		//SetWindowText(hLink, L"Click here to visit Google");

		// Undeline font
		HFONT hFont = (HFONT)SendMessage(hLink, WM_GETFONT, 0, 0);
		LOGFONT lf;
		GetObject(hFont, sizeof(lf), &lf);
		lf.lfUnderline = TRUE;
		HFONT hUnderlineFont = CreateFontIndirect(&lf);
		SendMessage(hLink, WM_SETFONT, (WPARAM)hUnderlineFont, TRUE);
	}
		return TRUE;

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_HYPERLINK_STATIC:
			if (HIWORD(wParam) == STN_CLICKED)
			{
				ShellExecute(NULL, L"open", L"https://www.rapidcompact.com/product/", NULL, NULL, SW_SHOWNORMAL);
				//OpenHyperlink(L"https://www.rapidcompact.com/product/");
			}
			return 1;

		case IDOK:
			HH_PostProcess = IsDlgButtonChecked(hWnd, IDC_CHECK1);

			::EndDialog(hWnd, 1);
			return 1;
		}
		break;

	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;
		HWND hwndStatic = (HWND)lParam;
		if (GetDlgCtrlID(hwndStatic) == IDC_HYPERLINK_STATIC)
		{
			SetTextColor(hdcStatic, RGB(0, 0, 255));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (INT_PTR)GetStockObject(NULL_BRUSH);
		}
	}
	break;

	default:
		return (BOOL)DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

//======================================================================
// Parameter Setting Dialog CallBack
//======================================================================
INT_PTR CALLBACK KHRglTFExporterOptionsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	static KHRglTFExporter* exp = nullptr;
	//static ISpinnerControl *pSpin;

	switch (message) {
	case WM_INITDIALOG:
		exp = (KHRglTFExporter*)lParam;

		ShowWindow(GetDlgItem(hWnd, IDC_ANIMPTR_CHK), HH_ViewAnimPointer);
		if (!HH_ViewAnimPointer) HH_AnimPointer = FALSE;

		CheckDlgButton(hWnd, IDC_COPYIMAGE_CHECK, HH_CopyImage);
		CheckDlgButton(hWnd, IDC_DRACO_CHECK, HH_DracoCompress);
		if (HH_ExportShapeObj == 0) {
			CheckDlgButton(hWnd, IDC_EXPSHAPE_CHK, 0);
			EnableWindow(GetDlgItem(hWnd, IDC_RENDERSHAPE_CHK), FALSE);
		}
		else {
			EnableWindow(GetDlgItem(hWnd, IDC_RENDERSHAPE_CHK), TRUE);
			CheckDlgButton(hWnd, IDC_EXPSHAPE_CHK, TRUE);
			CheckDlgButton(hWnd, IDC_RENDERSHAPE_CHK, (HH_ExportShapeObj == 2));
		}
		CheckDlgButton(hWnd, IDC_INSTANCE_CHK, HH_Instancing);
		CheckDlgButton(hWnd, IDC_GPUINSTNCE_CHK, HH_GPUInstance);
		CheckDlgButton(hWnd, IDC_EXPORTTAN_CHK, HH_ExportTangent);
		CheckDlgButton(hWnd, IDC_MORPHNRM_CHK, HH_ExportMorphNrm);
		CheckDlgButton(hWnd, IDC_FTYPE_RADIO1, (HH_ExportFileType == 1));
		CheckDlgButton(hWnd, IDC_FTYPE_RADIO2, (HH_ExportFileType == 2));
		CheckDlgButton(hWnd, IDC_FTYPE_RADIO3, (HH_ExportFileType == 3));
		CheckDlgButton(hWnd, IDC_ANIM_CHK, HH_ExportAnimation);
		CheckDlgButton(hWnd, IDC_FULLFRAME_CHK, HH_FullFrame);
		CheckDlgButton(hWnd, IDC_CUBICSPLINE_CHK, HH_CubicSplineT);
		CheckDlgButton(hWnd, IDC_ANIMPTR_CHK, HH_AnimPointer);
		CheckDlgButton(hWnd, IDC_WCLRMTL_CHK, HH_WireClrToMtl);
		CheckDlgButton(hWnd, IDC_EXPORTUPROP_CHK, HH_ExportUserProp);
		CheckDlgButton(hWnd, IDC_SCNLAYER_CHK, HH_MultiScene);
		CheckDlgButton(hWnd, IDC_WM_CHK, HH_InstanceWithMtl);
		CheckDlgButton(hWnd, IDC_RESETXFORM_CHK, HH_ResetXFormMod);
		CheckDlgButton(hWnd, IDC_SCL_CHECK, HH_ApplyScaling);
		CheckDlgButton(hWnd, IDC_PHYSQ_CHK, HH_Collision);
		CheckDlgButton(hWnd, IDC_INTERACT_CHK, HH_Interactivity);
		CheckDlgButton(hWnd, IDC_QUANT_CHK, HH_Quantization);

		CheckRadioButton(hWnd, IDC_MRO_RADIO1, IDC_MRO_RADIO3, IDC_MRO_RADIO1 + HH_MROMapExportMode);
		if (GetCOREInterface()->GetRootNode()->GetXRefFileCount() > 0) {
			INode* pRootNode = GetCOREInterface()->GetRootNode();
			int numXref = pRootNode->GetXRefFileCount();
			for (int i = 0; i < numXref; i++) {
				std::filesystem::path pp = pRootNode->GetXRefFile(i).GetFileName().data();
				SendMessage(GetDlgItem(hWnd, IDC_DEF_SCENE), CB_ADDSTRING, 0, (LPARAM)pp.stem().c_str());
			}
			SendMessage(GetDlgItem(hWnd, IDC_DEF_SCENE), CB_SETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(hWnd, IDC_DEF_SCENE), (HH_MultiScene == 2));
			EnableWindow(GetDlgItem(hWnd, IDC_DEFSCN_STATIC), (HH_MultiScene == 2));
		}
		else {
			EnableWindow(GetDlgItem(hWnd, IDC_SCENE_RADIO3), 0);
			EnableWindow(GetDlgItem(hWnd, IDC_DEF_SCENE), 0);
			EnableWindow(GetDlgItem(hWnd, IDC_DEFSCN_STATIC), 0);
			if (HH_MultiScene > 1)HH_MultiScene = 0;
		}
		CheckRadioButton(hWnd, IDC_SCENE_RADIO1, IDC_SCENE_RADIO3, IDC_SCENE_RADIO1 + HH_MultiScene);

		SendMessage(GetDlgItem(hWnd, IDC_SIZE_LIST), CB_ADDSTRING, 0, (LPARAM)_T("Original Image Size"));
		SendMessage(GetDlgItem(hWnd, IDC_SIZE_LIST), CB_ADDSTRING, 0, (LPARAM)_T("512x512"));
		SendMessage(GetDlgItem(hWnd, IDC_SIZE_LIST), CB_ADDSTRING, 0, (LPARAM)_T("1024x1024"));
		SendMessage(GetDlgItem(hWnd, IDC_SIZE_LIST), CB_ADDSTRING, 0, (LPARAM)_T("2048x2048"));
		SendMessage(GetDlgItem(hWnd, IDC_SIZE_LIST), CB_ADDSTRING, 0, (LPARAM)_T("4096x4096"));
		SendMessage(GetDlgItem(hWnd, IDC_SIZE_LIST), CB_SETCURSEL, HH_MROImageSize, 0);

		SendMessage(GetDlgItem(hWnd, IDC_TYPE_LIST), CB_ADDSTRING, 0, (LPARAM)_T("JPG"));
		SendMessage(GetDlgItem(hWnd, IDC_TYPE_LIST), CB_ADDSTRING, 0, (LPARAM)_T("PNG"));
		SendMessage(GetDlgItem(hWnd, IDC_TYPE_LIST), CB_SETCURSEL, HH_MROImageType, 0);

		for (const tstring& ver : glTF_File_Ver) {
			SendMessage(GetDlgItem(hWnd, IDC_VER_COMBO1), CB_ADDSTRING, 0, (LPARAM)ver.c_str());
		}
		SendMessage(GetDlgItem(hWnd, IDC_VER_COMBO1), CB_SETCURSEL, HH_glTFFileVer, 0);

		EnableWindow(GetDlgItem(hWnd, IDC_FULLFRAME_CHK), HH_ExportAnimation);
		EnableWindow(GetDlgItem(hWnd, IDC_ANIMPTR_CHK), HH_ExportAnimation);
		EnableWindow(GetDlgItem(hWnd, IDC_CUBICSPLINE_CHK), HH_ExportAnimation);

		EnableWindow(GetDlgItem(hWnd, IDC_GPUINSTNCE_CHK), HH_Instancing);
		EnableWindow(GetDlgItem(hWnd, IDC_WM_CHK), HH_Instancing);

		ShowWindow(GetDlgItem(hWnd, IDC_LICENSE_BTN), FALSE);

		ShowWindow(GetDlgItem(hWnd, IDC_WM_CHK), Open_InstanceWithMtl);
		SetWindowText(GetDlgItem(hWnd, IDCLICENSE_EDIT), pLicenseStr);
		//CenterWindow(hWnd, GetParent(hWnd));

		CheckDlgButton(hWnd, IDC_REFERENCE_CHK, HH_ReferenceFileMode);

		if (theExporterCore.m_InteractiveLayerTable.size()) {
			tstring BaseLayerName = _T("KHRInteractiveGraphLayer");
			for (ILayer* pLayer : theExporterCore.m_InteractiveLayerTable) {
				tstring name(pLayer->GetName().data());
				if (name == _T("KHRInteractiveGraphLayer__ExtensionNode__")) continue;
				if (name.find(BaseLayerName) != std::string::npos) {
					tstring str = name.substr(BaseLayerName.size());
					SendMessage(GetDlgItem(hWnd, IDC_INTERACT_COMBO), CB_ADDSTRING, 0, (LPARAM)str.c_str());
				}
			}
			SendMessage(GetDlgItem(hWnd, IDC_INTERACT_COMBO), CB_SETCURSEL, HH_InteractiveGraphID, 0);
		}
		else {
			EnableWindow(GetDlgItem(hWnd, IDC_INTERACT_CHK), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_INTERACT_COMBO), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_CAMERA_COMBO), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_STATIC_INT1), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_STATIC_INT2), FALSE);
		}

		return TRUE;

	case WM_CLOSE:
		//ReleaseISpinner(pSpin);
		EndDialog(hWnd, 0);
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EXPSHAPE_CHK:
			EnableWindow(GetDlgItem(hWnd, IDC_RENDERSHAPE_CHK), IsDlgButtonChecked(hWnd, IDC_EXPSHAPE_CHK));
			break;
		case IDC_ANIM_CHK:
			EnableWindow(GetDlgItem(hWnd, IDC_FULLFRAME_CHK), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_ANIMPTR_CHK), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_CUBICSPLINE_CHK), IsDlgButtonChecked(hWnd, IDC_ANIM_CHK));
			break;
		case IDC_INSTANCE_CHK:
			EnableWindow(GetDlgItem(hWnd, IDC_GPUINSTNCE_CHK), IsDlgButtonChecked(hWnd, IDC_INSTANCE_CHK));
			EnableWindow(GetDlgItem(hWnd, IDC_WM_CHK), IsDlgButtonChecked(hWnd, IDC_INSTANCE_CHK));
			break;

		case IDC_MRO_RADIO1:
		case IDC_MRO_RADIO2:
		case IDC_MRO_RADIO3:
			EnableWindow(GetDlgItem(hWnd, IDC_SIZE_LIST), !IsDlgButtonChecked(hWnd, IDC_MRO_RADIO1));
			EnableWindow(GetDlgItem(hWnd, IDC_TYPE_LIST), !IsDlgButtonChecked(hWnd, IDC_MRO_RADIO1));
			EnableWindow(GetDlgItem(hWnd, IDC_SIZE_STATIC), !IsDlgButtonChecked(hWnd, IDC_MRO_RADIO1));
			EnableWindow(GetDlgItem(hWnd, IDC_TYPE_STATIC), !IsDlgButtonChecked(hWnd, IDC_MRO_RADIO1));
			break;
		case IDC_SCENE_RADIO1:
		case IDC_SCENE_RADIO2:
		case IDC_SCENE_RADIO3:
			EnableWindow(GetDlgItem(hWnd, IDC_DEF_SCENE), IsDlgButtonChecked(hWnd, IDC_SCENE_RADIO3));
			EnableWindow(GetDlgItem(hWnd, IDC_DEFSCN_STATIC), IsDlgButtonChecked(hWnd, IDC_SCENE_RADIO3));
			break;
		case IDC_IMGSET_BTN:
			theExporterCore.ImageSetting(hWnd, (int)SendMessage(GetDlgItem(hWnd, IDC_TYPE_LIST), CB_GETCURSEL, 0, 0));
			break;

		case IDC_QUANT_CHK:
			CheckDlgButton(hWnd, IDC_DRACO_CHECK, FALSE);
			break;
		case IDC_DRACO_CHECK:
			CheckDlgButton(hWnd, IDC_QUANT_CHK, FALSE);
			break;

		case IDOK:
			HH_CopyImage = IsDlgButtonChecked(hWnd, IDC_COPYIMAGE_CHECK);
			HH_DracoCompress = IsDlgButtonChecked(hWnd, IDC_DRACO_CHECK);
			//HH_EncodeSpeed = pSpin->GetIVal();
			if (IsDlgButtonChecked(hWnd, IDC_EXPSHAPE_CHK)) {
				if (IsDlgButtonChecked(hWnd, IDC_RENDERSHAPE_CHK))
					HH_ExportShapeObj = 2;
				else
					HH_ExportShapeObj = 1;
			}
			else {
				HH_ExportShapeObj = 0;
			}
			HH_ExportAnimation = IsDlgButtonChecked(hWnd, IDC_ANIM_CHK);
			HH_FullFrame = IsDlgButtonChecked(hWnd, IDC_FULLFRAME_CHK);
			HH_CubicSplineT = IsDlgButtonChecked(hWnd, IDC_CUBICSPLINE_CHK);
			HH_ExportTangent = IsDlgButtonChecked(hWnd, IDC_EXPORTTAN_CHK);
			HH_ExportMorphNrm = IsDlgButtonChecked(hWnd, IDC_MORPHNRM_CHK);
			HH_AnimPointer = IsDlgButtonChecked(hWnd, IDC_ANIMPTR_CHK);
			HH_Instancing = IsDlgButtonChecked(hWnd, IDC_INSTANCE_CHK);
			HH_GPUInstance = IsDlgButtonChecked(hWnd, IDC_GPUINSTNCE_CHK);
			HH_WireClrToMtl = IsDlgButtonChecked(hWnd, IDC_WCLRMTL_CHK);
			HH_ExportUserProp = IsDlgButtonChecked(hWnd, IDC_EXPORTUPROP_CHK);
			HH_InstanceWithMtl = IsDlgButtonChecked(hWnd, IDC_WM_CHK);
			HH_ResetXFormMod = IsDlgButtonChecked(hWnd, IDC_RESETXFORM_CHK);
			HH_ApplyScaling = IsDlgButtonChecked(hWnd, IDC_SCL_CHECK);
			HH_ReferenceFileMode = IsDlgButtonChecked(hWnd, IDC_REFERENCE_CHK);
			HH_Collision = IsDlgButtonChecked(hWnd, IDC_PHYSQ_CHK);
			HH_Interactivity = IsDlgButtonChecked(hWnd, IDC_INTERACT_CHK);
			HH_Quantization = IsDlgButtonChecked(hWnd, IDC_QUANT_CHK);

				HH_glTFFileVer = SendMessage(GetDlgItem(hWnd, IDC_VER_COMBO1), CB_GETCURSEL, 0, 0);

			if (IsDlgButtonChecked(hWnd, IDC_MRO_RADIO1)) HH_MROMapExportMode = 0;
			if (IsDlgButtonChecked(hWnd, IDC_MRO_RADIO2)) HH_MROMapExportMode = 1;
			if (IsDlgButtonChecked(hWnd, IDC_MRO_RADIO3)) HH_MROMapExportMode = 2;
			HH_MROImageSize = (int)SendMessage(GetDlgItem(hWnd, IDC_SIZE_LIST), CB_GETCURSEL, 0, 0);
			HH_MROImageType = (int)SendMessage(GetDlgItem(hWnd, IDC_TYPE_LIST), CB_GETCURSEL, 0, 0);

			if (IsDlgButtonChecked(hWnd, IDC_SCENE_RADIO1)) HH_MultiScene = 0;
			if (IsDlgButtonChecked(hWnd, IDC_SCENE_RADIO2)) HH_MultiScene = 1;
			if (IsDlgButtonChecked(hWnd, IDC_SCENE_RADIO3)) HH_MultiScene = 2;
			if (HH_MultiScene == 2) HH_DefSceneIdx = (int)SendMessage(GetDlgItem(hWnd, IDC_DEF_SCENE), CB_GETCURSEL, 0, 0);

			if (IsDlgButtonChecked(hWnd, IDC_FTYPE_RADIO1))
				HH_ExportFileType = 1;
			else if (IsDlgButtonChecked(hWnd, IDC_FTYPE_RADIO2))
				HH_ExportFileType = 2;
			else if (IsDlgButtonChecked(hWnd, IDC_FTYPE_RADIO3))
				HH_ExportFileType = 3;
			::EndDialog(hWnd, 1);
			break;

		case IDCANCEL:
			::EndDialog(hWnd, 0);
			break;
		}
		break;

	default:
		return (BOOL)DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

//======================================================================
// Main Dialog CallBack
//======================================================================
INT_PTR CALLBACK KHRglTFExporterMainDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static KHRglTFExporter* exp = nullptr;
	static HWND hExportWnd = NULL;
	static HWND hRapidCompWnd = NULL;

	switch (message) {
	case WM_INITDIALOG:
	{
		TC_ITEM tabItem;
		memset(&tabItem, 0, sizeof(TC_ITEM));

		tabItem.mask = TCIF_TEXT;
		tabItem.pszText = const_cast<TCHAR*>(_T("Main Settings"));
		tabItem.cchTextMax = 16;
		TabCtrl_InsertItem(::GetDlgItem(hWnd, IDC_TAB1), 0, &tabItem);
		hExportWnd = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_EXPORT_DIALOG), hWnd, KHRglTFExporterOptionsDlgProc, (LPARAM)NULL);
#ifdef _DEBUG
		tabItem.pszText = _T("Post Proc");
		TabCtrl_InsertItem(::GetDlgItem(hWnd, IDC_TAB1), 1, &tabItem);
		hRapidCompWnd = ::CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_RAPIDCOMP_DIALOG), hWnd, KHRglTFRapidCompOptionsDlgProc, (LPARAM)NULL);
#endif
	}

		//if (!hRapidCompWnd)
		//{
		//	DWORD dwError = GetLastError();
		//	wchar_t szError[256];
		//	swprintf_s(szError, sizeof(szError) / sizeof(wchar_t), L"Dialog creation failed! Error code: %d", dwError);
		//	MessageBox(NULL, szError, L"Error", MB_OK | MB_ICONERROR);
		//}

		SetWindowPos(hExportWnd,NULL, 0, 35, 0, 0, SWP_NOSIZE);
		SetWindowPos(hRapidCompWnd, NULL, 0, 35, 0, 0, SWP_NOSIZE);

		ShowWindow(hRapidCompWnd, SW_HIDE);
		SetWindowText(hWnd, s_TitleString.c_str());

		exp = (KHRglTFExporter*)lParam;

		ShowWindow(GetDlgItem(hWnd, IDC_LICENSE_BTN), FALSE);

		ShowWindow(GetDlgItem(hWnd, IDC_WM_CHK), Open_InstanceWithMtl);
		SetWindowText(GetDlgItem(hWnd, IDCLICENSE_EDIT), pLicenseStr);
		CenterWindow(hWnd, GetParent(hWnd));
		return TRUE;

	case WM_CLOSE:
		//ReleaseISpinner(pSpin);
		EndDialog(hWnd, 0);
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDOK:
			SendMessage(hExportWnd, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), (LPARAM)0);
			SendMessage(hRapidCompWnd, WM_COMMAND, MAKELONG(IDOK, BN_CLICKED), (LPARAM)0);
			::EndDialog(hWnd, 1);
			break;
		case IDCANCEL:
			::EndDialog(hWnd, 0);
			break;
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case TCN_SELCHANGE:
			{
			int n = TabCtrl_GetCurSel(::GetDlgItem(hWnd, IDC_TAB1));
			ShowWindow(hExportWnd, n == 0 ? SW_SHOW : SW_HIDE);
			ShowWindow(hRapidCompWnd, n == 1 ? SW_SHOW : SW_HIDE);
			}
			break;
		}
		break;
	}

	return 0;
}


//--- KHRglTFImporter -------------------------------------------------------
KHRglTFExporter::KHRglTFExporter()
{
}

KHRglTFExporter::~KHRglTFExporter()
{
}

int KHRglTFExporter::ExtCount()
{
	return 2;
}

const TCHAR *KHRglTFExporter::Ext(int i)
{		
	switch(i) {
	case 0:	return _T("glTF");
	case 1:	return _T("glb");
	}

	return _T("glTF");
}

const TCHAR *KHRglTFExporter::LongDesc()
{
	return _T("glTF 2.0 Exporter for 3ds Max");
}
	
const TCHAR *KHRglTFExporter::ShortDesc()
{			
	return _T("glTF 2.0 (KHRglTFExporter)");
}

const TCHAR *KHRglTFExporter::AuthorName()
{			
	return _T("Satoshi Hayashi");
}

const TCHAR *KHRglTFExporter::CopyrightMessage()
{	
	return _T("Satoshi Hayashi");
}

const TCHAR *KHRglTFExporter::OtherMessage1()
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *KHRglTFExporter::OtherMessage2()
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int KHRglTFExporter::Version()
{				
	return 100;
}

void KHRglTFExporter::ShowAbout(HWND /*hWnd*/)
{			
	// Optional
}

// -----------------------------------------------------------
// -----------------------------------------------------------
int KHRglTFExporter::DoExport(const TCHAR* filename, ExpInterface* exporterInt, Interface* ip, BOOL suppressPrompts, DWORD options)
{
	LogInit(tstring(filename));
	LogOutput(_T("DoExport Start."));

	LogOutput(_T("DoExport End."));

	exportSelected = (options & SCENE_EXPORT_SELECTED) ? TRUE : FALSE;

	int ret = theExporterCore.ExportPreProcess(filename, suppressPrompts, 1);
	return ret;
}

// -----------------------------------------------------------
// -----------------------------------------------------------
void *glTFExporter_Core::SecureMemory(int size)
{
#if 1
	m_BufferByteOffset += size;
	void *ptr = realloc(m_glTf_Buffer, m_BufferByteOffset);

	m_glTf_Buffer = ptr;
	//m_glTf_Buffer = m_BufferByteOffset;

	return ptr;
#else
	if (size <= 0) return nullptr;

	// Prevent overflow
	size_t oldOffset = m_BufferByteOffset;
	size_t newOffset = oldOffset + static_cast<size_t>(size);
	if (newOffset < oldOffset) return nullptr; // overflow

	void* newBuf = realloc(m_glTf_Buffer, newOffset);
	if (!newBuf) {
		// allocation failed, keep existing buffer intact
		return nullptr;
	}

	m_glTf_Buffer = newBuf;
	m_BufferByteOffset = newOffset;

	// Return pointer to the start of newly allocated chunk
	return static_cast<unsigned char*>(m_glTf_Buffer) + oldOffset;
#endif
}

// -----------------------------------------------------------
// -----------------------------------------------------------
void glTFExporter_Core::FreeSceneData(void)
{
/*
	for (auto p : m_model.accessors) delete &p;
	for (auto p : m_model.nodes) delete &p;
	for (auto p : m_model.meshes) delete &p;
	for (auto p : m_model.bufferViews) delete &p;
	for (auto p : m_model.materials) delete &p;
	for (auto p : m_model.textures) delete &p;
	for (auto p : m_model.images) delete &p;
	for (auto p : m_model.skins) delete &p;
	for (auto p : m_model.cameras) delete &p;
	for (auto p : m_model.lights) delete &p;

	if (m_model.animations.size() > 0) {
		tinygltf::Animation anim = m_model.animations[0];
		for (auto p : anim.channels) delete &p;
		for(auto p : anim.samplers) delete &p;
	}
*/
	if (m_glTf_Buffer) {
		free(m_glTf_Buffer);
		m_glTf_Buffer = nullptr;
	}
	m_BufferByteOffset = 0;


	// Clear containers to release owned resources and avoid dangling refs
	m_model.accessors.clear();
	m_model.animations.clear();
	m_model.buffers.clear();
	m_model.bufferViews.clear();
	m_model.materials.clear();
	m_model.meshes.clear();
	m_model.nodes.clear();
	m_model.textures.clear();
	m_model.images.clear();
	m_model.skins.clear();
	m_model.samplers.clear();
	m_model.cameras.clear();
	m_model.scenes.clear();
	m_model.lights.clear();
	m_model.extensions.clear();

	m_imagePathTable.clear();
	m_NodeMap.clear();
	m_MeshMap.clear();
	m_LightMap.clear();
	m_LightIESMap.clear();
	m_mimeTable.clear();
	m_nameTable.clear();
	m_skinNodeTable.clear();
	m_morphNodeTable.clear();
	m_animation.channels.clear();
	m_animation.samplers.clear();
	m_WireColorMtlMap.clear();
	m_GPUInstanceMap.clear();
	m_GPUInstanceNodeList.clear();
	m_PhysicMtlTable.clear();
	m_CollisionShapeTable.clear();
}

//======================================================================
//======================================================================
BOOL glTFExporter_Core::ExportPreProcess(const TCHAR* filename, BOOL suppressPrompts, int ver)
{
	s_TitleString = _T("Khronos glTF exporter for 3dsmax ") + tstring(KHR_GLTF_EXPORTER_VER);

	TSTR profle;
	profle.printf(_T("%s\\%s"), GetCOREInterface()->GetDir(APP_PLUGCFG_DIR), _T("KHRglTFImporter.ini"));

	int x = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("scale"), 39370, profle);
	HH_scale = x / 1000.0f;
	HH_CopyImage = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("CopyImage"), 0, profle);
	HH_ExportFileType = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ExportFileType"), 1, profle);
	HH_DracoCompress = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("DracoCompress"), 0, profle);
	HH_ExportShapeObj = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ExportShapeObj"), 0, profle);
	HH_Instancing = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("Instancing"), 0, profle);
	//HH_EncodeSpeed = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("EncodeSpeed"), 0, profle);
	HH_EncodeSpeed = 0;
	HH_ExportAnimation = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ExportAnimation"), 1, profle);
	HH_FullFrame = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("FullFrame"), 0, profle);
	HH_ExportTangent = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ExportTangent"), 0, profle);
	HH_ExportMorphNrm = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ExportMorphNrm"), 1, profle);
	HH_AnimPointer = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("AnimPointer"), 0, profle);
	HH_ViewAnimPointer = 1;// MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ViewAnimPointer"), 0, profle);
	HH_WireClrToMtl = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("WireClrToMtl"), 0, profle);
	HH_ExportUserProp = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ExportUserProp"), 0, profle);
	HH_MROMapExportMode = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("MROMapExportMode"), 0, profle);
	HH_MROImageSize = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("MROImageSize"), 1, profle);
	HH_MROImageType = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("MROImageType"), 1, profle);
	HH_GPUInstance = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("GPUInstance"), 1, profle);
	HH_MultiScene = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("MultiScene"), 0, profle);
	HH_Collision = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("Collision"), 0, profle);
	HH_AttachRigidInfo = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("AttachRigidInfo"), 0, profle);
	HH_InstanceWithMtl = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("InstanceWithMtl"), 0, profle);
	HH_ResetXFormMod = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ResetXFormMod"), 0, profle);
	HH_ApplyScaling = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ApplyScaling"), 0, profle);
	HH_ReferenceFileMode = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("ReferenceFileMode"), 0, profle);
	HH_Interactivity = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("Interactivity"), 0, profle);
	HH_CubicSplineT	= MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("CubicSplineT"), 0, profle);
	HH_Quantization = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("Quantization"), 0, profle);
	HH_glTFFileVer = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("glTFFileVer"), 0, profle);

	HH_PostProcess = FALSE;

	Open_InstanceWithMtl = MaxSDK::Util::GetPrivateProfileInt(_T("ExpSettings"), _T("Open_InstanceWithMtl"), 0, profle);

//#ifdef _DEBUG
	//HH_Collision = 1;
	HH_AttachRigidInfo = 0;
//#endif

	CreateInteractiveLayerTable();

	if (!suppressPrompts) {
		if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), GetActiveWindow(), KHRglTFExporterMainDlgProc, (LPARAM)this) == 0) {
			return TRUE;
		}

		TCHAR buf[MAX_PATH];
		_stprintf_s(buf, MAX_PATH, _T("%d"), (int)(HH_scale*1000.0f));
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("scale"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_CopyImage ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("CopyImage"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ExportFileType);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ExportFileType"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_DracoCompress);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("DracoCompress"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ExportShapeObj);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ExportShapeObj"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_Instancing);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("Instancing"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_EncodeSpeed);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("EncodeSpeed"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ExportAnimation);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ExportAnimation"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_FullFrame);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("FullFrame"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ExportTangent);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ExportTangent"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ExportMorphNrm ? 1 : 0);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ExportMorphNrm"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_AnimPointer);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("AnimPointer"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_WireClrToMtl);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("WireClrToMtl"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ExportUserProp);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ExportUserProp"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_MROMapExportMode);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("MROMapExportMode"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_MROImageSize);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("MROImageSize"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_MROImageType);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("MROImageType"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_GPUInstance);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("GPUInstance"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_MultiScene);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("MultiScene"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_AttachRigidInfo);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("HAttachRigidInfo"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_InstanceWithMtl);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("InstanceWithMtl"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ResetXFormMod);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ResetXFormMod"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ApplyScaling);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ApplyScaling"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_ReferenceFileMode);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ReferenceFileMode"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_Collision);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("Collision"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_Interactivity);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("Interactivity"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_CubicSplineT);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("CubicSplineT"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_Quantization);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("Quantization"), buf, profle);
		_stprintf_s(buf, MAX_PATH, _T("%d"), HH_glTFFileVer);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("glTFFileVer"), buf, profle);
	}

	// only show messagebox if not suppressed
	if(!suppressPrompts) {
		{
#define MESSAGE_STR _T("Negative transform(Mirrored Object) detected, meshes may not export correctly to glTF.\nThe use of the ResetXForm utility is highly recommended.")
			INodeTab tbl;
			GetMirroredNode(tbl);
			if(tbl.Count() > 0) {
				int ret = MessageBox(NULL, MESSAGE_STR, _T("WARNING:Mirrored Objexct detected"), MB_OKCANCEL | MB_ICONWARNING);
				if(ret == IDCANCEL) {
					if(!exportSelected) GetCOREInterface()->SelectNodeTab(tbl, TRUE);
					return TRUE;
				}
			}
		}
	}

	m_scale = 1.0f;
	if (HH_ApplyScaling)
	{
		//scale = 39.37f;
		int type = 0;
#if MAX_RELEASE < 24000
		GetMasterUnitInfo(&type, &m_scale);
#else
		GetSystemUnitInfo(&type, &m_scale);
#endif
		switch (type)
		{
		case UNITS_INCHES:		m_scale *= 0.0254f;		break;
		case UNITS_FEET:		m_scale *= 0.3048f;		break;
		case UNITS_MILES:		m_scale *= 1609.34f;	break;
		case UNITS_MILLIMETERS:	m_scale *= 0.001f;		break;
		case UNITS_CENTIMETERS:	m_scale *= 0.01f;		break;
		case UNITS_METERS:		m_scale *= 1.0f;		break;
		case UNITS_KILOMETERS:	m_scale *= 1000.0f;		break;
		default:				m_scale *= 1.0f;		break;
		}

	}

	if (HH_ReferenceFileMode) {
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ReferenceFileMode"), _T("0"), profle);
		HH_ReferenceFileMode = FALSE;
		CreateGLTFXFile(filename);
		MaxSDK::Util::WritePrivateProfileString(_T("ExpSettings"), _T("ReferenceFileMode"), _T("1"), profle);
		HH_ReferenceFileMode = TRUE;
		return TRUE;
	}

	//m_CopyImage = HH_CopyImage;
	m_ExportFileType = HH_ExportFileType;
	if (m_ExportFileType == 1) m_CopyImage = 1;
	else m_CopyImage = 0;
	m_DracoCompress = HH_DracoCompress;
	m_ExportShapeObj = HH_ExportShapeObj;
	m_Instancing = HH_Instancing;
	m_GPUInstance = HH_GPUInstance & HH_Instancing;
	m_EncodeSpeed = HH_EncodeSpeed;
	m_ExportAnimation = HH_ExportAnimation;
	m_FullFrame = HH_FullFrame;
	m_ExportTangent = HH_ExportTangent;
	m_ExportMorphNrm = HH_ExportMorphNrm;
	m_WireClrToMtl = HH_WireClrToMtl;
	m_ExportUserProp = HH_ExportUserProp;
	m_MROMapExportMode = HH_MROMapExportMode;
	m_MultiScene = HH_MultiScene;
	m_Collision = HH_Collision;
	m_Interactivity = HH_Interactivity;
	m_ForceTRSMode = m_Interactivity;
	m_AttachRigidInfo = HH_AttachRigidInfo;
	switch (HH_MROImageSize) {
	case 0:m_CreateBitmapSize = IPoint2(0, 0); break;
	case 1:m_CreateBitmapSize = IPoint2(512, 512); break;
	case 2:m_CreateBitmapSize = IPoint2(1024, 1024); break;
	case 3:m_CreateBitmapSize = IPoint2(2048, 2048); break;
	default:m_CreateBitmapSize = IPoint2(4096, 4096); break;
	}
	m_MROImageType = HH_MROImageType;
	m_InstanceWithMtl = HH_InstanceWithMtl;
	m_ResetXFormMod = HH_ResetXFormMod;
	m_ReferenceFileMode = HH_ReferenceFileMode;
	m_InteractiveGraphID = HH_InteractiveGraphID;
	m_ResetPivotTM = FALSE;
	m_CubicSplineT = HH_CubicSplineT;
	m_Mesh_quantization_Used = HH_Quantization;

	int dc = GetSpinnerPrecision();

	m_time = 0;	//GetCOREInterface()->GetTime();
	m_TimeScale = (float)(GetTicksPerFrame() * GetFrameRate());

	m_fullpath = std::wstring(filename);

#if MAX_RELEASE>=25000
	GetCOREInterface()->ProgressStart(_M("gtTF file Exporting."), FALSE, NULL, NULL);
#else
	GetCOREInterface()->ProgressStart(_M("gtTF file Exporting."), FALSE, StatusBarFn, NULL);
#endif


	m_IncorrectSkinDataFound = FALSE;

	ExportScene(ver);

	GetCOREInterface()->ProgressEnd();

	if (m_IncorrectSkinDataFound && !suppressPrompts) {
		MessageBox(
			NULL,
			_T("There are Skin modifiers with more than 4 bones assigned to a single vertex.\nThis may result in incorrect export."),
			_T("Warning"),
			MB_OK | MB_ICONWARNING
		);
	}

	return TRUE;
}

// -----------------------------------------------------------
// -----------------------------------------------------------
void glTFExporter_Core::ExportScene(int ver)
{
	LogOutput(_T("Start."));

	m_pIGameScene = GetIGameInterface();
	m_pIGameScene->InitialiseIGame(GetCOREInterface()->GetRootNode());

	tstring stem = m_fullpath.stem();
	tstring ext = m_fullpath.extension();
	std::transform(ext.cbegin(), ext.cend(), ext.begin(), tolower);

	if (m_ExportFileType == 3) {
		m_fullpath = tstring(m_fullpath.parent_path()) + tstring(_T("\\")) + stem + tstring(_T(".glb"));
	}
	else {
		m_fullpath = tstring(m_fullpath.parent_path()) + tstring(_T("\\")) + stem + tstring(_T(".gltf"));
	}
	std::filesystem::path binFile = ExportFolder() + tstring(_T("\\")) + stem + tstring(_T(".bin"));

	m_model.accessors.clear();
	m_model.animations.clear();
	m_model.buffers.clear();
	m_model.bufferViews.clear();
	m_model.materials.clear();
	m_model.meshes.clear();
	m_model.nodes.clear();
	m_model.textures.clear();
	m_model.images.clear();
	m_model.skins.clear();
	m_model.samplers.clear();
	m_model.cameras.clear();
	m_model.scenes.clear();
	m_model.lights.clear();
	m_model.extensions.clear();

	m_imagePathTable.clear();
	m_NodeMap.clear();
	m_MeshMap.clear();
	m_LightMap.clear();
	m_LightIESMap.clear();
	m_mimeTable.clear();
	m_nameTable.clear();
	m_skinNodeTable.clear();
	m_morphNodeTable.clear();
	m_animation.channels.clear();
	m_animation.samplers.clear();
	m_WireColorMtlMap.clear();
	m_GPUInstanceMap.clear();
	m_GPUInstanceNodeList.clear();
	m_PhysicMtlTable.clear();
	m_CollisionShapeTable.clear();

	m_glTf_Buffer = NULL;
	m_BufferByteOffset = 0;
	m_DracoBufferViewIndex = 0;

	m_MtlTransmission_Used = FALSE;
	m_MtlVolume_Used = FALSE;
	m_MtlSpecular_Used = FALSE;
	m_MtlSheen_Used = FALSE;
	m_MtlClearCoat_Used = FALSE;
	m_MtlEmissive_Used = FALSE;
	m_MtlIor_Used = FALSE;
	m_MtlPbrSpcGls_Used = FALSE;
	m_MtlTtranslucency_Used = FALSE;
	m_MtlUnlit_Used = FALSE;
	m_TexTransform_Used = FALSE;
	m_MtlVariants_Used = FALSE;
	m_MtlEmitStrength_Used = FALSE;
	m_LlightsPunctual_Used = FALSE;
	m_LlightsIES_Used = FALSE;
	m_AnimationPointer_Used = FALSE;
	m_MtlAnisotropy_Used = FALSE;
	m_MtlIridescence_Used = FALSE;
	m_AnimationPointer_Used = FALSE;
	m_Mesh_gpu_instancing_Used = FALSE;
	m_collision_shapes_Used = FALSE;
	m_Physic_RigidBody_Used = FALSE;
	m_MtlDispersion_Used = FALSE;
	m_MtlDiffuseTransmission_Used = FALSE;
	m_TextureWebp_Used = FALSE;
	m_MaterialBump_Used = FALSE;
	m_Interactivity_Used = FALSE;
	m_Visibility_Used = FALSE;
	m_Selectability_Used = FALSE;
	m_Hoverability_Used = FALSE;

	m_MtlDiffuseTransmission_Used = FALSE;
	m_MtlSSS_Used = FALSE;
	m_TexBasisu_Used = FALSE;


	m_model.extensionsUsed.clear();
	m_model.extensionsRequired.clear();

	if (m_Mesh_quantization_Used) {
		CreateQuatizationMap();
	}

	CreateMorphTable();

	if (m_WireClrToMtl) {
		INode* pRootNode = GetCOREInterface()->GetRootNode();
		for (int i = 0; i < pRootNode->NumChildren(); i++) {
			CreateWireColorMtlMap(pRootNode->GetChildNode(i));
		}

		int numXref = pRootNode->GetXRefFileCount();
		for (int i = 0; i < numXref; i++) {
			INode* pXRefRootNode = pRootNode->GetXRefTree(i);
			for (int j = 0; j < pXRefRootNode->NumChildren(); j++) {
				CreateWireColorMtlMap(pXRefRootNode->GetChildNode(j));
			}
		}

	}

	LogOutput(_T("Create Material Table."));

	CreateMaterialMap(exportSelected);

	LogOutput(_T("Create Material Table->Finish."));
	LogOutput(_T("Create Scene Table.\n"));

	m_model.defaultScene = 0;
	if (m_MultiScene==1) {
		ILayerManager* pLayerMgr = GetCOREInterface13()->GetLayerManager();
		for (int li = 0; li < pLayerMgr->GetLayerCount(); li++) {
			ILayer* pLayer = pLayerMgr->GetLayer(li);
			//if (pLayer == m_pInteractiveGraphLayer) continue;
			if (std::find(m_InteractiveLayerTable.begin(), m_InteractiveLayerTable.end(), pLayer) != m_InteractiveLayerTable.end()) continue;
			tinygltf::Scene scene;
			CreateSceneData(scene, -1, pLayer);
			scene.name = WStringToString(pLayer->GetName().data());
			m_model.scenes.push_back(scene);
			if(pLayerMgr->GetCurrentLayer()== pLayer)
				m_model.defaultScene = li;
		}
	}
	else if (m_MultiScene == 2) {
		tinygltf::Scene Basescene;
		CreateSceneData(Basescene, Ignore_XRefScene);

		INode* pRootNode = GetCOREInterface()->GetRootNode();
		int numXref = pRootNode->GetXRefFileCount();
		for (int i = 0; i < numXref; i++) {
			tinygltf::Scene scene(Basescene);
			CreateSceneData(scene, i);

			std::filesystem::path pp = pRootNode->GetXRefFile(i).GetFileName().data();
			scene.name = WStringToString(pp.stem());
			m_model.scenes.push_back(scene);
		}
		m_model.defaultScene = HH_DefSceneIdx;
	}
	else {
		tinygltf::Scene scene;
		CreateSceneData(scene);
		SetSceneExtras(scene);
		m_model.scenes.push_back(scene);
	}

	LogOutput(_T("Create Scene Table->Finish."));
	LogOutput(_T("Create Skin Table."));

	for (auto tbl: m_skinNodeTable) {
		CreateSkin(tbl.first, tbl.second);
	}

	LogOutput(_T("Create Skin Table->Finish."));
	LogOutput(_T("Create Image Buffer."));

	if (m_ExportFileType == 3) {
		CreateImageBuffer();
	}

	LogOutput(_T("Create Image Buffer->Finish."));

	if (m_ExportAnimation) {
		LogOutput(_T("Create Animation.\n"));
		
		CreateAnimation();
		if(HH_AnimPointer)
			CreateAnimationPointer();

		LogOutput(_T("Create Animation->Finish."));
	}

	std::vector<std::string> interactiveExtensionList;
	if (m_Interactivity) {
		GetInteractivityNodeList(interactiveExtensionList);
	}

	unsigned char *ptr = (unsigned char *)m_glTf_Buffer;
	tinygltf::Buffer buffer;
	for (int i = 0; i < m_BufferByteOffset;i++) {
		buffer.data.push_back(*ptr++);
	}

	SetSceneExtensions();

	LogOutput(_T("Create Data Buffer."));

	if(m_animation.channels.size()>0)
		m_model.animations.push_back(m_animation);
	m_model.buffers.push_back(buffer);

	LogOutput(_T("Create Data Buffer->Finish."));

	if (m_MtlTransmission_Used)			m_model.extensionsUsed.push_back("KHR_materials_transmission");
	if (m_MtlVolume_Used)				m_model.extensionsUsed.push_back("KHR_materials_volume");
	if (m_MtlSpecular_Used)				m_model.extensionsUsed.push_back("KHR_materials_specular");
	if (m_MtlSheen_Used)				m_model.extensionsUsed.push_back("KHR_materials_sheen");
	if (m_MtlClearCoat_Used)			m_model.extensionsUsed.push_back("KHR_materials_clearcoat");
	if (m_MtlEmissive_Used)				m_model.extensionsUsed.push_back("KHR_materials_emissive_strength");
	if (m_MtlIor_Used)					m_model.extensionsUsed.push_back("KHR_materials_ior");
	if (m_MtlPbrSpcGls_Used)			m_model.extensionsUsed.push_back("KHR_materials_pbrSpecularGlossiness");
	if (m_MtlTtranslucency_Used)		m_model.extensionsUsed.push_back("KHR_materials_translucency");
	if (m_TexTransform_Used)			m_model.extensionsUsed.push_back("KHR_texture_transform");
	if (m_MtlVariants_Used)				m_model.extensionsUsed.push_back("KHR_materials_variants");
	if (m_MtlEmitStrength_Used)			m_model.extensionsUsed.push_back("KHR_materials_emissive_strength");
	if (m_AnimationPointer_Used)		m_model.extensionsUsed.push_back("KHR_animation_pointer");
	if (m_MtlAnisotropy_Used)			m_model.extensionsUsed.push_back("KHR_materials_anisotropy");
	if (m_MtlIridescence_Used)			m_model.extensionsUsed.push_back("KHR_materials_iridescence");
	if (m_MtlSSS_Used)					m_model.extensionsUsed.push_back("KHR_materials_sss");
	if (m_collision_shapes_Used)		m_model.extensionsUsed.push_back("KHR_implicit_shapes");
	if (m_Physic_RigidBody_Used)		m_model.extensionsUsed.push_back("KHR_physics_rigid_bodies");
	if (m_MtlDispersion_Used)			m_model.extensionsUsed.push_back("KHR_materials_dispersion");
	if (m_TextureWebp_Used)				m_model.extensionsUsed.push_back("EXT_texture_webp");
	if (m_MtlDiffuseTransmission_Used)	m_model.extensionsUsed.push_back("KHR_materials_diffuse_transmission");
	if (m_Mesh_gpu_instancing_Used)		m_model.extensionsUsed.push_back("EXT_mesh_gpu_instancing");
	if (m_MaterialBump_Used)			m_model.extensionsUsed.push_back("EXT_materials_bump");
	if (m_Interactivity_Used)			m_model.extensionsUsed.push_back("KHR_interactivity");
	if (m_Visibility_Used)				m_model.extensionsUsed.push_back("KHR_node_visibility");
	if (m_Selectability_Used)			m_model.extensionsUsed.push_back("KHR_node_selectability");
	if (m_Hoverability_Used)			m_model.extensionsUsed.push_back("KHR_node_hoverability");
	if (m_MtlUnlit_Used)				m_model.extensionsUsed.push_back("KHR_materials_unlit");
	if (m_LlightsPunctual_Used) 		m_model.extensionsUsed.push_back("KHR_lights_punctual");
	if(m_TexBasisu_Used) 				m_model.extensionsUsed.push_back("KHR_texture_basisu");

	if (m_LlightsIES_Used) {
		m_model.extensionsUsed.push_back("EXT_lights_ies");
		//m_model.extensionsRequired.push_back("EXT_lights_ies");
	}
	if (m_DracoCompress) {
		m_model.extensionsUsed.push_back("KHR_draco_mesh_compression");
		m_model.extensionsRequired.push_back("KHR_draco_mesh_compression");
	}
	if (m_Mesh_quantization_Used) {
		m_model.extensionsUsed.push_back("KHR_mesh_quantization");
		m_model.extensionsRequired.push_back("KHR_mesh_quantization");
	}

	for (auto p : interactiveExtensionList) {
		m_model.extensionsUsed.push_back(p);
	}

	{
		s_TitleString += _T(" (C)Khronos Group Inc.");
		m_model.asset.generator = WStringToString(s_TitleString);
		m_model.asset.copyright = WStringToString(GetCompanyString());
		m_model.asset.version = WStringToString(glTF_File_Ver[HH_glTFFileVer]);
	}

	std::string fname = WStringToString(m_fullpath);

	LogOutput(_T("Export File."));

	tinygltf::TinyGLTF gltf;
	gltf.SetImageWriter(nullptr, nullptr);
	gltf.WriteGltfSceneToFile(&m_model, fname,
		(m_ExportFileType != 1), // embedImages
		(m_ExportFileType != 1), // embedBuffers
		true, // pretty print
		(m_ExportFileType == 3)); // write binary

	LogOutput(_T("Export File->Finish."));
	LogOutput(_T("Copy Image File."));

	if (m_CopyImage && (m_ExportFileType == 1)) {
		for (auto tbl : m_imagePathTable) {
			std::filesystem::path SrcPath = tbl;
			tstring fname = SrcPath.filename();
			if (!std::filesystem::exists(SrcPath)) {
				MaxSDK::AssetManagement::AssetUser asset = MaxSDK::AssetManagement::IAssetManager::GetInstance()->GetAsset(TSTR(fname.c_str()), MaxSDK::AssetManagement::AssetType::kBitmapAsset);
				TSTR str(fname.c_str());
				asset.GetFullFilePath(str);
				SrcPath = tstring(str);
			}
			std::filesystem::path DstPath = tstring(m_fullpath.parent_path()) + tstring(_T("\\")) + fname;
			CopyFile(SrcPath.c_str(), DstPath.c_str(), FALSE);
		}
	}

	LogOutput(_T("Copy Image File->Finish."));

	FreeSceneData();

	for (auto m : m_WireColorMtlMap) if(m.second) m.second->DeleteThis();

	if (HH_PostProcess) PostProcess(m_fullpath);

	LogOutput(_T("End."));
}


//----------------------------------------------------------
//----------------------------------------------------------
void glTFExporter_Core::ImageSetting(HWND hWnd, int type)
{
	BitmapInfo bi;
	bi.SetHeight(10);
	bi.SetWidth(10);
	bi.SetType(BMM_TRUE_64);
	bi.SetFlags(0);

	switch (type) {
	case 0:
		bi.SetName(_T("dummy.jpg"));
		break;
	case 1:
		bi.SetName(_T("dummy.png"));
		break;
	}

	BitmapIO* pBmpIO = TheManager->ioList.CreateDevInstance(bi.Device());
	if (pBmpIO) {
		pBmpIO->ShowControl(hWnd, BMMIO_WRITER);
		delete pBmpIO;
	}

}


//----------------------------------------------------------
//----------------------------------------------------------
void glTFExporter_Core::CreateInteractiveLayerTable(void)
{
	m_InteractiveLayerTable.clear();
	ILayerManager* pLayerMan = GetCOREInterface13()->GetLayerManager();
	for (int i = 0; i < pLayerMan->GetLayerCount(); i++) {
		ILayer* pLayer = pLayerMan->GetLayer(i);
		tstring name(pLayer->GetName().data());
		if (name.find(InteractiveLayerName) != std::string::npos) {
			m_InteractiveLayerTable.push_back(pLayer);
		}
	}

}


//----------------------------------------------------------
// Retrieve the specified modifier
// Returns the modifier instance if found, otherwise returns null
// //----------------------------------------------------------
int FindModifier(INode* pNode, const Class_ID &CID, Modifier **pMod)
{
	if (!pNode) return -1;

	*pMod = NULL;
	Object* pObj = pNode->GetObjectRef();
	if (!pObj) return -1;

	const TCHAR *ptr = pNode->GetName();
	SClass_ID ss = pObj->SuperClassID();

	// If the referenced object is a derived object, it has modifiers
	if (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID) {
		IDerivedObject *pDerivedObject = static_cast<IDerivedObject*>(pObj);
		// Loop through the modifier stack
		for (int i = 0; i < pDerivedObject->NumModifiers(); i++) {
			*pMod = pDerivedObject->GetModifier(i);
			if ((*pMod)->ClassID() == CID) return i;
		}
	}
	*pMod = NULL;
	return -1;
}


//======================================================================
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


//======================================================================
// If the texture UV is animated
//======================================================================
BOOL UVGenAnimated(StdUVGen* pUVGen)
{
	if (!HH_AnimPointer) return FALSE;
	if (!pUVGen) return FALSE;

	IParamBlock* pBlock = GetParamBlock(pUVGen, 0);
	if (!pBlock) return FALSE;

	Control* pOffsetUC = pBlock->GetController(0);
	Control* pOffsetVC = pBlock->GetController(1);
	Control* pScaleUC = pBlock->GetController(2);
	Control* pScaleVC = pBlock->GetController(3);

	if (pOffsetUC && pOffsetUC->IsAnimated()) return TRUE;
	if (pOffsetVC && pOffsetVC->IsAnimated()) return TRUE;
	if (pScaleUC && pScaleUC->IsAnimated()) return TRUE;
	if (pScaleVC && pScaleVC->IsAnimated()) return TRUE;

	return FALSE;
}

//======================================================================
//	wstring to string
//======================================================================
std::string WStringToString(std::wstring oWString)
{
	int iBufferSize = WideCharToMultiByte(CP_UTF8, 0, oWString.c_str(), -1, (char *)NULL, 0, NULL, NULL);
	CHAR* cpMultiByte = new CHAR[iBufferSize];

	// wstring → UTF
	WideCharToMultiByte(CP_UTF8, 0, oWString.c_str(), -1, cpMultiByte, iBufferSize, NULL, NULL);

	// create the string
	std::string oRet(cpMultiByte, cpMultiByte + iBufferSize - 1);

	delete[] cpMultiByte;

	return(oRet);
}
//======================================================================
//	string to wstring
//======================================================================
std::wstring StringToWString(const char *oString)
{
	int iBufferSize = MultiByteToWideChar(CP_ACP, 0, oString, -1, (wchar_t*)NULL, 0);
	if (iBufferSize == 0) return _T("");

	wchar_t* cpUCS2 = new wchar_t[iBufferSize];

	// SJIS ->wstring
	MultiByteToWideChar(CP_ACP, 0, oString, -1, cpUCS2, iBufferSize);

	// creqate a string
	std::wstring oRet(cpUCS2, cpUCS2 + iBufferSize - 1);

	delete[] cpUCS2;

	return(oRet);
}

//================================================================
// Truncate to the specified decimal places.
//================================================================
double truncateDecimal(float value) 
{
	double multiplier = std::pow(10.0, GetSpinnerPrecision());
	return (double)std::floor(value * multiplier) / multiplier;
}

