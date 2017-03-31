#include "CloudShader.h"



CCloudShader::CCloudShader()
{
	mpVertexShader = nullptr;
	mpPixelShader = nullptr;
	mpLayout = nullptr;
	mpTrilinearWrap = nullptr;
	mpCloudBuffer = nullptr;
	mpCloudTexture1 = nullptr;
	mpCloudTexture2 = nullptr;
	mBrightness = 0.0f;
	mCloud1Movement = D3DXVECTOR2(0.0f, 0.0f);
	mCloud2Movement = D3DXVECTOR2(0.0f, 0.0f);
}


CCloudShader::~CCloudShader()
{
}

bool CCloudShader::Initialise(ID3D11Device * device, HWND hwnd)
{
	bool result = InitialiseShader(device, hwnd, "Shaders/Clouds.vs.hlsl", "Shaders/Clouds.ps.hlsl");
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise cloud shader.");
		return false;
	}

	return true;
}

void CCloudShader::Shutdown()
{
	ShutdownShader();
}

bool CCloudShader::Render(ID3D11DeviceContext * deviceContext, int indexCount)
{
	bool result = SetShaderParameters(deviceContext);
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to set the shader parameters for cloud shader.");
		return false;
	}

	RenderShader(deviceContext, indexCount);

	return true;
}

bool CCloudShader::InitialiseShader(ID3D11Device * device, HWND hwnd, std::string vsFilename, std::string psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D11_BUFFER_DESC cloudBufferDesc;

	// Initialise pointers in this function to null.
	errorMessage = nullptr;
	vertexShaderBuffer = nullptr;
	pixelShaderBuffer = nullptr;

	// Compile the vertex shader code.
	result = D3DX11CompileFromFile(vsFilename.c_str(), NULL, NULL, "CloudVS", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &vertexShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, vsFilename);
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
	result = D3DX11CompileFromFile(psFilename.c_str(), NULL, NULL, "CloudPS", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &pixelShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		// If we recieved an error message.
		if (errorMessage)
		{
			// Output our error message.
			OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
		}
		// If we couldn't find the shader file.
		else
		{
			// Output a message to the log and a message box.
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

	// Setup the layout of the data that goes into the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "TEXCOORD";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].InputSlot = 0;
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &mpLayout);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create polygon layout in cloud shader.");
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;


	D3D11_SAMPLER_DESC samplerDesc;
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

	result = device->CreateSamplerState(&samplerDesc, &mpTrilinearWrap);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the trilinear sample state in cloud shader.");
		return false;
	}


	if (!SetupMatrixBuffer(device))
	{
		logger->GetInstance().WriteLine("Failed to set up the matrix buffer.");
		return false;
	}

	// Set up the constant gradient buffer 
	cloudBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cloudBufferDesc.ByteWidth = sizeof(CloudBufferType);
	cloudBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cloudBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cloudBufferDesc.MiscFlags = 0;
	cloudBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the pixel shader constant buffer from within this class.
	result = device->CreateBuffer(&cloudBufferDesc, NULL, &mpCloudBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the cloud buffer pointer from within the cloud shader class.");
		return false;
	}

	return true;
}

void CCloudShader::ShutdownShader()
{
	if (mpCloudBuffer != nullptr)
	{
		mpCloudBuffer->Release();
		mpCloudBuffer = nullptr;
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

	if (mpVertexShader)
	{
		mpVertexShader->Release();
		mpVertexShader = nullptr;
	}
}

void CCloudShader::OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, std::string shaderFilename)
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

bool CCloudShader::SetShaderParameters(ID3D11DeviceContext * deviceContext)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	unsigned int bufferNumber;

	/////////////////////////////
	// Matrix buffer
	/////////////////////////////

	bufferNumber = 0;
	if (!SetMatrixBuffer(deviceContext, bufferNumber, ShaderType::Vertex))
	{
		logger->GetInstance().WriteLine("Failed to set the vertex shader.");
		return false;
	}

	/////////////////////////////
	// Cloud buffer
	/////////////////////////////

	// Lock the matrix buffer for writing to.
	result = deviceContext->Map(mpCloudBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	// If we did not successfully lock the constant buffer.
	if (FAILED(result))
	{
		// Output error message to the logs.
		logger->GetInstance().WriteLine("Failed to lock the cloud buffer before writing to it in cloud shader class.");
		return false;
	}

	// Grab pointer to the matrix const buff.
	CloudBufferType* cloudBufferPtr = static_cast<CloudBufferType*>(mappedResource.pData);

	// Set data in the structure.
	cloudBufferPtr->Cloud1Movement = mCloud1Movement;
	cloudBufferPtr->Cloud2Movement = mCloud2Movement;
	cloudBufferPtr->Brightness = mBrightness;
	cloudBufferPtr->cloudBufferPadding = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	// Unlock the const buffer and write modifications to it.
	deviceContext->Unmap(mpCloudBuffer, 0);

	// Pass buffer to shader.
	bufferNumber = 0;
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpCloudBuffer);

	/////////////////////////////
	// Shader resources
	/////////////////////////////

	deviceContext->PSSetShaderResources(0, 1, &mpCloudTexture1);
	deviceContext->PSSetShaderResources(1, 1, &mpCloudTexture2);

	return true;
}

void CCloudShader::RenderShader(ID3D11DeviceContext * deviceContext, int indexCount)
{
	deviceContext->IASetInputLayout(mpLayout);
	deviceContext->VSSetShader(mpVertexShader, NULL, 0);
	deviceContext->PSSetShader(mpPixelShader, NULL, 0);
	deviceContext->PSSetSamplers(0, 1, &mpTrilinearWrap);
	deviceContext->DrawIndexed(indexCount, 0, 0);
}

void CCloudShader::SetCloudTexture1(ID3D11ShaderResourceView * resource)
{
	mpCloudTexture1 = resource;
}

void CCloudShader::SetCloudTexture2(ID3D11ShaderResourceView * resource)
{
	mpCloudTexture2 = resource;
}

void CCloudShader::SetBrightness(float brightness)
{
	mBrightness = brightness;
}

void CCloudShader::SetCloud1Movement(float movementX, float movementZ)
{
	mCloud1Movement = D3DXVECTOR2(movementX, movementZ);
}

void CCloudShader::SetCloud2Movement(float movementX, float movementZ)
{
	mCloud2Movement = D3DXVECTOR2(movementX, movementZ);
}
