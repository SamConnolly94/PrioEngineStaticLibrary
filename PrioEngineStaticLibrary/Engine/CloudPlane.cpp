#include "CloudPlane.h"



CCloudPlane::CCloudPlane()
{
	mpCloudPlane = nullptr;
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
	mpCloudTexture1 = nullptr;
	mpCloudTexture2 = nullptr;
	mVertexCount = 0;
	mIndexCount = 0;
}


CCloudPlane::~CCloudPlane()
{
}

bool CCloudPlane::Initialise(ID3D11Device * device, std::string textureFilename1, std::string textureFilename2)
{
	int planeResolution = 10;
	int textureRepeat = 2;
	float planeWidth = 10.0f;
	float planeTop = 0.5f;
	float planeBottom = 0.0f;

	// Control just how white the clouds will be.
	mBrightness = 0.7f;

	// Cloud texture 1 movement
	mMovementSpeed[0] = D3DXVECTOR2(0.015f, 0.0f);
	// Cloud texture 2 movement speed
	mMovementSpeed[1] = D3DXVECTOR2(0.0075f, 0.0f);
	
	// Initialise the current translation speeds.
	mTextureMovement[0] = D3DXVECTOR2(0.0f, 0.0f);
	mTextureMovement[1] = D3DXVECTOR2(0.0f, 0.0f);

	bool result = InitialisePlane(planeResolution, planeWidth, planeTop, planeBottom, textureRepeat);
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise cloud plane.");
		return false;
	}

	result = InitialiseBuffers(device, planeResolution);
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise buffers for cloud plane.");
		return false;
	}

	result = LoadCloudTextures(device, textureFilename1, textureFilename2);
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to load the cloud textures in cloud plane.");
		return false;
	}

	return true;
}

bool CCloudPlane::Render(ID3D11DeviceContext * deviceContext)
{
	RenderBuffers(deviceContext);
	return true;
}

void CCloudPlane::Shutdown()
{
	ReleaseCloudTextures();
	ShutdownBuffers();
	ShutdownPlane();
}

void CCloudPlane::Update(float updateTime)
{
	mTextureMovement[0].x += mMovementSpeed[0].x * updateTime;
	mTextureMovement[0].y += mMovementSpeed[0].y * updateTime;
	mTextureMovement[1].x += mMovementSpeed[1].x * updateTime;
	mTextureMovement[1].y += mMovementSpeed[1].y * updateTime;

	if (mTextureMovement[0].x > 1.0f)
	{
		mTextureMovement[0].x = 0.0f;
	}
	if (mTextureMovement[0].y > 1.0f)
	{
		mTextureMovement[0].y = 0.0f;
	}
	if (mTextureMovement[1].x > 1.0f)
	{
		mTextureMovement[1].x = 0.0f;
	}
	if (mTextureMovement[1].y > 1.0f)
	{
		mTextureMovement[1].y = 0.0f;
	}
}

int CCloudPlane::GetIndexCount()
{
	return mIndexCount;
}

ID3D11ShaderResourceView * CCloudPlane::GetCloudTexture1()
{
	return mpCloudTexture1->GetTexture();
}

ID3D11ShaderResourceView * CCloudPlane::GetCloudTexture2()
{
	return mpCloudTexture2->GetTexture();
}

float CCloudPlane::GetBrightness()
{
	return mBrightness;
}

D3DXVECTOR2 CCloudPlane::GetMovement(int cloudIndex)
{
	return mTextureMovement[cloudIndex];
}

bool CCloudPlane::InitialisePlane(int planeResolution, float planeWidth, float planeTop, float planeBottom, int textureRepeat)
{
	mpCloudPlane = new VertexType[(planeResolution + 1) * (planeResolution + 1)];
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpCloudPlane).name());
	
	float quadSize = planeWidth / static_cast<float>(planeResolution);
	float radius = planeWidth / 2.0f;
	float constant = (planeTop - planeBottom) / (radius * radius);
	float texStep = static_cast<float>(textureRepeat) / static_cast<float>(planeResolution);

	for (int heightCount = 0; heightCount <= planeResolution; heightCount++)
	{
		for (int widthCount = 0; widthCount <= planeResolution; widthCount++)
		{
			D3DXVECTOR3 pos;
			float widthPos = (-0.5f * planeWidth);
			pos.x = widthPos + (static_cast<float>(widthCount) * quadSize);
			pos.z = widthPos + (static_cast<float>(heightCount) * quadSize);
			pos.y = planeTop - (constant * ((pos.x * pos.x) + (pos.z * pos.z)));

			D3DXVECTOR2 uv;
			uv.x = static_cast<float>(widthCount) * texStep;
			uv.y = static_cast<float>(heightCount) * texStep;

			int index = heightCount * (planeResolution + 1) + widthCount;
			mpCloudPlane[index].position = pos;
			mpCloudPlane[index].uv = uv;
		}
	}
	return true;
}

void CCloudPlane::ShutdownPlane()
{
	if (mpCloudPlane)
	{
		delete[] mpCloudPlane;
		mpCloudPlane = nullptr;
	}
}

bool CCloudPlane::InitialiseBuffers(ID3D11Device * device, int planeResolution)
{
	VertexType* vertices;
	unsigned int* indices;
	HRESULT result;

	mVertexCount = (planeResolution + 1) * (planeResolution + 1) * 6;
	mIndexCount = mVertexCount;

	vertices = new VertexType[mVertexCount];
	indices = new unsigned int[mIndexCount];

	int vertex = 0;

	for (int heightCount = 0; heightCount < planeResolution; heightCount++)
	{
		for (int widthCount = 0; widthCount < planeResolution; widthCount++)
		{
			int index1 = heightCount * (planeResolution + 1) + widthCount;
			int index2 = heightCount * (planeResolution + 1) + (widthCount + 1);
			int index3 = (heightCount + 1) * (planeResolution + 1) + widthCount;
			int index4 = (heightCount + 1) * (planeResolution + 1) + (widthCount + 1);

			// Top left
			vertices[vertex].position = mpCloudPlane[index1].position;
			vertices[vertex].uv = mpCloudPlane[index1].uv;
			indices[vertex] = vertex;
			vertex++;

			// Top right
			vertices[vertex].position = mpCloudPlane[index2].position;
			vertices[vertex].uv = mpCloudPlane[index2].uv;
			indices[vertex] = vertex;
			vertex++;

			// Bottom left
			vertices[vertex].position = mpCloudPlane[index3].position;
			vertices[vertex].uv = mpCloudPlane[index3].uv;
			indices[vertex] = vertex;
			vertex++;

			// Bottom left
			// No need to calc index, it hasn't changed.
			vertices[vertex].position = mpCloudPlane[index3].position;
			vertices[vertex].uv = mpCloudPlane[index3].uv;
			indices[vertex] = vertex;
			vertex++;


			// Top right
			vertices[vertex].position = mpCloudPlane[index2].position;
			vertices[vertex].uv = mpCloudPlane[index2].uv;
			indices[vertex] = vertex;
			vertex++;

			// Bottom right
			vertices[vertex].position = mpCloudPlane[index4].position;
			vertices[vertex].uv = mpCloudPlane[index4].uv;
			indices[vertex] = vertex;
			vertex++;
		}
	}

	// Setup the descriptor of the vertex buffer.
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage					= D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth				= sizeof(VertexType) * mVertexCount;
	vertexBufferDesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags			= 0;
	vertexBufferDesc.MiscFlags				= 0;
	vertexBufferDesc.StructureByteStride	= 0;

	// Setup the descriptor for initial vertex data to be used in the vertex buffer.
	D3D11_SUBRESOURCE_DATA vertexData;
	vertexData.pSysMem						= vertices;
	vertexData.SysMemPitch					= 0;
	vertexData.SysMemSlicePitch				= 0;

	// Create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &mpVertexBuffer);

	// If we failed to create the vertex buffer, return unsuccessful.
	if (FAILED(result))
	{
		// Output error message to the debug log.
		logger->GetInstance().WriteLine("Failed to create the vertex buffer for cloud plane.");
		// Return early, failed setup.
		return false;
	}

	// Set up the descriptor for the index buffer.
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage					= D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth				= sizeof(unsigned int) * mIndexCount;
	indexBufferDesc.BindFlags				= D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags			= 0;
	indexBufferDesc.MiscFlags				= 0;
	indexBufferDesc.StructureByteStride		= 0;

	// Set up the descriptor for the initial data in the index buffer.
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem						= indices;
	indexData.SysMemPitch					= 0;
	indexData.SysMemSlicePitch				= 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &mpIndexBuffer);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the index buffer for cloud plane.");
		return false;
	}

	delete[] vertices;
	delete[] indices;
	vertices	= nullptr;
	indices		= nullptr;

	return true; 
}

void CCloudPlane::ShutdownBuffers()
{
	if (mpIndexBuffer)
	{
		mpIndexBuffer->Release();
		mpIndexBuffer = nullptr;
	}

	if (mpVertexBuffer)
	{
		mpVertexBuffer->Release();
		mpVertexBuffer = nullptr;
	}
}

void CCloudPlane::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Define the size of each vertex element placed on the pipeline.
	stride = sizeof(VertexType);
	// Define the starting position.
	offset = 0;

	// Place vertex and index data on the pipeline to be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

bool CCloudPlane::LoadCloudTextures(ID3D11Device * device, std::string filename1, std::string filename2)
{
	mpCloudTexture1 = new CTexture();
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpCloudTexture1).name());
	mpCloudTexture2 = new CTexture();
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpCloudTexture2).name());

	bool result = mpCloudTexture1->Initialise(device, filename1);
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise the first cloud texture for cloud plane.");
		return false;
	}

	result = mpCloudTexture2->Initialise(device, filename2);
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise the second cloud texture for cloud plane.");
		return false;
	}

	return true;
}

void CCloudPlane::ReleaseCloudTextures()
{
	if (mpCloudTexture1 != nullptr)
	{
		mpCloudTexture1->Shutdown();
		delete mpCloudTexture1;
		mpCloudTexture1 = nullptr;
	}

	if (mpCloudTexture2 != nullptr)
	{
		mpCloudTexture2->Shutdown();
		delete mpCloudTexture2;
		mpCloudTexture2 = nullptr;
	}
}
