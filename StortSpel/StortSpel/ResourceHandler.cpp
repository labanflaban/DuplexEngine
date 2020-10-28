#include "3DPCH.h"
#include "ResourceHandler.h"

ResourceHandler::~ResourceHandler()
{
	Destroy();
}

void ResourceHandler::isResourceHandlerReady()
{
	if (!DeviceAndContextPtrsAreSet)
	{
		// Renderer::initialize needs to be called and it needs to call setDeviceAndContextPtrs()
		// before this function call be called.
		ErrorLogger::get().logError("Problem in ResourceHandler::isResourceHandlerReady(). \nRenderer::initialize needs to be called and it needs to call setDeviceAndContextPtrs() \nbefore this function call be called. ");
		assert(false);
	}
}

ID3D11ShaderResourceView* ResourceHandler::loadTexture(const WCHAR* texturePath, bool isCubeMap)
{
	if (m_textureCache.count(texturePath))
		return m_textureCache[texturePath];
	else
	{
		isResourceHandlerReady();

		HRESULT hr;
		ID3D11ShaderResourceView* srv = nullptr;
		std::wstring path = m_TEXTURES_PATH + texturePath;

		size_t i = path.rfind('.', path.length());
		std::wstring fileExtension = path.substr(i + 1, path.length() - i);
		if (fileExtension == L"dds" || fileExtension == L"DDS")
		{
			if (isCubeMap == true)
			{
				//hr = CreateDDSTextureFromFileEx(m_devicePtr, path.c_str(), 0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, false, NULL, &srv, nullptr);
				hr = CreateDDSTextureFromFileEx(m_devicePtr, m_dContextPtr, path.c_str(), 5, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, false, NULL, &srv, nullptr);
			}
			else
			{
				hr = CreateDDSTextureFromFile(m_devicePtr, path.c_str(), nullptr, &srv);
			}

		}
		else
			hr = CreateWICTextureFromFile(m_devicePtr, path.c_str(), nullptr, &srv);

		if (FAILED(hr)) // failed to load new texture, return error texture
		{
			if (m_textureCache.count(m_ERROR_TEXTURE_NAME))  // if error texture is loaded
				return m_textureCache[m_ERROR_TEXTURE_NAME.c_str()];
			else // Load error texture
			{
				path = m_TEXTURES_PATH + m_ERROR_TEXTURE_NAME;
				hr = CreateWICTextureFromFile(m_devicePtr, path.c_str(), nullptr, &m_textureCache[m_ERROR_TEXTURE_NAME]);
				if (SUCCEEDED(hr))
					return m_textureCache[m_ERROR_TEXTURE_NAME];
				else
				{
					//std::wstring errorMessage = L"ERROR, '" + m_ERROR_TEXTURE_NAME + L"' texture can not be found in '" + m_TEXTURES_PATH + L"'!";
					ErrorLogger::get().logError("error texture can not be found in texture folder!");
					assert(!"ERROR, error texture can not be found in texture folder!");
				}
			}
		}
		else
		{
			m_textureCache[texturePath] = srv;
			return srv;
		}
	}
	return nullptr;
}

ID3D11ShaderResourceView* ResourceHandler::loadErrorTexture()
{
	return loadTexture(m_ERROR_TEXTURE_NAME.c_str());
}

MeshResource* ResourceHandler::loadLRMMesh(const char* path)
{

	isResourceHandlerReady();
	int num = m_meshCache.count(path);
	// checks if the mesh is in the cache 
	auto it = m_meshCache.find(path);
	if (it != m_meshCache.end())
	{

		// returns the buffers
		return m_meshCache[path];
	}
	
	// or loads the mesh and makes new buffers
	std::string modelPath = m_MODELS_PATH + path;
	std::ifstream fileStream(modelPath, std::ifstream::in | std::ifstream::binary);

	// Check filestream failure
	if (!fileStream)
	{
		// Error message
		std::string errormsg("loadLRMMesh failed to open filestream: "); errormsg.append(path);
		ErrorLogger::get().logError(errormsg.c_str());
		// Properly clear and close file buffer
		fileStream.clear();
		fileStream.close();
		
		// If the path that couldn't be opened was the error model path.
		if (path == m_ERROR_MODEL_NAME.c_str())
		{
			ErrorLogger::get().logError("error model can not be found in model folder");
			assert(!"ERROR, error model can not be found in model folder!");
			return nullptr;
		}
		else
			return loadLRMMesh(m_ERROR_MODEL_NAME.c_str()); // recursively load the error model instead
	}

	// .lrm has 14 floats per vertex
	const std::uint32_t nrOfFloatsInVertex = 14;
	
	// Read file vertex count
	std::uint32_t vertexCount;
	fileStream.read((char*)&vertexCount, sizeof(std::uint32_t));
	
	// Read vertices to array
	float* vertexArray = new float[vertexCount * nrOfFloatsInVertex];
	fileStream.read((char*)&vertexArray[0], sizeof(float) * vertexCount * nrOfFloatsInVertex);

	// Read file index count
	std::uint32_t indexCount;
	fileStream.read((char*)&indexCount, sizeof(std::uint32_t));

	// Read indices to array
	std::uint32_t* indexArray = new std::uint32_t[indexCount];
	fileStream.read((char*)&indexArray[0], sizeof(std::uint32_t) * indexCount);

	// Make sure all data was read
	char overByte;
	fileStream.read(&overByte, 1);
	if (!fileStream.eof())
	{
		std::string errormsg("loadLRMMesh : Filestream did not reach end of: "); errormsg.append(path);
		ErrorLogger::get().logError(errormsg.c_str());
		return nullptr;
	}

	// Close filestream
	fileStream.close();

	//Create a new entry in the meshcache
	m_meshCache[path] = new MeshResource;

	//Init it with the data
	m_meshCache[path]->getVertexBuffer().initializeBuffer(m_devicePtr, false, D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER, vertexArray, vertexCount, false, sizeof(float) * nrOfFloatsInVertex);
	m_meshCache[path]->getIndexBuffer().initializeBuffer(m_devicePtr, false, D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER, indexArray, indexCount);

	LRM_VERTEX* vertexArray2 = (LRM_VERTEX*)vertexArray;
	XMFLOAT3 min = { 99999, 99999, 99999 }, max = { -99999, -99999, -99999 };
	for (int i = 0; i < vertexCount; i++)
	{
		XMFLOAT3 currentPos = vertexArray2[i].pos;
		if (currentPos.x >= max.x)
			max.x = currentPos.x;
		if (currentPos.y >= max.y)
			max.y = currentPos.y;
		if (currentPos.z >= max.z)
			max.z = currentPos.z;

		if (currentPos.x <= min.x)
			min.x = currentPos.x;
		if (currentPos.y <= min.y)
			min.y = currentPos.y;
		if (currentPos.z <= min.z)
			min.z = currentPos.z;

	}

	m_meshCache[path]->setMinMax(min, max);
	m_meshCache[path]->storeVertexArray(vertexArray2, vertexCount);

	delete[] vertexArray;
	delete[] indexArray;

	//Return the pointer of the new entry
	return m_meshCache[path];
}

MeshResource* ResourceHandler::loadLRSMMesh(const char* path)
{
	isResourceHandlerReady();

	// checks if the mesh is in the cache 
	if (m_meshCache.find(path) != m_meshCache.end())
	{
		// returns the buffers
		return m_meshCache[path];
	}

	// or loads the mesh and makes new buffers
	std::string modelPath = m_MODELS_PATH + path;
	std::ifstream fileStream(modelPath, std::ifstream::in | std::ifstream::binary);
	
	size_t i = modelPath.rfind('.', modelPath.length());
	std::string fileExtension = modelPath.substr(i + 1, modelPath.length() - i);
	bool correctFileType = (fileExtension == "lrsm" || fileExtension == "LRSM");
	
		// Check filestream failure
	if (!fileStream || !correctFileType)
	{
		// Error message
		std::string errormsg("loadLRSMMesh failed to open filestream: "); errormsg.append(path);
		ErrorLogger::get().logError(errormsg.c_str());
		// Properly clear and close file buffer
		fileStream.clear();
		fileStream.close();

		return loadLRMMesh(m_ERROR_MODEL_NAME.c_str()); // recursively load the error model instead, this'll be weird because it needs another input layout, but whatever
	}

	// .lrm has 14 floats per vertex
	const std::uint32_t nrOfFloatsInVertex = 22;

	// Read file vertex count
	std::uint32_t vertexCount;
	fileStream.read((char*)&vertexCount, sizeof(std::uint32_t));

	// Read vertices to array
	float* vertexArray = new float[vertexCount * nrOfFloatsInVertex];
	fileStream.read((char*)&vertexArray[0], sizeof(float) * vertexCount * nrOfFloatsInVertex);

	// Read file index count
	std::uint32_t indexCount;
	fileStream.read((char*)&indexCount, sizeof(std::uint32_t));

	// Read indices to array
	std::uint32_t* indexArray = new std::uint32_t[indexCount];
	fileStream.read((char*)&indexArray[0], sizeof(std::uint32_t) * indexCount);
	
	// Read file joint count
	std::uint32_t jointCount;
	fileStream.read((char*)&jointCount, sizeof(std::uint32_t));
	
	// Read file rootJointIdx
	std::uint32_t rootJointIdx;
	fileStream.read((char*)&rootJointIdx, sizeof(std::uint32_t));

	// Read joints to array
	LRSM_JOINT* jointArray = new LRSM_JOINT[jointCount];
	fileStream.read((char*)&jointArray[0], sizeof(LRSM_JOINT) * jointCount);

	// Make sure all data was read
	char overByte;
	fileStream.read(&overByte, 1);
	if (!fileStream.eof())
	{
		std::string errormsg("loadLRSMMesh : Filestream did not reach end of: "); errormsg.append(path);
		ErrorLogger::get().logError(errormsg.c_str());
		return nullptr;
	}

	// Close filestream
	fileStream.close();

	SkeletalMeshResource* thisSkelRes = new SkeletalMeshResource;

	//Init it with the data
	thisSkelRes->getVertexBuffer().initializeBuffer(m_devicePtr, false, D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER, vertexArray, vertexCount, false, sizeof(float) * nrOfFloatsInVertex);
	thisSkelRes->getIndexBuffer().initializeBuffer(m_devicePtr, false, D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER, indexArray, indexCount);
	thisSkelRes->setJointCount(jointCount);
	thisSkelRes->setRootIndex(rootJointIdx);
	thisSkelRes->setJoints(jointArray);

	//Create a new entry in the meshcache
	m_meshCache[path] = thisSkelRes;

	delete[] vertexArray;
	delete[] indexArray;

	//Return the pointer of the new entry
	return m_meshCache[path];
}

AnimationResource* ResourceHandler::loadAnimation(std::string path)
{
	// checks if the animation is in the cache 
	if (m_animationCache.find(path) != m_animationCache.end())
	{
		// returns the resource
		return m_animationCache[path];
	}

	// or loads the mesh and makes new buffers
	std::string animationPath = m_ANIMATION_PATH + path;
	std::ifstream fileStream(animationPath, std::ifstream::in | std::ifstream::binary);

	size_t i = animationPath.rfind('.', animationPath.length());
	std::string fileExtension = animationPath.substr(i + 1, animationPath.length() - i);
	bool correctFileType = (fileExtension == "lra" || fileExtension == "LRA");

	// Check filestream failure
	if (!fileStream || !correctFileType)
	{
		// Error message
		std::string errormsg("loadAnimation failed to open filestream: "); errormsg.append(path);
		ErrorLogger::get().logError(errormsg.c_str());
		// Properly clear and close file buffer
		fileStream.clear();
		fileStream.close();

		return nullptr;
	}

	float timeSpan;
	fileStream.read((char*)&timeSpan, sizeof(float));

	std::uint32_t frameCount;
	fileStream.read((char*)&frameCount, sizeof(std::uint32_t));

	std::uint32_t jointCount;
	fileStream.read((char*)&jointCount, sizeof(std::uint32_t));
	
	assert(jointCount <= MAX_JOINT_COUNT); // If it asserts here there are more joints than we support. MAX_JOINT_COUNT might need to be changed if the skeleton can't be adjusted.
	
	std::uint32_t dataSize = (sizeof(float) + sizeof(JOINT_TRANSFORM) * jointCount) * frameCount;

	char* animData = new char[dataSize];
	fileStream.read(animData, dataSize);
	
	//ANIMATION_FRAME* animationFrameArray = new ANIMATION_FRAME[frameCount];
	//fileStream.read((char*)&animationFrameArray[0], sizeof(ANIMATION_FRAME) * frameCount);

	// Make sure all data was read
	char overByte;
	fileStream.read(&overByte, 1);
	if (!fileStream.eof())
	{
		std::string errormsg("loadAnimation : Filestream did not reach end of: "); errormsg.append(path);
		ErrorLogger::get().logError(errormsg.c_str());
		return nullptr;
	}
	
	AnimationResource* animation = new AnimationResource();
	
	animation->setTimeSpan(timeSpan);
	animation->setFrameCount(frameCount);
	animation->setJointCount(jointCount);
	
	ANIMATION_FRAME** animationFramesArray = animation->getFrames();
	*animationFramesArray = new ANIMATION_FRAME[frameCount];
	
	int offset = 0;
	for (int u = 0; u < frameCount; u++)
	{
		//memcpy(&animations.at(i).frames.at(u).timeStamp, animData + offset, sizeof(float));
		memcpy(&(*animationFramesArray)[u].timeStamp, animData + offset, sizeof(float));

		offset += sizeof(float);

		(*animationFramesArray)[u].jointTransforms = new JOINT_TRANSFORM[jointCount];

		for (int b = 0; b < jointCount; b++)
		{
			//memcpy(&animations.at(i).frames.at(u).jointTransforms[b], animData + offset, sizeof(JointTransformValues));
			memcpy(&(*animationFramesArray)[u].jointTransforms[b], animData + offset, sizeof(JOINT_TRANSFORM));
			
			offset += sizeof(JOINT_TRANSFORM);
		}

	}

	m_animationCache[path] = animation;

	// Close filestream
	fileStream.close();

	delete[] animData;

	return animation;
}

SoundEffect* ResourceHandler::loadSound(std::wstring soundPath, AudioEngine* audioEngine)
{
	if (!m_soundCache.count(soundPath))
	{
		std::wstring path = m_SOUNDS_PATH + soundPath;
		try
		{
			m_soundCache[soundPath] = new SoundEffect(audioEngine, path.c_str());
		}
		catch (std::exception e) // Error, could not load sound file
		{
			std::wstring error(soundPath);
			error = L"Could not load sound file: " + error;
			ErrorLogger::get().logError(error.c_str());

			if (!m_soundCache.count(m_ERROR_SOUND_NAME)) // if error sound is not loaded
			{
				path = m_SOUNDS_PATH + m_ERROR_SOUND_NAME;
				try
				{
					m_soundCache[m_ERROR_SOUND_NAME] = new SoundEffect(audioEngine, path.c_str());
				}
				catch (std::exception e) // Fatal Error, error sound file does not exist
				{
					ErrorLogger::get().logError("error sound file can not be found in audio folder!");
					assert(!"ERROR, error sound file can not be found in audio folder!");
				}
			}
			return m_soundCache[m_ERROR_SOUND_NAME];
		}
	}
	return m_soundCache[soundPath];
}

void ResourceHandler::setDeviceAndContextPtrs(ID3D11Device* devicePtr, ID3D11DeviceContext* dContextPtr)
{
	m_devicePtr = devicePtr;
	m_dContextPtr = dContextPtr;

	DeviceAndContextPtrsAreSet = true;
}

void ResourceHandler::Destroy()
{
	for (std::pair<std::wstring, ID3D11ShaderResourceView*> element : m_textureCache)
		element.second->Release();
		//delete element.second;
	
	
	/*for (auto it = m_meshCache.cbegin(); it != m_meshCache.cend();)
	{
		m_meshCache.erase(it++);
	}*/

	for (auto element : m_meshCache)
	{
		delete element.second;
	}
	for (auto element : m_animationCache)
	{
		delete element.second;
	}
	for (std::pair<std::wstring, SoundEffect*> element : m_soundCache)
	{
		delete element.second;
	}

	m_textureCache.clear();
	m_meshCache.clear();
	m_animationCache.clear();
	m_soundCache.clear();
}