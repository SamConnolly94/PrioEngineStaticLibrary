#include "RainShader.h"

CRainShader::CRainShader()
{
	mpVertexShader = nullptr;
	mpPixelShader = nullptr;
	mpLayout = nullptr;
	mpMatrixBuffer = nullptr;
	mpTrilinearWrap = nullptr;
}

CRainShader::~CRainShader()
{
}

bool CRainShader::Initialise(ID3D11Device * device, HWND hwnd)
{
	bool result;

	// Initialise the vertex pixel shaders.
	std::string vsFilename = "Shaders/Rain.vs.hlsl";
	std::string gsFilename = "Shaders/Rain.gs.hlsl";
	std::string psFilename = "Shaders/Rain.ps.hlsl";
	std::string vsUpdateFilename = "Shaders/RainParticleUpdate.vs.hlsl";
	std::string gsUpdateFilename = "Shaders/RainParticleUpdate.gs.hlsl";

	result = InitialiseShader(device, hwnd, vsFilename, gsFilename, psFilename, vsUpdateFilename, gsUpdateFilename);

	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise the vertex and pixel shaders when initialising the terrain shader class.");
		return false;
	}

	return true;
}

void CRainShader::Shutdown()
{
	// Shutodwn the vertex and pixel shaders as well as all related objects.
	ShutdownShader();
}

bool CRainShader::Render(ID3D11DeviceContext* deviceContext)
{
	bool result;

	// Set the shader parameters that it will use for rendering.
	result = SetShaderParameters(deviceContext);
	if (!result)
	{
		return false;
	}

	/////////////////////////////
	// Shader resources / textures
	/////////////////////////////

	deviceContext->PSSetShaderResources(0, 1, &mpRainTexture);

	// Now render the prepared buffers with the shader.
	RenderShader(deviceContext);

	return true;
}

bool CRainShader::UpdateRender(ID3D11DeviceContext * deviceContext)
{
	bool result;

	// Set the shader parameters that it will use for rendering.
	result = SetShaderParameters(deviceContext);
	if (!result)
	{
		return false;
	}

	deviceContext->GSSetShaderResources(0, 1, &mpRandomTexture);

	// Now render the prepared buffers with the shader.
	UpdateParticles(deviceContext);

	return true;
}

bool CRainShader::InitialiseShader(ID3D11Device * device, HWND hwnd, std::string vsFilename, std::string gsFilename, std::string psFilename, std::string vsUpdateFilename, std::string gsUpdateFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* geometryShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	ID3D10Blob* vertexShaderUpdateBuffer;
	ID3D10Blob* geometryShaderUpdateBuffer;
	const int kNumberOfPolygonElements = 5;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[kNumberOfPolygonElements];
	D3D11_SO_DECLARATION_ENTRY gsPolyStream[kNumberOfPolygonElements];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC rainBufferDesc;
	D3D11_BUFFER_DESC frameBufferDesc;


	// Initialise pointers in this function to null.
	errorMessage = nullptr;
	vertexShaderBuffer = nullptr;
	pixelShaderBuffer = nullptr;

	// Compile the vertex shader code.
	result = D3DX11CompileFromFile(vsFilename.c_str(), NULL, NULL, "RainVS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &vertexShaderBuffer, &errorMessage, NULL);
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

	// Compile the geometry shader code.
	result = D3DX11CompileFromFile(gsFilename.c_str(), NULL, NULL, "RainGS", "gs_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &geometryShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, gsFilename.c_str());
		}
		else
		{
			logger->GetInstance().WriteLine("Could not find a shader file with name '" + gsFilename + "'");
			MessageBox(hwnd, gsFilename.c_str(), "Missing shader file. ", MB_OK);
		}
		logger->GetInstance().WriteLine("Failed to compile the geometry shader named '" + gsFilename + "'");
		return false;
	}

	// Compile the pixel shader code.
	result = D3DX11CompileFromFile(psFilename.c_str(), NULL, NULL, "RainPS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &pixelShaderBuffer, &errorMessage, NULL);
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

	// Create the geometry shader from the buffer.
	result = device->CreateGeometryShader(geometryShaderBuffer->GetBufferPointer(), geometryShaderBuffer->GetBufferSize(), NULL, &mpGeometryShader);
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

	/////////////////////////////////////////
	// Update shaders
	/////////////////////////////////////////

	// Compile the vertex shader code.
	result = D3DX11CompileFromFile(vsUpdateFilename.c_str(), NULL, NULL, "RainUpdateVS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &vertexShaderUpdateBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, vsUpdateFilename.c_str());
		}
		else
		{
			logger->GetInstance().WriteLine("Could not find a shader file with name '" + vsUpdateFilename + "'");
			MessageBox(hwnd, vsUpdateFilename.c_str(), "Missing shader file. ", MB_OK);
		}
		logger->GetInstance().WriteLine("Failed to compile the vertex shader named '" + vsUpdateFilename + "'");
		return false;
	}

	// Compile the geometry shader code.
	result = D3DX11CompileFromFile(gsUpdateFilename.c_str(), NULL, NULL, "RainUpdateGS", "gs_5_0", D3D10_SHADER_ENABLE_STRICTNESS || (1 << 0), 0, NULL, &geometryShaderUpdateBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, gsUpdateFilename.c_str());
		}
		else
		{
			logger->GetInstance().WriteLine("Could not find a shader file with name '" + gsUpdateFilename + "'");
			MessageBox(hwnd, gsUpdateFilename.c_str(), "Missing shader file. ", MB_OK);
		}
		logger->GetInstance().WriteLine("Failed to compile the geometry shader named '" + gsUpdateFilename + "'");
		return false;
	}

	// Create the vertex shader from the buffer.
	result = device->CreateVertexShader(vertexShaderUpdateBuffer->GetBufferPointer(), vertexShaderUpdateBuffer->GetBufferSize(), NULL, &mpUpdateVertexShader);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the update vertex shader from the buffer.");
		return false;
	}


	/* 
	* Define poly layout for GS.
	*/
	gsPolyStream[0].SemanticName = "POSITION";
	gsPolyStream[0].SemanticIndex = 0;
	gsPolyStream[0].StartComponent = 0;
	gsPolyStream[0].ComponentCount = 3;
	gsPolyStream[0].OutputSlot = 0;
	gsPolyStream[0].Stream = 0;

	gsPolyStream[1].SemanticName = "VELOCITY";
	gsPolyStream[1].SemanticIndex = 0;
	gsPolyStream[1].StartComponent = 0;
	gsPolyStream[1].ComponentCount = 3;
	gsPolyStream[1].OutputSlot = 0;
	gsPolyStream[1].Stream = 0;

	gsPolyStream[2].SemanticName = "SIZE";
	gsPolyStream[2].SemanticIndex = 0;
	gsPolyStream[2].StartComponent = 0;
	gsPolyStream[2].ComponentCount = 2;
	gsPolyStream[2].OutputSlot = 0;
	gsPolyStream[2].Stream = 0;

	gsPolyStream[3].SemanticName = "AGE";
	gsPolyStream[3].SemanticIndex = 0;
	gsPolyStream[3].StartComponent = 0;
	gsPolyStream[3].ComponentCount = 1;
	gsPolyStream[3].OutputSlot = 0;
	gsPolyStream[3].Stream = 0;

	gsPolyStream[4].SemanticName = "TYPE";
	gsPolyStream[4].SemanticIndex = 0;
	gsPolyStream[4].StartComponent = 0;
	gsPolyStream[4].ComponentCount = 1;
	gsPolyStream[4].OutputSlot = 0;
	gsPolyStream[4].Stream = 0;

	UINT stride[] = { sizeof(VertexType) };
	UINT numEntries = (sizeof(gsPolyStream) / sizeof(D3D11_SO_DECLARATION_ENTRY));
	int numBuffers = 1;

	result = device->CreateGeometryShaderWithStreamOutput(geometryShaderUpdateBuffer->GetBufferPointer(), geometryShaderUpdateBuffer->GetBufferSize(), gsPolyStream,
		numEntries, stride, numBuffers, D3D11_SO_NO_RASTERIZED_STREAM, NULL, &mpUpdateGeometryShader);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the udpate geometry shader stream.");
		return false;
	}

	/*
	* The polygonLayout.Format describes what size item should be placed in here, check if it's a float3 or float2 basically, and pass in DXGI_FORMAT_R32/G32/B32_FLOAT accordingly.
	*/


	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "VELOCITY";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	polygonLayout[2].SemanticName = "SIZE";
	polygonLayout[2].SemanticIndex = 0;
	polygonLayout[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[2].InputSlot = 0;
	polygonLayout[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate = 0;

	polygonLayout[3].SemanticName = "AGE";
	polygonLayout[3].SemanticIndex = 0;
	polygonLayout[3].Format = DXGI_FORMAT_R32_FLOAT;
	polygonLayout[3].InputSlot = 0;
	polygonLayout[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[3].InstanceDataStepRate = 0;

	polygonLayout[4].SemanticName = "TYPE";
	polygonLayout[4].SemanticIndex = 0;
	polygonLayout[4].Format = DXGI_FORMAT_R32_UINT;
	polygonLayout[4].InputSlot = 0;
	polygonLayout[4].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[4].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[4].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &mpLayout);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create polygon layout in Rain Shader class.");
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = nullptr;

	geometryShaderBuffer->Release();
	geometryShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	vertexShaderUpdateBuffer->Release();
	vertexShaderBuffer = nullptr;
	
	geometryShaderUpdateBuffer->Release();
	geometryShaderUpdateBuffer = nullptr;

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
		logger->GetInstance().WriteLine("Failed to create the sampler state in RainShader.cpp");
		return false;
	}

	if (!SetupMatrixBuffer(device))
	{
		logger->GetInstance().WriteLine("Failed to set up the matrix buffer in rain shader class.");
		return false;
	}

	//////////////////////////////
	// Frame buffer 
	//////////////////////////////

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	frameBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	frameBufferDesc.ByteWidth = sizeof(FrameBufferType);
	frameBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	frameBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	frameBufferDesc.MiscFlags = 0;
	frameBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&frameBufferDesc, NULL, &mpFrameBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create frame buffer from the description provided in Rain Shader class.");
		return false;
	}

	return true;
}

void CRainShader::ShutdownShader()
{

	if (mpFrameBuffer)
	{
		mpFrameBuffer->Release();
		mpFrameBuffer = nullptr;
	}

	if (mpMatrixBuffer)
	{
		mpMatrixBuffer->Release();
		mpMatrixBuffer = nullptr;
	}

	if (mpTrilinearWrap)
	{
		mpTrilinearWrap->Release();
		mpTrilinearWrap = nullptr;
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

	if (mpGeometryShader)
	{
		mpGeometryShader->Release();
		mpGeometryShader = nullptr;
	}

	if (mpVertexShader)
	{
		mpVertexShader->Release();
		mpVertexShader = nullptr;
	}
}

void CRainShader::OutputShaderErrorMessage(ID3D10Blob *errorMessage, HWND hwnd, std::string shaderFilename)
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

bool CRainShader::SetShaderParameters(ID3D11DeviceContext* deviceContext)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	unsigned int bufferNumber;


	bufferNumber = 0;

	if (!SetMatrixBuffer(deviceContext, bufferNumber, ShaderType::Vertex))
	{
		logger->GetInstance().WriteLine("Failed to set the matrix buffer in the vertex shader in rain shader class.");
		return false;
	}
	if (!SetMatrixBuffer(deviceContext, bufferNumber, ShaderType::Geometry))
	{
		logger->GetInstance().WriteLine("Failed to set the matrix buffer in the geometry shader in rain shader class.");
		return false;
	}

	/////////////////////////////
	// Frame Buffer
	/////////////////////////////

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(mpFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	FrameBufferType* frameBufferPtr = (FrameBufferType*)mappedResource.pData;

	// Copy the data into the constant buffer.
	frameBufferPtr->CameraPos = mCameraWorldPosition;
	frameBufferPtr->EmitPos = mEmitWorldPosition;
	frameBufferPtr->GameTime = mGameTime;
	frameBufferPtr->FrameTime = mFrameTime;
	frameBufferPtr->Gravity = mGravityAcceleration;
	frameBufferPtr->WindX = mWindX;
	frameBufferPtr->WindZ = mWindZ;

	// Unlock the constant buffer.
	deviceContext->Unmap(mpFrameBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 1;

	// Now set the constant buffer in the vertex shader with the updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &mpFrameBuffer);
	deviceContext->GSSetConstantBuffers(bufferNumber, 1, &mpFrameBuffer);

	return true;
}

void CRainShader::RenderShader(ID3D11DeviceContext * deviceContext)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(mpLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetShader(mpVertexShader, NULL, 0);
	deviceContext->GSSetShader(mpGeometryShader, NULL, 0);
	deviceContext->PSSetShader(mpPixelShader, NULL, 0);

	// Set the sampler state in the pixel shader.
	deviceContext->PSSetSamplers(0, 1, &mpTrilinearWrap);

	deviceContext->DrawAuto();

	deviceContext->GSSetShader(NULL, NULL, 0);

	return;
}

bool CRainShader::UpdateParticles(ID3D11DeviceContext * deviceContext)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(mpLayout);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	deviceContext->VSSetShader(mpUpdateVertexShader, NULL, 0);
	deviceContext->GSSetShader(mpUpdateGeometryShader, NULL, 0);
	deviceContext->GSSetSamplers(0, 1, &mpTrilinearWrap);
	deviceContext->PSSetShader(NULL, NULL, 0);
	
	if (mFirstRun)
	{
		deviceContext->Draw(1, 0);
	}
	else
	{
		deviceContext->DrawAuto();
	}

	deviceContext->GSSetShader(NULL, NULL, NULL);

	return true;
}

void CRainShader::SetRainTexture(ID3D11ShaderResourceView * resource)
{
	mpRainTexture = resource;
}

void CRainShader::SetRandomTexture(ID3D11ShaderResourceView * resource)
{
	mpRandomTexture = resource;
}

void CRainShader::SetCameraWorldPosition(D3DXVECTOR3 pos)
{
	mCameraWorldPosition = pos;
}

void CRainShader::SetEmitterWorldPosition(D3DXVECTOR3 pos)
{
	mEmitWorldPosition = pos;
}

void CRainShader::SetEmitterWorldDirection(D3DXVECTOR3 dir)
{
	mEmitWorldDirection = dir;
}

void CRainShader::SetGameTime(float time)
{
	mGameTime = time;
}

void CRainShader::SetFrameTime(float updateTime)
{
	mFrameTime = updateTime;
}

void CRainShader::SetGravityAcceleration(float gravityAcceleration)
{
	mGravityAcceleration = gravityAcceleration;
}

void CRainShader::SetWindX(float windX)
{
	mWindX = windX;
}

void CRainShader::SetWindZ(float windZ)
{
	mWindZ = windZ;
}

void CRainShader::SetFirstRun(bool firstRun)
{
	mFirstRun = firstRun;
}