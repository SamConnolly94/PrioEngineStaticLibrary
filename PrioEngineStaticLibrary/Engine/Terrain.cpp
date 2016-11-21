#include "Terrain.h"

CTerrainGrid::CTerrainGrid(ID3D11Device* device)
{
	// Output alloc message to memory log.
	gLogger->MemoryAllocWriteLine(typeid(this).name());

	mpDevice = device;

	// Initialise pointers to nullptr.
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;

	// Initialise all variables to null.
	mVertexCount = NULL;
	mIndexCount = NULL;

	mHeightMapLoaded = false;
	mpHeightMap = nullptr;

	mLowestPoint = 0.0f;
	mHighestPoint = 0.0f;
}


CTerrainGrid::~CTerrainGrid()
{
	// Output dealloc message to memory log.
	gLogger->MemoryDeallocWriteLine(typeid(this).name());

	ShutdownBuffers();
}

/* Create an instance of the grid so that it is ready to be rendered. */
bool CTerrainGrid::CreateGrid()
{
	bool result;

	// Set the width and height of the terrain.
	if (mWidth == NULL)
	{
		mWidth = 100;
	}
	if (mHeight == NULL)
	{
		mHeight = 100;
	}

	// Initialise the vertex buffers.
	result = InitialiseBuffers(mpDevice);

	// If we weren't successful in creating buffers.
	if (!result)
	{
		// Output error to log.
		gLogger->WriteLine("Failed to initialise buffers in Terrain.cpp.");
		return false;
	}

	return true;
}

/* Prepares the buffers and passes them over to the GPU ready for rendering. */
void CTerrainGrid::Render(ID3D11DeviceContext * context)
{
	// Render the data contained in the buffers..
	RenderBuffers(context);
}

/* This function is designed to create vertex and index buffers according to a heightmap that has already been set.
* @PARAM ID3D11Device* device - ptr to a direct x 11 device, can usually be retrieved from the graphics class.
*/
bool CTerrainGrid::InitialiseBuffers(ID3D11Device * device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3DXVECTOR3* normals;
	int index;
	int vertex;

	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	D3D11_SUBRESOURCE_DATA indexData;
	HRESULT result;
	const int kNumIndicesInSquare = 6;
	int numberOfNormals;

	// Calculate the number of vertices in the terrain mesh. 
	mVertexCount = (mWidth * mHeight);

	mIndexCount = (mWidth - 1) * (mHeight - 1) * kNumIndicesInSquare;
	
	numberOfNormals = ((mWidth - 1) * (mHeight - 1)) * 2;

	// Create the vertex array.
	vertices = new VertexType[mVertexCount];
	// Output the allocation message to the log.
	gLogger->MemoryAllocWriteLine(typeid(vertices).name());
	// If we failed to allocate memory to the vertices array.
	if (!vertices)
	{
		// Output error message to the debug log.
		gLogger->WriteLine("Failed to create the vertex array in InitialiseBuffers function, Terrain.cpp.");
		// Don't continue any more.
		return false;
	}

	// Create the index array.
	indices = new unsigned long[mIndexCount];
	// Output allocation message to the debug log.
	gLogger->MemoryAllocWriteLine(typeid(indices).name());
	// If we failed to allocate memory to the indices array.
	if (!indices)
	{
		// Output failure message to the debug log.
		gLogger->WriteLine("Failed to create the index array in InitialiseBuffers function, Terrain.cpp.");
		// Don't continue any further.
		return false;
	}

	// Create the normals array.
	normals = new D3DXVECTOR3[numberOfNormals];
	// Output the allocation message to the log.
	gLogger->MemoryAllocWriteLine(typeid(normals).name());
	// If we failed to allocate memory to the normals array.
	if (!normals)
	{
		// Output error message to the debug log.
		gLogger->WriteLine("Failed to create the normals array in InitialiseBuffers function, Terrain.cpp.");
		// Don't continue any more.
		return false;
	}

	// Reset the index and vertex we're starting from to 0.
	index = 0;
	vertex = 0;

	/// Plot the vertices of the grid.

	// For the height of our height map.
	for (int heightCount = 0; heightCount < mHeight; heightCount++)
	{
		// For the width of our height map.
		for (int widthCount = 0; widthCount < mWidth; widthCount++)
		{
			// Calculate the positions the X and Z of this vertex (Only the height varies for now).
			float posX = static_cast<float>(widthCount);
			float posZ = static_cast<float>(heightCount);

			// If we've loaded in a height map.
			if (mHeightMapLoaded)
			{
				// Set the height to whatever we found in this coordinate of our array.
				vertices[vertex].position = D3DXVECTOR3{ posX, static_cast<float>(mpHeightMap[heightCount][widthCount]), posZ };
			}
			else
			{
				// Set the position to a default of 0.0f.
				vertices[vertex].position = D3DXVECTOR3{ posX, 0.0f, posZ };
			}

			// Set the default colour.
			vertices[vertex].colour = D3DXVECTOR4{ 1.0f, 1.0f, 1.0f, 1.0f };

			// Onto the next vertex.
			vertex++;
		}
	}

	vertex = 0;
	int norms = 0;

	/// Calculate indices.
	// Iterate through the height ( - 1 because we'll draw a triangle which uses the vertex above current point, don't want to cause issues when we hit the boundary).
	for (int heightCount = 0; heightCount < mHeight - 1; heightCount++)
	{
		// Iterate through the width ( - 1 because we'll draw a triangle which uses the vertex to the right of the current point, don't want to cause issue's at the boundary).
		for (int widthCount = 0; widthCount < mWidth - 1; widthCount++)
		{
			/// Triangle 1.

			// Starting point.
			indices[index] = vertex;
			// Directly above.
			indices[index + 1] = vertex + mWidth;
			// Directly to the right.
			indices[index + 2] = vertex + 1;

			/// Triangle 2.

			// Directly to the right.
			indices[index + 3] = vertex + 1;
			// Directly above.
			indices[index + 4] = vertex + mWidth;
			// Above and to the right.
			indices[index + 5] = vertex + mWidth + 1;

			// We've added 6 indices so increment the count by 6.
			index += 6;

			/// Calculate normals.
			// Bottom left
			PrioEngine::Math::VEC3 point1 = { vertices[vertex].position.x, vertices[vertex].position.y, vertices[vertex].position.z };
			// Bottom right
			PrioEngine::Math::VEC3 point2 = { vertices[vertex + 1].position.x, vertices[vertex + 1].position.y, vertices[vertex + 1].position.z };
			// Upper right
			PrioEngine::Math::VEC3 point3 = { vertices[vertex + mWidth + 1].position.x, vertices[vertex + mWidth + 1].position.y, vertices[vertex + mWidth + 1].position.z };
			// Upper left
			PrioEngine::Math::VEC3 point4 = { vertices[vertex + mWidth].position.x, vertices[vertex + mWidth].position.y, vertices[vertex + mWidth].position.z };

			PrioEngine::Math::VEC3 U = PrioEngine::Math::Subtract(point2, point1);
			PrioEngine::Math::VEC3 V = PrioEngine::Math::Subtract(point3, point1);

			PrioEngine::Math::VEC3 face1Vec = PrioEngine::Math::CrossProduct(U, V);
			normals[norms] = D3DXVECTOR3{ face1Vec.x, -face1Vec.y, face1Vec.z };
			norms++;

			// Calculate second triangle face normal.
			U = PrioEngine::Math::Subtract(point2, point1);
			V = PrioEngine::Math::Subtract(point4, point1);
			PrioEngine::Math::VEC3 face2Vec = PrioEngine::Math::CrossProduct(U, V);
			normals[norms] = D3DXVECTOR3{ face2Vec.x, -face2Vec.y, face2Vec.z };
			norms++;
			
			// Increase the vertex which is our primary point.
			vertex++;
		}
		// We missed 1 off of the width count so auto adjust the vertex count here.
		vertex++;
	}


	// Set up the descriptor of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * mVertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &mpVertexBuffer);
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to create the vertex buffer from the buffer description.");
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * mIndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &mpIndexBuffer);
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to create the index buffer from the buffer description.");
		return false;
	}

	// Clean up the memory allocated to arrays.
	delete[] vertices;
	gLogger->MemoryDeallocWriteLine(typeid(vertices).name());
	vertices = nullptr;

	delete[] indices;
	gLogger->MemoryDeallocWriteLine(typeid(indices).name());
	indices = nullptr;

	delete[] normals;
	gLogger->MemoryDeallocWriteLine(typeid(indices).name());

	return true;
}

void CTerrainGrid::ShutdownBuffers()
{
	// Release any memory given to the vertex buffer.
	if (mpVertexBuffer)
	{
		mpVertexBuffer->Release();
		mpVertexBuffer = nullptr;
	}

	// Release any memory given to the index buffer.
	if (mpIndexBuffer)
	{
		mpIndexBuffer->Release();
		mpIndexBuffer = nullptr;
	}
}

void CTerrainGrid::RenderBuffers(ID3D11DeviceContext * context)
{
	unsigned int stride;
	unsigned int offset;

	// Set the vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler.
	context->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler.
	context->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	
	// Tell directx we've passed it a triangle list in the form of indices.
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

/* LoadHeightMap - Loads in a height map (usually from a perlin noise function) and stores ptrs to the data. 
* @PARAM double ** heightMap - A dynamically allocated 2D array of type doubles, it must contain all the data to be used for the heightmap already.
* @WARNING: Must not delete the 2D array until after this class has been destroyed.
* @WARNING: Must set height and width before calling this function.
*/
void CTerrainGrid::LoadHeightMap(double ** heightMap)
{
	// Copy the pointers to the 2D array and store them in memory.
	mpHeightMap = heightMap;

	// Store the lowest point to be the first point.
	mLowestPoint = static_cast<float>(mpHeightMap[0][0]);
	// Store the highest point to be the first point.
	mHighestPoint = static_cast<float>(mpHeightMap[0][0]);
	
	// Outpout a log to let the user know where we're up to in the function.
	gLogger->WriteLine("Copied height map over to terrain, time to find the heights and lowest points.");
	
	// Iterate through the height.
	for (int y = 0; y < mHeight; y++)
	{
		// Iterate through the width.
		for (int x = 0; x < mWidth; x++)
		{
			/// Perform comparison.
			// If this point is lower than the one we have on record.
			if (mpHeightMap[y][x] < mLowestPoint)
			{
				// Store this as the lowest point.
				mLowestPoint = static_cast<float>(mpHeightMap[y][x]);
			}
			// If this point is higher than the highest point we have on record.
			else if (mpHeightMap[y][x] > mHighestPoint)
			{
				// Store this as the highest point.
				mHighestPoint = static_cast<float>(mpHeightMap[y][x]);
			}
		}
	}

	// Adjust the Y position of the map model to be equal to the lowest point.
	SetYPos(0.0f - mLowestPoint);
	// Set the X position to be half of the width.
	SetXPos(0 - (static_cast<float>(mWidth) / 2.0f));

	// We've loaded a map, set the flag!
	mHeightMapLoaded = true;
}