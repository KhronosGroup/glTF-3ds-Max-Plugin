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
#include <wM3.h>

//======================================================================
//======================================================================
void glTFExporter_Core::CreateAnimation(void)
{
	for (int i = 0; i < GetCOREInterface()->GetRootNode()->NumChildren(); i++) {
		CreateAnimationRec(GetCOREInterface()->GetRootNode()->GetChildNode(i));
	}
}

//======================================================================
//======================================================================
void glTFExporter_Core::GetFulFrameAnimation(INode* pNode, Tab<TimeValue> &PosFrameList,Tab<TimeValue> &RotFrameList,Tab<TimeValue> &SclFrameList)
{
	//Interval ii = GetCOREInterface()->GetAnimRange();
	TimeValue start = GetCOREInterface()->GetAnimRange().Start();
	TimeValue end = GetCOREInterface()->GetAnimRange().End();
	TimeValue step = GetTicksPerFrame();

	TimeValue preT = 0;
	Point3 prePos(-1000, -1000, -99);
	Point3 preScl(-1000, -1000, -99);
	Quat preRot(-1000.0f, -1000.0f, -99.0f, 0.0f);
	for (TimeValue t = start; t <= end; t+=step) {
		Matrix3 tm = pNode->GetNodeTM(t);
		if (pNode->GetParentNode()->IsRootNode()) tm = tm * YupTM;
		else tm = tm * Inverse(pNode->GetParentTM(t));
		AffineParts parts;
		decomp_affine(tm, &parts);

		if (prePos != parts.t) {
			if (PosFrameList.Count() > 0)
				if (PosFrameList[PosFrameList.Count()-1] != preT)
					PosFrameList.Append(1, &preT);

			PosFrameList.Append(1, &t);
		}
		if (preRot != parts.q) {
			if (RotFrameList.Count() > 0)
				if (RotFrameList[RotFrameList.Count() - 1] != preT)
					RotFrameList.Append(1, &preT);

			RotFrameList.Append(1, &t);
		}
		//if (preScl != parts.k) {
		if (FLength(preScl-parts.k) > 0.0001f) {
			if (SclFrameList.Count() > 0)
				if (SclFrameList[SclFrameList.Count() - 1] != preT)
					SclFrameList.Append(1, &preT);

			SclFrameList.Append(1, &t);
		}
		prePos = parts.t;
		preRot = parts.q;
		preScl = parts.k;
		preT = t;
	}
}
//======================================================================
//======================================================================
void glTFExporter_Core::GetFullFrameAnimationColor(Control *pC, std::list<TimeValue>& FrameList)
{
	//Interval ii = GetCOREInterface()->GetAnimRange();
	TimeValue start = GetCOREInterface()->GetAnimRange().Start();
	TimeValue end = GetCOREInterface()->GetAnimRange().End();
	TimeValue step = GetTicksPerFrame();

	FrameList.clear();

	TimeValue preT = 0;
	AColor preColor(-1.0, -1.0, -1.0, -1.0);
	for (TimeValue t = start; t <= end; t += step) {
		AColor col;
		pC->GetValue(t, &col, FOREVER);

		if (preColor != col) {
			if(FrameList.size()>0)
				if(FrameList.back()!=preT)
					FrameList.push_back(preT);

			FrameList.push_back(t);
		}
		preColor = col;
		preT = t;
	}
}
//======================================================================
//======================================================================
void glTFExporter_Core::GetFullFrameAnimationInt(Control* pC, std::list<TimeValue>& FrameList)
{
	//Interval ii = GetCOREInterface()->GetAnimRange();
	TimeValue start = GetCOREInterface()->GetAnimRange().Start();
	TimeValue end = GetCOREInterface()->GetAnimRange().End();
	TimeValue step = GetTicksPerFrame();

	FrameList.clear();

	TimeValue preT = 0;
	int preInt = -100000;
	for (TimeValue t = start; t <= end; t += step) {
		int i;
		pC->GetValue(t, &i, FOREVER);

		if (preInt != i) {
			if (FrameList.size() > 0)
				if (FrameList.back() != preT)
					FrameList.push_back(preT);

			FrameList.push_back(t);
		}
		preInt = i;
		preT = t;
	}
}
//======================================================================
//======================================================================
void glTFExporter_Core::GetFullFrameAnimationFloat(Control* pC, std::list<TimeValue>& FrameList)
{
	//Interval ii = GetCOREInterface()->GetAnimRange();
	TimeValue start = GetCOREInterface()->GetAnimRange().Start();
	TimeValue end = GetCOREInterface()->GetAnimRange().End();
	TimeValue step = GetTicksPerFrame();

	FrameList.clear();

	TimeValue preT = 0;
	float preFloat = -100000;
	for (TimeValue t = start; t <= end; t += step) {
		float f;
		pC->GetValue(t, &f, FOREVER);

		if (preFloat != f) {
			if (FrameList.size() > 0)
				if (FrameList.back() != preT)
					FrameList.push_back(preT);

			FrameList.push_back(t);
		}
		preFloat = f;
		preT = t;
	}
}

//======================================================================
//======================================================================
void GetXYZKeyInTanOutTanList(Control* pCtrl, std::map<TimeValue, AnimKeyInfo>& keyInfo)
{
	keyInfo.clear();

	int num = pCtrl->NumKeys();

	if (pCtrl->ClassID() == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0)) {
		IKeyControl* pIkeyCrl = GetKeyControlInterface(pCtrl);
		if (!pIkeyCrl || pIkeyCrl->GetNumKeys() != num) return;

		for (int i = 0; i < num; i++) {
			IBezPoint3Key pt3Key;
			pIkeyCrl->GetKey(i, &pt3Key);

			TimeValue t = pCtrl->GetKeyTime(i);

			AnimKeyInfo info;

			// --- InTangent (Hermite = Bezier_Deg / dt * 3) ---
			if (i > 0) {
				float dt_in = (float)(t - pCtrl->GetKeyTime(i - 1));// / (float)(GetTicksPerFrame() * GetFrameRate());
				info.inTan = -pt3Key.intan * dt_in / 3.0f;
			}
			else {
				info.inTan = Point3(0, 0, 0);
			}

			// --- OutTangent (Hermite = Bezier_Deg / dt * 3) ---
			if (i < num - 1) {
				float dt_out = (float)(pCtrl->GetKeyTime(i + 1) - t);// *(float)(GetTicksPerFrame() * GetFrameRate());
				info.outTan = pt3Key.outtan * dt_out / 3.0f;
			}
			else {
				info.outTan = Point3(0, 0, 0);
			}

			// --- Value ---
			info.val = pt3Key.val;

			keyInfo[t] = info;
		}

		return;
	}






	Control* pCX = pCtrl->GetXController();
	Control* pCY = pCtrl->GetYController();
	Control* pCZ = pCtrl->GetZController();
	if (!pCX || !pCY || !pCZ) return;
	if (pCX->ClassID() != Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID, 0)) return;
	if (pCY->ClassID() != Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID, 0)) return;
	if (pCZ->ClassID() != Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID, 0)) return;

	IKeyControl* pIkeyXCrl = GetKeyControlInterface(pCtrl->GetXController());
	IKeyControl* pIkeyYCrl = GetKeyControlInterface(pCtrl->GetYController());
	IKeyControl* pIkeyZCrl = GetKeyControlInterface(pCtrl->GetZController());
	if (!pIkeyXCrl|| !pIkeyYCrl || !pIkeyZCrl) return;
	if (pIkeyXCrl->GetNumKeys() != num) return;
	if (pIkeyYCrl->GetNumKeys() != num) return;
	if (pIkeyZCrl->GetNumKeys() != num) return;

	for (int i = 0; i < num; i++) {
		IBezFloatKey xKey;
		pIkeyXCrl->GetKey(i, &xKey);
		IBezFloatKey yKey;
		pIkeyYCrl->GetKey(i, &yKey);
		IBezFloatKey zKey;
		pIkeyZCrl->GetKey(i, &zKey);


		TimeValue t = pCtrl->GetKeyTime(i);

		AnimKeyInfo info;

		// --- InTangent (Hermite = Bezier_Deg / dt * 3) ---
		if (i > 0) {
			float dt_in = (float)(t - pCtrl->GetKeyTime(i - 1));// / (float)(GetTicksPerFrame() * GetFrameRate());
			info.inTan = Point3(-(xKey.intan), -(yKey.intan), -(zKey.intan)) * dt_in / 3.0f;
			//info.inTan = Point3(fabs(xKey.intan), fabs(yKey.intan), fabs(zKey.intan)) * dt_in / 3.0f;
		}else{
			info.inTan = Point3(0,0,0);
		}


		// --- OutTangent (Hermite = Bezier_Deg / dt * 3) ---
		if (i < num - 1) {
			float dt_out = (float)(pCtrl->GetKeyTime(i + 1) - t);// *(float)(GetTicksPerFrame() * GetFrameRate());
			info.outTan = Point3((xKey.outtan), (yKey.outtan), (zKey.outtan)) * dt_out / 3.0f;
			//info.outTan = Point3(fabs(xKey.outtan), fabs(yKey.outtan), fabs(zKey.outtan)) * dt_out / 3.0f;
		} else {
			info.outTan = Point3(0, 0, 0);
		}

		// --- Value ---
		info.val = Point3(xKey.val, yKey.val, zKey.val);

		keyInfo[t] = info;
	}
}

//======================================================================
//======================================================================
void glTFExporter_Core::CreateKeyFrameList(Control *pCtrl, Tab<TimeValue> &KeyFrameList, BOOL Clear)
{
	if(Clear)
		KeyFrameList.ZeroCount();

	if (!pCtrl) return;

	if (pCtrl->NumKeys() > 0) {
		for (int i = 0; i < pCtrl->NumKeys(); i++) {
			TimeValue t = pCtrl->GetKeyTime(i);
			KeyFrameList.Append(1, &t);
		}

	}
}

//======================================================================
//======================================================================
void glTFExporter_Core::CreateKeyFrameList(Control* pCtrl, std::list<TimeValue>& KeyFrameList, BOOL Clear)
{
	if (Clear)
		KeyFrameList.clear();

	if (!pCtrl) return;

	if (pCtrl->NumKeys() > 0) {
		for (int i = 0; i < pCtrl->NumKeys(); i++) {
			TimeValue t = pCtrl->GetKeyTime(i);
			if (std::find(KeyFrameList.begin(), KeyFrameList.end(), t) == KeyFrameList.end()) {
				KeyFrameList.push_back(t);
			}
		}
	}
}

//======================================================================
//======================================================================
void glTFExporter_Core::CreateAnimationRec(INode *pNode)
{
	float quantScale = 1.0f;
	float quantParentScale = 1.0f;
	if (m_Mesh_quantization_Used) {
		QuantizationInfo info;
		GetQuatizationInfo(pNode, info);
		quantScale = info.meshScale;
		GetQuatizationInfo(pNode->GetParentNode(), info);
		quantParentScale = info.meshScale;
	}


	AffineParts parts;
	Control *pC = pNode->GetTMController();
	if (pC->IsAnimated() || m_FullFrame) {
		Tab<TimeValue> PosFrameList;
		Tab<TimeValue> RotFrameList;
		Tab<TimeValue> SclFrameList;
		PosFrameList.ZeroCount();
		RotFrameList.ZeroCount();
		SclFrameList.ZeroCount();

		if (m_FullFrame) {
			GetFulFrameAnimation(pNode, PosFrameList, RotFrameList, SclFrameList);
			if (PosFrameList.Count() == 1)PosFrameList.ZeroCount();
			if (RotFrameList.Count() == 1)RotFrameList.ZeroCount();
			if (SclFrameList.Count() == 1)SclFrameList.ZeroCount();
		}
		else {
			Control* pRotC = pC->GetRotationController();
			CreateKeyFrameList(pRotC, RotFrameList);

			pC->GetKeyTimes(PosFrameList, FOREVER, KEYAT_POSITION);
			pC->GetKeyTimes(SclFrameList, FOREVER, KEYAT_SCALE);
		}

		{
			UINT mod = m_BufferByteOffset % 4;
			if (mod) SecureMemory(4 - mod);
		}
		//tinygltf::Node *node = m_NodeMap[pNode];
		if (PosFrameList.Count() > 0) {
			std::map<TimeValue, AnimKeyInfo> keyInfo;
			if(m_CubicSplineT)
				GetXYZKeyInTanOutTanList(pC->GetPositionController(), keyInfo);

			tinygltf::AnimationSampler sampler;// = Create_glTFAnimSampler();
			sampler.interpolation = keyInfo.size()>0 ? "CUBICSPLINE" : "LINEAR";

			tinygltf::Accessor accIn;
			accIn.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			accIn.type = TINYGLTF_TYPE_SCALAR;
			accIn.count = PosFrameList.Count();

			tinygltf::BufferView bfViewIn;
			bfViewIn.buffer = 0;
			bfViewIn.byteOffset = m_BufferByteOffset;
			bfViewIn.byteLength = PosFrameList.Count() * sizeof(float);

			void *ptr = SecureMemory(bfViewIn.byteLength);
			float *pFrame = (float*)((char*)ptr + bfViewIn.byteOffset);
			float time_min = (float)PosFrameList[0] / m_TimeScale;
			float time_max = (float)PosFrameList[0] / m_TimeScale;
			for (int i = 0; i < PosFrameList.Count(); i++) {
				float fr = (float)PosFrameList[i] / m_TimeScale;
				if (time_min > fr) time_min = fr;
				if (time_max < fr) time_max = fr;
				*pFrame++ = fr;
			}
			accIn.maxValues.push_back(time_max);
			accIn.minValues.push_back(time_min);

			m_model.bufferViews.push_back(bfViewIn);
			accIn.bufferView = m_model.bufferViews.size() - 1;
			m_model.accessors.push_back(accIn);
			sampler.input = m_model.accessors.size() - 1;

			tinygltf::Accessor accOut;
			accOut.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			accOut.type = TINYGLTF_TYPE_VEC3;
			accOut.count = PosFrameList.Count();
			if (keyInfo.size() > 0) accOut.count *= 3;

			tinygltf::BufferView bfViewOut;
			bfViewOut.buffer = 0;
			bfViewOut.byteOffset = m_BufferByteOffset;
			bfViewOut.byteLength = accOut.count * sizeof(float) * 3;

			ptr = SecureMemory(bfViewOut.byteLength);
			float *pData = (float*)((char*)ptr + bfViewOut.byteOffset);
			//Control *pPosC = pC->GetPositionController();
#if MAX_RELEASE<=22000
			for (int tt = 0; tt < PosFrameList.Count(); tt++) {
				TimeValue fr = PosFrameList[tt];
				if (keyInfo.size() > 0) {
					Point3 inTan = keyInfo[fr].inTan * YupTM;
					*pData++ = inTan.x / quantParentScale * m_scale;
					*pData++ = inTan.y / quantParentScale * m_scale;
					*pData++ = inTan.z / quantParentScale * m_scale;
				}
				Matrix3 tm = pNode->GetNodeTM(fr);
				if (pNode->GetParentNode()->IsRootNode()) tm = tm * YupTM;
				else tm = tm * Inverse(pNode->GetParentTM(fr));
				decomp_affine(tm, &parts);
				*pData++ = parts.t.x / quantParentScale * m_scale;
				*pData++ = parts.t.y / quantParentScale * m_scale;
				*pData++ = parts.t.z / quantParentScale * m_scale;
				if (keyInfo.size() > 0) {
					Point3 outTan = keyInfo[fr].outTan * YupTM;
					*pData++ = outTan.x / quantParentScale * m_scale;
					*pData++ = outTan.y / quantParentScale * m_scale;
					*pData++ = outTan.z / quantParentScale * m_scale;
				}

			}
#else
			for (auto fr : PosFrameList) {
				float scale = m_Mesh_quantization_Used ? 1.0f : m_scale;
				if (keyInfo.size() > 0) {
					Point3 inTan = keyInfo[fr].inTan * YupTM;
					*pData++ = inTan.x / quantParentScale * scale;
					*pData++ = inTan.y / quantParentScale * scale;
					*pData++ = inTan.z / quantParentScale * scale;
				}
				Matrix3 tm = pNode->GetNodeTM(fr);
				if (pNode->GetParentNode()->IsRootNode()) tm = tm * YupTM;
				else tm = tm * Inverse(pNode->GetParentTM(fr));
				decomp_affine(tm, &parts);
				*pData++ = parts.t.x / quantParentScale * scale;
				*pData++ = parts.t.y / quantParentScale * scale;
				*pData++ = parts.t.z / quantParentScale * scale;
				if (keyInfo.size() > 0) {
					Point3 outTan = keyInfo[fr].outTan * YupTM;
					*pData++ = outTan.x / quantParentScale * scale;
					*pData++ = outTan.y / quantParentScale * scale;
					*pData++ = outTan.z / quantParentScale * scale;
				}
			}
#endif
			m_model.bufferViews.push_back(bfViewOut);
			accOut.bufferView = m_model.bufferViews.size() - 1;
			m_model.accessors.push_back(accOut);
			sampler.output = m_model.accessors.size() - 1;
			m_animation.samplers.push_back(sampler);

			tinygltf::AnimationChannel channel;// = Create_glTFAnimChannel();
			channel.target_node = findNodeIndex(pNode);
			channel.target_path = "translation";
			channel.sampler = m_animation.samplers.size() - 1;
			m_animation.channels.push_back(channel);
		}


		if (RotFrameList.Count() > 0) {
			tinygltf::AnimationSampler sampler;// = Create_glTFAnimSampler();
			sampler.interpolation = "LINEAR";

			tinygltf::Accessor accIn;
			accIn.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			accIn.type = TINYGLTF_TYPE_SCALAR;
			accIn.count = RotFrameList.Count();

			tinygltf::BufferView bfViewIn;
			bfViewIn.buffer = 0;
			bfViewIn.byteOffset = m_BufferByteOffset;
			bfViewIn.byteLength = RotFrameList.Count() * sizeof(float);

			void *ptr = SecureMemory(bfViewIn.byteLength);
			float *pFrame = (float*)((char*)ptr + bfViewIn.byteOffset);
			float time_min = (float)RotFrameList[0] / m_TimeScale;
			float time_max = (float)RotFrameList[0] / m_TimeScale;
			for (int i = 0; i < RotFrameList.Count(); i++) {
				float fr = (float)RotFrameList[i] / m_TimeScale;
				if (time_min > fr) time_min = fr;
				if (time_max < fr) time_max = fr;
				*pFrame++ = fr;
			}
			accIn.maxValues.push_back(time_max);
			accIn.minValues.push_back(time_min);

			m_model.bufferViews.push_back(bfViewIn);
			accIn.bufferView = m_model.bufferViews.size() - 1;
			m_model.accessors.push_back(accIn);
			sampler.input = m_model.accessors.size() - 1;

			tinygltf::Accessor accOut;
			accOut.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			accOut.type = TINYGLTF_TYPE_VEC4;
			accOut.count = RotFrameList.Count();

			tinygltf::BufferView bfViewOut;
			bfViewOut.buffer = 0;
			bfViewOut.byteOffset = m_BufferByteOffset;
			bfViewOut.byteLength = accOut.count * sizeof(float) * 4;

			ptr = SecureMemory(bfViewOut.byteLength);
			float *pData = (float*)((char*)ptr + bfViewOut.byteOffset);
			//Control *pRotC = pC->GetRotationController();
#if MAX_RELEASE<=22000
			for (int tt = 0; tt < RotFrameList.Count(); tt++) {
				TimeValue fr = RotFrameList[tt];
				Matrix3 tm(pNode->GetObjTMAfterWSM(fr));
				if (pNode->GetParentNode()->IsRootNode()) tm = tm * YupTM;
				else tm = tm * Inverse(pNode->GetParentTM(fr));
				decomp_affine(tm, &parts);
				*pData++ = parts.q.x;
				*pData++ = parts.q.y;
				*pData++ = parts.q.z;
				*pData++ = -parts.q.w;
			}
#else
			for (auto fr : RotFrameList) {
				//Matrix3 tm = pNode->GetNodeTM(fr);
				Matrix3 tm(pNode->GetObjTMAfterWSM(fr));
				if (pNode->GetParentNode()->IsRootNode()) tm = tm * YupTM;
				else tm = tm * Inverse(pNode->GetParentTM(fr));
				decomp_affine(tm, &parts);
				*pData++ = parts.q.x;
				*pData++ = parts.q.y;
				*pData++ = parts.q.z;
				*pData++ = -parts.q.w;
			}
#endif
			m_model.bufferViews.push_back(bfViewOut);
			accOut.bufferView = m_model.bufferViews.size() - 1;
			m_model.accessors.push_back(accOut);
			sampler.output = m_model.accessors.size() - 1;
			m_animation.samplers.push_back(sampler);

			tinygltf::AnimationChannel channel;// = Create_glTFAnimChannel();
			channel.target_node = findNodeIndex(pNode);
			channel.target_path = "rotation";
			channel.sampler = m_animation.samplers.size() - 1;
			m_animation.channels.push_back(channel);
		}


		if (SclFrameList.Count() > 0) {
			tinygltf::AnimationSampler sampler;// = Create_glTFAnimSampler();
			sampler.interpolation = "LINEAR";

			tinygltf::Accessor accIn;
			accIn.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			accIn.type = TINYGLTF_TYPE_SCALAR;
			accIn.count = SclFrameList.Count();

			tinygltf::BufferView bfViewIn;
			bfViewIn.buffer = 0;
			bfViewIn.byteOffset = m_BufferByteOffset;
			bfViewIn.byteLength = SclFrameList.Count() * sizeof(float);

			void *ptr = SecureMemory(bfViewIn.byteLength);
			float *pFrame = (float*)((char*)ptr + bfViewIn.byteOffset);	
			float time_min = (float)SclFrameList[0] / m_TimeScale;
			float time_max = (float)SclFrameList[0] / m_TimeScale;
			for (int i = 0; i < SclFrameList.Count(); i++) {
				float fr = (float)SclFrameList[i] / m_TimeScale;
				if (time_min > fr) time_min = fr;
				if (time_max < fr) time_max = fr;
				*pFrame++ = fr;
			}
			accIn.maxValues.push_back(time_max);
			accIn.minValues.push_back(time_min);

			m_model.bufferViews.push_back(bfViewIn);
			accIn.bufferView = m_model.bufferViews.size() - 1;
			m_model.accessors.push_back(accIn);
			sampler.input = m_model.accessors.size() - 1;

			tinygltf::Accessor accOut;
			accOut.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			accOut.type = TINYGLTF_TYPE_VEC3;
			accOut.count = SclFrameList.Count();

			tinygltf::BufferView bfViewOut;
			bfViewOut.buffer = 0;
			bfViewOut.byteOffset = m_BufferByteOffset;
			bfViewOut.byteLength = accOut.count * sizeof(float) * 3;

			ptr = SecureMemory(bfViewOut.byteLength);
			float *pData = (float*)((char*)ptr + bfViewOut.byteOffset);
			//Control *pSclC = pC->GetScaleController();
#if MAX_RELEASE<=22000
			for (int tt = 0; tt < SclFrameList.Count(); tt++) {
				TimeValue fr = SclFrameList[tt];
				Matrix3 tm = pNode->GetNodeTM(fr);
				if (pNode->GetParentNode()->IsRootNode()) tm = tm * YupTM;
				else tm = tm * Inverse(pNode->GetParentTM(fr));
				decomp_affine(tm, &parts);
				*pData++ = parts.k.x / quantScale;
				*pData++ = parts.k.y / quantScale;
				*pData++ = parts.k.z / quantScale;
			}
#else
			for (auto fr : SclFrameList) {
				Matrix3 tm = pNode->GetNodeTM(fr);
				if (pNode->GetParentNode()->IsRootNode()) tm = tm * YupTM;
				else tm = tm * Inverse(pNode->GetParentTM(fr));
				decomp_affine(tm, &parts);
				*pData++ = parts.k.x * quantScale / quantParentScale;
				*pData++ = parts.k.y * quantScale / quantParentScale;
				*pData++ = parts.k.z * quantScale / quantParentScale;
			}
#endif
			m_model.bufferViews.push_back(bfViewOut);
			accOut.bufferView = m_model.bufferViews.size() - 1;
			m_model.accessors.push_back(accOut);
			sampler.output = m_model.accessors.size() - 1;
			m_animation.samplers.push_back(sampler);

			tinygltf::AnimationChannel channel;// = Create_glTFAnimChannel();
			channel.target_node = findNodeIndex(pNode);
			channel.target_path = "scale";
			channel.sampler = m_animation.samplers.size() - 1;
			m_animation.channels.push_back(channel);
		}
	}

	int morphCount = 0;
	Modifier* pMorphMod = NULL;
	if (FindModifier(pNode, MR3_CLASS_ID, &pMorphMod) >= 0) {
		morphCount = GetMorphTargetNum(pMorphMod);
	}
	if (morphCount > 0) {
		std::vector<Control*> ctrlTable;
		for (int i = 0; i < 100; i++) {
			Control* pCtrl = GetMorphController(pMorphMod, i);
			if (pCtrl) ctrlTable.push_back(pCtrl);
		}

		std::vector<TimeValue> wFrameList;
		wFrameList.clear();
		for (auto p : ctrlTable) {
			for (int i = 0; i < p->NumKeys(); i++) {
				TimeValue t = p->GetKeyTime(i);
				wFrameList.push_back(t);
			}
		}

		std::sort(wFrameList.begin(), wFrameList.end());
		wFrameList.erase(std::unique(wFrameList.begin(), wFrameList.end()), wFrameList.end());

		if (wFrameList.size() > 0) {
			tinygltf::AnimationSampler sampler;// = Create_glTFAnimSampler();
			sampler.interpolation = "LINEAR";

			tinygltf::Accessor accIn;
			accIn.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			accIn.type = TINYGLTF_TYPE_SCALAR;
			accIn.count = wFrameList.size();

			tinygltf::BufferView bfViewIn;
			bfViewIn.buffer = 0;
			bfViewIn.byteOffset = m_BufferByteOffset;
			bfViewIn.byteLength = wFrameList.size() * sizeof(float);

			void* ptr = SecureMemory(bfViewIn.byteLength);
			float* pFrame = (float*)((char*)ptr + bfViewIn.byteOffset);
			float time_min = (float)wFrameList[0] / m_TimeScale;
			float time_max = (float)wFrameList[0] / m_TimeScale;
			for (int i = 0; i < wFrameList.size(); i++) {
				float fr = (float)wFrameList[i] / m_TimeScale;
				if (time_min > fr) time_min = fr;
				if (time_max < fr) time_max = fr;
				*pFrame++ = fr;
			}
			accIn.maxValues.push_back(time_max);
			accIn.minValues.push_back(time_min);

			m_model.bufferViews.push_back(bfViewIn);
			accIn.bufferView = m_model.bufferViews.size() - 1;
			m_model.accessors.push_back(accIn);
			sampler.input = m_model.accessors.size() - 1;

			tinygltf::Accessor accOut;
			accOut.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			accOut.type = TINYGLTF_TYPE_SCALAR;
			accOut.count = wFrameList.size() * morphCount;

			tinygltf::BufferView bfViewOut;
			bfViewOut.buffer = 0;
			bfViewOut.byteOffset = m_BufferByteOffset;
			bfViewOut.byteLength = accOut.count * sizeof(float);

			ptr = SecureMemory(bfViewOut.byteLength);
			float* pData = (float*)((char*)ptr + bfViewOut.byteOffset);
			float weight_min = (float)0.0f;
			float weight_max = (float)0.0f;
			for (int i = 0; i < wFrameList.size(); i++) {
				for (int c = 0; c < morphCount; c++) {
					float val = 0.0f;
					if(c < ctrlTable.size())
						ctrlTable[c]->GetValue(wFrameList[i], &val, FOREVER);
					*pData++ = val/100.0f;
					if (weight_min > val) weight_min = val;
					if (weight_max < val) weight_max = val;
				}
			}
			accOut.maxValues.push_back(weight_max / 100.0f);
			accOut.minValues.push_back(weight_min / 100.0f);

			m_model.bufferViews.push_back(bfViewOut);
			accOut.bufferView = m_model.bufferViews.size() - 1;
			m_model.accessors.push_back(accOut);
			sampler.output = m_model.accessors.size() - 1;
			m_animation.samplers.push_back(sampler);

			tinygltf::AnimationChannel channel;// = Create_glTFAnimChannel();
			channel.target_node = findNodeIndex(pNode);
			channel.target_path = "weights";
			channel.sampler = m_animation.samplers.size() - 1;
			m_animation.channels.push_back(channel);

		}
	
	}

	for (int i = 0; i < pNode->NumChildren(); i++) {
		CreateAnimationRec(pNode->GetChildNode(i));
	}
}
