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
#include <iostream>
#include <json/json.h>

#pragma comment(lib, "jsoncpp_static.lib")

#define MASSFX_RIGID_BODY			Class_ID(0xa750e, 0x6e6ba)
#define HingeConstraintID			Class_ID(0x549039fe, 0x4ff74db6)


#define FX_rigid_type		0
#define FX_density			6
#define FX_mass				7
#define FX_staticFriction	8
#define FX_dynamicFriction	9
#define FX_bounciness	10

#define FX_meshtype	13

#define FX_meshRadius	31

#define FX_massCenterX	45
#define FX_massCenterY	46
#define FX_massCenterZ	47


struct CollisionShapeStruct {
    int type;
    int mesh;
    float param1;
    float param2;
    float param3;
    float param4;
};
struct PhysicdMtlStruct {
    float staticFriction;
    float dynamicFriction;
    float restitution;
    std::string restitutionCombine;
};
struct CollisionFiltertruct {
    int collisionSystems;
    int collideWithSystems;
};

struct PhysicsJointStruct {
    int LinearAngularMode;
    IPoint3 linearAxes;
    IPoint3 angularAxes;
    float min;
    float max;
    float stiffness;
    float damping;
};


static std::vector<CollisionShapeStruct> CollisionShapeTable;
static std::vector<PhysicdMtlStruct> PhysicdMtlTable;
static std::vector<CollisionFiltertruct> CollisionFilterTable;
static std::vector<INode*> gravityForceTable;

INode* GetGravityForceNode(float g)
{
    IParamBlock2* pBlock = NULL;
    for (auto pNode : gravityForceTable) {
        Object* pObj = pNode->GetObjectRef();
        pBlock = pObj->GetParamBlock(0);
        float f = pBlock->GetFloat(0, 0);
        if (f == g) return pNode;
    }

    Object* pObj = (Object*)GetCOREInterface()->CreateInstance(WSM_OBJECT_CLASS_ID, Class_ID(0xe523c, 0));
    pBlock = pObj->GetParamBlock(0);
    pBlock->SetValue(0, 0, g);
    INode* pNode = GetCOREInterface()->CreateObjectNode(pObj);
    gravityForceTable.push_back(pNode);

    return pNode;
}

//==========================================================
//==========================================================
void GetJSONRoot(char *data, Json::Value &root)
{
    Json::CharReaderBuilder builder;
    builder["collectComments"] = true;
    JSONCPP_STRING errs;
    auto reader = builder.newCharReader();
    reader->parse(data, data + strlen(data), &root, &errs);
}

//==========================================================
//==========================================================
BOOL FindRigidModefier(void)
{
    SubClassList* subList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(MATERIAL_CLASS_ID);
    int idx = subList->FindClass(MASSFX_RIGID_BODY);

    return (idx>=0);
}

//==========================================================
//==========================================================
void glTFImporter_Core::CreateRigidTable(INode* pNode, char* data)
{
    Json::Value root;
    GetJSONRoot(data, root);

    CollisionInfo info;
    info.rigidType = -1;
    info.mass = -1;
    info.shape = -1;
    info.physicsMaterial = -1;
    info.collisionFilter = -1;
    info.inertiaDiagonal = Point3();
    info.inertiaOrientation = Quat();
    info.mass = 1.0f;
    info.rigidType = 3;
    info.COM = 0;
    info.gravityNode = NULL;
    info.joint = -1;
    info.connectedNode = NULL;
    if (root.isMember("motion")) {
        auto v1 = root["motion"];
        if (v1.isMember("mass")) {
            info.mass = v1["mass"].asFloat();
            info.rigidType = 1;
        }
        if (v1.isMember("centerOfMass")) {
            info.COM = 2;
            info.COM_pt.x = v1["centerOfMass"][0].asFloat();
            info.COM_pt.y = v1["centerOfMass"][1].asFloat();
            info.COM_pt.z = v1["centerOfMass"][2].asFloat();
        }
        if (v1.isMember("gravityFactor")) {
            info.gravityNode = GetGravityForceNode(v1["gravityFactor"].asFloat());
        }
        if (v1.isMember("inertiaDiagonal")) {
            info.inertiaDiagonal.x = v1["inertiaDiagonal"][0].asFloat();
            info.inertiaDiagonal.y = v1["inertiaDiagonal"][1].asFloat();
            info.inertiaDiagonal.z = v1["inertiaDiagonal"][2].asFloat();
        }
        if (v1.isMember("inertiaOrientation")) {
            info.inertiaOrientation.x = v1["inertiaOrientation"][0].asFloat();
            info.inertiaOrientation.y = v1["inertiaOrientation"][1].asFloat();
            info.inertiaOrientation.z = v1["inertiaOrientation"][2].asFloat();
            info.inertiaOrientation.w = v1["inertiaOrientation"][3].asFloat();
        }
        if (v1.isMember("linearVelocity")) {
            info.linearVelocity.x = v1["linearVelocity"][0].asFloat();
            info.linearVelocity.y = v1["linearVelocity"][1].asFloat();
            info.linearVelocity.z = v1["linearVelocity"][2].asFloat();
        }
        else {
            info.linearVelocity.x = info.linearVelocity.y = info.linearVelocity.z = 0.0f;
        }
        if (v1.isMember("angularVelocity")) {
            info.angularVelocity.x = v1["angularVelocity"][0].asFloat();
            info.angularVelocity.y = v1["angularVelocity"][1].asFloat();
            info.angularVelocity.z = v1["angularVelocity"][2].asFloat();
        }
        else {
            info.angularVelocity.x = info.angularVelocity.y = info.angularVelocity.z = 0.0f;
        }
        if (v1.isMember("isKinematic")) {
        }
    }

    if (root.isMember("collider")) {
        auto v1 = root["collider"];
        if (v1.isMember("shape"))           info.shape = v1["shape"].asInt();
        if (v1.isMember("physicsMaterial")) info.physicsMaterial = v1["physicsMaterial"].asInt();
        if (v1.isMember("collisionFilter")) info.collisionFilter = v1["collisionFilter"].asInt();
        if (v1.isMember("geometry")) {
            auto v2 = v1["geometry"];
            if (v2.isMember("shape"))        info.shape = v2["shape"].asInt();
        }
    } else if (root.isMember("collider")) {
    }
    else {
    }

    if (root.isMember("joint")) {
        auto v1 = root["joint"];
        if (v1.isMember("joint"))           info.joint = v1["joint"].asInt();
        //if (v1.isMember("connectedNode")) info.connectedNode = v1["connectedNode"].asInt();
        if (v1.isMember("enableCollision")) info.enableCollision = v1["enableCollision"].asInt();
    }

    m_CollisionTable[pNode] = info;
}

//==========================================================
//==========================================================
void CreateCollisionShapeTable(char* data)
{
    Json::Value root;
    GetJSONRoot(data, root);

    auto shape = root["shapes"];
    for (auto s : shape) {
        CollisionShapeStruct csInfo;
        std::string type = s["type"].asString();
        if (type == "sphere") {
            csInfo.type = 1;
            csInfo.param1 = s["sphere"]["radius"].asFloat();
        }
        else if (type == "box") {
            csInfo.type = 2;
            csInfo.param1 = s["box"]["size"][0].asFloat();
            csInfo.param2 = s["box"]["size"][1].asFloat();
            csInfo.param3 = s["box"]["size"][2].asFloat();
        }
        else  if (type == "capsule") {
            csInfo.type = 4;
            csInfo.mesh = s["mesh"].asInt();
        }
        else  if (type == "cylinder") {
            csInfo.type = 4;
            csInfo.mesh = s["mesh"].asInt();
        }
        else  if (type == "convex") {
            csInfo.type = 4;
            csInfo.mesh = s["mesh"].asInt();
        }
        else  if (type == "concave") {
            csInfo.type = 7;
            csInfo.mesh = s["mesh"].asInt();
        }
        else if (type == "trimesh") {
            csInfo.type = 5;
            csInfo.mesh = s["mesh"].asInt();
        }

        CollisionShapeTable.push_back(csInfo);
    }
}

//==========================================================
//==========================================================
void CreatePhysicdMtlTableTable(char* data)
{
    Json::Value root;
    GetJSONRoot(data, root);


    auto pmList = root["physicsMaterials"];
    for (auto pm : pmList) {
        PhysicdMtlStruct fmInfo;
        fmInfo.staticFriction = pm["staticFriction"].asFloat();
        fmInfo.dynamicFriction = pm["dynamicFriction"].asFloat();
        fmInfo.restitution = pm["restitution"].asFloat();
        fmInfo.restitutionCombine = pm["restitutionCombine"].asString();

        PhysicdMtlTable.push_back(fmInfo);
    }

    auto csList = root["collisionFilters"];
    for (auto cs : csList) {
        CollisionFiltertruct csInfo;
        CollisionFilterTable.push_back(csInfo);
    }
}

//==========================================================
//==========================================================
void CreatePhysicsJointTable(char* data)
{
/*
    Json::Value root;
    GetJSONRoot(data, root);

    auto pjList = root["physicsJoints"];
    for (auto pj : pjList) {
        PhysicsJointStruct pjInfo;
        pjInfo.linearAxes = pj["linearAxes"].asInt();
        pjInfo.angularAxes = pj["angularAxes"].asInt();
        pjInfo.min = pj["min"].asFloat();
        pjInfo.max = pj["max"].asFloat();
        pjInfo.stiffness = pj["stiffness"].asFloat();
        pjInfo.damping = pj["damping"].asFloat();

        PhysicdMtlTable.push_back(pjInfo);
    }
*/
}
//==========================================================
//==========================================================
void glTFImporter_Core::SetRigidModefiers(void)
{
    if (m_CollisionTable.size() == 0) return;

    CollisionShapeTable.clear();
    PhysicdMtlTable.clear();
    CollisionFilterTable.clear();
    gravityForceTable.clear();

    cgltf_extension* ext = m_glTF_data->data_extensions;
    for (int cnt = 0; cnt < m_glTF_data->data_extensions_count; cnt++, ext++) {
        char* name = ext->name;
        char* data = ext->data;
        if (!_stricmp(name, "KHR_implicit_shapes"))
            CreateCollisionShapeTable(data);
        if (!_stricmp(name, "KHR_physics_rigid_bodies"))
            CreatePhysicdMtlTableTable(data);
    }

    for (auto tbl : m_CollisionTable)
    {
        INode* pNode = tbl.first;
        TSTR name = pNode->GetName();
        Point3 scl(1.0f, 1.0f, 1.0f);
        {
            Matrix3 tm = pNode->GetNodeTM(m_time);
            AffineParts parts;
            decomp_affine(tm, &parts);
            scl = parts.k;
        }
        BOOL DummyObject = FALSE;
        {
            Object* pObj = pNode->GetObjectRef();
            if (pObj->ClassID() == Class_ID(DUMMY_CLASS_ID, 0))
                DummyObject = TRUE;
        }
        CollisionInfo info = tbl.second;

#if 1
        TSTR ComStr;
        FPValue fpv;
        //ComStr = _T("global tempMod = MassFX_RBody ()");
        ComStr.printf(_T("global tempMod = MassFX_RBody mass:%f"), info.mass);
#if MAX_RELEASE >= 24000
		ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE, &fpv);
#else
		ExecuteMAXScriptScript(ComStr, TRUE, &fpv);
#endif
		/*
        Modifier* pMod = NULL;
        if (fpv.type == TYPE_REFTARG) {
            //Class_ID id = pv->get_max_class_id();
            pMod = (Modifier*)fpv.r;
            //pMod = pv->to_modifier();
        }
        */
        GetCOREInterface()->SelectNode(pNode);
#if MAX_RELEASE >= 24000
		ComStr = _T("addmodifier $ tempMod");
        ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE, &fpv);
        ComStr.printf(_T("tempMod.type = %d"), info.rigidType);
        ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
		ComStr = _T("addmodifier $ tempMod");
		ExecuteMAXScriptScript(ComStr, TRUE, &fpv);
		ComStr.printf(_T("tempMod.type = %d"), info.rigidType);
		ExecuteMAXScriptScript(ComStr, TRUE);
#endif
		
        if (info.shape < 0) {
#if MAX_RELEASE >= 24000
            if (DummyObject) {
                ComStr.printf(_T("tempMod.meshType = 1"));
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
                ComStr.printf(_T("tempMod.meshRadius = 0.001"));
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
            }
            else {
                ComStr.printf(_T("tempMod.meshType = 5"));
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
                ComStr.printf(_T("tempMod.meshCustomMesh = undefined"));
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
            }
#else
            ComStr.printf(_T("tempMod.meshType = 5"));
            ExecuteMAXScriptScript(ComStr, TRUE);
            ComStr.printf(_T("tempMod.meshCustomMesh = undefined"));
            ExecuteMAXScriptScript(ComStr, TRUE);
#endif
        } else {
            CollisionShapeStruct cs = CollisionShapeTable[info.shape];
            //ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].meshType = %d"), cs.type);
            ComStr.printf(_T("tempMod.meshType = %d"), cs.type);
#if MAX_RELEASE >= 24000
			ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
			ExecuteMAXScriptScript(ComStr, TRUE);
#endif
            if (cs.type == 1) {
                float r = cs.param1 * m_scale * scl.x;
                //ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].meshRadius = %f"), r);
                ComStr.printf(_T("tempMod.meshRadius = %f"), r);
#if MAX_RELEASE >= 24000
				ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
				ExecuteMAXScriptScript(ComStr, TRUE);
#endif
			}
            else if (cs.type == 2) {
                Point3 p(cs.param1, cs.param2, cs.param3);
                p *= scl;
                p *= m_scale;
#if MAX_RELEASE >= 24000
                //ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].meshLength = %f"), p.x);
                ComStr.printf(_T("tempMod.meshLength = %f"), p.x);
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
                //ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].meshWidth = %f"), p.y);
                ComStr.printf(_T("tempMod.meshWidth = %f"), p.y);
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
                //ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].meshHeight = %f"), p.z);
                ComStr.printf(_T("tempMod.meshHeight = %f"), p.z);
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
				//ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].meshLength = %f"), p.x);
				ComStr.printf(_T("tempMod.meshLength = %f"), p.x);
				ExecuteMAXScriptScript(ComStr, TRUE);
				//ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].meshWidth = %f"), p.y);
				ComStr.printf(_T("tempMod.meshWidth = %f"), p.y);
				ExecuteMAXScriptScript(ComStr, TRUE);
				//ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].meshHeight = %f"), p.z);
				ComStr.printf(_T("tempMod.meshHeight = %f"), p.z);
				ExecuteMAXScriptScript(ComStr, TRUE);
#endif
			}
            if (info.COM == 2) {
                Point3 p = info.COM_pt * scl;
                p *= m_scale;
#if MAX_RELEASE >= 24000
				ComStr.printf(_T("tempMod.MassCenterMode = %d"), info.COM);
				ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
				ComStr.printf(_T("tempMod.MassCenterX = %f"), p.x);
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
                ComStr.printf(_T("tempMod.MassCenterY = %f"), p.y);
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
                ComStr.printf(_T("tempMod.MassCenterZ = %f"), p.z);
                ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
				ComStr.printf(_T("tempMod.MassCenterMode = %d"), info.COM);
				ExecuteMAXScriptScript(ComStr, TRUE);
				ComStr.printf(_T("tempMod.MassCenterX = %f"), p.x);
				ExecuteMAXScriptScript(ComStr, TRUE);
				ComStr.printf(_T("tempMod.MassCenterY = %f"), p.y);
				ExecuteMAXScriptScript(ComStr, TRUE);
				ComStr.printf(_T("tempMod.MassCenterZ = %f"), p.z);
				ExecuteMAXScriptScript(ComStr, TRUE);
#endif
			}
        }

        if (info.physicsMaterial >= 0) {
            PhysicdMtlStruct pm = PhysicdMtlTable[info.physicsMaterial];
#if MAX_RELEASE >= 24000
            ComStr.printf(_T("tempMod.staticFriction = %f"), pm.staticFriction);
            ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
            ComStr.printf(_T("tempMod.dynamicFriction = %f"), pm.dynamicFriction);
            ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
            ComStr.printf(_T("tempMod.bounciness = %f"), pm.restitution);
            ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
			ComStr.printf(_T("tempMod.staticFriction = %f"), pm.staticFriction);
			ExecuteMAXScriptScript(ComStr, TRUE);
			ComStr.printf(_T("tempMod.dynamicFriction = %f"), pm.dynamicFriction);
			ExecuteMAXScriptScript(ComStr, TRUE);
			ComStr.printf(_T("tempMod.bounciness = %f"), pm.restitution);
			ExecuteMAXScriptScript(ComStr, TRUE);
#endif
            std::string restitutionCombine = pm.restitutionCombine;
            if (restitutionCombine.size() > 0) {
                TSTR str = TSTR(StringToWString(restitutionCombine.c_str()).data());
                pNode->SetUserPropString(_T("restitutionCombine"), str);
            }
        }

        if (info.collisionFilter >= 0) {
            //CollisionFiltertruct cf = CollisionFilterTable[info.collisionFilter];
        }

        if (info.gravityNode) {
            GetCOREInterface()->SelectNode(info.gravityNode);
#if MAX_RELEASE >= 24000
			ComStr.printf(_T("tempMod.enableGravity = off"));
			ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
			ComStr.printf(_T("tempMod.forcesList = #($)"));
			ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
			ComStr.printf(_T("tempMod.enableGravity = off"));
			ExecuteMAXScriptScript(ComStr, TRUE);
			ComStr.printf(_T("tempMod.forcesList = #($)"));
			ExecuteMAXScriptScript(ComStr, TRUE);
#endif
            //IParamBlock2* pBlock0 = pMod->GetParamBlock(0);
            //pBlock0->SetValue(78, 0, info.gravityNode, 0);
        }

        //ComStr.printf(_T("$.modifiers[#MassFX_Rigid_Body].mass = %f"), info.mass);
        ComStr.printf(_T("tempMod.mass = %f"), info.mass);
#if MAX_RELEASE >= 24000
		ExecuteMAXScriptScript(ComStr, MAXScript::ScriptSource::NonEmbedded, TRUE);
#else
		ExecuteMAXScriptScript(ComStr, TRUE);
#endif
        SetPhysicImportStatus(1);

#else

        Modifier* pMod = (Modifier*)CreateInstance(OSM_CLASS_ID, MASSFX_RIGID_BODY);
        IParamBlock2* pBlock0 = pMod->GetParamBlock(0);
        pBlock0->SetValue(FX_rigid_type, 0, info.rigidType);

        if(info.shape >= 0){
            CollisionShapeStruct cs = CollisionShapeTable[info.shape];
            pBlock0->SetValueByName(_T("meshType"), cs.type, m_time);
            if (cs.type == 1) {
                pBlock0->SetValueByName(_T("meshRadius"), cs.param1 * m_scale * scl.x, m_time);
            }
            else if (cs.type == 2) {
                Point3 p(cs.param1, cs.param2, cs.param3);
                p *= scl;
                p *= m_scale;
                pBlock0->SetValueByName(_T("meshLength"), p.x, m_time);
                pBlock0->SetValueByName(_T("meshWidth"), p.y, m_time);
                pBlock0->SetValueByName(_T("meshHeight"), p.z, m_time);
            }
        }


        pBlock0->SetValueByName(_T("mass"), info.mass, m_time);

        if (info.physicsMaterial >= 0) {
            PhysicdMtlStruct pm = PhysicdMtlTable[info.physicsMaterial];
            pBlock0->SetValueByName(_T("staticFriction"), pm.staticFriction, m_time);
            pBlock0->SetValueByName(_T("dynamicFriction"), pm.dynamicFriction, m_time);
            pBlock0->SetValueByName(_T("bounciness"), pm.restitution, m_time);
        }
        if (info.collisionFilter >= 0) {
            //CollisionFiltertruct cf = CollisionFilterTable[info.collisionFilter];
        }
        AddModifier(pNode, pMod);
#endif
    }
}


void dmy(void)
{
    "PhysXPanelData.useGroundPlane = off";
}
