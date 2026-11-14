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

#define MaterialSwitcherClassID		Class_ID(0x4ecd74a6, 0x0)

void glTFExporter_Core::SetSceneExtensions(void)
{
	if (m_VariantMtlMap.size() > 0) {
		tinygltf::Value::Object variant;
		for (auto pm : m_VariantMtlMap) {
			tinygltf::Value::Array v;

			if (pm.first->ClassID() == MaterialSwitcherClassID) {
				IParamBlock2* pBlock = pm.first->GetParamBlock(0);
				for (int i = 0; i < pm.second.size(); i++) {
					const TCHAR *s = pBlock->GetStr(1, m_time, FOREVER, i);
					std::string name;
					if (s)
						name = WStringToString(tstring(s));
					else
						name = std::string("item_") + std::to_string(i);
					tinygltf::Value::Object o;
					o.insert(std::make_pair("name", tinygltf::Value(name)));
					v.push_back(tinygltf::Value(o));
				}
			}
			else {
				for (auto m : pm.second) {
					if (!m) continue;
					std::string name = WStringToString(tstring(m->GetName()));
					int pos = name.find_last_of("__");
					if (pos > 0) {
						std::string n = name.substr(pos + 1, name.size() - pos);
						name = n;
					}
					tinygltf::Value::Object o;
					o.insert(std::make_pair("name", tinygltf::Value(name)));
					v.push_back(tinygltf::Value(o));
				}
			}
			variant.insert(std::make_pair("variants", v));
		}
		m_model.extensions.insert(std::make_pair("KHR_materials_variants", variant));
	}

	if (m_PhysicMtlTable.size() > 0) {
		tinygltf::Value::Array val;
		for (auto pm : m_PhysicMtlTable) {
			tinygltf::Value::Array v;
			tinygltf::Value::Object o;
			o.insert(std::make_pair("staticFriction", tinygltf::Value(truncateDecimal(pm.staticFriction))));
			o.insert(std::make_pair("dynamicFriction", tinygltf::Value(truncateDecimal(pm.dynamicFriction))));
			o.insert(std::make_pair("restitution", tinygltf::Value(truncateDecimal(pm.restitution))));
			if (pm.restitutionCombine.size() > 0) {
				o.insert(std::make_pair("restitutionCombine", tinygltf::Value(pm.restitutionCombine)));
			}
			val.push_back(tinygltf::Value(o));
		}

		tinygltf::Value::Object variant;
		variant.insert(std::make_pair("physicsMaterials", val));

		tinygltf::Value::Object v;
		tinygltf::Value::Array o;
		o.push_back(tinygltf::Value(std::string("System_0")));
		v.insert(std::make_pair("collisionSystems", tinygltf::Value(o)));
		v.insert(std::make_pair("collideWithSystems", tinygltf::Value(o)));
		variant.insert(std::make_pair("collisionFilters", v));

		m_model.extensions.insert(std::make_pair("KHR_physics_rigid_bodies", variant));
		m_Physic_RigidBody_Used = TRUE;
	}

	if (m_CollisionShapeTable.size() > 0) {
		tinygltf::Value::Array val;
		for (auto cs : m_CollisionShapeTable) {
			if (cs.type == 1) {			//Sphere
				tinygltf::Value::Object o;
				o.insert(std::make_pair("type", tinygltf::Value(std::string("sphere"))));

				tinygltf::Value::Object s;
				s.insert(std::make_pair("radius", tinygltf::Value(cs.param1)));
				o.insert(std::make_pair("sphere", tinygltf::Value(s)));
				val.push_back(tinygltf::Value(o));
			}
			else if (cs.type == 2) {	//Box
				tinygltf::Value::Object o;
				o.insert(std::make_pair("type", tinygltf::Value(std::string("box"))));

				tinygltf::Value::Array size;
				size.push_back(tinygltf::Value(cs.param1));
				size.push_back(tinygltf::Value(cs.param2));
				size.push_back(tinygltf::Value(cs.param3));
				tinygltf::Value::Object s;
				s.insert(std::make_pair("size", tinygltf::Value(size)));
				o.insert(std::make_pair("box", tinygltf::Value(s)));
				val.push_back(tinygltf::Value(o));
			}
			else if (cs.type == 3) {	//Capsule
				tinygltf::Value::Object o;
				o.insert(std::make_pair("type", tinygltf::Value(std::string("capsule"))));

				tinygltf::Value::Array size;
				size.push_back(tinygltf::Value(cs.param1));
				size.push_back(tinygltf::Value(cs.param2));
				size.push_back(tinygltf::Value(cs.param3));
				tinygltf::Value::Object s;
				s.insert(std::make_pair("size", tinygltf::Value(size)));
				o.insert(std::make_pair("capsule", tinygltf::Value(s)));
				val.push_back(tinygltf::Value(o));
			}
			/*
			else if (cs.type == 4) {	//Convex
				tinygltf::Value::Object o;
				o.insert(std::make_pair("type", tinygltf::Value(std::string("convex"))));

				tinygltf::Value::Object s;
				int meshIdx = GetMeshIdFromNode(cs.node);
				s.insert(std::make_pair("mesh", tinygltf::Value(meshIdx)));
				o.insert(std::make_pair("convex", tinygltf::Value(s)));
				val.push_back(tinygltf::Value(o));
			}
			*/
			else if (cs.type == 6) {	//Custom
				tinygltf::Value::Object o;
				o.insert(std::make_pair("type", tinygltf::Value(std::string("trimesh"))));

				tinygltf::Value::Object s;
				int meshIdx = GetMeshIdFromNode(cs.node);
				s.insert(std::make_pair("mesh", tinygltf::Value(meshIdx)));
				o.insert(std::make_pair("trimesh", tinygltf::Value(s)));
				val.push_back(tinygltf::Value(o));
			}
			else if (cs.type == 7) {	//Concave
				tinygltf::Value::Object o;
				o.insert(std::make_pair("type", tinygltf::Value(std::string("trimesh"))));

				tinygltf::Value::Object s;
				int meshIdx = GetMeshIdFromNode(cs.node);
				s.insert(std::make_pair("mesh", tinygltf::Value(meshIdx)));
				o.insert(std::make_pair("trimesh", tinygltf::Value(s)));
				val.push_back(tinygltf::Value(o));
			}
		}

		tinygltf::Value::Object shapes;
		shapes.insert(std::make_pair("shapes", tinygltf::Value(val)));

		m_model.extensions.insert(std::make_pair("KHR_implicit_shapes", tinygltf::Value(shapes)));
		m_collision_shapes_Used = TRUE;
	}

}

void glTFExporter_Core::SetSceneExtras(tinygltf::Scene &Scene)
{
	tinygltf::Value::Object params;

	int num = GetCOREInterface()->GetNumProperties(PROPSET_USERDEFINED);
	for (int i = 0; i < num; i++) {
		const PROPSPEC* pPropSpec = GetCOREInterface()->GetPropertySpec(PROPSET_USERDEFINED, i);
		std::string str = WStringToString(pPropSpec->lpwstr);
		const PROPVARIANT* pPropVar = GetCOREInterface()->GetPropertyVariant(PROPSET_USERDEFINED, i);
		switch (pPropVar->vt) {
		case VT_I4:
			params.insert(std::make_pair(str, pPropVar->intVal));
			break;

		case VT_R8:
			params.insert(std::make_pair(str, pPropVar->dblVal));
			break;

		case VT_LPWSTR:
			params.insert(std::make_pair(str, WStringToString(pPropVar->pwszVal)));
			break;

		case VT_BOOL:
			if(pPropVar->boolVal)
				params.insert(std::make_pair(str, "true"));
			else
				params.insert(std::make_pair(str, "false"));
			break;

		case VT_FILETIME:
			{
			FILETIME ftime = pPropVar->filetime;
			SYSTEMTIME stime;
			FileTimeToSystemTime(&ftime, &stime);
			char retStr[MAX_PATH];
			sprintf_s(retStr, MAX_PATH, "%04d/%02d/%02d", stime.wYear, stime.wMonth, stime.wDay);
			params.insert(std::make_pair(str, std::string(retStr)));
			}
			break;
		}
	}

	if (params.size() > 0)
		Scene.extras = tinygltf::Value(params);
}

tstring glTFExporter_Core::GetCompanyString(void)
{
	PROPSPEC	PropSpec;

	PropSpec.ulKind = PRSPEC_PROPID;
	PropSpec.propid = PIDDSI_COMPANY;
	int idx = GetCOREInterface()->FindProperty(PROPSET_DOCSUMMARYINFO, &PropSpec);
	if (idx == -1) return _T("");

	const PROPVARIANT* pPropVar = GetCOREInterface()->GetPropertyVariant(PROPSET_DOCSUMMARYINFO, idx);
	if (!pPropVar)  return _T("");

	return pPropVar->pwszVal;
}