#include "VertexTypeManager.h"


CVertexManager::CVertexManager(PrioEngine::ShaderType vertexType)
{
	// Set any pointers that are to be used to be null.
	mpVerticesTexture = nullptr;
	mpVerticesColour = nullptr;
	mpVerticesDiffuse = nullptr;
	mpVerticesSpecular = nullptr;
	mpVertexBuffer = nullptr;

	// We'll need to load in the number of vertices.
	mNumOfVertices = 0;

	mShaderType = vertexType;
}

CVertexManager::CVertexManager(PrioEngine::ShaderType vertexType, PrioEngine::Primitives shape)
{
	// Set any pointers that are to be used to be null.
	mpVerticesTexture = nullptr;
	mpVerticesColour = nullptr;
	mpVerticesDiffuse = nullptr;
	mpVertexBuffer = nullptr;

	// We'll need to load in the number of vertices.
	mNumOfVertices = 0;

	// Track what type of vertex array we will be using.
	mShaderType = vertexType;

	// Track what shape we are drawing.
	mShape = shape;
}

CVertexManager::~CVertexManager()
{
}

/* Clean up any memory we used. */
void CVertexManager::CleanArrays()
{
	// Release the local arrays for vertex and index buffers.
	if (mpVerticesColour != nullptr)
	{
		delete[] mpVerticesColour;
		mpVerticesColour = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpVerticesColour).name());
	}

	if (mpVerticesTexture != nullptr)
	{
		delete[] mpVerticesTexture;
		mpVerticesTexture = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpVerticesTexture).name());
	}

	if (mpVerticesDiffuse != nullptr)
	{
		delete[] mpVerticesDiffuse;
		mpVerticesDiffuse = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpVerticesDiffuse).name());
	}

	if (mpVerticesSpecular != nullptr)
	{
		delete[] mpVerticesSpecular;
		mpVerticesSpecular = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpVerticesDiffuse).name());
	}
}

// Sets the pointer to the device, this will be required when creating buffers.
void CVertexManager::SetDevicePtr(ID3D11Device* device)
{
	// Stash away our pointer to the device, we'll need it when we create the vertex buffers.
	mpDevice = device;
}

/* Allocate memory to the appropriate vertex array type. */
void CVertexManager::CreateVertexArray()
{
	if (mShaderType == PrioEngine::ShaderType::Colour)
	{
		CreateColourVerticesArray();
	}
	else if (mShaderType == PrioEngine::ShaderType::Texture)
	{
		CreateTextureVerticesArray();
	}
	else if (mShaderType == PrioEngine::ShaderType::Diffuse)
	{
		CreateDiffuseVerticesArray();
	}
	else if (mShaderType == PrioEngine::ShaderType::Specular)
	{
		CreateSpecularVerticesArray();
	}
}

/* Set the number of vertices in this shape. */
void CVertexManager::SetNumberOfVertices(int amount)
{
	mNumOfVertices = amount;
}

/* Initialise the colour vertex array, for colouring in with solid colours. */
void CVertexManager::CreateColourVerticesArray()
{
	mpVerticesColour = new VertexColourType[mNumOfVertices];
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpVerticesColour).name());

	if (!mpVerticesColour)
	{
		logger->GetInstance().WriteLine("Failed to create a vertex array for colour.");
	}
}

/* Initialise the texture vertex array, for use of textures on models. */
void CVertexManager::CreateTextureVerticesArray()
{
	mpVerticesTexture = new VertexTextureType[mNumOfVertices];
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpVerticesTexture).name());
	
	if (!mpVerticesTexture)
	{
		logger->GetInstance().WriteLine("Failed to create a vertex array for texture.");
	}
}

/* Initialise the diffuse vertex array, for using diffuse light ontop of textures. */
void CVertexManager::CreateDiffuseVerticesArray()
{
	mpVerticesDiffuse = new VertexDiffuseLightingType[mNumOfVertices];
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpVerticesDiffuse).name());

	if (!mpVerticesDiffuse)
	{
		logger->GetInstance().WriteLine("Failed to create a vertex array for texture using diffuse lighting.");
	}
}

/* Initialise the specular vertex array, for using diffuse light ontop of textures. */
void CVertexManager::CreateSpecularVerticesArray()
{
	mpVerticesSpecular = new VertexSpecularLightingType[mNumOfVertices];
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpVerticesDiffuse).name());

	if (!mpVerticesSpecular)
	{
		logger->GetInstance().WriteLine("Failed to create a vertex array for texture using diffuse lighting.");
	}
}

/* Initialise the buffers of the child model. */
void CVertexManager::SetVertexArray(float x, float y, float z)
{
	// Depending on which shape we are drawing, initialise the buffers appropriately.
	switch (mShape)
	{
		// If we're drawing a cube.
	case PrioEngine::Primitives::cube:
		// Determine which vertex type we are using.
		switch (mShaderType)
		{
		case PrioEngine::ShaderType::Colour:
			SetColourCube(x, y, z);
			break;
		case PrioEngine::ShaderType::Texture:
			SetTextureCube(x, y, z);
			break;
		case PrioEngine::ShaderType::Diffuse:
			SetDiffuseCube(x, y, z);
			break;
		case PrioEngine::ShaderType::Specular:
			SetSpecularCube(x, y, z);
		}
		return;
	case PrioEngine::Primitives::triangle:
	{
		// Determine which vertex type we are using.
		switch (mShaderType)
		{
		case PrioEngine::ShaderType::Colour:
			SetColourTriangle(x, y, z);
			break;
		case PrioEngine::ShaderType::Texture:
			SetTextureTriangle(x, y, z);
			break;
		case PrioEngine::ShaderType::Diffuse:
			SetDiffuseTriangle(x, y, z);
			break;
		case PrioEngine::ShaderType::Specular:
			SetSpecularTriangle(x, y, z);
			break;
		}
		return;
	}
	}

	logger->GetInstance().WriteLine("Failed to set any buffers to be drawn.");
}

/* Sets the vertex array of a mesh which has been loaded in. */
void CVertexManager::SetVertexArray(float x, float y, float z, std::vector<D3DXVECTOR3> vertices, std::vector<D3DXVECTOR2> UV, std::vector<D3DXVECTOR3> normals)
{
	/// Normals can be used for a few different kinds of lighting, just throw them in an if statement.
	/// TODO: Move these into functions, it looks messy here. 

	// Diffuse lighting.
	if (mShaderType == PrioEngine::ShaderType::Diffuse)
	{
		if (!mpVerticesDiffuse)
		{
			mpVerticesDiffuse = new VertexDiffuseLightingType[mNumOfVertices];
			logger->GetInstance().MemoryAllocWriteLine(typeid(mpVerticesDiffuse).name());
		}


		// Set the positions of vertices first.
		for (int i = 0; i < mNumOfVertices; i++)
		{
			mpVerticesDiffuse[i].position = vertices[i];
			mpVerticesDiffuse[i].texture = UV[i];
			mpVerticesDiffuse[i].normal = normals[i];

		}
	}
	// Specular lighting.
	else if (mShaderType == PrioEngine::ShaderType::Specular)
	{
		if (!mpVerticesSpecular)
		{
			mpVerticesSpecular = new VertexSpecularLightingType[mNumOfVertices];
			logger->GetInstance().MemoryAllocWriteLine(typeid(mpVerticesSpecular).name());
		}


		// Set the positions of vertices first.
		for (int i = 0; i < mNumOfVertices; i++)
		{
			mpVerticesSpecular[i].position = vertices[i];
			mpVerticesSpecular[i].texture = UV[i];
			mpVerticesSpecular[i].normal = normals[i];

		}
	}
}

/* Sets the vertex array of a mesh which has been loaded in. */
void CVertexManager::SetVertexArray(float x, float y, float z, std::vector<D3DXVECTOR3> vertices, std::vector<D3DXVECTOR2> UV)
{
	if (!mpVerticesTexture)
	{
		mpVerticesTexture = new VertexTextureType[mNumOfVertices];
		logger->GetInstance().MemoryAllocWriteLine(typeid(mpVerticesTexture).name());
	}


	// Set the positions of vertices first.
	for (int i = 0; i < mNumOfVertices; i++)
	{
		mpVerticesTexture[i].position = vertices[i];
		mpVerticesTexture[i].texture = UV[i];

	}
}


/* Sets the vertex array of a mesh which has been loaded in. */
void CVertexManager::SetVertexArray(float x, float y, float z, std::vector<D3DXVECTOR3> vertices, std::vector<D3DXVECTOR4> colours)
{
	if (!mpVerticesColour)
	{
		mpVerticesColour = new VertexColourType[mNumOfVertices];
		logger->GetInstance().MemoryAllocWriteLine(typeid(mpVerticesColour).name());
	}


	// Set the positions of vertices first.
	for (int i = 0; i < mNumOfVertices; i++)
	{
		mpVerticesColour[i].position = vertices[i];
		mpVerticesColour[i].colour = colours[i];

	}
}


/* Place the vertex points positions into our array, for when using colour shader. */
void CVertexManager::SetColourCube(float x, float y, float z)
{
	// Set the positions of vertices first.
	for (int i = 0; i < PrioEngine::Cube::kNumOfVertices; i++)
	{
		// If we're using a solid colour shader, place the position of the vertices into the colour vertex array.
		mpVerticesColour[i].position = D3DXVECTOR3(x + PrioEngine::Cube::kCubeVerticesCoords[i].x,
			y + PrioEngine::Cube::kCubeVerticesCoords[i].y,
			z + PrioEngine::Cube::kCubeVerticesCoords[i].z);

		// Set the colour of each vertex to the outside colour of our cube.
		mpVerticesColour[i].colour = D3DXVECTOR4(mColour.r, mColour.g, mColour.b, mColour.a);
	}
}

/* Place the vertex points positions into our array, for when using texture shader. */
void CVertexManager::SetTextureCube(float x, float y, float z)
{
	float U = 0.0f;
	float V = 0.0f;

	const float additionAmmount = 1.0f;

	// Set the positions of vertices first.
	for (int i = 0; i < PrioEngine::Cube::kNumOfVertices; i++)
	{
		// If we're using a texture without any lighting, place the positions in the texture vertex array.
		mpVerticesTexture[i].position = D3DXVECTOR3(x + PrioEngine::Cube::kCubeVerticesCoords[i].x,
			y + PrioEngine::Cube::kCubeVerticesCoords[i].y,
			z + PrioEngine::Cube::kCubeVerticesCoords[i].z);
		// Tell the vertices buffer what it should use as UV values.
		mpVerticesTexture[i].texture = D3DXVECTOR2(U, V);
		// Cube has been written so it goes across, this will only work if wrap mode is used as the texture address mode.
		if (U == V)
		{
			V += additionAmmount;
		}
		else
		{
			U += additionAmmount;
		}
	}
}

/* Place the vertex points positions into our array, for when using diffuse shader. */
void CVertexManager::SetDiffuseCube(float x, float y, float z)
{
	float U = 0.0f;
	float V = 0.0f;

	// Set the positions of vertices first.
	for (int i = 0; i < PrioEngine::Cube::kNumOfVertices; i++)
	{
		// If we're using a texture combined with diffuse lighting, place the position of the vertices into the diffuse lighting vertex array.
		mpVerticesDiffuse[i].position = D3DXVECTOR3(x + PrioEngine::Cube::kCubeVerticesCoords[i].x,
			y + PrioEngine::Cube::kCubeVerticesCoords[i].y,
			z + PrioEngine::Cube::kCubeVerticesCoords[i].z);
		// Tell the vertices buffer what it should use as UV values.
		mpVerticesDiffuse[i].texture = D3DXVECTOR2(U, V);
		// Cube has been written so it goes across, this will only work if wrap mode is used as the texture address mode.
		if (U == V)
		{
			V += 1.0f;
		}
		else
		{
			U += 1.0f;
		}
	}
}

void CVertexManager::SetSpecularCube(float x, float y, float z)
{
	float U = 0.0f;
	float V = 0.0f;

	// Set the positions of vertices first.
	for (int i = 0; i < PrioEngine::Cube::kNumOfVertices; i++)
	{
		// If we're using a texture combined with diffuse lighting, place the position of the vertices into the diffuse lighting vertex array.
		mpVerticesSpecular[i].position = D3DXVECTOR3(x + PrioEngine::Cube::kCubeVerticesCoords[i].x,
			y + PrioEngine::Cube::kCubeVerticesCoords[i].y,
			z + PrioEngine::Cube::kCubeVerticesCoords[i].z);
		// Tell the vertices buffer what it should use as UV values.
		mpVerticesSpecular[i].texture = D3DXVECTOR2(U, V);

		// Cube has been written so it goes across, this will only work if wrap mode is used as the texture address mode.
		if (U == V)
		{
			V += 1.0f;
		}
		else
		{
			U += 1.0f;
		}
	}
}

/* Place the vertex points positions into our array, for when using colour shader. */
void CVertexManager::SetColourTriangle(float x, float y, float z)
{
	// Set the positions of vertices first.
	for (int i = 0; i < PrioEngine::Triangle::kNumOfVertices; i++)
	{
		/* Set the vertex points for the triangle. */
		mpVerticesColour[i].position = D3DXVECTOR3(x + PrioEngine::Triangle::vertices[i].x,
			y + PrioEngine::Triangle::vertices[i].y,
			z + PrioEngine::Triangle::vertices[i].z);
	}

	// Bottom left
	mpVerticesColour[0].colour = D3DXVECTOR4(mColour.r, mColour.g, mColour.b, mColour.a);
	// Top middle
	mpVerticesColour[1].colour = D3DXVECTOR4(mColour.r, mColour.g, mColour.b, mColour.a);
	// Bottom right
	mpVerticesColour[2].colour = D3DXVECTOR4(mColour.r, mColour.g, mColour.b, mColour.a);
}

/* Place the vertex points positions into our array, for when using texture shader. */
void CVertexManager::SetTextureTriangle(float x, float y, float z)
{
	// Set the positions of vertices first.
	for (int i = 0; i < PrioEngine::Triangle::kNumOfVertices; i++)
	{
		/* Set the vertex points for the triangle. */
		mpVerticesTexture[i].position = D3DXVECTOR3(x + PrioEngine::Triangle::vertices[i].x,
			y + PrioEngine::Triangle::vertices[i].y,
			z + PrioEngine::Triangle::vertices[i].z);

		// Bottom left
		mpVerticesTexture[0].texture = D3DXVECTOR2(0.0f, 1.0f);
		// Top middle
		mpVerticesTexture[1].texture = D3DXVECTOR2(0.5f, 0.0f);
		// Bottom right
		mpVerticesTexture[2].texture = D3DXVECTOR2(1.0f, 1.0f);
	}
}

/* Place the vertex points positions into our array, for when using diffuse shader. */
void CVertexManager::SetDiffuseTriangle(float x, float y, float z)
{
	// Set the positions of vertices first.
	for (int i = 0; i < PrioEngine::Triangle::kNumOfVertices; i++)
	{
		/* Set the vertex points for the triangle. */
		mpVerticesDiffuse[i].position = D3DXVECTOR3(x + PrioEngine::Triangle::vertices[i].x,
			y + PrioEngine::Triangle::vertices[i].y,
			z + PrioEngine::Triangle::vertices[i].z);
	}
	// Bottom left
	mpVerticesDiffuse[0].texture = D3DXVECTOR2(0.0f, 1.0f);
	mpVerticesDiffuse[0].normal = D3DXVECTOR3(0.0f, 0.0f, -1.0f);

	// Top middle
	mpVerticesDiffuse[1].texture = D3DXVECTOR2(0.5f, 0.0f);
	mpVerticesDiffuse[1].normal = D3DXVECTOR3(0.0f, 0.0f, -1.0f);

	// Bottom right
	mpVerticesDiffuse[2].texture = D3DXVECTOR2(1.0f, 1.0f);
	mpVerticesDiffuse[2].normal = D3DXVECTOR3(0.0f, 0.0f, -1.0f);
}

void CVertexManager::SetSpecularTriangle(float x, float y, float z)
{
	// Set the positions of vertices first.
	for (int i = 0; i < PrioEngine::Triangle::kNumOfVertices; i++)
	{
		/* Set the vertex points for the triangle. */
		mpVerticesSpecular[i].position = D3DXVECTOR3(x + PrioEngine::Triangle::vertices[i].x,
			y + PrioEngine::Triangle::vertices[i].y,
			z + PrioEngine::Triangle::vertices[i].z);
	}
	// Bottom left
	mpVerticesSpecular[0].texture = D3DXVECTOR2(0.0f, 1.0f);
	mpVerticesSpecular[0].normal = D3DXVECTOR3(0.0f, 0.0f, -1.0f);

	// Top middle
	mpVerticesSpecular[1].texture = D3DXVECTOR2(0.5f, 0.0f);
	mpVerticesSpecular[1].normal = D3DXVECTOR3(0.0f, 0.0f, -1.0f);

	// Bottom right
	mpVerticesSpecular[2].texture = D3DXVECTOR2(1.0f, 1.0f);
	mpVerticesSpecular[2].normal = D3DXVECTOR3(0.0f, 0.0f, -1.0f);
}

/* Set the colour of the shape. Note: Will not override any texture. */
void CVertexManager::SetColour(PrioEngine::RGBA colour)
{
	mColour = colour;
}

/* Creates a buffer and fills it with the data we have already found about the current shape. */
bool CVertexManager::CreateVertexBuffer()
{
	HRESULT hResult;
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	/* Set up the descriptor for the vertex buffer. */

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	switch (mShaderType)
	{
	case PrioEngine::ShaderType::Colour:
		vertexBufferDesc.ByteWidth = sizeof(VertexColourType) * mNumOfVertices;
		// Give the subresource struct a pointer to the vertex data.
		vertexData.pSysMem = mpVerticesColour;
		break;
	case PrioEngine::ShaderType::Texture:
		vertexBufferDesc.ByteWidth = sizeof(VertexTextureType) * mNumOfVertices;
		// Give the subresource struct a pointer to the vertex data.
		vertexData.pSysMem = mpVerticesTexture;
		break;
	case PrioEngine::ShaderType::Diffuse:
		vertexBufferDesc.ByteWidth = sizeof(VertexDiffuseLightingType) * mNumOfVertices;
		// Give the subresource struct a pointer to the vertex data.
		vertexData.pSysMem = mpVerticesDiffuse;
		break;
	case PrioEngine::ShaderType::Specular:
		vertexBufferDesc.ByteWidth = sizeof(VertexSpecularLightingType) * mNumOfVertices;
		// Give the subresource struct a pointer to the vertex data.
		vertexData.pSysMem = mpVerticesSpecular;
		break;
	default:
		logger->GetInstance().WriteLine("Failed to find any vertex type. This prevents us from creating a vertex buffer.");
		return false;
	}

	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	/* Create the vertex buffer. */

	if (mpDevice == nullptr)
	{
		logger->GetInstance().WriteLine("The device hasn't been initialised, you need to pass it into the VertexManager class first.");
	}

	hResult = mpDevice->CreateBuffer(&vertexBufferDesc, &vertexData, &mpVertexBuffer);
	if (FAILED(hResult))
	{
		logger->GetInstance().WriteLine("Failed to create the vertex buffer.");
		return false;
	}

	return true;
}

/* Sets last second key info before rendering, and then tells DirectX to render. */
void CVertexManager::RenderBuffers(ID3D11DeviceContext * deviceContext, ID3D11Buffer* indexBuffer)
{
	unsigned int stride;
	unsigned int offset;

	// Detect which type of vertex we are rendering, and set the stride to the size of that.
	switch (mShaderType)
	{
	case PrioEngine::ShaderType::Colour:
		stride = sizeof(VertexColourType);
		break;
	case PrioEngine::ShaderType::Texture:
		stride = sizeof(VertexTextureType);
		break;
	case PrioEngine::ShaderType::Diffuse:
		stride = sizeof(VertexDiffuseLightingType);
		break;
	case PrioEngine::ShaderType::Specular:
		stride = sizeof(VertexSpecularLightingType);
		break;
	default:
		logger->GetInstance().WriteLine("Neither texture nor colour is being used when rendered. You're probably going to crash here when attempting to render.");
		break;
	}

	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}