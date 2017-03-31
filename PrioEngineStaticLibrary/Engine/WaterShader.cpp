#include "WaterShader.h"

CWaterShader::CWaterShader()
{
	mpSurfaceVertexShader = nullptr;
	mpSurfacePixelShader = nullptr;
	mpHeightPixelShader = nullptr;
	mpLayout = nullptr;
	mpTrilinearWrap = nullptr;
	mpWaterBuffer = nullptr;
	mpCameraBuffer = nullptr;
	mpViewportBuffer = nullptr;
	mpLightBuffer = nullptr;
}

CWaterShader::~CWaterShader()
{
}

bool CWaterShader::Initialise(ID3D11Device * device, HWND hwnd)
{
	bool result;

	// Initialise the vertex pixel shaders.
	std::string surfaceVSName = "Shaders/WaterSurface.vs.hlsl";
	std::string surfacePSName = "Shaders/WaterSurface.ps.hlsl";
	std::string heightPSName = "Shaders/WaterHeight.ps.hlsl";
	result = InitialiseShader(device, hwnd, surfaceVSName, surfacePSName, heightPSName);

	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise the vertex and pixel shaders when initialising the terrain shader class.");
		return false;
	}

	return true;
}

void CWaterShader::Shutdown()
{
	// Shutodwn the vertex and pixel shaders as well as all related objects.
	ShutdownShader();
}

bool CWaterShader::RenderSurface(ID3D11DeviceContext* deviceContext, int indexCount)
{
	bool result;

	// Set the shader parameters that it will use for rendering.
	result = SetSurfaceShaderParameters(deviceContext);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderSurfaceShader(deviceContext, indexCount);

	return true;
}

bool CWaterShader::RenderHeight(ID3D11DeviceContext* deviceContext, int indexCount)
{
	bool result;

	// Set the shader parameters that it will use for rendering.
	result = SetHeightShaderParameters(deviceContext);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderHeightShader(deviceContext, indexCount);

	return true;
}

bool CWaterShader::InitialiseShader(ID3D11Device* device, HWND hwnd, std::string surfaceVSFilename, std::string surfacePsFilename, std::string heightPsFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	const int kNumberOfPolygonElements = 3;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[kNumberOfPolygonElements];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_BUFFER_DESC waterBufferDesc;
	D3D11_BUFFER_DESC cameraBufferDesc;
	D3D11_BUFFER_DESC viewportBufferDesc;
	D3D11_BUFFER_DESC lightBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;

	// Initialise pointers in this function to null.
	errorMessage = nullptr;
	vertexShaderBuffer = nullptr;
	pixelShaderBuffer = nullptr;

	/////////////////////////////////
	// Surface shaders
	////////////////////////////////

	// Compile the vertex shader code.
	result = D3DX11CompileFromFile(surfaceVSFilename.c_str(), NULL, NULL, "WaterSurfaceVS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &vertexShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, surfaceVSFilename.c_str());
		}
		else
		{
			logger->GetInstance().WriteLine("Could not find a shader file with name '" + surfaceVSFilename + "'");
			MessageBox(hwnd, surfaceVSFilename.c_str(), "Missing shader file. ", MB_OK);
		}
		logger->GetInstance().WriteLine("Failed to compile the vertex shader named '" + surfaceVSFilename + "'");
		return false;
	}

	// Compile the pixel shader code.
	result = D3DX11CompileFromFile(surfacePsFilename.c_str(), NULL, NULL, "WaterSurfacePS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &pixelShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, surfacePsFilename.c_str());
		}
		else
		{
			logger->GetInstance().WriteLine("Could not find a shader file with name '" + surfacePsFilename + "'");
			MessageBox(hwnd, surfacePsFilename.c_str(), "Missing shader file.", MB_OK);
		}
		logger->GetInstance().WriteLine("Failed to compile the pixel shader named '" + surfacePsFilename + "'");
		return false;
	}

	// Create the vertex shader from the buffer.
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &mpSurfaceVertexShader);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the vertex shader from the buffer.");
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &mpSurfacePixelShader);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the surface pixel shader from the buffer.");
		return false;
	}

	/////////////////////////////////
	// Height pixel shader.
	////////////////////////////////

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	// Compile the pixel shader code.
	result = D3DX11CompileFromFile(heightPsFilename.c_str(), NULL, NULL, "WaterHeightPS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &pixelShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, heightPsFilename.c_str());
		}
		else
		{
			logger->GetInstance().WriteLine("Could not find a shader file with name '" + heightPsFilename + "'");
			MessageBox(hwnd, heightPsFilename.c_str(), "Missing shader file.", MB_OK);
		}
		logger->GetInstance().WriteLine("Failed to compile the pixel shader named '" + heightPsFilename + "'");
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &mpHeightPixelShader);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the water height pixel shader from the buffer.");
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
		logger->GetInstance().WriteLine("Failed to create polygon layout in Water Shader class.");
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
	result = device->CreateSamplerState(&samplerDesc, &mpTrilinearWrap);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the trilinear wrap sampler state in WaterShader.cpp");
		return false;
	}

	// Create a texture sampler state description.
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;

	// Create the texture sampler state.
	result = device->CreateSamplerState(&samplerDesc, &mpBilinearMirror);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the bilinear mirror sampler state in WaterShader.cpp");
		return false;
	}

	////////////////////////////
	// Matrix buffer
	////////////////////////////

	if (!SetupMatrixBuffer(device))
	{
		logger->GetInstance().WriteLine("Failed to set up matrix buffer in water shader class.");
		return false;
	}

	////////////////////////////
	// Water buffer
	////////////////////////////

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	waterBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	waterBufferDesc.ByteWidth = sizeof(WaterBufferType);
	waterBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	waterBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	waterBufferDesc.MiscFlags = 0;
	waterBufferDesc.StructureByteStride = 0;


	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&waterBufferDesc, NULL, &mpWaterBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the water buffer in WaterShader.cpp.");
		return false;
	}

	////////////////////////////
	// Camera buffer
	////////////////////////////

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	cameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cameraBufferDesc.ByteWidth = sizeof(CameraBufferType);
	cameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraBufferDesc.MiscFlags = 0;
	cameraBufferDesc.StructureByteStride = 0;


	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&cameraBufferDesc, NULL, &mpCameraBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the camera buffer in WaterShader.cpp.");
		return false;
	}

	////////////////////////////
	// Viewport buffer
	////////////////////////////

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	viewportBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	viewportBufferDesc.ByteWidth = sizeof(ViewportBufferType);
	viewportBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	viewportBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	viewportBufferDesc.MiscFlags = 0;
	viewportBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&viewportBufferDesc, NULL, &mpViewportBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the viewport buffer in WaterShader.cpp.");
		return false;
	}

	////////////////////////////
	// Light buffer
	////////////////////////////

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	lightBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	lightBufferDesc.ByteWidth = sizeof(LightBufferType);
	lightBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightBufferDesc.MiscFlags = 0;
	lightBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&lightBufferDesc, NULL, &mpLightBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the light buffer in WaterShader.cpp.");
		return false;
	}

	return true;
}

void CWaterShader::ShutdownShader()
{
	if (mpTrilinearWrap)
	{
		mpTrilinearWrap->Release();
		mpTrilinearWrap = nullptr;
	}

	if (mpBilinearMirror)
	{
		mpBilinearMirror->Release();
		mpBilinearMirror = nullptr;
	}

	if (mpWaterBuffer)
	{
		mpWaterBuffer->Release();
		mpWaterBuffer = nullptr;
	}

	if (mpCameraBuffer)
	{
		mpCameraBuffer->Release();
		mpCameraBuffer = nullptr;
	}

	if (mpViewportBuffer)
	{
		mpViewportBuffer->Release();
		mpViewportBuffer = nullptr;
	}

	if (mpLightBuffer)
	{
		mpLightBuffer->Release();
		mpLightBuffer = nullptr;
	}

	if (mpLayout)
	{
		mpLayout->Release();
		mpLayout = nullptr;
	}

	if (mpHeightPixelShader)
	{
		mpHeightPixelShader->Release();
		mpHeightPixelShader = nullptr;
	}

	if (mpSurfacePixelShader)
	{
		mpSurfacePixelShader->Release();
		mpSurfacePixelShader = nullptr;
	}

	if (mpSurfaceVertexShader)
	{
		mpSurfaceVertexShader->Release();
		mpSurfaceVertexShader = nullptr;
	}
}

void CWaterShader::OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND hwnd, std::string shaderFilename)
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

bool CWaterShader::SetSurfaceShaderParameters(ID3D11DeviceContext* deviceContext)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	unsigned int bufferNumber;

	///////////////////////////
	// Matrix buffer
	///////////////////////////

	bufferNumber = 0;

	if (!SetMatrixBuffer(deviceContext, bufferNumber, ShaderType::Vertex))
	{
		logger->GetInstance().WriteLine("Failed to set the matrix buffer ub vertex shader in water shader.");
		return false;
	}
	if (!SetMatrixBuffer(deviceContext, bufferNumber, ShaderType::Pixel))
	{
		logger->GetInstance().WriteLine("Failed to set the matrix buffer in pixel shader in water shader.");
		return false;
	}

	///////////////////////////
	// Water buffer
	///////////////////////////

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(mpWaterBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	WaterBufferType * waterBufferPtr = (WaterBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	waterBufferPtr->WaterMovement = mWaterMovement;
	waterBufferPtr->WaveHeight = mWaveHeight;
	waterBufferPtr->WaveScale = mWaveScale;
	waterBufferPtr->RefractionDistortion = mRefractionDistortion;
	waterBufferPtr->ReflectionDistortion = mReflectionDistortion;
	waterBufferPtr->MaxDistortionDistance = mMaxDistortion;
	waterBufferPtr->RefractionStrength = mRefractionStrength;
	waterBufferPtr->ReflectionStrength = mReflectionStrength;
	waterBufferPtr->waterBufferPadding = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	// Unlock the constant buffer.
	deviceContext->Unmap(mpWaterBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 1;

	// Now set the constant buffer in the vertex and pixel shader with the updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &mpWaterBuffer);
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpWaterBuffer);

	///////////////////////////
	// Camera buffer
	///////////////////////////

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(mpCameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	CameraBufferType * cameraBufferPtr = (CameraBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	cameraBufferPtr->CameraMatrix = mCameraMatrix;
	cameraBufferPtr->CameraPosition = mCameraPosition;

	// Unlock the constant buffer.
	deviceContext->Unmap(mpCameraBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 2;

	// Now set the constant buffer in the vertex and pixel shader with the updated values.
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpCameraBuffer);

	///////////////////////////
	// Viewport buffer
	///////////////////////////

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(mpViewportBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	ViewportBufferType * viewportBufferPtr = (ViewportBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	viewportBufferPtr->ViewportSize = mViewportSize;

	// Unlock the constant buffer.
	deviceContext->Unmap(mpViewportBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 3;

	// Now set the constant buffer in the vertex and pixel shader with the updated values.
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpViewportBuffer);

	///////////////////////////
	// Light buffer
	///////////////////////////

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(mpLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	LightBufferType * lightBufferPtr = (LightBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	lightBufferPtr->AmbientColour = mAmbientColour;
	lightBufferPtr->DiffuseColour = mDiffuseColour;
	lightBufferPtr->LightDirection = mLightDirection;
	lightBufferPtr->mSpecularColour = mSpecularColour;
	lightBufferPtr->mSpecularPower = mSpecularPower;
	lightBufferPtr->mLightPosition = mLightPosition;
	lightBufferPtr->lightBufferPadding = 0.0f;

	// Unlock the constant buffer.
	deviceContext->Unmap(mpLightBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 4;

	// Now set the constant buffer in the vertex and pixel shader with the updated values.
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpLightBuffer);

	///////////////////////////
	// Shader resources
	///////////////////////////
	deviceContext->VSSetShaderResources(0, 1, &mpNormalMap);
	deviceContext->PSSetShaderResources(0, 1, &mpNormalMap);
	deviceContext->PSSetShaderResources(1, 1, &mpRefractionMap);
	deviceContext->PSSetShaderResources(2, 1, &mpReflectionMap);

	return true;
}

bool CWaterShader::SetHeightShaderParameters(ID3D11DeviceContext* deviceContext)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	unsigned int bufferNumber;

	///////////////////////////
	// Matrix buffer
	///////////////////////////

	bufferNumber = 0;

	if (!SetMatrixBuffer(deviceContext, bufferNumber, ShaderType::Vertex))
	{
		logger->GetInstance().WriteLine("Failed to set the matrix buffer ub vertex shader in water shader.");
		return false;
	}
	if (!SetMatrixBuffer(deviceContext, bufferNumber, ShaderType::Pixel))
	{
		logger->GetInstance().WriteLine("Failed to set the matrix buffer in pixel shader in water shader.");
		return false;
	}

	///////////////////////////
	// Water buffer
	///////////////////////////

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(mpWaterBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	WaterBufferType * waterBufferPtr = (WaterBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	waterBufferPtr->WaterMovement = mWaterMovement;
	waterBufferPtr->WaveHeight = mWaveHeight;
	waterBufferPtr->WaveScale = mWaveScale;
	waterBufferPtr->RefractionDistortion = mRefractionDistortion;
	waterBufferPtr->ReflectionDistortion = mReflectionDistortion;
	waterBufferPtr->MaxDistortionDistance = mMaxDistortion;
	waterBufferPtr->RefractionStrength = mRefractionStrength;
	waterBufferPtr->ReflectionStrength = mReflectionStrength;
	waterBufferPtr->waterBufferPadding = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	// Unlock the constant buffer.
	deviceContext->Unmap(mpWaterBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 1;

	// Now set the constant buffer in the vertex and pixel shader with the updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &mpWaterBuffer);

	///////////////////////////
	// Shader resources
	///////////////////////////
	deviceContext->VSSetShaderResources(0, 1, &mpNormalMap);

	return true;
}

void CWaterShader::RenderSurfaceShader(ID3D11DeviceContext * deviceContext, int indexCount)
{	
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(mpLayout);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	deviceContext->VSSetShader(mpSurfaceVertexShader, NULL, 0);
	deviceContext->PSSetShader(mpSurfacePixelShader, NULL, 0);

	// Set the sampler states
	deviceContext->VSSetSamplers(0, 1, &mpTrilinearWrap);
	deviceContext->PSSetSamplers(0, 1, &mpTrilinearWrap);
	deviceContext->PSSetSamplers(1, 1, &mpBilinearMirror);

	// Render the triangle.
	deviceContext->DrawIndexed(indexCount, 0, 0);


	// Unbind the shader resources we used.
	ID3D11ShaderResourceView* nullResource = nullptr;
	deviceContext->VSSetShaderResources(0, 1, &nullResource);
	deviceContext->PSSetShaderResources(0, 1, &nullResource);
	deviceContext->PSSetShaderResources(1, 1, &nullResource);
	deviceContext->PSSetShaderResources(2, 1, &nullResource);

	return;
}

void CWaterShader::RenderHeightShader(ID3D11DeviceContext * deviceContext, int indexCount)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(mpLayout);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	deviceContext->VSSetShader(mpSurfaceVertexShader, NULL, 0);
	deviceContext->PSSetShader(mpHeightPixelShader, NULL, 0);

	// Set the sampler state in the vertex shader.
	deviceContext->VSSetSamplers(0, 1, &mpTrilinearWrap);

	// Render the triangle.
	deviceContext->DrawIndexed(indexCount, 0, 0);

	// Unbind the shader resources we used.
	ID3D11ShaderResourceView* nullResource = nullptr;
	deviceContext->VSSetShaderResources(0, 1, &nullResource);

	return;
}

void CWaterShader::SetWaterMovement(D3DXVECTOR2 translation)
{
	mWaterMovement = translation;
}

void CWaterShader::SetWaveHeight(float height)
{
	mWaveHeight = height;
}

void CWaterShader::SetWaveScale(float scale)
{
	mWaveScale = scale;
}

void CWaterShader::SetDistortion(float refractionDistortion, float reflectionDistortion)
{
	mRefractionDistortion = refractionDistortion;
	mReflectionDistortion = reflectionDistortion;
}

void CWaterShader::SetMaxDistortion(float maxDistortion)
{
	mMaxDistortion = maxDistortion;
}

void CWaterShader::SetRefractionStrength(float strength)
{
	mRefractionStrength = strength;
}

void CWaterShader::SetReflectionStrength(float strength)
{
	mReflectionStrength = strength;
}

void CWaterShader::SetCameraMatrix(D3DXMATRIX cameraWorld)
{
	mCameraMatrix = cameraWorld;
}

void CWaterShader::SetCameraPosition(D3DXVECTOR3 position)
{
	mCameraPosition = mCameraPosition;
}

void CWaterShader::SetViewportSize(int screenWidth, int screenHeight)
{
	mViewportSize = D3DXVECTOR2(static_cast<float>(screenWidth), static_cast<float>(screenHeight));
}

void CWaterShader::SetLightProperties(CLight * light)
{
	mAmbientColour = light->GetAmbientColour();
	mDiffuseColour = light->GetDiffuseColour();
	mLightDirection = light->GetDirection();
	mSpecularPower = light->GetSpecularPower();
	mSpecularColour = light->GetSpecularColour();
	mLightPosition = light->GetPos();
}

void CWaterShader::SetNormalMap(CTexture * normalMap)
{
	mpNormalMap = normalMap->GetTexture();
}

void CWaterShader::SetRefractionMap(ID3D11ShaderResourceView * refractionMap)
{
	mpRefractionMap = refractionMap;
}

void CWaterShader::SetReflectionMap(ID3D11ShaderResourceView * reflectionMap)
{
	mpReflectionMap = reflectionMap;
}
