#include "SkyBox.h"

CSkyBox::CSkyBox()
{
	mpModel = nullptr;
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
}


CSkyBox::~CSkyBox()
{
}

bool CSkyBox::Initialise(ID3D11Device * device)
{
	if (!LoadSkyBoxModel( "Resources/Models/Sphere.fbx"))
	{
		logger->GetInstance().WriteLine("Failed to load the sky box model in initialisation function of skybox.");
		return false;
	}

	if (!InitialiseBuffers(device))
	{
		logger->GetInstance().WriteLine("Failed to initialise the buffers for skybox. ");
		return false;
	}
	
	mIsDayTime = true;
	mIsNightTime = false;
	mIsEveningTime = false;

	mApexColour = kDayApexColour;
	mCentreColour = kDayCentreColour;

	return true;
}

void CSkyBox::Shutdown()
{
	ReleaseBuffers();

	ReleaseSkyBoxModel();
}

bool CSkyBox::Render(ID3D11DeviceContext * deviceContext)
{
	RenderBuffers(deviceContext);
	return true;
}

void CSkyBox::ReleaseBuffers()
{
	if (mpVertexBuffer)
	{
		mpVertexBuffer->Release();
		mpVertexBuffer = nullptr;
	}

	if (mpIndexBuffer)
	{
		mpIndexBuffer->Release();
		mpIndexBuffer = nullptr;
	}
}

void CSkyBox::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);

	deviceContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

bool CSkyBox::IsDayTime()
{
	return mIsDayTime;
}

bool CSkyBox::IsEveningTime()
{
	return mIsEveningTime;
}

bool CSkyBox::IsNightTime()
{
	return mIsNightTime;
}

bool CSkyBox::UpdateTimeOfDay(float updateTime)
{
	// Daytime goes to evening
	if (mIsDayTime)
	{
		if (UpdateToEvening(updateTime))
		{
			return true;
		}
	}
	// If evening, want to go to night time next.
	else if (mIsEveningTime)
	{
		if (UpdateToNight(updateTime))
		{
			return true;
		}
	}
	// If night, want to go to daytime or morning time next.
	else if (mIsNightTime)
	{
		if (UpdateToDay(updateTime))
		{
			return true;
		}
	}

	return false;
}

bool CSkyBox::UpdateToEvening(float updateTime)
{
	////////////////////////
	// Red greater than target
	////////////////////////

	// If the red of our apex colour is more than our target evening red 
	if (mApexColour.x > kEveningApexColour.x)
	{
		// Reduce colour from red, will cause gradual change.
		mApexColour.x -= kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mApexColour.x <= kEveningApexColour.x)
		{
			// Set the apex red to our target red.
			mApexColour.x = kEveningApexColour.x;
		}
	}


	// If the red of our apex colour is more than our target evening red 
	if (mCentreColour.x > kEveningCentreColour.x)
	{
		// Reduce colour from red, will cause gradual change.
		mCentreColour.x -= kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mCentreColour.x <= kEveningCentreColour.x)
		{
			// Set the apex red to our target red.
			mCentreColour.x = kEveningCentreColour.x;
		}
	}

	////////////////////////
	// Red less than target
	////////////////////////

	// If the red of our apex colour is less than our target evening red 
	if (mApexColour.x < kEveningApexColour.x)
	{
		// Increase red, will cause gradual change.
		mApexColour.x += kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mApexColour.x >= kEveningApexColour.x)
		{
			// Set the apex red to our target red.
			mApexColour.x = kEveningApexColour.x;
		}
	}

	// If the red of our apex colour is less than our target evening red 
	if (mCentreColour.x < kEveningCentreColour.x)
	{
		// Increase red, will cause gradual change.
		mCentreColour.x += kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mCentreColour.x >= kEveningCentreColour.x)
		{
			// Set the apex red to our target red.
			mCentreColour.x = kEveningCentreColour.x;
		}
	}

	////////////////////////
	// Green greater than target
	////////////////////////

	// If the green of our apex colour is more than our target evening green 
	if (mApexColour.y > kEveningApexColour.y)
	{
		// Reduce colour from green, will cause gradual change.
		mApexColour.y -= kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mApexColour.y <= kEveningApexColour.y)
		{
			// Set the apex green to our target green.
			mApexColour.y = kEveningApexColour.y;
		}
	}

	// If the green of our apex colour is more than our target evening green 
	if (mCentreColour.y > kEveningCentreColour.y)
	{
		// Reduce colour from green, will cause gradual change.
		mCentreColour.y -= kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mCentreColour.y <= kEveningCentreColour.y)
		{
			// Set the apex green to our target green.
			mCentreColour.y = kEveningCentreColour.y;
		}
	}

	////////////////////////
	// Green less than target
	////////////////////////

	// If the green of our apex colour is less than our target evening green 
	if (mApexColour.y < kEveningApexColour.y)
	{
		// Increase green, will cause gradual change.
		mApexColour.y += kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mApexColour.y >= kEveningApexColour.y)
		{
			// Set the apex green to our target green.
			mApexColour.y = kEveningApexColour.y;
		}
	}

	// If the green of our apex colour is less than our target evening green 
	if (mCentreColour.y < kEveningCentreColour.y)
	{
		// Increase green, will cause gradual change.
		mCentreColour.y += kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mCentreColour.y >= kEveningCentreColour.y)
		{
			// Set the apex green to our target green.
			mCentreColour.y = kEveningCentreColour.y;
		}
	}

	////////////////////////
	// blue greater than target
	////////////////////////

	// If the blue of our apex colour is more than our target evening blue 
	if (mApexColour.z > kEveningApexColour.z)
	{
		// Reduce colour from blue, will cause gradual change.
		mApexColour.z -= kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mApexColour.z <= kEveningApexColour.z)
		{
			// Set the apex blue to our target blue.
			mApexColour.z = kEveningApexColour.z;
		}
	}

	// If the blue of our apex colour is more than our target evening blue 
	if (mCentreColour.z > kEveningCentreColour.z)
	{
		// Reduce colour from blue, will cause gradual change.
		mCentreColour.z -= kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mCentreColour.z <= kEveningCentreColour.z)
		{
			// Set the apex blue to our target blue.
			mCentreColour.z = kEveningCentreColour.z;
		}
	}

	////////////////////////
	// blue less than target
	////////////////////////

	// If the blue of our apex colour is less than our target evening blue 
	if (mApexColour.z < kEveningApexColour.z)
	{
		// Increase blue, will cause gradual change.
		mApexColour.z += kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mApexColour.z >= kEveningApexColour.z)
		{
			// Set the apex blue to our target blue.
			mApexColour.z = kEveningApexColour.z;
		}
	}

	// If the blue of our apex colour is less than our target evening blue 
	if (mCentreColour.z < kEveningCentreColour.z)
	{
		// Increase blue, will cause gradual change.
		mCentreColour.z += kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mCentreColour.z >= kEveningCentreColour.z)
		{
			// Set the apex blue to our target blue.
			mCentreColour.z = kEveningCentreColour.z;
		}
	}

	if (mCentreColour == kEveningCentreColour && mApexColour == kEveningApexColour)
	{
		mIsDayTime = false;
		mIsEveningTime = true;
		mIsNightTime = false;
		return true;
	}

	return false;
}

bool CSkyBox::UpdateToNight(float updateTime)
{
	////////////////////////
	// Red greater than target
	////////////////////////

	// If the red of our apex colour is more than our target evening red 
	if (mApexColour.x > kNightApexColour.x)
	{
		// Reduce colour from red, will cause gradual change.
		mApexColour.x -= kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mApexColour.x <= kNightApexColour.x)
		{
			// Set the apex red to our target red.
			mApexColour.x = kNightApexColour.x;
		}
	}


	// If the red of our apex colour is more than our target evening red 
	if (mCentreColour.x > kNightCentreColour.x)
	{
		// Reduce colour from red, will cause gradual change.
		mCentreColour.x -= kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mCentreColour.x <= kNightCentreColour.x)
		{
			// Set the apex red to our target red.
			mCentreColour.x = kNightCentreColour.x;
		}
	}

	////////////////////////
	// Red less than target
	////////////////////////

	// If the red of our apex colour is less than our target evening red 
	if (mApexColour.x < kNightApexColour.x)
	{
		// Increase red, will cause gradual change.
		mApexColour.x += kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mApexColour.x >= kNightApexColour.x)
		{
			// Set the apex red to our target red.
			mApexColour.x = kNightApexColour.x;
		}
	}

	// If the red of our apex colour is less than our target evening red 
	if (mCentreColour.x < kNightCentreColour.x)
	{
		// Increase red, will cause gradual change.
		mCentreColour.x += kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mCentreColour.x >= kNightCentreColour.x)
		{
			// Set the apex red to our target red.
			mCentreColour.x = kNightCentreColour.x;
		}
	}

	////////////////////////
	// Green greater than target
	////////////////////////

	// If the green of our apex colour is more than our target evening green 
	if (mApexColour.y > kNightApexColour.y)
	{
		// Reduce colour from green, will cause gradual change.
		mApexColour.y -= kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mApexColour.y <= kNightApexColour.y)
		{
			// Set the apex green to our target green.
			mApexColour.y = kNightApexColour.y;
		}
	}

	// If the green of our apex colour is more than our target evening green 
	if (mCentreColour.y > kNightCentreColour.y)
	{
		// Reduce colour from green, will cause gradual change.
		mCentreColour.y -= kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mCentreColour.y <= kNightCentreColour.y)
		{
			// Set the apex green to our target green.
			mCentreColour.y = kNightCentreColour.y;
		}
	}

	////////////////////////
	// Green less than target
	////////////////////////

	// If the green of our apex colour is less than our target evening green 
	if (mApexColour.y < kNightApexColour.y)
	{
		// Increase green, will cause gradual change.
		mApexColour.y += kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mApexColour.y >= kNightApexColour.y)
		{
			// Set the apex green to our target green.
			mApexColour.y = kNightApexColour.y;
		}
	}

	// If the green of our apex colour is less than our target evening green 
	if (mCentreColour.y < kNightCentreColour.y)
	{
		// Increase green, will cause gradual change.
		mCentreColour.y += kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mCentreColour.y >= kNightCentreColour.y)
		{
			// Set the apex green to our target green.
			mCentreColour.y = kNightCentreColour.y;
		}
	}

	////////////////////////
	// blue greater than target
	////////////////////////

	// If the blue of our apex colour is more than our target evening blue 
	if (mApexColour.z > kNightApexColour.z)
	{
		// Reduce colour from blue, will cause gradual change.
		mApexColour.z -= kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mApexColour.z <= kNightApexColour.z)
		{
			// Set the apex blue to our target blue.
			mApexColour.z = kNightApexColour.z;
		}
	}

	// If the blue of our apex colour is more than our target evening blue 
	if (mCentreColour.z > kNightCentreColour.z)
	{
		// Reduce colour from blue, will cause gradual change.
		mCentreColour.z -= kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mCentreColour.z <= kNightCentreColour.z)
		{
			// Set the apex blue to our target blue.
			mCentreColour.z = kNightCentreColour.z;
		}
	}

	////////////////////////
	// blue less than target
	////////////////////////

	// If the blue of our apex colour is less than our target evening blue 
	if (mApexColour.z < kNightApexColour.z)
	{
		// Increase blue, will cause gradual change.
		mApexColour.z += kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mApexColour.z >= kNightApexColour.z)
		{
			// Set the apex blue to our target blue.
			mApexColour.z = kNightApexColour.z;
		}
	}

	// If the blue of our apex colour is less than our target evening blue 
	if (mCentreColour.z < kNightCentreColour.z)
	{
		// Increase blue, will cause gradual change.
		mCentreColour.z += kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mCentreColour.z >= kNightCentreColour.z)
		{
			// Set the apex blue to our target blue.
			mCentreColour.z = kNightCentreColour.z;
		}
	}

	if (mCentreColour == kNightCentreColour && mApexColour == kNightApexColour)
	{
		mIsDayTime = false;
		mIsEveningTime = false;
		mIsNightTime = true;
		return true;
	}

	return false;
}

bool CSkyBox::UpdateToDay(float updateTime)
{

	////////////////////////
	// Red greater than target
	////////////////////////

	// If the red of our apex colour is more than our target evening red 
	if (mApexColour.x > kDayApexColour.x)
	{
		// Reduce colour from red, will cause gradual change.
		mApexColour.x -= kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mApexColour.x <= kDayApexColour.x)
		{
			// Set the apex red to our target red.
			mApexColour.x = kDayApexColour.x;
		}
	}


	// If the red of our apex colour is more than our target evening red 
	if (mCentreColour.x > kDayCentreColour.x)
	{
		// Reduce colour from red, will cause gradual change.
		mCentreColour.x -= kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mCentreColour.x <= kDayCentreColour.x)
		{
			// Set the apex red to our target red.
			mCentreColour.x = kDayCentreColour.x;
		}
	}

	////////////////////////
	// Red less than target
	////////////////////////

	// If the red of our apex colour is less than our target evening red 
	if (mApexColour.x < kDayApexColour.x)
	{
		// Increase red, will cause gradual change.
		mApexColour.x += kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mApexColour.x >= kDayApexColour.x)
		{
			// Set the apex red to our target red.
			mApexColour.x = kDayApexColour.x;
		}
	}

	// If the red of our apex colour is less than our target evening red 
	if (mCentreColour.x < kDayCentreColour.x)
	{
		// Increase red, will cause gradual change.
		mCentreColour.x += kColourChangeModifier * updateTime;

		// If the red of the apex colour has gone below our target.
		if (mCentreColour.x >= kDayCentreColour.x)
		{
			// Set the apex red to our target red.
			mCentreColour.x = kDayCentreColour.x;
		}
	}

	////////////////////////
	// Green greater than target
	////////////////////////

	// If the green of our apex colour is more than our target evening green 
	if (mApexColour.y > kDayApexColour.y)
	{
		// Reduce colour from green, will cause gradual change.
		mApexColour.y -= kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mApexColour.y <= kDayApexColour.y)
		{
			// Set the apex green to our target green.
			mApexColour.y = kDayApexColour.y;
		}
	}

	// If the green of our apex colour is more than our target evening green 
	if (mCentreColour.y > kDayCentreColour.y)
	{
		// Reduce colour from green, will cause gradual change.
		mCentreColour.y -= kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mCentreColour.y <= kDayCentreColour.y)
		{
			// Set the apex green to our target green.
			mCentreColour.y = kDayCentreColour.y;
		}
	}

	////////////////////////
	// Green less than target
	////////////////////////

	// If the green of our apex colour is less than our target evening green 
	if (mApexColour.y < kDayApexColour.y)
	{
		// Increase green, will cause gradual change.
		mApexColour.y += kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mApexColour.y >= kDayApexColour.y)
		{
			// Set the apex green to our target green.
			mApexColour.y = kDayApexColour.y;
		}
	}

	// If the green of our apex colour is less than our target evening green 
	if (mCentreColour.y < kDayCentreColour.y)
	{
		// Increase green, will cause gradual change.
		mCentreColour.y += kColourChangeModifier * updateTime;

		// If the green of the apex colour has gone below our target.
		if (mCentreColour.y >= kDayCentreColour.y)
		{
			// Set the apex green to our target green.
			mCentreColour.y = kDayCentreColour.y;
		}
	}

	////////////////////////
	// blue greater than target
	////////////////////////

	// If the blue of our apex colour is more than our target evening blue 
	if (mApexColour.z > kDayApexColour.z)
	{
		// Reduce colour from blue, will cause gradual change.
		mApexColour.z -= kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mApexColour.z <= kDayApexColour.z)
		{
			// Set the apex blue to our target blue.
			mApexColour.z = kDayApexColour.z;
		}
	}

	// If the blue of our apex colour is more than our target evening blue 
	if (mCentreColour.z > kDayCentreColour.z)
	{
		// Reduce colour from blue, will cause gradual change.
		mCentreColour.z -= kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mCentreColour.z <= kDayCentreColour.z)
		{
			// Set the apex blue to our target blue.
			mCentreColour.z = kDayCentreColour.z;
		}
	}

	////////////////////////
	// blue less than target
	////////////////////////

	// If the blue of our apex colour is less than our target evening blue 
	if (mApexColour.z < kDayApexColour.z)
	{
		// Increase blue, will cause gradual change.
		mApexColour.z += kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mApexColour.z >= kDayApexColour.z)
		{
			// Set the apex blue to our target blue.
			mApexColour.z = kDayApexColour.z;
		}
	}

	// If the blue of our apex colour is less than our target evening blue 
	if (mCentreColour.z < kDayCentreColour.z)
	{
		// Increase blue, will cause gradual change.
		mCentreColour.z += kColourChangeModifier * updateTime;

		// If the blue of the apex colour has gone below our target.
		if (mCentreColour.z >= kDayCentreColour.z)
		{
			// Set the apex blue to our target blue.
			mCentreColour.z = kDayCentreColour.z;
		}
	}

	if (mCentreColour == kDayCentreColour && mApexColour == kDayApexColour)
	{
		mIsDayTime = true;
		mIsEveningTime = false;
		mIsNightTime = false;
		return true;
	}

	return false;
}

int CSkyBox::GetIndexCount()
{
	return mIndexCount;
}

D3DXVECTOR4 CSkyBox::GetApexColor()
{
	return mApexColour;
}

D3DXVECTOR4 CSkyBox::GetCenterColour()
{
	return mCentreColour;
}

bool CSkyBox::InitialiseBuffers(ID3D11Device * device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	D3D11_SUBRESOURCE_DATA indexData;
	HRESULT result;

	vertices = new VertexType[mVertexCount];
	for (unsigned int i = 0; i < mVertexCount; i++)
	{
		vertices[i].position = mpVerticesList[i];
	}

	indices = new unsigned long[mIndexCount];
	for (unsigned int i = 0; i < mIndexCount; i++)
	{
		indices[i] = mpIndicesList[i];
	}

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * mVertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &mpVertexBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the vertex buffer for the skybox.");
		return false;
	}

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * mIndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexBufferDesc, &indexData, &mpIndexBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the index buffeer for the skybox.");
		return false;
	}

	delete[] vertices;
	vertices = nullptr;
	delete[] indices;
	indices = nullptr;

	return true;
}

bool CSkyBox::LoadSkyBoxModel(char * modelName)
{
	// Grab the mesh object for the last mesh we loaded.

	Assimp::Importer importer;
	const std::string name = modelName;
	logger->GetInstance().WriteLine("Attempting to open " + name + " using Assimp.");

	// Read in the file, store this mesh in the scene.
	const aiScene* scene = importer.ReadFile(modelName,
		aiProcess_ConvertToLeftHanded |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_SortByPType);

	// If scene hasn't been initialised then something has gone wrong!
	if (!scene)
	{
		logger->GetInstance().WriteLine(importer.GetErrorString());
		logger->GetInstance().WriteLine("Failed to create scene.");
		return nullptr;
	}

	// Iterate through all our meshes to be loaded.
	for (unsigned int meshCount = 0; meshCount < scene->mNumMeshes; meshCount++)
	{
		// Load the current mesh.
		const aiMesh& mesh = *scene->mMeshes[meshCount];
	
		// Store info about the mesh.
		unsigned int numFaces = mesh.mNumFaces;
		unsigned int numVertices = mesh.mNumVertices;

		for (unsigned int vertexCount = 0; vertexCount < numVertices; vertexCount++)
		{
			// Parse a singular vertex info.

			const aiVector3D& vertexCoords = mesh.mVertices[vertexCount];

			// Load the vertex info into our array.
			mpVerticesList.push_back(D3DXVECTOR3(vertexCoords.x, vertexCoords.y, vertexCoords.z));

			// Parse information on the UV of a singular vertex.
			const aiVector3D& textureCoords = mesh.mTextureCoords[0][vertexCount];

			// Only need to parse the U and V channels.
			mpUVList.push_back(D3DXVECTOR2(textureCoords.x, textureCoords.y));

			// Load the normals data of this singular vertex.
			const aiVector3D& normals = mesh.mNormals[vertexCount];

			// Store this normals data in our array.
			mpNormalsList.push_back(D3DXVECTOR3(normals.x, normals.y, normals.z));
		}

		for (unsigned int faceCount = 0; faceCount < numFaces; faceCount++)
		{
			const aiFace& face = mesh.mFaces[faceCount];

			for (unsigned int indexCount = 0; indexCount < face.mNumIndices; indexCount++)
			{
				mpIndicesList.push_back(face.mIndices[indexCount]);
			}
		}
	}

	logger->GetInstance().WriteLine("Successfully initialised our arrays for mesh '" + name + "'. ");

	// Close off this section in the log.
	logger->GetInstance().CloseSubtitle();

	mVertexCount = mpVerticesList.size();
	mIndexCount = mpIndicesList.size();
	// Success!
	return true;
}

void CSkyBox::ReleaseSkyBoxModel()
{
}
