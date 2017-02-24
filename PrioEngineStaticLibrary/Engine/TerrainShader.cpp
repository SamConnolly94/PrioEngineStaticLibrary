#include "TerrainShader.h"

CTerrainShader::CTerrainShader()
{
	mpVertexShader = nullptr;
	mpPixelShader = nullptr;
	mpLayout = nullptr;
	mpMatrixBuffer = nullptr;
	mpSampleState = nullptr;
	mpLightBuffer = nullptr;
	mpPatchMap = new CTexture();
}

CTerrainShader::~CTerrainShader()
{
	mpPatchMap->Shutdown();
	delete mpPatchMap;
}

bool CTerrainShader::Initialise(ID3D11Device * device, HWND hwnd)
{
	bool result;

	// Initialise the vertex pixel shaders.
	result = InitialiseShader(device, hwnd, "Shaders/Terrain.vs.hlsl", "Shaders/Terrain.ps.hlsl");

	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise the vertex and pixel shaders when initialising the terrain shader class.");
		return false;
	}

	result = mpPatchMap->Initialise(device, "Resources/Patch Maps/PatchMap.png");

	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to load the blend mask when initialising the terrain shader.");
		return false;
	}

	return true;
}

void CTerrainShader::Shutdown()
{
	// Shutodwn the vertex and pixel shaders as well as all related objects.
	ShutdownShader();
}

bool CTerrainShader::Render(ID3D11DeviceContext* deviceContext, int indexCount, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix,
	D3DXMATRIX projMatrix, CTexture** texturesArray, unsigned int numberOfTextures, CTexture** grassTexturesArray, unsigned int numberOfGrassTextures,
	CTexture** rockTexturesArray, unsigned int numberOfRockTextures,
	D3DXVECTOR3 lightDirection, D3DXVECTOR4 diffuseColour, D3DXVECTOR4 ambientColour, float highestPos, float lowestPos, D3DXVECTOR3 worldPosition,
	float snowHeight, float grassHeight, float dirtHeight, float sandHeight)
{
	bool result;

	// Set the shader parameters that it will use for rendering.
	result = SetShaderParameters(deviceContext, worldMatrix, viewMatrix, projMatrix, texturesArray, 
		numberOfTextures, grassTexturesArray, numberOfGrassTextures, rockTexturesArray, numberOfRockTextures, lightDirection, 
		diffuseColour, ambientColour, highestPos, lowestPos, worldPosition, snowHeight, grassHeight, dirtHeight, sandHeight);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderShader(deviceContext, indexCount);

	return true;
}

bool CTerrainShader::InitialiseShader(ID3D11Device * device, HWND hwnd, std::string vsFilename, std::string psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	const int kNumberOfPolygonElements = 3;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[kNumberOfPolygonElements];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC lightBufferDesc;
	D3D11_BUFFER_DESC positioningBufferDesc;
	D3D11_BUFFER_DESC terrainAreaBufferDesc;

	// Initialise pointers in this function to null.
	errorMessage = nullptr;
	vertexShaderBuffer = nullptr;
	pixelShaderBuffer = nullptr;

	// Compile the vertex shader code.
	result = D3DX11CompileFromFile(vsFilename.c_str(), NULL, NULL, "TerrainVertex", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &vertexShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, vsFilename.c_str());
		}
		else
		{
			logger->GetInstance().WriteLine("Could not find a shader file with name '" + vsFilename + "'");
			MessageBox(hwnd, vsFilename.c_str(), "Missing shader file. ", MB_OK);
		}
		logger->GetInstance().WriteLine("Failed to compile the vertex shader named '" + vsFilename + "'");
		return false;
	}

	// Compile the pixel shader code.
	result = D3DX11CompileFromFile(psFilename.c_str(), NULL, NULL, "TerrainPixel", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &pixelShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, psFilename.c_str());
		}
		else
		{
			logger->GetInstance().WriteLine("Could not find a shader file with name '" + psFilename + "'");
			MessageBox(hwnd, psFilename.c_str(), "Missing shader file.", MB_OK);
		}
		logger->GetInstance().WriteLine("Failed to compile the pixel shader named '" + psFilename + "'");
		return false;
	}

	// Create the vertex shader from the buffer.
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &mpVertexShader);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the vertex shader from the buffer.");
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &mpPixelShader);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the pixel shader from the buffer.");
		return false;
	}

	/*
	* The polygonLayout.Format describes what size item should be placed in here, check if it's a float3 or float2 basically, and pass in DXGI_FORMAT_R32/G32/B32_FLOAT accordingly.
	*/

	int polyIndex = 0;

	// Setup the layout of the data that goes into the shader.
	polygonLayout[polyIndex].SemanticName = "POSITION";
	polygonLayout[polyIndex].SemanticIndex = 0;
	polygonLayout[polyIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[polyIndex].InputSlot = 0;
	polygonLayout[polyIndex].AlignedByteOffset = 0;
	polygonLayout[polyIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[polyIndex].InstanceDataStepRate = 0;

	polyIndex = 1;
	
	// Position only has 2 co-ords. Only need format of R32G32.
	polygonLayout[polyIndex].SemanticName = "TEXCOORD";
	polygonLayout[polyIndex].SemanticIndex = 0;
	polygonLayout[polyIndex].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[polyIndex].InputSlot = 0;
	polygonLayout[polyIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[polyIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[polyIndex].InstanceDataStepRate = 0;

	polyIndex = 2;

	polygonLayout[polyIndex].SemanticName = "NORMAL";
	polygonLayout[polyIndex].SemanticIndex = 0;
	polygonLayout[polyIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[polyIndex].InputSlot = 0;
	polygonLayout[polyIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[polyIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[polyIndex].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &mpLayout);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create polygon layout in Terrain Shader class.");
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	// Set up the sampler state descriptor.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = device->CreateSamplerState(&samplerDesc, &mpSampleState);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the sampler state in TextureShader.cpp");
		return false;
	}

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;


	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &mpMatrixBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the buffer pointer to access the vertex shader from within the texture shader class.");
		return false;
	}

	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBufferType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&lightBufferDesc, NULL, &mpLightBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the buffer from the light buffer descriptor from within the texture diffuse light shader class.");
		return false;
	}

	///////////////////////////////
	// Position buffer description
	positioningBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	positioningBufferDesc.ByteWidth = sizeof(PositioningBufferType);
	positioningBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	positioningBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	positioningBufferDesc.MiscFlags = 0;
	positioningBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&positioningBufferDesc, NULL, &mpPositioningBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the constant position information buffer in the terrrain shader from the positioning buffer description given.");
		return false;
	}

	///////////////////////////
	// Terrain area buffer description

	terrainAreaBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	terrainAreaBufferDesc.ByteWidth = sizeof(TerrainAreaBufferType);
	terrainAreaBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	terrainAreaBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	terrainAreaBufferDesc.MiscFlags = 0;
	terrainAreaBufferDesc.StructureByteStride = 0;

	result = device->CreateBuffer(&terrainAreaBufferDesc, NULL, &mpTerrainAreaBuffer);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the terrain area constant buffer from the description provided.");
		return false;
	}

	return true;
}

void CTerrainShader::ShutdownShader()
{
	if (mpLightBuffer)
	{
		mpLightBuffer->Release();
		mpLightBuffer = nullptr;
	}
	
	if (mpTerrainAreaBuffer)
	{
		mpTerrainAreaBuffer->Release();
		mpTerrainAreaBuffer = nullptr;
	}

	if (mpPositioningBuffer)
	{
		mpPositioningBuffer->Release();
		mpPositioningBuffer = nullptr;
	}

	if (mpSampleState)
	{
		mpSampleState->Release();
		mpSampleState = nullptr;
	}

	if (mpMatrixBuffer)
	{
		mpMatrixBuffer->Release();
		mpMatrixBuffer = nullptr;
	}

	if (mpLayout)
	{
		mpLayout->Release();
		mpLayout = nullptr;
	}

	if (mpPixelShader)
	{
		mpPixelShader->Release();
		mpPixelShader = nullptr;
	}

	if (mpVertexShader)
	{
		mpVertexShader->Release();
		mpVertexShader = nullptr;
	}
}

void CTerrainShader::OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND hwnd, std::string shaderFilename)
{
	std::string errMsg;
	char* compileErrors;
	unsigned long bufferSize;

	// Grab pointer to the compile errors.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Reset string to store message in to be empty.
	errMsg = "";

	// Compile the error message into a string variable.
	for (unsigned int i = 0; i < bufferSize; i++)
	{
		errMsg += compileErrors[i];
	}

	// Write the error string to the logs.
	logger->GetInstance().WriteLine(errMsg);

	// Clean up the BLOB file used to store the error message.
	errorMessage->Release();
	errorMessage = nullptr;

	// Output a message box containing info describing what went wrong. Redirect to the logs.
	MessageBox(hwnd, "Error compiling the shader. Check the logs for a more detailed error message.", shaderFilename.c_str(), MB_OK);
}

bool CTerrainShader::SetShaderParameters(ID3D11DeviceContext* deviceContext, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix,
	D3DXMATRIX projMatrix, CTexture** textureArray, unsigned int numberOfTextures, CTexture** grassTexturesArray, unsigned int numberOfGrassTextures,
	CTexture** rockTexturesArray, unsigned int numberOfRockTextures,
	D3DXVECTOR3 lightDirection, D3DXVECTOR4 diffuseColour, D3DXVECTOR4 ambientColour,
	float highestPos, float lowestPos, D3DXVECTOR3 worldPosition, float snowHeight, float grassHeight, float dirtHeight, float sandHeight)
{
	ID3D11ShaderResourceView** textures = new ID3D11ShaderResourceView*[numberOfTextures];
	ID3D11ShaderResourceView** grassTextures = new ID3D11ShaderResourceView*[numberOfGrassTextures];
	ID3D11ShaderResourceView* patchMap = mpPatchMap->GetTexture();
	ID3D11ShaderResourceView** rockTextures = new ID3D11ShaderResourceView*[numberOfRockTextures];

	for (unsigned int i = 0; i < numberOfTextures; i++)
	{
		textures[i] = textureArray[i]->GetTexture();
	}

	for (unsigned int i = 0; i < numberOfGrassTextures; i++)
	{
		grassTextures[i] = grassTexturesArray[i]->GetTexture();
	}

	for (unsigned int i = 0; i < numberOfRockTextures; i++)
	{
		rockTextures[i] = rockTexturesArray[i]->GetTexture();
	}

	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	unsigned int bufferNumber;
	MatrixBufferType* dataPtr;
	LightBufferType* dataPtr2;
	PositioningBufferType* positioningConstBuffPtr;
	TerrainAreaBufferType* terrainAreaConstBuffPtr;

	// Transpose the matrices to prepare them for the shader.
	D3DXMatrixTranspose(&worldMatrix, &worldMatrix);
	D3DXMatrixTranspose(&viewMatrix, &viewMatrix);
	D3DXMatrixTranspose(&projMatrix, &projMatrix);

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(mpMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr = (MatrixBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	dataPtr->world = worldMatrix;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projMatrix;

	// Unlock the constant buffer.
	deviceContext->Unmap(mpMatrixBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 0;

	// Now set the constant buffer in the vertex shader with the updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &mpMatrixBuffer);

	// Set shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(0, numberOfTextures, textures);
	deviceContext->PSSetShaderResources(numberOfTextures, numberOfGrassTextures, grassTextures);
	deviceContext->PSSetShaderResources(numberOfTextures + numberOfGrassTextures, 1, &patchMap);
	deviceContext->PSSetShaderResources(numberOfTextures + numberOfGrassTextures + 1, numberOfRockTextures, rockTextures);

	// Lock the light constant buffer so it can be written to.
	result = deviceContext->Map(mpLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr2 = (LightBufferType*)mappedResource.pData;

	// Copy the lighting variables into the constant buffer.
	dataPtr2->diffuseColour = diffuseColour;
	dataPtr2->ambientColour = ambientColour;
	dataPtr2->lightDirection = lightDirection;
	dataPtr2->padding = 0.0f;

	// Unlock the constant buffer.
	deviceContext->Unmap(mpLightBuffer, 0);

	// Set the position of the light constant buffer in the pixel shader.
	bufferNumber = 0;

	// Finally set the light constant buffer in the pixel shader with the updated values.
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpLightBuffer);

	////////////////////////////////////////
	// Positioning buffer.

	// Lock the terrain info constant buffer so it can be written to.
	result = deviceContext->Map(mpPositioningBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to map the positioning cosntant buffer when setting shader parameters in terrain shader class.");
		return false;
	}

	positioningConstBuffPtr = (PositioningBufferType*)mappedResource.pData;

	// Append the positions before it gets locked in the const buffer.
	positioningConstBuffPtr->yOffset = worldPosition.y;
	positioningConstBuffPtr->posPadding = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	// Unlock the terrain constant buffer.
	deviceContext->Unmap(mpPositioningBuffer, 0);

	// We'll need to modify the buffer position in the pixel shader as we're looking at the next buffer now.
	bufferNumber = 1;

	// Update the terrain constant buffer in the pixel shader.
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpPositioningBuffer);

	result = deviceContext->Map(mpTerrainAreaBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	terrainAreaConstBuffPtr = (TerrainAreaBufferType*)mappedResource.pData;

	// Append the positions before it gets locked in the const buffer.
	terrainAreaConstBuffPtr->snowHeight = snowHeight + worldPosition.y;
	terrainAreaConstBuffPtr->grassHeight = grassHeight + worldPosition.y;
	terrainAreaConstBuffPtr->dirtHeight = dirtHeight + worldPosition.y;
	terrainAreaConstBuffPtr->sandHeight = sandHeight + worldPosition.y;

	// Unlock the terrain constant buffer.
	deviceContext->Unmap(mpTerrainAreaBuffer, 0);

	// We'll need to modify the buffer position in the pixel shader as we're looking at the next buffer now.
	bufferNumber = 2;

	// Update the terrain constant buffer in the pixel shader.
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpTerrainAreaBuffer);

	delete[] textures;
	delete[] grassTextures;
	delete[] rockTextures;

	return true;
}

void CTerrainShader::RenderShader(ID3D11DeviceContext * deviceContext, int indexCount)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(mpLayout);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	deviceContext->VSSetShader(mpVertexShader, NULL, 0);
	deviceContext->PSSetShader(mpPixelShader, NULL, 0);

	// Set the sampler state in the pixel shader.
	deviceContext->PSSetSamplers(0, 1, &mpSampleState);

	// Render the triangle.
	deviceContext->DrawIndexed(indexCount, 0, 0);

	return;
}
