#include "Triangle.h"



CTriangle::CTriangle(WCHAR* filename)
{
	// Initialise all variables to be null.
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
	mpTexture = nullptr;
	mpTextureFilename = nullptr;
	ResetColour();
	mUseDiffuseLighting = false;

	// Store the filename for later use.
	mpTextureFilename = filename;

	mShape = PrioEngine::Primitives::triangle;

	mpVertexManager = new CVertexManager(PrioEngine::VertexType::Texture, PrioEngine::Primitives::triangle);
	mpLogger->GetLogger().MemoryAllocWriteLine(typeid(mpVertexManager).name());
}

CTriangle::CTriangle(WCHAR * filename, bool useLighting)
{
	// Initialise all variables to be null.
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
	mpTexture = nullptr;
	mpTextureFilename = nullptr;
	ResetColour();

	// Set the flag to use diffuse lighting on our texture.
	mUseDiffuseLighting = useLighting;

	// Store the filename for later use.
	mpTextureFilename = filename;

	mShape = PrioEngine::Primitives::triangle;

	if (useLighting)
	{
		mpVertexManager = new CVertexManager(PrioEngine::VertexType::Diffuse, PrioEngine::Primitives::triangle);
	}
	else
	{
		mpVertexManager = new CVertexManager(PrioEngine::VertexType::Texture, PrioEngine::Primitives::triangle);
	}
	mpLogger->GetLogger().MemoryAllocWriteLine(typeid(mpVertexManager).name());
}

CTriangle::CTriangle(PrioEngine::RGBA colour)
{
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
	mpTexture = nullptr;
	mpTextureFilename = nullptr;
	mUseDiffuseLighting = false;
	ResetColour();

	// Store the colour which we have passed in.
	mColour = colour;

	mShape = PrioEngine::Primitives::triangle;

	mpVertexManager = new CVertexManager(PrioEngine::VertexType::Colour, PrioEngine::Primitives::triangle);
	mpLogger->GetLogger().MemoryAllocWriteLine(typeid(mpVertexManager).name());

	mpVertexManager->SetColour(colour);
}


CTriangle::~CTriangle()
{
}

bool CTriangle::Initialise(ID3D11Device * device)
{
	mpDevice = device;
	mpVertexManager->SetDevicePtr(device);
	bool result;
	mApplyTexture = mpTextureFilename != nullptr;

	// Initialise the vertex and index buffer that hold geometry for the triangle.
	result = InitialiseBuffers(device);
	if (!result)
		return false;

	// Load the texture for this model.

	if (mApplyTexture)
	{
		result = LoadTexture(device);
		if (!result)
		{
			return false;
		}
	}

	// Return our success / failure to init the vertex and index buffer.
	return true;
}

bool CTriangle::InitialiseBuffers(ID3D11Device * device)
{
	unsigned long* indices;
	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA indexData;
	HRESULT result;

	// Set the number of vertices in the vertex array.
	mVertexCount = PrioEngine::Triangle::kNumOfVertices;
	mpVertexManager->SetNumberOfVertices(mVertexCount);

	// Set the number of indices on the index array.
	mIndexCount = PrioEngine::Triangle::kNumOfIndices;

	// Create a vertex array
	mpVertexManager->CreateVertexArray();

	// Create the points of the model.
	mpVertexManager->SetVertexArray(0.0f, 0.0f, 0.0f);

	// Create the index array.
	indices = new unsigned long[mIndexCount];
	mpLogger->GetLogger().MemoryAllocWriteLine(typeid(indices).name());
	if (!indices)
	{
		return false;
	}

	// Load the index array with data according to the predefined indicies defined in the engines namespace.
	for (int i = 0; i < PrioEngine::Triangle::kNumOfIndices; i++)
	{
		indices[i] = PrioEngine::Triangle::indices[i];
	}

	// Create the vertex buffer.
	if (!mpVertexManager->CreateVertexBuffer())
	{
		return false;
	}

	/* Set up the descriptor of the index buffer. */
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * mIndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	/* Give the subresource structure a pointer to the index data. */
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &mpIndexBuffer);
	if (FAILED(result))
	{
		return false;
	}

	mpVertexManager->CleanArrays();

	delete[] indices;
	indices = nullptr;
	mpLogger->GetLogger().MemoryDeallocWriteLine(typeid(indices).name());

	return true;
}
