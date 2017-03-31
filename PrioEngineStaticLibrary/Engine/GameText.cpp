#include "GameText.h"



CGameText::CGameText()
{
	mpFont = nullptr;
	mpFontShader = nullptr;
}


CGameText::~CGameText()
{
}

bool CGameText::Initialise(ID3D11Device * device, ID3D11DeviceContext * deviceContext, HWND hWnd, int screenWidth, int screenHeight, D3DXMATRIX baseViewMatrix)
{
	bool result;

	// Store the screen width and height.
	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;

	// Store the base matrix view.
	mBaseViewMatrix = baseViewMatrix;

	// Create font obj.
	mpFont = new CGameFont();
	if (!mpFont)
	{
		logger->GetInstance().WriteLine("Failed to allocate memory to the font pointer in CGameText.");
		return false;
	}
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpFont).name());
	char* fontDataFile = "Resources/Fonts/font01.txt";
	std::string fontTextureFile = "Resources/Fonts/font01.dds";

	// Initailise the font.
	result = mpFont->Initialise(device, fontDataFile, fontTextureFile);

	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to load Fonts/defaultData.txt or Fonts/font.dds or both.");
		return false;
	}

	// Initialise the font shader.
	mpFontShader = new CFontShader();
	result = mpFontShader->Initialise(device, hWnd);

	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise the font object in GameText.cpp.");
		return false;
	}
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpFontShader).name());

	return true;
}

void CGameText::Shutdown()
{

	// Iterate through all of the sentences.
	for (auto sentence : mpSentences)
	{
		// If the sentence has had memory allocated to it.
		if (sentence)
		{
			// Release the memory.
			ReleaseSentence(sentence);
			// Remove the pointer from the sentence.
			sentence = nullptr;
		}
	}

	// Empty the list.
	mpSentences.clear();

	if (mpFontShader)
	{
		mpFontShader->Shutdown();
		delete mpFontShader;
		mpFontShader = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpFontShader).name());
	}

	if (mpFont)
	{
		mpFont->Shutdown();
		delete mpFont;
		mpFont = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpFont).name());
	}
}

// Attempt to render all texts.
bool CGameText::Render(ID3D11DeviceContext * deviceContext, D3DXMATRIX worldMatrix, D3DXMATRIX orthoMatrix)
{
	// Boolean flag to check whetehr things are succesfully rendered.
	bool result = false;

	// Iterate through the sentences list.
	for (auto sentence : mpSentences)
	{
		// Render each sentence on the list.
		result = RenderSentence(deviceContext, sentence, worldMatrix, orthoMatrix);
		
		// If we failed to render.
		if (!result)
		{
			// Output error message to the log.
			logger->GetInstance().WriteLine("Failed to render text.");
			// Failure! Stop processing.
			return false;
		}
	}

	// Success!
	return true;
}

bool CGameText::InitialiseSentence(SentenceType** sentence, int maxLength, ID3D11Device * device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	D3D11_SUBRESOURCE_DATA indexData;
	HRESULT result;

	// Allocate memory to the sentence object.
	*sentence = new SentenceType();
	if (!sentence)
	{
		logger->GetInstance().WriteLine("Failed to allocate memory to a sentence object in GameText.cpp.");
		return false;
	}
	logger->GetInstance().MemoryAllocWriteLine(typeid(sentence).name());

	// Track the pointer on our sentences list, don't want to lose this.
	//mpSentences.push_back(sentence);

	// Initialise buffer properties to be null.
	(*sentence)->vertexBuffer = nullptr;
	(*sentence)->indexBuffer = nullptr;

	// Set max length of the sentence.
	(*sentence)->maxLength = maxLength;

	// Set number of vertices in vertex array.
	(*sentence)->vertexCount = 6 * maxLength;

	// Set the number of indexes in the index array to be equal to the number of vertices.
	(*sentence)->indexCount = (*sentence)->vertexCount;

	// Create vertex array.
	vertices = new VertexType[(*sentence)->vertexCount];
	if (!vertices)
	{
		logger->GetInstance().WriteLine("Failed to allocate memory to the vertices buffer in GameText.cpp.");
		return false;
	}

	// Create the index array.
	indices = new unsigned long[(*sentence)->indexCount];
	if (!indices)
	{
		logger->GetInstance().WriteLine("Failed to allocate memory to the indices buffer in GameText.cpp.");
		return false;
	}
	// Set vertex array to 0's to begin with.
	memset(vertices, 0, (sizeof(VertexType) * (*sentence)->vertexCount));

	// Init index array.
	for (int i = 0; i < (*sentence)->indexCount; i++)
	{
		indices[i] = i;
	}

	// Set up descriptor for vertex buffer
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * (*sentence)->vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Pass subresource ptr to vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Create the buffer and store it in sentence objects properties.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &(*sentence)->vertexBuffer);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the vertex buffer in DirectX after passing it vertex data in GameText.cpp.");
		return false;
	}

	// Set up descriptor for index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * (*sentence)->indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give subresource pointer to index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &(*sentence)->indexBuffer);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Faield to create the index buffer in DirectX after passing it index data in GameText.cpp.");
		return false;
	}

	delete[] vertices;
	vertices = nullptr;

	delete[] indices;
	indices = nullptr;

	return true;
}

bool CGameText::UpdateSentence(SentenceType* &sentence, std::string text, int posX, int posY, float red, float green, float blue, ID3D11DeviceContext* deviceContext)
{
	int numberOfLetters;
	VertexType* vertices;
	float x;
	float y;
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	VertexType* verticesPtr;

	// Store the colour which the sentence will be drawn in.
	sentence->red = red;
	sentence->green = green;
	sentence->blue = blue;

	// Get the number of letters in the sentence.
	numberOfLetters = static_cast<int>(text.length());
	
	// Check for buffer overflow.
	if (numberOfLetters > sentence->maxLength)
	{
		logger->GetInstance().WriteLine("Sentence was too long. Would have caused buffer overflow.");
		return false;
	}

	// Create vertex array.
	vertices = new VertexType[sentence->vertexCount];
	
	// Check vertices array was allocated memory.
	if (!vertices)
	{
		logger->GetInstance().WriteLine("Failed to allocate memory to vertices array in GameText.cpp.");
		return false;
	}

	// Initialise the array to be 0.
	memset(vertices, 0, (sizeof(VertexType) * sentence->vertexCount));

	// Calculate positions which we will draw the text.
	x = (float)(((mScreenWidth / 2) * -1) + posX);
	y = (float)((mScreenHeight / 2) - posY);

	// Build the vertex array.
	mpFont->BuildVertexArray((void*)vertices, text, x, y);

	// Lock vertex array.
	result = deviceContext->Map(sentence->vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to map the resource when updating sentence in GameText.cpp.");
		return false;
	}

	// Grab pointer to data in vertex buffer.
	verticesPtr = (VertexType*)mappedResource.pData;

	// Copy the data into vertex buffer.
	memcpy(verticesPtr, (void*)vertices, (sizeof(VertexType) * sentence->vertexCount));

	// Unlock vertex buff.
	deviceContext->Unmap(sentence->vertexBuffer, 0);

	// Release vertex array.
	delete[] vertices;
	vertices = nullptr;

	return true;
}

bool CGameText::RemoveSentence(SentenceType *& sentence)
{
	std::list<SentenceType*>::iterator it = mpSentences.begin();

	while (it != mpSentences.end())
	{
		if (*it == sentence)
		{
			delete sentence;
			mpSentences.erase(it);
			sentence = nullptr;
			return true;
		}
		it++;
	}
	return false;
}

/* Defines and allocates memory to a sentence object. 
*  Please note that this sentence will be stored in a list, all rendering will be done for you, all you need to do is update it.
* @Returns pointer to the sentence object of type SentenceType. 
*/
SentenceType* CGameText::CreateSentence(ID3D11Device* device, ID3D11DeviceContext* deviceContext, std::string text, int posX, int posY, int maxLength)
{
	// Define a sentence.
	SentenceType* sentence;

	bool result = InitialiseSentence(&sentence, maxLength, device);
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise sentences.");
		return nullptr;
	}

	result = UpdateSentence(sentence, text.c_str(), posX, posY, 1.0f, 1.0f, 1.0f, deviceContext);

	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to update sentence.");
		return nullptr;
	}

	mpSentences.push_back(sentence);

	return sentence;
}

void CGameText::ReleaseSentence(SentenceType* sentence)
{
	if (sentence)
	{
		// Vertex buffer isn't null.
		if ((sentence)->vertexBuffer)
		{
			//delete[](sentence)->vertexBuffer;
			sentence->vertexBuffer->Release();
			(sentence)->vertexBuffer = nullptr;
		}

		// Index buffer isn't null.
		if ((sentence)->indexBuffer)
		{
			//delete[](sentence)->indexBuffer;
			sentence->indexBuffer->Release();
			(sentence)->indexBuffer = nullptr;
		}

		delete (sentence);
		(sentence) = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(sentence).name());
	}
}

bool CGameText::RenderSentence(ID3D11DeviceContext * deviceContext, SentenceType * sentence, D3DXMATRIX worldMatrix, D3DXMATRIX orthoMatrix)
{
	unsigned int stride, offset;
	D3DXVECTOR4 pixelColor;
	bool result;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &sentence->vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(sentence->indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create a pixel color vector with the input sentence color.
	pixelColor = D3DXVECTOR4(sentence->red, sentence->green, sentence->blue, 1.0f);

	mpFontShader->SetWorldMatrix(worldMatrix);
	mpFontShader->SetViewMatrix(mBaseViewMatrix);
	mpFontShader->SetProjMatrix(orthoMatrix);
	mpFontShader->SetViewProjMatrix(mBaseViewMatrix * orthoMatrix);

	// Render the text using the font shader.
	result = mpFontShader->Render(deviceContext, sentence->indexCount, mpFont->GetTexture(),
		pixelColor);
	if (!result)
	{
		false;
	}

	return true;
}

