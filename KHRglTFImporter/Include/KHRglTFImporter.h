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

#pragma once

#include "3dsmaxsdk_preinclude.h"
#include "Max.h"
#include "resource.h"
#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <maxtypes.h>
//SIMPLE TYPE

#include <impexp.h>
#include <direct.h>
#include <commdlg.h>

#include <vector>
#include <map>
#include <filesystem>
#include <string>

#include <meshdelta.h>
#include <stdmat.h>
#include <modstack.h>
#include <iskin.h>
#include <simpobj.h>
#include <dummy.h>
#include "decomp.h"
#include "maxscript/maxscript.h"
#include <ILayerControl.h>
#include <templt.h>
#include <iparamwire.h>
#include <iCustAttribContainer.h>
#include <CustAttrib.h>

#ifdef UNICODE
#define tstring std::wstring
#define to_tstring(a) std::to_wstring(a)
#else
#define tstring std::string
#define to_tstring(a) std::to_string(a)
#endif

extern std::string UTF8toSjis(std::string srcUTF8);
extern void LogOutput(const std::string& str, int pcs = 0);
extern BOOL IsLogOut(void);

#include <cgltf.h>



#define KHR_APP_CODE 200
#define KHR_GLTF_IMPORTER_VER (_T("2.01"))

#define KHR_UV_MODE 1

#define KHRglTFImporter_CLASS_ID	Class_ID(0x87a91a65, 0xa2560866)
#define KHRglTF2Importer_CLASS_ID	Class_ID(0x24081954, 0x30fd74c7)
#define KHRglTFExporter_CLASS_ID	Class_ID(0x38586030, 0x1b315b3e)
#define KHRglTF2Exporter_CLASS_ID	Class_ID(0x56337879, 0x5a732c12)

#define KHRGLTFIMP_INTERFACE_ID		Interface_ID(0xc230161, 0x37060a2b)

#define EDIT_NORMALS_CLASS_ID		Class_ID(0x4aa52ae3, 0x35ca1cde)

//#define IPOS_CONTROL_CLASS_ID		Class_ID(0x118f7e02,0xffee238a)
#define bmptexClassID				Class_ID(BMTEX_CLASS_ID, 0)
#define CompositeTexClassID			Class_ID(0x280, 0)
#define multiClassID				Class_ID(MULTI_CLASS_ID,0)
#define CompositeMtlClassID			Class_ID(0x61dc0cd7, 0x13640af6)
#define ColorMapTexID				Class_ID(0x139f22c6, 0x13f6a914)
#define ColorCorrectTexID			Class_ID(COLORCORRECTION_CLASS_ID, 0x0)
#define MixTexID					Class_ID(0x230, 0x0)

#define MaterialSwitcherClassID		Class_ID(0x4ecd74a6, 0x0)
#define glTFMtlSwitcherClassID		Class_ID(0x9587a1a, 0x42bc9eb6)
#define PysicMtlSwitcherClassID		Class_ID(0x6af0dc4, 0x624849a5)
#define StdMtlSwitcherClassID		Class_ID(0x2fb3468d, 0x25fd57a2)
#define PBRMtlSwitcherClassID		Class_ID(0x2f4e61ce, 0x7a754a41)
#define USDMtlSwitcherClassID		Class_ID(0x506a52fd, 0x75430b22)

#ifndef PHYSICALMATERIAL_CLASS_ID
#define PHYSICALMATERIAL_CLASS_ID Class_ID(0x3d6b1cec, 0xdeadc001)
#endif
#define StandardMtlID				Class_ID(0x2, 0x0)
#define glTFMaterialID				Class_ID(0x38420192, 0x45fe4e1b)
#define PBRMetalMtlID				Class_ID(0xd00f1e00, 0xbe77e500)
#define PBRSpecGlossMtlID			Class_ID(0xd00f1e00, 0x1dbad33)
#define Arnold_StandardSufaceID		Class_ID(0x7e73161f, 0x62f74b4c)
#define ArnoldNormalMapID			Class_ID(0x7e73161f, 0x461fb78c)
#define ArnoldSwitchShaderID		Class_ID(0x7e73161f, 0xa844c228)
#define VRayMaterialID				Class_ID(0x37bf3f2f, 0x7034695c)
#define VRayNormalMapID				Class_ID(0x71fa6e51, 0x72057c2f)
#define VRayBitmapID				Class_ID(0x6769144b, 0x2c1017d)
#define VRayCompTexID				Class_ID(0x20d9613e, 0x2af82b3c)
#define VRayColorID					Class_ID(0x58f82b74, 0x73b75d7f)
#define VRayDirtID					Class_ID(0x2f567899, 0x90d5ea4)
#define CoronaMaterialID			Class_ID(0x6912ab89, 0x87151720)
#define CoronaNormalMapID			Class_ID(0x2870ceaa, 0xcc18437c)
#define CoronaBitmapID				Class_ID(0xabba5784, 0x65484244)
#define CoronaMixID					Class_ID(0x6847286f, 0x3fffaab4)
#define CoronaColorID				Class_ID(0x68426dca, 0x372362d4)
#define CoronaSelectMtlID			Class_ID(0xc74e9f94, 0xc319f213)
#define USDMaterialID				Class_ID(0x6afa4933, 0x4787f1c7)
//#define Pencil4MaterialID			Class_ID(0x20160205, 0x81454de)
#define OpenPBRMaterialID			Class_ID(0xf1551e33, 0x37fb1337)

#define OSL_UnSupport		0
#define OSL_BitmapLookUp	1
#define OSL_UberBitmap		2
#define OSL_CutOff			3
#define OSL_AlphaCh			4
#define OSL_ColorScale		5
#define OSL_ColorMultiply	6





#define NORMAL_SCALE 20.0f
inline float GetNormalScale(void) {	return NORMAL_SCALE;}

inline const MCHAR* GetCustomAttrName(CustAttrib* p)
{
#if MAX_RELEASE >= 26000
	return p->GetName(FALSE);
#else
	return p->GetName();
#endif
}

inline std::vector<uint32_t> FloatToUInt32IndexBuffer(const std::vector<float>& in)
{
	std::vector<uint32_t> out(in.size());
	for (size_t i = 0; i < in.size(); ++i)
		out[i] = static_cast<uint32_t>(in[i]);
	return out;
}

enum DracoDecodeType {
	INVALID = -1,
	POSITION = 0,
	NORMAL,
	COLOR,
	TEX_COORD,
	GENERIC,
	WEIGHTS,
	JOINTS,
	NAMED_ATTRIBUTES_COUNT,
};

enum class TargetTex {
	BaseColorMap = 0,
	EmissiveMap,
	MetalnessMap,
	RoughnessMap,
	NormalMap,
	OcclusionMap,
	ClearcoatMap,
	ClearcoatRoughnessMap,
	ClearcoatNormalMap,
	SheenColorMap,
	SheenRoughnessMap,
	SpecularMap,
	SpecularColorMap,
	TransmissionMap,
	VolumeThicknessMap,
	IridescenceMap,
	IridescenceThicknessMap,
	AnisotropyMap,
	DiffuseTransmissionMap,
	DiffuseTransmissionColorMap,
};

enum class FileType {
	SCRIPT = 0,
	IMAGE = 1,
};

struct AnimKeyInfo {
	static constexpr int  LINEAR = 0;
	static constexpr int  CUBICSPLINE = 1;
	static constexpr int  STEP = 2;

	Point3 pos;
	Quat rot;
	Point3 inTan;
	Point3 outTan;
	Quat inTanQ;
	Quat outTanQ;

	Point4 clr4;
	Point4 inTan4;
	Point4 outTan4;

	int i = 0;
	float f = 0.0f;
	int interpolation = 0;
};

struct custAttrParam {
	int type = 0;
	std::string name;
	float fParam = 0.0f;
	float fminParam = 0.0f;
	float fmaxParam = 0.0f;
	int iParam = 0;
	int iminParam = 0;
	int imaxParam = 0;
	std::string sParam;
	Color cParam = Color(0.0f, 0.0f, 0.0f);
	Animatable* pParam = nullptr;
	BOOL inVisible = FALSE;
};

struct CollisionInfo {
	int rigidType = 0;
	float mass = 0;
	Point3 inertiaDiagonal = Point3(0.0,0.0,0.0);
	Quat inertiaOrientation;
	Point3 linearVelocity = Point3(0.0, 0.0, 0.0);
	Point3 angularVelocity = Point3(0.0, 0.0, 0.0);
	int shape = 0;
	int physicsMaterial = 0;
	int collisionFilter = 0;
	INode* gravityNode = nullptr;
	int COM = 0;
	Point3 COM_pt;
	int joint = 0;
	INode* connectedNode = nullptr;
	BOOL enableCollision = FALSE;
};

struct vrayExtStruct {
	float roughness;
};

struct SelectabilityStruct {
	BOOL selectable;
};

struct HoverabilityStruct {
	BOOL hoverable;
};

struct VisibilityStruct {
	BOOL visible;
};

struct InteractivityStruct {
	DWORD id;//GetTickCount
};

inline const Matrix3 YupTM(Point3(1, 0, 0), Point3(0, 0, 1), Point3(0, -1, 0), Point3(0, 0, 0));
//static Matrix3 YupTM(Point3(-1, 0, 0), Point3(0, 0, 1), Point3(0, 1, 0), Point3(0, 0, 0));

extern HINSTANCE hInstance;
extern std::wstring StringToWString(const char *oString, int code = CP_UTF8);
extern std::string WStringToString(std::wstring oWString, int code = CP_UTF8);

extern TCHAR *GetString(int id);
extern TriObject* GetTriObjectFromNode(INode *pNode, TimeValue t, int &deleteIt);
//extern BOOL CheckIfVRayInstalled(void);

extern Modifier *AddModifier(INode* pNode, const Class_ID &CID);
extern void AddModifier(INode* pNode, Modifier* pMod);
extern int FindModifier(INode* pNode, const Class_ID &CID, Modifier **pMod);
//INode *CreateDummyNode(const TSTR &name);
//Matrix3 Mtx4x4ToMatrix3(aiMatrix4x4 &m, float scale = 1.0f);
cgltf_accessor* findAttrAccessor(cgltf_primitive *pr, const char *str);

extern Texmap *CreateColorMap(AColor &c);
extern Texmap *CreateBaseColorMap(void);
//extern void CorrectBitmapGamma(Texmap *pBmpTex, float gamma, BOOL custom = TRUE);
extern void SetPNGInfo(IBitmapIO_Png *pPNG_BmpIO, Bitmap *pBitmap);

extern Texmap *CreateSubtractOSLNode(Texmap *pTex);
extern Texmap* CreateColorScaleOSLNode(Texmap* pTex, float scale=1.0f);
extern Texmap *CreateCutOffOSLNode(Texmap *pTex, float value, Texmap* pAlphaTex=NULL);
//extern Texmap* CreateAlphaChOSLNode(AColor * pTex);
extern Texmap* CreateFlipNormalOSLNode(Texmap* pTex, BOOL FlipGreen, BOOL FlipRed);
extern Texmap *CreateMetalRoughOccOSLNode(Texmap *pTex, float value);
extern Texmap* CreateBitmapLookupOSLNode(const TSTR& fname);
extern Texmap* CreateUberBitmapOSLNode(const TSTR& fname);
extern Texmap* CreateSpecGlossFilterOSLNode(Texmap* pTex1, Texmap* pTex2);
extern Texmap* CreateAlphaChOSLNode(AColor col);
extern Texmap* CreateColorMultiplyOSLNode(Texmap* pTex, Color col);
extern Texmap* CreateFlowMapTransformOSLNode(Texmap* pTex);

extern void GetDracoMeshIndexList(cgltf_buffer_view* bufferView, std::vector<uint32_t> &tbl);
extern void DracoDecodeProc(cgltf_buffer_view* bufferView, cgltf_primitive* primitive, std::vector<float> &tbl, DracoDecodeType type, UINT AttrCh=0);

extern void OpenProgreessDlg(cgltf_data* m_glTF_data);
extern void CloseProgreessDlg(void);
extern void SetNodeImportStatus(int Count);
extern void SetMtlImportStatus(int Count);
extern void SetTexImportStatus(int Count);
extern void SetAnimImportStatus(int Count);
extern void SetSkinImportStatus(int Count);
extern void SetPhysicImportStatus(int Count);
extern int GetMtlType(void);
extern IParamBlock* GetParamBlock(Animatable* pAnim, int idx);

//extern int GetMapCh(cgltf_texture_view* texView);
//extern void SetMapCh(Texmap* pTex, int mapCh);
//extern void SetMapCh(Texmap* pTex, cgltf_texture_view* texView) { SetMapCh(pTex, GetMapCh(texView)); }
extern StdUVGen* GetUVGen(Texmap* pTex);

extern int GetOSLMapType(Texmap* pTex);
extern void SetNormal(Mesh* pMesh, const std::vector<Point3>& VertNormalTable);
extern Texmap* CreateAlphaFilterMap(Texmap* pTarget, AColor col=AColor(1,1,1,1));
extern void SetEnvironmentMap(const tstring& mapName);

//extern int LicenseTest(void);
extern void SetTextureOutputScale(Texmap* pTex, float scale);

extern tstring urlDecode(tstring str);

extern void SetCurrentAttrObj(FPValue& pObj);

//extern void InitWireConnectTable(void);
//extern void SetWireConnectTable(Mtl* pMtl, Texmap* pTex);
//extern void CreateWireConnect(void);

extern void LogOutput(const tstring& str, int pcs = 0);
extern BOOL ISFlatHierarchy(void);

//======================================================================
//======================================================================
class glTFImporter_Core
{
public:
	BOOL ImportPreProcess(const TCHAR* filename, BOOL suppressPrompts, int ver);
	BOOL ImportScene(BOOL suppressPrompts);

	BOOL GetDataList(std::vector<float>& data, cgltf_accessor *acc);
	void SetSparseData(std::vector<float>& retVal, cgltf_accessor *acc);

	void CreateNodeInfosRec(cgltf_node *node, INode *targetParent=NULL);
	INode* CreateMaxNode(cgltf_node *node, INode* pParent);
	INode* CreateCamera(cgltf_node *node);
	INode* CreateLight(cgltf_node *node);
	void AttachNodeExtensions(INode* pNode, cgltf_node* node);

	BOOL FindAnimationChannels(cgltf_node *node, cgltf_animation *animation, std::vector<size_t> &ChannelList);
	BOOL FindMtlAnimationChannels(cgltf_material* mtl, cgltf_animation* animation, std::vector<size_t>& ChannelList);

	//void GetAnimKeyFrameList(const gltf2::AnimationSampler &sampler, gltf2::TargetPath path);
	void GetPosAnimKeyFrameList(cgltf_animation_sampler *sampler, std::map<TimeValue, AnimKeyInfo> &PosKeyList);
	void GetRotAnimKeyFrameList(cgltf_animation_sampler *sampler, std::map<TimeValue, AnimKeyInfo> &RotKeyList);
	void GetSclAnimKeyFrameList(cgltf_animation_sampler *sampler, std::map<TimeValue, AnimKeyInfo> &SclKeyList);
	void GetWeightAnimKeyFrameList(cgltf_animation_sampler *sampler, std::map<TimeValue, std::vector<float> > &WeightKeyList, int weightCount);
	void GetClr3AnimKeyFrameList(cgltf_animation_sampler* sampler, std::map<TimeValue, AnimKeyInfo>& ClrKeyList);
	void GetClr4AnimKeyFrameList(cgltf_animation_sampler* sampler, std::map<TimeValue, AnimKeyInfo>& ClrKeyList);
	void GetFloatAnimKeyFrameList(cgltf_animation_sampler* sampler, std::map<TimeValue, AnimKeyInfo>& FloatKeyList);
	void GetPoint2AnimKeyFrameList(cgltf_animation_sampler* sampler, std::map<TimeValue, AnimKeyInfo>& Point2KeyList);

	void GetAnimatedNodeTable(void);
	void GetAnimatedNodeTableRec(INode *pNode, int animIdx);
	void SetAnimationRec(INode *pNode, int animIdx);
	void SetSkin(cgltf_node *node);
	void SetMorph(void);
	//void SetMorphAnimation(cgltf_node *node, MorphR3 *pMorph);
	void SetMorphWeightAnimation(INode *pNode, std::map<TimeValue, std::vector<float> > &WeightKeyList);
	void SetAnimationPointer(int anim);
	//void SetPropertyAnimation(int animID);
	int SetMorphChannelNameTable(cgltf_mesh* mesh, std::vector<tstring> &morphTargetTbl);

	Control* CreateFloatController(const std::map<TimeValue, AnimKeyInfo>& KeyList, float scale=1.0f);
	Control* CreateColorController(const std::map<TimeValue, AnimKeyInfo>& KeyList, cgltf_type type, Control *pOriginalC=NULL);
	void SetXYZController(Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start, const std::map<TimeValue, AnimKeyInfo>& KeyList, float scale = 1.0f);
	void SetBaseColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetNrmScaleController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetMetalScaleController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetRoughScaleController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetOccStrengthController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetAlphaCutOffController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetEmissiveStrengthController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetIORController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetTransmissionController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetEmissiveColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetUVScaleController(Mtl* pMtl, Control* pUC, Control* pVC, cgltf_interpolation_type InterpType, TimeValue start, TargetTex target);
	void SetUVOffsetController(Mtl* pMtl, Control* pUC, Control* pVC, cgltf_interpolation_type InterpType, TimeValue start, TargetTex target);
	void SetUVRotateController(Mtl* pMtl, Control* pRotWC, cgltf_interpolation_type InterpType, TimeValue start, TargetTex target);
	void SetVolumeThicknessController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetVolumeDistanceController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetVolumeColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetIridescenceFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetIridescenceIorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetIridescenceMaxController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetIridescenceMinController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetClearcoatFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetClearcoatRoughFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetSheenColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetSheenRoughFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetUVAnimation(Mtl* pMtl, cgltf_animation_sampler* sampler, TargetTex target);
	void SetDispersionController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetAnisotropyStrengthController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetAnisotropyRotationController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetDiffTransFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetDiffTransColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetSpecularFactorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetSpecularColorController(Mtl* pMtl, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);

	void SetCamPZnearController(GenCamera* pCamera, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetCamPZfarController(GenCamera* pCamera, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetCamPYfovController(GenCamera* pCamera, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetCamOYmagController(GenCamera* pCamera, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetCamOXmagController(GenCamera* pCamera, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetCamOZnearController(GenCamera* pCamera, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetCamOZfarController(GenCamera* pCamera, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetLightIntensController(GenLight* pLight, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetLightRangeController(GenLight* pLight, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetLightColorController(GenLight* pLight, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetLightOutAngleController(GenLight* pLight, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);
	void SetLightInAngleController(GenLight* pLight, Control* pCtrl, cgltf_interpolation_type InterpType, TimeValue start);

	tstring CreateTextureFileName(cgltf_texture* tex, tstring &originalFname);
	void CreateTextureTable(void);
	void CreateScanlineMaterial(void);
	void CreatePhysicalMaterial(void);
	void CreatePBRMetalMaterial(void);
	Mtl* CreatePBRSpecGlossMtl(cgltf_material* mtl);
	void CreateglTFMaterial(void);
	void CreateArnoldMaterial(void);
	void CreateVRayMaterial(void);
	void CreateCoronaMaterial(void);
	void CreateUSDMaterial(void);
	void CreateOpenPBRMaterial(void);
	void CreateVRayExtAttr(Mtl* pMtl, const vrayExtStruct& vray, BOOL enabled=TRUE);
	void CreateSelectabilityAttr(INode* pNode, const SelectabilityStruct &str, BOOL enabled);
	void CreateHoverabilityAttr(INode* pNode, const HoverabilityStruct& str, BOOL enabled);
	void CreateVisibilityAttr(INode* pNode, const VisibilityStruct& str, BOOL enabled);
	DWORD CreateInteractivityAttr(ReferenceTarget* pRef, const InteractivityStruct& str, BOOL enabled);
	BOOL GetInteractivityPointerID(ReferenceTarget* pRef, DWORD& id);
	ReferenceTarget* GetAnimByUniqueID(DWORD id);
	INode* GetNodeByUniqueIDRec(INode* pNode, DWORD id);
	BOOL RemoveInteractivityAttr(ReferenceTarget *pRef);

	BitmapTex *GetBitmapTexFromglTexture(cgltf_texture *tex);
	Point2 SetTextureUVoffset(Texmap *pBmpTex, cgltf_texture_view *textview);
	void RescaleUVOffset(Mtl *pMtl, Box2D &size);

	BitmapTex* SplitOcclusionTexture(BitmapTex *pOrgTexBmp);
	BitmapTex* SplitRoughnessTexture(BitmapTex *pOrgTexBmp);
	BitmapTex* SplitMetalnessTexture(BitmapTex *pOrgTexBmp);
	Texmap* SetMetalRoughOccOSLMap(BitmapTex *pOrgTexBmp, Texmap **pMetalTex, Texmap **pRoughTex, Texmap **pOccTex, BOOL Metallic, BOOL Roughness, BOOL Occlusion, cgltf_texture_view* texview);
	Texmap* SetMetalRoughOccClrCorrectMap(Texmap* pOrgTexBmp, Texmap** pMetalTex, Texmap** pRoughTex, Texmap** pOccTex, BOOL Metallic, BOOL Roughness, BOOL Occlusion);
	Texmap* SetAlphaClrCorrectMap(Texmap* pOrgTexBmp, Texmap** pAlphaTex);

	void CreateParamTableFromExtras(cgltf_extras& extras, cgltf_size size, std::vector<custAttrParam>& attrTbl, BOOL FileAttFlae = TRUE);
	void CreateTargetListFromExtras(cgltf_extras& extras, cgltf_size size, std::vector<tstring>& tbl);
	Class_ID AttachCustAttr(Animatable* pAnim, std::vector<custAttrParam>& attrTbl, tstring name=_T(""));
	void AttachAlphaModeCustAttr(Mtl* pMtl, int alphamode);
	void SetUserPropParam(INode *pNode, std::vector<custAttrParam>& attrTbl);

	void CreateUnlitAttr(Mtl* pMtl, BOOL unlit);
	void CreateIridescenceAttr(Mtl* pMtl, cgltf_iridescence* iridescence, BOOL enabled);
	void CreateIORAttr(Mtl* pMtl, cgltf_ior* ior, BOOL enabled);
	void CreateVolumeAttr(Mtl* pMtl, cgltf_volume* volume, BOOL enabled);
	void CreateTransmissionAttr(Mtl* pMtl, cgltf_transmission* transmission, BOOL enabled);
	void CreateSheenAttr(Mtl* pMtl, cgltf_sheen* sheen, BOOL enabled);
	void CreateClearcoatAttr(Mtl* pMtl, cgltf_clearcoat* clearcoat, BOOL enabled);
	void CreateEmissiveStrengthAttr(Mtl* pMtl, cgltf_emissive_strength* strength, BOOL enabled);
	void CreateDispersionAttr(Mtl* pMtl, cgltf_dispersion* dispersion, BOOL enabled);
	void CreateAnisotropyAttr(Mtl* pMtl, cgltf_anisotropy* anisotropy, BOOL enabled);
	void CreateDiffuseTransmissionAttr(Mtl* pMtl, cgltf_diffuse_transmission* diffuse_transmission, BOOL enabled);
	void CreateSpecularAttr(Mtl* pMtl, cgltf_specular* specular, BOOL enabled);
	void CreateWebpEncodingAttr(Texmap* pTex, const tstring& path, BOOL enabled);
	void CreateKTX2EncodingAttr(Texmap* pTex, const tstring& path, BOOL enabled);

	Texmap* BitmapTexToVRayBitmap(BitmapTex* pBmpTex, float gamma = 0.0f);
	Texmap* BitmapTexToCoronaBitmap(BitmapTex* pBmpTex, float gamma = 0.0f);

	BOOL KTX2ImageCreater(const tstring &ktxname, tstring& retname);
	cgltf_texture *GetglTFTexByTexmap(Texmap* pTex);

	int GetCustAttrPBlock(ReferenceTarget* pRef, const tstring& AttName, IParamBlock2* &pBlock);

	void SetSceneProperties(void);
	void AttachSceneProp(std::vector<custAttrParam>& attrTbl);
	void SetSceneInfos(void);
	void CreateRigidTable(INode *pNode, char *data);
	void SetRigidModefiers(void);

	BOOL gltfx_reference(const TCHAR* filename);

	BOOL WebpDecode(const tstring& fname, tstring& retname);

	cgltf_data* m_glTF_data;
	float m_scale;
	float m_TimeScale;
	TimeValue m_time;
	TimeValue m_StartTime;
	TimeValue m_LastTime;
	BOOL m_EmbedFormat;
	std::filesystem::path m_fullpath;
	tstring m_SourceImageFolder;
	tstring m_WorkImageFolder;

	std::map<cgltf_node*, INode*> m_NodeMap;
	std::map<cgltf_mesh*, INode*> m_MeshNodeMap;
	std::map<cgltf_material*, Mtl*> m_MaterialMap;
	std::map<cgltf_texture*, BitmapTex*> m_TextureMap;
	std::map<Texmap*, cgltf_texture_view*> m_TextureViewMap;
	std::map<cgltf_camera*, GenCamera*> m_CameraMap;
	std::map<cgltf_light*, GenLight*> m_LightMap;
	std::vector<cgltf_node*> m_MorphTable;
	std::vector<std::string> m_VariantTable;
	INodeTab m_AnimationNodeTab;
	std::map<Class_ID, tstring> m_CustAttrMap;
	std::map<INode*, CollisionInfo> m_CollisionTable;

	int m_SceneChannel;
	BOOL m_SceneMode;
	int m_AnimChannel;
	BOOL m_MatchAnim;
	BOOL m_FlipNormalGrn;
	BOOL m_FlipNormalRed;
	BOOL m_CompositeMtl;
	BOOL m_ViewVertexColor;
	BOOL m_HideDummy;
	//BOOL m_NameObject;
	BOOL m_AvoidDupName;
	BOOL m_UniqueTexture;
	BOOL m_UseColorComposite;
	BOOL m_CorrectGamma;
	float m_GammaValue;
	int m_MapUnpackMode;
	BOOL m_LaunchScript;
	BOOL m_Instancing;
	BOOL m_EnableBkColor;
	Color m_BkColor;
	float m_LiteIntensityScale;
	BOOL m_ExtraToCustAttr;
	BOOL m_ExtraToUserProp;
	BOOL m_ScaleLightIntensity;
	BOOL m_UseQuatCtrl;
	BOOL m_Quantization;
	BOOL m_ColorManagement;

	IBitmapIO_Png *m_pPNG_BmpIO;

	cgltf_node* FindNodeByINode(INode *pNode) {
		for (const auto m : m_NodeMap) {
			if (m.second == pNode) return m.first;
		}
		return NULL;
	}
	cgltf_texture* findGlTexByTexmap(BitmapTex* targetTex) {
		for (const auto t : m_TextureMap) {
			if (t.second == targetTex) return t.first;
		}
		return NULL;
	}
	void SetTexMapTable(cgltf_texture* tex , BitmapTex* targetTex) {
		m_TextureMap[tex] = targetTex;
	}
	void CorrectBitmapGamma(BitmapTex*& pBmpTex, float gamma, BOOL custom=TRUE);

	//FPValue m_CurrentCustAttrObj = FPValue(TYPE_VALUE, &undefined);
	//FPValue GetCurrentAnim(void) {	return s_CurrentCustAttrObj; }
	FPValue GetImportGeomArray(void) {
		static INodeTab tab;
		tab.ZeroCount();
		for (auto n : m_NodeMap) {
			if (!n.second) continue;
			if (n.second->GetObjectRef()->SuperClassID() == GEOMOBJECT_CLASS_ID) {
				tab.AppendNode(n.second);
			}
		}
		return FPValue(TYPE_INODE_TAB_BV, tab);
	}


#ifdef EXT_PROP_ANIM
	void SetPropertyAnimation(int animID);
#endif
};