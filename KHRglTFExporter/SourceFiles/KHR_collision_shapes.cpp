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

#include "KHRglTFExporter.h"

#define MASSFX_RIGID_BODY		Class_ID(0xa750e, 0x6e6ba)
#define Gravity_ClassID			Class_ID(0xe523c, 0x0)


#define FX_rigid_type		0
#define FX_density			6
#define FX_mass				7
#define FX_staticFriction	8
#define FX_dynamicFriction	9
#define FX_bounciness		10

#define FX_meshtype			13

#define FX_meshRadius		31
#define FX_enableGravity	35
#define FX_masterCtrMode	71
#define FX_forceList		78

#define FX_massCenterX	45
#define FX_massCenterY	46
#define FX_massCenterZ	47


//======================================================================
//======================================================================
void glTFExporter_Core::CreateCollisionShape(INode *pNode, tinygltf::Node &node)
{
	if (!pNode) return;

	Modifier* pMod = NULL;
	FindModifier(pNode, MASSFX_RIGID_BODY, &pMod);

	IParamBlock2* pBlock0 = NULL;
	if (pMod) {
		pBlock0 = pMod->GetParamBlock(0);
	} else if (!m_AttachRigidInfo) return;

	Point3 scl(1.0f, 1.0f, 1.0f);
	{
		Matrix3 tm = pNode->GetNodeTM(0);
		AffineParts parts;
		decomp_affine(tm, &parts);
		scl = parts.k;
	}

	//-----------------------------------
	CollisionShapesStruct cs;
	cs.node = pNode;
	cs.type = pBlock0 ? pBlock0->GetInt(FX_meshtype): 6;
	int shapeIdx = -1;
	if (cs.type == 1) {			//Sphere
		cs.param1 = pBlock0->GetFloat(31) / scl.x;
		m_CollisionShapeTable.push_back(cs);
		shapeIdx = (int)(m_CollisionShapeTable.size() - 1);
	}
	else if (cs.type == 2) {	//Box
		cs.param1 = pBlock0->GetFloat(32) / scl.x;
		cs.param2 = pBlock0->GetFloat(33) / scl.y;
		cs.param3 = pBlock0->GetFloat(34) / scl.z;
		m_CollisionShapeTable.push_back(cs);
		shapeIdx = (int)(m_CollisionShapeTable.size() - 1);
	}
	else if (cs.type == 3) {	//Capsule
		cs.param1 = pBlock0->GetFloat(31)/scl.x;
		cs.param2 = pBlock0->GetFloat(34) / scl.x;
		m_CollisionShapeTable.push_back(cs);
		shapeIdx = (int)(m_CollisionShapeTable.size() - 1);
	}
	else if (cs.type == 4) {	//Convex
	}
	else if (cs.type == 5) {	//Original
	}
	else if (cs.type == 6) {	//Custom
	}
	else if (cs.type == 7) {	//Concave
	}


	//-----------------------------------
	float sf = pBlock0 ? pBlock0->GetFloat(FX_staticFriction):1.0f;
	float df = pBlock0 ? pBlock0->GetFloat(FX_dynamicFriction) : 1.0f;
	float rt = pBlock0 ? pBlock0->GetFloat(FX_bounciness) : 0.0f;
	std::string rc = "";
	{
		TSTR str;
		if(pNode->GetUserPropString(_T("restitutionCombine"), str))
			rc = WStringToString(str.data());
	}

	int idx = -1;
	for (auto t = m_PhysicMtlTable.begin(); t<m_PhysicMtlTable.end();t++) {
		if (t->staticFriction == sf && t->dynamicFriction == df && t->restitution == rt) {
			idx = (int)std::distance(m_PhysicMtlTable.begin(), t);
			break;
		}
	}

	if (idx == -1) {
		PhysicsMaterials pm;
		pm.dynamicFriction = df;
		pm.staticFriction = sf;
		pm.restitution = rt;
		pm.restitutionCombine = rc;
		m_PhysicMtlTable.push_back(pm);
		idx = (int)(m_PhysicMtlTable.size() - 1);
	}

	tinygltf::Value::Object obj;
	{
		tinygltf::Value::Object v;
		tinygltf::Value::Object motion;

		int rigid = pBlock0 ? pBlock0->GetInt(FX_rigid_type):0;
		if (rigid == 1) {
			float mass = pBlock0->GetFloat(FX_mass);
			motion.insert(std::make_pair("mass", tinygltf::Value(mass)));
		}

		int gr = pBlock0 ? pBlock0->GetInt(FX_enableGravity) : 1;
		if (gr==0) {
			if (pBlock0->Count(FX_forceList) > 0) {
				INode *pf = pBlock0->GetINode(FX_forceList,0);
				Object* pObj = pf->GetObjectRef();
				if (pObj->ClassID() == Gravity_ClassID) {
					IParamBlock2* pb = pObj->GetParamBlock(0);
					float gr = pb->GetFloat(0);
					motion.insert(std::make_pair("gravityFactor", tinygltf::Value(truncateDecimal(gr))));
				}
			}
		}

		int mc = pBlock0 ? pBlock0->GetInt(FX_masterCtrMode) : 0;
		if (mc == 2) {
			tinygltf::Value::Object o;
			tinygltf::Value::Array ary;
			Point3 p;
			p.x = pBlock0->GetFloat(FX_massCenterX);
			p.y = pBlock0->GetFloat(FX_massCenterY);
			p.z = pBlock0->GetFloat(FX_massCenterZ);
			ary.push_back(tinygltf::Value(p.x));
			ary.push_back(tinygltf::Value(p.y));
			ary.push_back(tinygltf::Value(p.z));
			motion.insert(std::make_pair("centerOfMass", tinygltf::Value(ary)));
		}
		if(motion.size()>0) obj.insert(std::make_pair("motion", tinygltf::Value(motion)));
	}

	BOOL meshObject = TRUE;
	{
		BOOL deleteIt = FALSE;
		TriObject* pTri = GetTriObjectFromNode(pNode, m_time, deleteIt);
		if (!pTri) meshObject = FALSE;
		if (deleteIt) delete pTri;
	}

	if(meshObject){
		tinygltf::Value::Object v;
		tinygltf::Value::Object o;

		if (shapeIdx < 0) {
			v.insert(std::make_pair("convexHull", tinygltf::Value(false)));
			size_t idx = m_model.nodes.size()+1;
			v.insert(std::make_pair("node", tinygltf::Value((int)idx)));
			o.insert(std::make_pair("geometry", tinygltf::Value(v)));
		} else {
			v.insert(std::make_pair("shape", tinygltf::Value(shapeIdx)));
			o.insert(std::make_pair("geometry", tinygltf::Value(v)));
		}

		o.insert(std::make_pair("physicsMaterial", tinygltf::Value(idx)));
		o.insert(std::make_pair("collisionFilter", tinygltf::Value(0)));
		obj.insert(std::make_pair("collider", tinygltf::Value(o)));

		node.extensions_json_string = "KHR_physics_rigid_bodies";
	}

	tinygltf::Value val(obj);
	node.extensions.insert(std::make_pair("KHR_physics_rigid_bodies", val));
}

//==========================================================
//==========================================================
void glTFExporter_Core::CreateMeshDataNode(const tinygltf::Node& basenode)
{
	tinygltf::Node node;

	node.name = basenode.name + ("MeshDataNode");
	node.mesh = basenode.mesh;

	node.translation.push_back(0);
	node.translation.push_back(0);
	node.translation.push_back(0);
	node.rotation.push_back(0);
	node.rotation.push_back(0);
	node.rotation.push_back(0);
	node.rotation.push_back(1);

	m_model.nodes.push_back(node);
}
