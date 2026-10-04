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
#include <AssetManagement/iassetmanager.h>
#include <AssetManagement/AssetUser.h>

//----------------------------------------------------------
//----------------------------------------------------------
void glTFExporter_Core::CreateImageBuffer(void)
{
	/*
	{
		int idx = 0;
		for (auto t : m_model.textures) {
			t.source += m_model.bufferViews.size();
			m_model.textures[idx++] = t;
		}
	}
	*/

	for (UINT it = 0; it < m_model.images.size();it++ ) {
		tinygltf::Image *image = &m_model.images[it];
		std::string fname = image->uri;
		tstring wfname = StringToWString(fname.c_str());
		if (!PathFileExists(wfname.c_str())) {
			MaxSDK::AssetManagement::AssetUser asset = MaxSDK::AssetManagement::IAssetManager::GetInstance()->GetAsset(TSTR(wfname.c_str()), MaxSDK::AssetManagement::AssetType::kBitmapAsset);
			MSTR str(wfname.c_str());
			asset.GetFullFilePath(str);
			fname = WStringToString(tstring(str));
		}

		struct stat stbuf;
		stat(fname.c_str(), &stbuf);

		int size = stbuf.st_size;

		char *buf = (char*)malloc(size);
		FILE* pFp = nullptr;
		errno_t err = fopen_s(&pFp, fname.c_str(), "rb");
		if (err) continue;

		fread(buf, sizeof(char), size, pFp);
		fclose(pFp);

		tinygltf::BufferView bfView;// = Create_glTFBufferView();
		bfView.buffer = 0;
		bfView.byteOffset = m_BufferByteOffset;
		bfView.byteLength = size;
		//bfView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

		void *ptr = SecureMemory(bfView.byteLength);
		memcpy((char*)ptr + bfView.byteOffset, buf, size);
		free(buf);

		m_model.bufferViews.push_back(bfView);
		image->bufferView = m_model.bufferViews.size() - 1;
		size_t idx = fname.find_last_of('.') + 1;
		image->mimeType = MimeTypes::getType(&fname[idx]);
		image->uri = "";
	}
}

//----------------------------------------------------------
// Create Base64 string from image file
//----------------------------------------------------------
tstring GetURILFromFile(std::string &f)
{
	tstring fname(StringToWString(f.c_str()));

	if (!PathFileExists(fname.c_str())) {
		MaxSDK::AssetManagement::AssetUser asset = MaxSDK::AssetManagement::IAssetManager::GetInstance()->GetAsset(TSTR(fname.c_str()), MaxSDK::AssetManagement::AssetType::kBitmapAsset);
		MSTR str(fname.c_str());
		asset.GetFullFilePath(str);
		fname = str;
	}

	struct _stat stbuf;
	_wstat(fname.c_str(), &stbuf);

	int size = stbuf.st_size;

	FILE* pFp = nullptr;
	errno_t err = _tfopen_s(&pFp, fname.c_str(), _T("rb"));
	if (err != 0 || pFp == nullptr ) return tstring(_T(""));

	unsigned char *data = (unsigned char*)malloc(size);
	if (data == nullptr) return tstring(_T(""));
	
	fread(data, sizeof(unsigned char), size, pFp);
	fclose(pFp);

	std::string str;
	base64_encode(data, size, str);

	// free memory, would leak otherwise
	free(data);

	return StringToWString(str.c_str());
}

static char encoding_table[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
								'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
								'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
								'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
								'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
								'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
								'w', 'x', 'y', 'z', '0', '1', '2', '3',
								'4', '5', '6', '7', '8', '9', '+', '/' };
static char *decoding_table = NULL;
static int mod_table[] = { 0, 2, 1 };

//----------------------------------------------------------
// Encode Base64 string from binary code
//----------------------------------------------------------
int base64_encode(const unsigned char *data, int size, std::string &ret)
{

	int output_length = 4 * ((size + 2) / 3);

	ret.resize(output_length);

	for (int i = 0, j = 0; i < size;) {

		uint32_t octet_a = i < size ? (unsigned char)data[i++] : 0;
		uint32_t octet_b = i < size ? (unsigned char)data[i++] : 0;
		uint32_t octet_c = i < size ? (unsigned char)data[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		ret[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		ret[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		ret[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		ret[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (int i = 0; i < mod_table[size % 3]; i++)
		ret[output_length - 1 - i] = '=';

	return output_length;
}
