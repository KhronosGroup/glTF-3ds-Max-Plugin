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

#ifndef NOMINMAX
#define NOMINMAX
#endif

#pragma warning( disable : 4267 )


#include <3dsmaxsdk_preinclude.h>
#include <Max.h>
#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <maxtypes.h>
//SIMPLE TYPE
#include <Shlwapi.h>
#include <iEditNormals.h>
#include <MeshNormalSpec.h>
#include <include\MorpherApi.h>
#include <IGame\IGame.h>
#include <ilayermanager.h>
#include <ilayer.h>

#include <impexp.h>
#include <direct.h>
#include <commdlg.h>

#include <vector>
#include <map>
#include <array>
#include <filesystem>

#include <meshdelta.h>
#include <stdmat.h>
#include <modstack.h>
#include <iskin.h>
#include <simpobj.h>
#include <dummy.h>
#include <decomp.h>
#include <MaxOSLInterface.h>
#include <splshape.h>
#include <linshape.h>
#include <iCustAttribContainer.h>
#include <CustAttrib.h>
#include <XRef\iXrefMaterial.h>

#include <tiny_gltf.h>

#include "resource.h"
#include "MimeTypes.h"
#include "define.h"

//using namespace tinygltf;


#define KHR_GLTF_EXPORTER_VER (_T("2.01"))

#define KHRglTFExporter_CLASS_ID	Class_ID(0x38586030, 0x1b315b3e)
#define KHRglTF2Exporter_CLASS_ID	Class_ID(0x56337879, 0x5a732c12)

#define IPOS_CONTROL_CLASS_ID		Class_ID(0x118f7e02,0xffee238a)
#define multiClassID				Class_ID(MULTI_CLASS_ID,0)
#define bmptexClassID				Class_ID(BMTEX_CLASS_ID, 0)
#define CompositeMtlClassID			Class_ID(0x61dc0cd7, 0x13640af6)
#define CompositeTexClassID			Class_ID(0x280, 0x0)
#define NormalBumpMapClassID		Class_ID(0x243e22c6, 0x63f6a014)
#define ColorCorrectTexID			Class_ID(COLORCORRECTION_CLASS_ID, 0x0)
#define RGBMultiTexID				Class_ID(RGBMULT_CLASS_ID, 0x0)


#define ScanLineMtlID				Class_ID(0x2, 0x0)
#define PBRMetalMtlID				Class_ID(0xd00f1e00, 0xbe77e500)
#define PBRSpecGlossMtlID			Class_ID(0xd00f1e00, 0x1dbad33)
#define Arnold_StandardSufaceID		Class_ID(0x7e73161f, 0x62f74b4c)
#define VRayMaterialID				Class_ID(0x37bf3f2f, 0x7034695c)
#define glTFMaterialID				Class_ID(0x38420192, 0x45fe4e1b)
#define USDMaterialID				Class_ID(0x6afa4933, 0x4787f1c7)
#define Pencil4MaterialID			Class_ID(0x20160205, 0x81454de)
#define OpenPBRMaterialID			Class_ID(0xf1551e33, 0x37fb1337)

#define MixTexID					Class_ID(0x230, 0x0)
#define ShellMaterialID				Class_ID(0x255, 0x0)

//#define Arnold_StandardSufaceID	Class_ID(0x7e73161f, 0x62f74b4c)
#define ArnoldNormalMapID			Class_ID(0x7e73161f, 0x461fb78c)
#define ArnoldSwitchShaderID		Class_ID(0x7e73161f, 0xa844c228)
//#define VRayMaterialID			Class_ID(0x37bf3f2f, 0x7034695c)
#define VRayNormalMapID				Class_ID(0x71fa6e51, 0x72057c2f)
#define VRayBitmapID				Class_ID(0x6769144b, 0x2c1017d)
#define VRayCompTexID				Class_ID(0x20d9613e, 0x2af82b3c)
#define VRayColorID					Class_ID(0x58f82b74, 0x73b75d7f)
#define CoronaMaterialID			Class_ID(0x6912ab89, 0x87151720)
#define CoronaNormalMapID			Class_ID(0x2870ceaa, 0xcc18437c)
#define CoronaBitmapID				Class_ID(0xabba5784, 0x65484244)
#define CoronaMixID					Class_ID(0x6847286f, 0x3fffaab4)
#define CoronaColorID				Class_ID(0x68426dca, 0x372362d4)
#define CoronaSelectMtlID			Class_ID(0xc74e9f94, 0xc319f213)

#define OSL_UnSupport		0
#define OSL_BitmapLookUp	1
#define OSL_UberBitmap		2
#define OSL_CutOff			3
#define OSL_AlphaCh			4
#define OSL_ColorScale		5
#define OSL_ColorMultiply	6

#define KHR_FLT_MAX      3.402823466e+37F        // max value

#define Ignore_XRefScene	-2

#ifdef UNICODE
#define tstring std::wstring
#define to_tstring(a) std::to_wstring(a)
#else
#define tstring std::string
#define to_tstring(a) std::to_string(a)
#endif

inline const MCHAR* GetCustomAttrName(CustAttrib* p)
{
#if MAX_RELEASE >= 26000
	return p->GetName(FALSE);
#else
	return p->GetName();
#endif
}
#ifdef PHYSICALMATERIAL_CLASS_ID
#else
#define PHYSICALMATERIAL_CLASS_ID Class_ID(0x3d6b1cec, 0xdeadc001)
#endif

#define UV_ANIMATE_OFFSET	0x1
#define UV_ANIMATE_SCALE	0x2
#define UV_ANIMATE_ROTATE	0x4

#define InteractiveLayerName (_T("KHRInteractiveGraphLayer"))
#define InteractiveNodeName (_T("KHRInteractiveGraphNode"))

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

//==========================================================
//==========================================================
struct AnimKeyInfo {
	static constexpr int  LINEAR = 0;
	static constexpr int  CUBICSPLINE = 1;
	static constexpr int  STEP = 2;

	Point3 val;
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

//==========================================================
//==========================================================
struct AnimationStruct {
	UINT samplerIdx;
	tstring pointerStr;
};

//==========================================================
//==========================================================
struct TransmissionStruct {
	Texmap* pTex = nullptr;
	float factor = 1.0f;
};

//==========================================================
//==========================================================
struct SpecularStruct {
	float factor = 0.0f;		// : float
	Texmap *pMap = nullptr;		// : texturemap
	Color color = Color(0.0f, 0.0f, 0.0f);	// : color
	Texmap *pColMap = nullptr;	// : texturemap
};

//==========================================================
//==========================================================
struct VolumeStruct {
	float thickness = 0.0f; 			// : float
	Texmap *pThicknessMap = nullptr;	// : texturemap
	float distance = 0.0f; 				// : float
	Color color = Color(0.0f, 0.0f, 0.0f);	// : color
};

//==========================================================
//==========================================================
struct SheenStruct {
	Color color = Color(0.0f, 0.0f, 0.0f);; 			// color
	Texmap *pColMap = nullptr;		// texturemap
	float roughness = 0.0f; 		// float
	Texmap *pRoughnessMap = nullptr; // texturemap
};

//==========================================================
//==========================================================
struct ClearCoatStruct {
	float factor = 0.0f; 			//: float
	Texmap *pMap = nullptr; 			//: texturemap
	float roughness = 0.0f; 		//: float
	Texmap *pRoughnessMap = nullptr; 	//: texturemap
	float normalValue = 0.0f;  	//: float
	Texmap *pNormalMap = nullptr; 	//: texturemap
};

//==========================================================
//==========================================================
struct UnlitStruct{
	BOOL unlit = FALSE;
};

//==========================================================
//==========================================================
struct IORStruct {
	float ior = 0.0f;
};

//==========================================================
//==========================================================
struct IridescenceStruct {
	float factor = 0.0f;
	float ior = 0.0f;
	float minimum = 0.0f;
	float maximum = 0.0f;
	Texmap* texture = nullptr;
	Texmap* thicknessTexture = nullptr;
};

//==========================================================
//==========================================================
struct EmissiveStrengthStruct {
	float strength = 0.0f;
};

//==========================================================
//==========================================================
struct AnisotropyStruct {
	float strength = 0.0f;
	float rotation = 0.0f;
	Texmap* texture = nullptr;	//: texturemap
};

//==========================================================
//==========================================================
struct DispersionStruct {
	float dispersion = 0.0f;
};

//==========================================================
//==========================================================
struct DiffuseTransmissionStruct {
	Texmap* TransmissionColorTexture = nullptr;
	Texmap* TransmissionTexture = nullptr;
	float TransmissionFactor = 0.0f;
	Color TransmissionColor = Color(0.0f, 0.0f, 0.0f);
};

//==========================================================
//==========================================================
struct PhysicsMaterials {
	float staticFriction = 0.0f;
	float dynamicFriction = 0.0f;
	float restitution = 0.0f;
	std::string restitutionCombine;
};

//==========================================================
//==========================================================
struct CollisionShapesStruct {
	int type = 0;
	INode* node = nullptr;
	float param1 = 0.0f;
	float param2 = 0.0f;
	float param3 = 0.0f;
	float param4 = 0.0f;
	float param5 = 0.0f;
};

//==========================================================
//==========================================================
struct WebpTextureStruct {
	tstring originalPathStr;
	float QualityFactor = 0.0f;
	BOOL LossLess = FALSE;
};

//==========================================================
//==========================================================
struct KTX2TextureStruct {
	tstring originalPathStr;
	int compression = 0;    // 1-5 (Default:4
	int quality = 0;		// 1-255 Defailt:128
	BOOL mipmap = FALSE;

	BOOL isSRGB = FALSE;    // TRUE: VK_FORMAT_R8G8B8A8_SRGB / FALSE: _UNORM
	BOOL useUASTC = FALSE;  // TRUE: Hi reso(UASTC/166) / FALSE: Hi comp(ETC1S/163)
};

//==========================================================
//==========================================================
struct VisibilityStruct {
	BOOL visible = TRUE;
};

//==========================================================
//==========================================================
struct SelectabilityStruct {
	BOOL selectable = TRUE;
};

//==========================================================
//==========================================================
struct HoverabilityStruct {
	BOOL hoverable = TRUE;
};

//==========================================================
//==========================================================
struct MaterialBumpStruct {
	Texmap* bumpTexture = nullptr;
	float bumpFactor = 0.0f;
};

//==========================================================
//==========================================================
struct vrayExtStruct {
	float roughness = 0.0f;
};

//==========================================================
//==========================================================
struct InteractivityStruct {
	DWORD id = 0;//GetTickCount
};


//==========================================================
// Vertex attribute information
//==========================================================
struct VertexProp {
	int faceID = 0;
	int corner = 0;
	int originalIdx = 0;
	int nrmID = 0;
	Point3 normal;
	int uv1 = 0;
	int uv2 = 0;
	int vc = 0;
};

//==========================================================
// Vertex attribute flags
//==========================================================
struct vertPropFlag {
	BOOL VColorUsed = FALSE;
	BOOL mapCh1Used = FALSE;
	BOOL mapCh2Used = FALSE;
	IGameMesh* pGameMesh = nullptr;
};

//==========================================================
// For Quantization of mesh and uvmap
//==========================================================
struct QuantizationInfo {
	Point3 meshSize;
	float meshScale;
	Point2 uvmap1Offset;
	float uvmap1Scale;
	Point2 uvmap2Offset;
	float uvmap2Scale;

	QuantizationInfo() {
		//meshOffset = Point3(0.0f, 0.0f, 0.0f);
		meshScale = 1.0f;
		uvmap1Offset = Point2(0.0f, 0.0f);
		uvmap1Scale = 1.0f;
		uvmap2Offset = Point2(0.0f, 0.0f);
		uvmap2Scale = 1.0f;
	}
};

inline const Matrix3 YupTM(Point3(1, 0, 0), Point3(0, 0, -1), Point3(0, 1, 0), Point3(0, 0, 0));
//static Matrix3 YupTM(Point3(-1, 0, 0), Point3(0, 0, 1), Point3(0, 1, 0), Point3(0, 0, 0));


extern HINSTANCE hInstance;
extern BOOL exportSelected;

extern TCHAR *GetString(int id);
extern BOOL IsGeometryObject(INode* pNode, TimeValue t = 0);
extern TriObject* GetTriObjectFromNode(INode *pNode, TimeValue t, int &deleteIt);
//extern SplineShape* GetShapeObjectFromNode(INode* pNode, TimeValue t, int& deleteIt);
extern LinearShape* GetShapeObjectFromNode(INode* pNode, TimeValue t, int& deleteIt);
extern void Matrix3ToFloat(Matrix3 &m, std::vector<double> &f, float scale);
extern std::wstring StringToWString(const char *oString);
extern std::string WStringToString(std::wstring oWString);
extern int FindModifier(INode* pNode, const Class_ID &CID, Modifier **pMod);
extern tstring GetURILFromFile(std::string &fname);
extern int base64_encode(const unsigned char *data, int size, std::string &ret);

extern int GetOSLMapType(Texmap* pTex);
extern BOOL GetCutOffValue(Texmap *pCutOffTex, Texmap* &pRetTex, float &val);
//extern Texmap *GetColorCorrectBaseMap(Texmap *pTex);
extern Texmap* GetBitmapTextureRec(Texmap* pTex);

extern int AnalyzeAttrBuf(TSTR &attrBuf, std::map<tstring, tstring> &attrMap);

extern IParamBlock* GetParamBlock(Animatable* pAnim, int idx);

extern float GetW(const Point3& normal, const Point3& tangent, const Point3& bitangent);;

//extern void CreateDracoMesh(void);

extern void LogOutput(const std::wstring& str, int pcs=0);


extern void GetMeshInfoXX(Mesh* pMesh);
extern BOOL GetTangentTM(Mtl* pMtl, Matrix3& mtx);
//extern BitmapTex* GetBitmapTexFromName(const TSTR& name);
extern const tstring ExportFolder(void);
extern tstring TextureTableCountStr(void);
extern BitmapTex* CreateBitmapTex(const tstring& texFilePath, Texmap* pTex, const IPoint2& size);
extern IPoint2 GetBitmapSize(void);
extern double truncateDecimal(float value);
extern int GetMirroredNode(INodeTab& tbl);
extern Mtl* GetSubMtlFromShellMtl(Mtl* pMtl);

//======================================================================
//======================================================================
class glTFExporter_Core
{
public:
	BOOL ExportPreProcess(const TCHAR* filename, BOOL suppressPrompts, int ver);
	void ExportScene(int ver);
	void CreateMtlIDTable(Mesh *pMesh, Mtl *pMtl, std::map<int, std::vector<int> > &mtlIDMap);
	void GetFulFrameAnimation(INode* pNode, Tab<TimeValue>& PosFrameList, Tab<TimeValue>& RotFrameList, Tab<TimeValue>& SclFrameList);
	void GetFullFrameAnimationColor(Control* pC, std::list<TimeValue>& FrameList);
	void GetFullFrameAnimationInt(Control* pC, std::list<TimeValue>& FrameList);
	void GetFullFrameAnimationFloat(Control* pC, std::list<TimeValue>& FrameList);

	void CreateSceneData(tinygltf::Scene &scene, int XRefIdx=-1, ILayer *pLayer=NULL);
	tinygltf::Node CreateNodeDataRec(INode *pNode, BOOL recursive=TRUE);
	void CreateMeshData(INode* pNode, tinygltf::Node& node);
	void ExCreateMeshData(INode* pNode, tinygltf::Node& node);
	void CreateShapeData(INode* pNode, tinygltf::Node& node);
	void CreateShapePositionData(tinygltf::Primitive& primitive, std::vector<Point3>& vertTable);
	//void CreateMorphPrimiteve(tinygltf::Primitive& primitive, Mesh* pMesh, Modifier* pMorphMod, std::vector<int>& faceIDTable);

	void CreateMaterialMap(BOOL exportSelected);
	void CreateMaterialMapRec(MtlBase *pMtl, BOOL VariantPart=FALSE);
	BOOL CreateBaseColorTexture(tinygltf::Material &material, Texmap *pTex);
	BOOL CreateNormalTexture(tinygltf::Material &material, Texmap *pTex, float scale = 1.0f);
	BOOL CreateEmitTexture(tinygltf::Material &material, Texmap *pTex);
	BOOL CreateEmitStrength(tinygltf::Material &material, float lum, BOOL animated=FALSE);
	BOOL CreateOcclusionTexture(tinygltf::Material &material, Texmap *pTex, float strength=1.0);
	BOOL CreateOpacityTexture(tinygltf::Material &material, Texmap *pTex);
	BOOL CreateMetalRoughTexture(tinygltf::Material &material, Texmap *pTex1, Texmap *pTex2, int *mapCh1, Texmap *pTex3=NULL, int *mapCh2=NULL);
	BOOL CreateTransmissionTexture(tinygltf::Material& material, const TransmissionStruct &str, BOOL animated);
	BOOL CreateSpecularTexture(tinygltf::Material& material, const SpecularStruct &str, BOOL animated);
	BOOL CreateVolumeTexture(tinygltf::Material& material, const VolumeStruct &str, BOOL animated);
	BOOL CreateSheenTexture(tinygltf::Material& material, const SheenStruct &str, BOOL animated);
	BOOL CreateClearCoatTexture(tinygltf::Material& material, const ClearCoatStruct &str, BOOL animated);
	BOOL CreateUnlitTexture(tinygltf::Material& material, const UnlitStruct& str, BOOL animated);
	BOOL CreateIORTexture(tinygltf::Material& material, const IORStruct& str, BOOL animated);
	BOOL CreateIridescenceTexture(tinygltf::Material& material, const IridescenceStruct& str, BOOL animated);
	BOOL CreateEmissiveStrengthTexture(tinygltf::Material& material, const EmissiveStrengthStruct& str, BOOL animated);
	BOOL CreateDispersionTexture(tinygltf::Material& material, const DispersionStruct& str, BOOL animated);
	BOOL CreateAnisotropyTexture(tinygltf::Material& material, const AnisotropyStruct& str, BOOL animated);
	BOOL CreateDiffuseTransmissionTexture(tinygltf::Material& material, const DiffuseTransmissionStruct& str, BOOL animated);
	BOOL CreateMaterialBumpTexture(tinygltf::Material& material, const MaterialBumpStruct& str, BOOL animated);

	BOOL CreateTextureTransformBlock(tinygltf::ExtensionMap& extension, Texmap* pTex, BOOL extent = TRUE);
	BOOL CreateTextureTransformBlockEx(tinygltf::Value::Object& object, Texmap* pTex, BOOL extent = TRUE);
	BOOL CreateTextureTransformBlockWithOSL(tinygltf::Value::Object& object, Texmap* pTex, BOOL extent=TRUE);

	void PBRMaterial(MtlBase* pMtl, tinygltf::Material& material);
	void PBRSpecGlossMaterial(MtlBase* pMtl, tinygltf::Material& material);
	void OpenPBRMaterial(MtlBase* pMtl, tinygltf::Material& material);
	void PhysicalMaterial(MtlBase *pMtl, tinygltf::Material &material);
	void StdMaterial(MtlBase *pMtl, tinygltf::Material &material);
	void glTFMaterial(MtlBase *pMtl, tinygltf::Material &material);
	void ArnoldMaterial(MtlBase *pMtl, tinygltf::Material &material);
	void VRayMaterial(MtlBase *pMtl, tinygltf::Material &material);
	void CoronaMaterial(MtlBase *pMtl, tinygltf::Material &material);
	void USDMaterial(MtlBase *pMtl, tinygltf::Material &material);

	void CreateKeyFrameList(Control* pCtrl, Tab<TimeValue>& KeyFrameList, BOOL Clear=TRUE);
	void CreateKeyFrameList(Control* pCtrl, std::list<TimeValue>& KeyFrameList, BOOL Clear=TRUE);
	void CreateAnimation(void);
	void CreateAnimationRec(INode *pNode);
	void CreateAnimationPointer(void);
	void SetFloatAnimation(std::list<TimeValue>& KeyFrameList, Control* pC, std::string& name, float scale = 1.0f);
	void SetVec2Animation(std::list<TimeValue>& KeyFrameList, Control* pC1, Control* pC2, std::string &name);
	void SetColorAnimation(std::list<TimeValue>& KeyFrameList, Control* pC, std::string& name, BOOL alpha=TRUE);
	inline int GetMeshIdFromNode(INode* pNode) {
		return pNode->GetObjectRef()? m_MeshMap[pNode->GetObjectRef()]:NULL;
	}

	void CreateSkin(INode *pNode, Modifier *pSkinMod);
	int GetRootNodeBoneID(ISkin* pISkin);
	tinygltf::Camera CreateCamera(INode *pNode);
	tinygltf::Light CreateLight(INode *pNode);
	UINT CreateImage(const TCHAR *uri);
	void CreateImageBuffer(void);
	BOOL IsVariantMtl(MtlBase *pMtl, int& cnt) const;
	Texmap* ReplaceAlphamap(Texmap* pTex, Texmap* pAlphaMap) {	return pAlphaMap;}

	BOOL HasMorphModifier(INode* pNode);
	Control* GetMorphCtroller(Modifier* pMod, int idx);
	void CreateMorph(INode *pNode, Modifier *pMorphMod);
	void CreateMorphTable(void);
	void SetMorphTagetList(tinygltf::Mesh& mesh, Modifier* pMod);
	void CreateMorphTableRec(INode *pNode);
	void SetMorphWeight(Modifier* pMorphMod, std::vector<double>& weights);
	int GetMorphTargetNum(Modifier* pMod);
	void CreateMorphVertMapTable(Modifier* pMorphMod, Mesh* pBaseMesh, MeshNormalSpec* pBaseNrmSpec, std::vector<std::map<int, Point3> >& morphNormalMapList);
	//Mesh* GetMorphTargetMesh(Modifier* pMod, int idx);
	void SetMorphTargetPositionTable(MaxMorphChannel& mc, const std::vector<int>& faceIDTable, Mesh* pMesh, std::vector<Point3> &targetPtTbl);
	void SetMorphTargetNormalTable(const std::vector<int>& faceIDTable, std::map<int, Point3>& morphNormalMap, std::vector<Point3>& targetNrmTbl);
	//void SetMorphTargetNormalTable(MaxMorphChannel& mc, const std::vector<int>& faceIDTable, Mesh* pMesh, MeshNormalSpec* pNrmSpec, std::vector<Point3>& targetNrmTbl);

	UINT IsUVAnimated(Texmap* pTex);
	BOOL CreateUVAnimation(Texmap* pTex, UINT mtlIdx, TargetTex target);
	BOOL CreateBaseColorAnimation(Control* pC, UINT mtlIdx, BOOL alpha);
	BOOL CreateAlphaCutOffAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateTransmissionAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateIORAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateMetalicFactorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateRoughnessFactorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateNormalScaleAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateOcclusionStrengthAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateEmissiveStrengthAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateEmissiveFactorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateThicknessFactorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateAttenuationDistanceAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateAttenuationColorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateIridescenceFactorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateIridescenceIorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateIridescenceThicknessMinAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateIridescenceThicknessMaxAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateSheenColorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateSheenRoughnessAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateClearcoatFactorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateClearcoatRoughnessAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateAnisotropyStrengthAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateAnisotropyRotationAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateDispersionAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateDiffTransFactorAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateDiffTransColorAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateSpecularFactorAnimation(Control* pC, UINT mtlIdx);
	BOOL CreateSpecularColorAnimation(Control* pC, UINT mtlIdx);

	BOOL CreateLightColorAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateLightIntensAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateLightRangeAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateLightOutAngleAnimation(Control* pC, UINT nodeIdx);
	BOOL CreateLightInAngleAnimation(Control* pC, UINT nodeIdx);

	BOOL CreateVisibilityNode(tinygltf::Node& node, const VisibilityStruct& str, BOOL animated);
	BOOL CreateSelectabilityNode(tinygltf::Node& node, const SelectabilityStruct& str, BOOL animated);
	BOOL CreateHoverabilityNode(tinygltf::Node& node, const HoverabilityStruct& str, BOOL animated);

	IParamBlock2* GetCustAttrPBlock(ReferenceTarget* pRef, tstring& AttName);

	void CreateDracoMeshProp(tinygltf::Primitive& primitive, Mesh* pMesh, MeshNormalSpec* pNrmSpec, std::vector<int>& faceIDTable, std::vector<VertexProp>& VertPropTable, std::map<int, int>& vertPropMap, vertPropFlag& flag, Mtl* pMtl, ISkinContextData* pSkinMC, Modifier* pMorphMod, const Matrix3 &OffsetTM);
	void CreateDracoMorphPrimitive(tinygltf::Primitive& primitive, Modifier* pMorphMod, std::vector<VertexProp>& VertPropTable);
	//void SetDracoMorphTargetPositionTable(Modifier* pMod, int chID, const std::vector<int>& faceIDTable, draco::Mesh* pMesh, std::vector<int> dracovIDTable, std::vector<Point3>& targetPtTbl);
	//void ExCreateDracoMeshProp(tinygltf::Primitive& primitive, Mesh* pMesh, MeshNormalSpec* pNrmSpec, std::vector<VertexProp>& vertPropTable, vertPropFlag &flag, Mtl* pMtl, ISkinContextData* pSkinMC, Modifier* pMorphMod, IGameMesh* pGameMesh, const Matrix3& OffsetTM);

	void SetSceneExtensions(void);

	void *SecureMemory(int size);
	void FreeSceneData(void);

	void SetName(void *ptr, const tstring &str) {
		if (str.size()==0)
			m_nameTable.insert(make_pair(ptr, std::string("NoName")));
		else {
			std::string s = WStringToString(str);
			m_nameTable.insert(make_pair(ptr, s));
		}
	}

	inline UINT findNodeIndex(INode *pNode) { return m_NodeMap[pNode]; }

	int findMaterialIndex(Mtl *pMtl) {
		if (m_MaterialMap.count(pMtl) > 0) {
			for (auto it : m_MaterialMap) {
				//if (it.first->ClassID() == XREFMATERIAL_CLASS_ID) {
				//	IXRefMaterial* pXrefMtl = IXRefMaterial::GetInterface(*it.first);
				//	if (pMtl == pXrefMtl->GetSourceMaterial()) return it.second;
				//}
				if (it.first == pMtl) return it.second;
			}
		}
		else {
			CreateMaterialMapRec(pMtl);
			for (auto it : m_MaterialMap) {
				if (it.first == pMtl) return it.second;
			}
		}
		return -1;
	}

	StdUVGen* GetUVGen(Texmap *pTex) {
		if (!pTex) return NULL;

		StdUVGen* pUVGen = NULL;
		if (pTex->ClassID() == bmptexClassID) {
			pUVGen = ((BitmapTex*)pTex)->GetUVGen();
		}
		else if (pTex->ClassID() == VRayBitmapID) {
			for (int i = 0; i < pTex->NumSubs(); i++) {
				Animatable* pAnim = pTex->SubAnim(i);
				if (pAnim->ClassID() == Class_ID(0x100, 0)) return (StdUVGen*)pAnim;
			}
		}
		else if (pTex->ClassID() == CoronaBitmapID) {
			for (int i = 0; i < pTex->NumSubs(); i++) {
				Animatable* pAnim = pTex->SubAnim(i);
				if (pAnim->ClassID() == Class_ID(0x100, 0)) return (StdUVGen*)pAnim;
			}
		}
		UVGen *p = pTex->GetTheUVGen();
		if (p) {
			return (StdUVGen*)p;
		}
		return pUVGen;
	}

	int findTextureIndex(Texmap* pTex, const TSTR& fname, BOOL KTX2isRGB, Texmap *pOSLMap = NULL);
	int SetSampler(Texmap* pTex);

	void SetCustomAttribute(tinygltf::Value::Object& params, ICustAttribContainer* pContainer);
	void SetUserPropString(tinygltf::Value::Object& params, TSTR &str);

	BOOL SetIORParams(MtlBase* pMtl, IORStruct& str, BOOL &animated);
	BOOL SetUnlitParams(MtlBase* pMtl, UnlitStruct& str);
	BOOL SetIridescenceParams(MtlBase* pMtl, IridescenceStruct &str, BOOL& animated);
	BOOL SetVolumeParams(MtlBase* pMtl, VolumeStruct& str, BOOL& animated);
	BOOL SetTransmissionParams(MtlBase* pMtl, TransmissionStruct& str, BOOL& animated);
	BOOL SetSheenParams(MtlBase* pMtl, SheenStruct& str, BOOL& animated);
	BOOL SetClearCoatParams(MtlBase* pMtl, ClearCoatStruct& str, BOOL& animated);
	BOOL SetEmissiveStrengthParams(MtlBase* pMtl, EmissiveStrengthStruct& str, BOOL& animated);
	BOOL SetDispersionParams(MtlBase* pMtl, DispersionStruct& str, BOOL& animated);
	BOOL SetAnisotropyParams(MtlBase* pMtl, AnisotropyStruct& str, BOOL& animated);
	BOOL SetSpecularParams(MtlBase* pMtl, SpecularStruct& str, BOOL& animated);
	BOOL SetDiffuseTransmissionParams(MtlBase* pMtl, DiffuseTransmissionStruct& str, BOOL& animated);
	BOOL SetVRayExtParams(MtlBase* pMtl, vrayExtStruct& str);
	BOOL SetWebpTextureParams(MtlBase* pTex, WebpTextureStruct& str);
	BOOL SetKTX2TextureParams(MtlBase* pTex, KTX2TextureStruct& str);

	BOOL SetVisibilityParams(INode* pNode, VisibilityStruct& str);
	BOOL SetSelectabilityParams(INode* pNode, SelectabilityStruct& str);
	BOOL SetHoverabilityParams(INode* pNode, HoverabilityStruct& str);
	BOOL SetInteractivityParams(ReferenceTarget* pRef, InteractivityStruct& str);

	void CreateWireColorMtlMap(INode* pNode);

	std::string GetAlphaMode(MtlBase* pMtl, const std::string& defMode="OPAQUE");
	BOOL CheckIfTextureIsUsed(MtlBase* pMtl);
	BOOL MapUsed(Mtl* pMtl);

	void SetSceneExtras(tinygltf::Scene& Scene);
	tstring GetCompanyString(void);
	void ImageSetting(HWND hWnd, int type);

	BOOL WebpEncode(Texmap* pTex, WebpTextureStruct &str);
	BOOL KTX2Encode(Texmap* pTex, KTX2TextureStruct& str);

	TimeValue m_time;
	float m_TimeScale;
	std::filesystem::path m_fullpath;
	inline const tstring ExportFolder(void) const { return m_fullpath.parent_path(); }
	inline size_t TextureTableCount(void) { return m_TextureTable.size(); }

	BOOL IsInstanced(INode* pNode);
	void ExportGPUInstanceSection(tinygltf::Scene& scene);

	void CreateInteractiveLayerTable(void);
	void GetInteractivityNodeList(std::vector<std::string>& interactiveExtensionList);
	void CreateDeclarationExtensionInfo(INode* pNode, const std::string& str);
	void SetNodeTable(INode* pNode, tinygltf::Value::Array& nodes);
	void SetDeclarationsTypesTable(tinygltf::Value::Array& declarations, std::vector<std::string>& interactiveExtensionList);
	void SetVariableTable(INode* pNode, tinygltf::Value::Array& variables);
	void SetEventTable(INode* pNode, tinygltf::Value::Array& events);
	int ConvertNodeIndexToInt(TSTR& val);

	void PostProcess(const tstring& fullpath);

	IGameScene* m_pIGameScene;

	tinygltf::Model m_model;
	tinygltf::Animation m_animation;
	//tinygltf::Animation m_glTF_animation;

	int m_BufferByteOffset;
	void *m_glTf_Buffer;
	int m_DracoBufferViewIndex;

	std::map<void*, std::string> m_nameTable;
	std::map<void*, std::string> m_mimeTable;
	std::map<INode*, UINT> m_NodeMap;
	std::map<Object*, UINT> m_MeshMap;
	std::map<GenLight*, UINT> m_LightMap;
	std::map<GenLight*, UINT> m_LightIESMap;
	std::map<MtlBase*, UINT> m_MaterialMap;
	std::vector<tstring> m_imagePathTable;
	std::vector<Texmap*> m_TextureTable;
	//std::vector<tstring> m_WorkFileList;
	std::map<MtlBase*, std::vector<Mtl*> > m_VariantMtlMap;
	std::map<INode*, Modifier*> m_skinNodeTable;
	std::map<INode*, Modifier*> m_morphNodeTable;
	std::vector<INode*> m_morphTargetTable;
	std::vector<AnimationStruct> m_ParamAnimationTable;
	std::map <DWORD, Mtl*> m_WireColorMtlMap;
	std::map<Object*, std::vector<INode*> > m_GPUInstanceMap;
	std::vector<tinygltf::Node> m_GPUInstanceNodeList;
	std::vector<PhysicsMaterials> m_PhysicMtlTable;
	std::vector<CollisionShapesStruct> m_CollisionShapeTable;
	std::vector<INode*> m_InteractiveGraph;

	int CreateInstanceTranslationSection(std::vector<Point3>& pos);
	int CreateInstanceRotationSection(std::vector<Quat>& rot);
	int CreateInstanceScaleSection(std::vector<Point3>& scl);
	void CreateCollisionShape(INode* pNode, tinygltf::Node& node);
	void CreateMeshDataNode(const tinygltf::Node& basenode);

	BOOL CreateGLTFXFile(const tstring& filename);

	void CreateQuatizationMap(void);
	BOOL GetQuatizationInfo(ReferenceTarget* pRef, QuantizationInfo& info);

	float m_scale;
	BOOL m_CopyImage;
	int m_ExportFileType;
	BOOL m_ForceTRSMode;
	BOOL m_DracoCompress;
	BOOL m_Instancing;
	int m_ExportShapeObj;
	BOOL m_ExportAnimation;
	BOOL m_FullFrame;
	BOOL m_CubicSplineT;
	BOOL m_ExportTangent;
	BOOL m_ExportMorphNrm;
	int m_EncodeSpeed;
	BOOL m_WireClrToMtl;
	BOOL m_ExportUserProp;
	BOOL m_GPUInstance;
	BOOL m_Collision;
	BOOL m_Interactivity;
	BOOL m_AttachRigidInfo;
	BOOL m_ResetXFormMod;
	BOOL m_ResetPivotTM;

	int m_MROMapExportMode;
	int m_MROImageType;
	IPoint2 m_CreateBitmapSize;
	int m_MultiScene;
	BOOL m_ReferenceFileMode;
	int m_InteractiveGraphID;

	UINT CreateInstanceMeshWithMtl(INode* pNode);
	BOOL m_InstanceWithMtl;
	//ILayer *m_pInteractiveGraphLayer;
	std::vector<ILayer*> m_InteractiveLayerTable;

	BOOL m_MtlTransmission_Used;
	BOOL m_MtlVolume_Used;
	BOOL m_MtlSpecular_Used;
	BOOL m_MtlSheen_Used;
	BOOL m_MtlClearCoat_Used;
	BOOL m_MtlEmissive_Used;
	BOOL m_MtlIor_Used;
	BOOL m_MtlPbrSpcGls_Used;
	BOOL m_MtlTtranslucency_Used;
	BOOL m_MtlUnlit_Used;
	BOOL m_TexTransform_Used;
	BOOL m_MtlVariants_Used;
	BOOL m_MtlEmitStrength_Used;
	BOOL m_LlightsPunctual_Used;
	BOOL m_LlightsIES_Used;
	BOOL m_MtlIridescence_Used;
	BOOL m_AnimationPointer_Used;
	BOOL m_MtlAnisotropy_Used;
	BOOL m_MtlDiffuseTransmission_Used;
	BOOL m_MtlSSS_Used;
	BOOL m_Mesh_gpu_instancing_Used;
	BOOL m_collision_shapes_Used;
	BOOL m_Physic_RigidBody_Used;
	BOOL m_MtlDispersion_Used;
	BOOL m_TextureWebp_Used;
	BOOL m_MaterialBump_Used;
	BOOL m_Interactivity_Used;
	BOOL m_Visibility_Used;
	BOOL m_Selectability_Used;
	BOOL m_Hoverability_Used;
	BOOL m_TexBasisu_Used;
	BOOL m_Mesh_quantization_Used;

	BOOL m_IncorrectSkinDataFound;
};
