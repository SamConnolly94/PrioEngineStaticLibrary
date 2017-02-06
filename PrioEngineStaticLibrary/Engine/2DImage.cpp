#include "2DImage.h"


C2DImage::C2DImage()
{
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
	mVertexCount = 0;
	mIndexCount = 0;
	mpTexture = nullptr;

	mScreenWidth = 0;
	mScreenHeight = 0;

	mImgWidth = 0;
	mImgHeight = 0;

	mPreviousPosX = 0;
	mPreviousPoxY = 0;
}


C2DImage::~C2DImage()
{
}

/* Prepares the object for use in other classes. 
* @Returns bool - false if failed, true if successfully initialised. 
*/
bool C2DImage::Initialise(ID3D11Device * device, int screenWidth, int screenHeight, WCHAR * textureFilename, int width, int height)
{
	bool result = false;

	// Store the screen width and height.
	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;

	// Store the size of the image we're going to use.
	mImgWidth = width;
	mImgHeight = height;

	// Set the previous positions to be -1 as they haven't been used yet.
	mPreviousPosX = -1;
	mPreviousPoxY = -1;

	// Set up vertex and index buffers.
	result = InitialiseBuffers(device);

	// Check if failed to initialised buffers.
	if (!result)
	{
		// Output error to logs.
		gLogger->WriteLine("Failed to initialise the buffers in 2DSprite.cpp.");
		// Return false to avoid continuing.
		return false;
	}
	
	// Load in the texture with filename passed into this func as param.
	result = LoadTexture(device, textureFilename);

	// If failed to load texture.
	if (!result)
	{
		// Output detailed error to the logs for the user to fix.
		gLogger->WriteLine("Failed to load texture in 2DSprite.cpp.");
		// Prevent continuing to avoid disappointment.
		return false;
	}

	// Success!
	return true;
}

/* Cleans up all allocated memory to resources. */
void C2DImage::Shutdown()
{
	// Get rid of the texture and any memory allocated to it.
	ReleaseTexture();

	// Get rid of any buffers we have left open and any memory allocated to them.
	ShutdownBuffers();
}

/* Prepare the buffers for rendering. */
bool C2DImage::Render(ID3D11DeviceContext * deviceContext, int posX, int posY)
{
	bool result = false;

	// Build dynamic vertex buffer for rendering to a different location on the screen.
	result = UpdateBuffers(deviceContext, posX, posY);

	// If we failed to update the buffers.
	if (!result)
	{
		// Output detailed error message to the logs.
		gLogger->WriteLine("Failed to update the buffers with any potential new positions on the screen in 2DSprite.cpp.");
		// Return false to avoid continuing any further.
		return false;
	}

	// Pass the index and vertex data into the graphics pipeline.
	RenderBuffers(deviceContext);

	// Success!
	return true;
}

/* Returns however many indices there are in the index array. */
int C2DImage::GetNumberOfIndices()
{
	return mIndexCount;
}

/* Returns the shader resource view (Direct X Texture obj) that is being used as a texture.*/
ID3D11ShaderResourceView * C2DImage::GetTexture()
{
	return mpTexture->GetTexture();
}

/* Set up the vertex and index buffers to be used for this bitmap. */
bool C2DImage::InitialiseBuffers(ID3D11Device * device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	D3D11_SUBRESOURCE_DATA indexData;
	HRESULT result;
	
	// set number of vertices
	mVertexCount = 6;

	// Set number of indices to be the same as our vertices.
	mIndexCount = mVertexCount;

	// Create vertex array.
	vertices = new VertexType[mVertexCount];

	// If memory has not been allocated to vertices.
	if (!vertices)
	{
		// Output detailed error message to the log.
		gLogger->WriteLine("Failed to create vertex array in 2DSprite.cpp.");
		// Return false to avoid continuing incorrectly.
		return false;
	}
	// Output message to memory allocation log.
	gLogger->MemoryAllocWriteLine(typeid(vertices).name());

	// Create index array.
	indices = new unsigned long[mIndexCount];

	// If indices array has not had memory allocated to it.
	if (!indices)
	{
		// Output detailed error message to the log.
		gLogger->WriteLine("Failed to create index array in 2DSprite.cpp.");
		// Return false to avoid continuing incorrectly.
		return false;
	}

	// Initialise vertex array to zeros at first.
	memset(vertices, 0, (sizeof(VertexType) * mVertexCount));

	// Initialise the indices array.
	for (int i = 0; i < mIndexCount; i++)
	{
		indices[i] = i;
	}

	// Set up descriptor of static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * mVertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource a pointer to our data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &mpVertexBuffer);

	// If failed to create the vertex buffer.
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to create the vertex buffer from descriptor in 2DSprite.cpp.");
		return false;
	}

	// Set up the descriptor of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * mIndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Pass a pointer to our data over to the subresource.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the buffer for index data.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &mpIndexBuffer);

	// Check if we failed to create index buffer.
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to create the index buffer from descriptor in 2DSprite.cpp.");
		return false;
	}

	// Deallocate memory given to the temp vertex array.
	delete[] vertices;
	// Take away the pointer to the now deallocated memory.
	vertices = nullptr;
	// Output deallocation message to the logs.
	gLogger->MemoryDeallocWriteLine(typeid(vertices).name());

	// Deallocate memory given to the temp index array.
	delete[] indices;
	// Take away the pointer to the now deallocated memory.
	indices = nullptr;
	// Output deallocation message to the logs.
	gLogger->MemoryDeallocWriteLine(typeid(indices).name());

	return true;
}

/* Deallocates any memory which may have been assigned to vertex or index buffers. */
void C2DImage::ShutdownBuffers()
{
	// If vertex buffer had memory allocated.
	if (mpVertexBuffer)
	{
		// Release allocated memory.
		mpVertexBuffer->Release();
		// Take away pointer to the memory location.
		mpVertexBuffer = nullptr;
	}

	// If index buffer had memory allocated.
	if (mpIndexBuffer)
	{
		// Release any allocated memory.
		mpIndexBuffer->Release();
		// Take away pointer from memory location.
		mpIndexBuffer = nullptr;
	}
}

bool C2DImage::UpdateBuffers(ID3D11DeviceContext * deviceContext, int posX, int posY)
{
	float left = 0.0f, right = 0.0f, top = 0.0f, bottom = 0.0f;
	VertexType* vertices = nullptr;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	VertexType* verticesPtr = nullptr;
	HRESULT result;

	// If the position is the same then don't update the buffers.
	if (posX == mPreviousPosX && posY == mPreviousPoxY)
	{
		return true;
	}

	// If the position has updated, update the previous vals to our current.
	mPreviousPosX = posX;
	mPreviousPoxY = posY;

	// Calculate the screen coordinates of the left side of the bitmap.
	left = (float)((mScreenWidth / 2) * -1) + (float)posX;

	// Calculate the screen coordinates of the right side of the bitmap.
	right = left + (float)mImgWidth;

	// Calculate the screen coordinates of the top of the bitmap.
	top = (float)(mScreenHeight / 2) - (float)posY;

	// Calculate the screen coordinates of the bottom of the bitmap.
	bottom = top - (float)mImgHeight;
	
	// Allocate memory to a new vertex array.
	vertices = new VertexType[mVertexCount];
	
	// Check memory was successfully allocated to the vertices array.
	if (!vertices)
	{
		// Output error to log.
		gLogger->WriteLine("Failed to allocate memory to the vertices array when updating vertices.");
		// Don't continue any further.
		return false;
	}
	// Output memory alloc message to memory logs.
	gLogger->MemoryAllocWriteLine(typeid(vertices).name());

	/// Load the vertex array with data.
	// First triangle.
	// Top left.
	vertices[0].position = D3DXVECTOR3(left, top, 0.0f);
	vertices[0].texture = D3DXVECTOR2(0.0f, 0.0f);
	
	// Bottom right.
	vertices[1].position = D3DXVECTOR3(right, bottom, 0.0f); 
	vertices[1].texture = D3DXVECTOR2(1.0f, 1.0f);

	// Bottom left.
	vertices[2].position = D3DXVECTOR3(left, bottom, 0.0f);
	vertices[2].texture = D3DXVECTOR2(0.0f, 1.0f);

	// Second triangle.
	// Top left.
	vertices[3].position = D3DXVECTOR3(left, top, 0.0f);
	vertices[3].texture = D3DXVECTOR2(0.0f, 0.0f);
	
	// Top right.
	vertices[4].position = D3DXVECTOR3(right, top, 0.0f);
	vertices[4].texture = D3DXVECTOR2(1.0f, 0.0f);
	
	// Bottom right.
	vertices[5].position = D3DXVECTOR3(right, bottom, 0.0f); 
	vertices[5].texture = D3DXVECTOR2(1.0f, 1.0f);

	// Lock vertex buffer so we can write to it.
	result = deviceContext->Map(mpVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	// If we failed to lock the vertex buffer.
	if (FAILED(result))
	{
		// Output error message to log.
		gLogger->WriteLine("Failed to lock the vertex buffer when updating it in 2DSprite.cpp.");
		// Don't continue any further.
		return false;
	}

	// Grab a pointer to the data in memory.
	verticesPtr = (VertexType*)mappedResource.pData;

	// Copy the data into the memory space.
	memcpy(verticesPtr, (void*)vertices, (sizeof(VertexType) * mVertexCount));

	// Unlock the vertex buffer, we don't need it any more.
	deviceContext->Unmap(mpVertexBuffer, 0);

	// Deallocate memory.
	delete[] vertices;
	// Remove pointer to deallocated memory.
	vertices = nullptr;
	// Output messaoge to logs.
	gLogger->MemoryDeallocWriteLine(typeid(vertices).name());

	return true;
}

void C2DImage::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}

/* Allocates memory to the texture object and initialises it. */
bool C2DImage::LoadTexture(ID3D11Device * device, WCHAR * textureFilename)
{
	bool result = false;

	// Create texture object.
	mpTexture = new CTexture();

	// Check that memory was successfully allocated.
	if (!mpTexture)
	{
		// Output failure message to log.
		gLogger->WriteLine("Failed to create texture object in 2DSprite.cpp.");
		// Prevent continuing any further.
		return false;
	}
	// Output memory allocation message to memory log.
	gLogger->MemoryAllocWriteLine(typeid(mpTexture).name());

	// Initialise the texture object.
	result = mpTexture->Initialise(device, textureFilename);

	// If failed to initialise the texture object.
	if (!result)
	{
		// Output error message.
		gLogger->WriteLine("Failed to initialise the texture object in 2DSprite.cpp.");
		// Don't continue any further.
		return false;
	}

	// Success!
	return true;
}

void C2DImage::ReleaseTexture()
{
	// If texture object was allocated memory.
	if (mpTexture)
	{
		// Release resources held by the texture object.
		mpTexture->Shutdown();
		// Deallocate memory from the texture object.
		delete mpTexture;
		// Remove pointer to that block of memory.
		mpTexture = nullptr;
		// Output deallocation message to the memory log.
		gLogger->MemoryDeallocWriteLine(typeid(mpTexture).name());
	}
}
