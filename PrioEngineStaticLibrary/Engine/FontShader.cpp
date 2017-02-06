#include "FontShader.h"

CFontShader::CFontShader()
{
	mpVertexShader = nullptr;
	mpPixelShader = nullptr;
	mpLayout = nullptr;
	mpConstantBuffer = nullptr;
	mpPixelBuffer = nullptr;
	mpSampleState = nullptr;
}

CFontShader::~CFontShader()
{
}

bool CFontShader::Initialise(ID3D11Device * device, HWND hwnd)
{
	bool result;

	// Initialise the vertex and pixel shaders.
	result = InitialiseShader(device, hwnd, L"Shaders/Font.vs.hlsl", L"Shaders/Font.ps.hlsl");
	if (!result)
	{
		gLogger->WriteLine("Failed to load either the pixel or vertex shader in FontShader.cpp.");
		return false;
	}

	return true;
}

void CFontShader::Shutdown()
{
	ShutdownShader();
}

bool CFontShader::Render(ID3D11DeviceContext * deviceContext, int indexCount, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix, D3DXMATRIX projMatrix, ID3D11ShaderResourceView* texture, D3DXVECTOR4 pixelColour)
{
	bool result;

	// Set the parameters which will be used for rendering.
	result = SetShaderParameters(deviceContext, worldMatrix, viewMatrix, projMatrix, texture, pixelColour);
	if (!result)
	{
		gLogger->WriteLine("Failed to set the shader parameters in FontShader.cpp");
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderShader(deviceContext, indexCount);

	return true;
}

bool CFontShader::InitialiseShader(ID3D11Device * device, HWND hwnd, WCHAR * vsFilename, WCHAR * psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;
	D3D11_BUFFER_DESC constantBufferDesc;
	D3D11_SAMPLER_DESC samplerDesc;
	D3D11_BUFFER_DESC pixelBufferDesc;

	// Convert the vs & ps filename to string for logging purposes.
	std::wstring wsVs(vsFilename);
	std::string vsFilenameStr(wsVs.begin(), wsVs.end());

	std::wstring wsPs(psFilename);
	std::string psFilenameStr(wsPs.begin(), wsPs.end());

	// Initialise pointers in this function to null.
	errorMessage = nullptr;
	vertexShaderBuffer = nullptr;
	pixelShaderBuffer = nullptr;

	// Compile the vertex shader code.
	result = D3DX11CompileFromFile(vsFilename, NULL, NULL, "FontVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &vertexShaderBuffer, &errorMessage, NULL);
	if (FAILED(result))
	{
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, vsFilename);
		}
		else
		{
			gLogger->WriteLine("Could not find a shader file with name '" + vsFilenameStr + "'");
			MessageBox(hwnd, vsFilename, L"Missing shader file. ", MB_OK);
		}
		gLogger->WriteLine("Failed to compile the vertex shader named '" + vsFilenameStr + "'");
		return false;
	}

	// Compile the pixel shader code.
	result = D3DX11CompileFromFile(psFilename, NULL, NULL, "FontPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &pixelShaderBuffer, &errorMessage, NULL);
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
			gLogger->WriteLine("Could not find a shader file with name '" + psFilenameStr + "'");
			MessageBox(hwnd, psFilename, L"Missing shader file.", MB_OK);
		}
		gLogger->WriteLine("Failed to compile the pixel shader named '" + psFilenameStr + "'");
		return false;
	}

	// Create the vertex shader from the buffer.
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &mpVertexShader);
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to create the vertex shader from the buffer.");
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &mpPixelShader);
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to create the pixel shader from the buffer.");
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
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), 
		vertexShaderBuffer->GetBufferSize(), &mpLayout);
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to create polygon layout.");
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.ByteWidth = sizeof(ConstantBufferType);
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&constantBufferDesc, NULL, &mpConstantBuffer);

	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to create the buffer pointer to access the vertex shader from within the Colour shader class.");
		return false;
	}

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
		gLogger->WriteLine("Failed to create the sampler state in FontShader.cpp");
		return false;
	}

	/// Set up the pixel buffer.

	// Setup the description of the dynamic pixel constant buffer that is in the pixel shader.
	pixelBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	pixelBufferDesc.ByteWidth = sizeof(PixelBufferType);
	pixelBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	pixelBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	pixelBufferDesc.MiscFlags = 0;
	pixelBufferDesc.StructureByteStride = 0;

	// Create the pixel constant buffer pointer so we can access the pixel shader constant buffer from within this class.
	result = device->CreateBuffer(&pixelBufferDesc, NULL, &mpPixelBuffer);
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to set up the pixel buffer.");
		return false;
	}

	return true;
}

void CFontShader::ShutdownShader()
{
	if (mpPixelBuffer)
	{
		mpPixelBuffer->Release();
		mpPixelBuffer = nullptr;
	}

	if (mpSampleState)
	{
		mpSampleState->Release();
		mpSampleState = nullptr;
	}

	if (mpConstantBuffer)
	{
		mpConstantBuffer->Release();
		mpConstantBuffer = nullptr;
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

void CFontShader::OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, WCHAR * shaderFilename)
{
	char* compileErrors;
	unsigned long bufferSize;

	// Get pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	std::string errStr;

	for (unsigned int i = 0; i < bufferSize; i++)
	{
		errStr += compileErrors[i];
	}

	gLogger->WriteLine("*** SHADER ERROR ***");
	gLogger->WriteLine(errStr);

	// Release the blob used to store error message data.
	errorMessage->Release();
	errorMessage = 0;

	// Output error in message box.
	MessageBox(hwnd, L"Error compiling shader. Check logs for a detailed error message.", shaderFilename, MB_OK);
}

bool CFontShader::SetShaderParameters(ID3D11DeviceContext* deviceContext, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix, D3DXMATRIX projMatrix, ID3D11ShaderResourceView* texture, D3DXVECTOR4 pixelColour)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ConstantBufferType* dataPtr;
	unsigned int bufferNumber;
	PixelBufferType* dataPtr2;

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(mpConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to the lock the constant buffer so we could write to it in TextureShader.cpp.");
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr = (ConstantBufferType*)mappedResource.pData;

	// Transpose the matrices so they are ready for the shader.
	D3DXMatrixTranspose(&worldMatrix, &worldMatrix);
	D3DXMatrixTranspose(&viewMatrix, &viewMatrix);
	D3DXMatrixTranspose(&projMatrix, &projMatrix);

	// Copy the matrices into the constant buffer.
	dataPtr->world = worldMatrix;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projMatrix;

	// Unlock the constant buffer.
	deviceContext->Unmap(mpConstantBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 0;

	// Set the constant buffer in the vertex shader with updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &mpConstantBuffer);

	// Set the shader texture resource in the pixel shader.
	deviceContext->PSSetShaderResources(0, 1, &texture);

	// Lock pixel buffer so we can write to it.
	result = deviceContext->Map(mpPixelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	if (FAILED(result))
	{
		gLogger->WriteLine("Failed to lock the pixel buffer before writing to it in FontShader.cpp");
		return false;
	}

	// Get a pointer to the data in the pixel constant buffer.
	dataPtr2 = (PixelBufferType*)(mappedResource.pData);

	// Copy pixel colour over to the constant buffer.
	dataPtr2->pixelColour = pixelColour;

	// Unlock the pixel const buffer.
	deviceContext->Unmap(mpPixelBuffer, 0);

	// Set the position of the pixel constant buffer in the pixel shader.
	bufferNumber = 0;

	// Now set the pixel constant buffer in the pixel shader with new val.
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpPixelBuffer);

	return true;
}

void CFontShader::RenderShader(ID3D11DeviceContext * deviceContext, int indexCount)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(mpLayout);

	// Set the vertex and pixel shaders that will be used to render the triangles.
	deviceContext->VSSetShader(mpVertexShader, NULL, 0);
	deviceContext->PSSetShader(mpPixelShader, NULL, 0);

	// Set the sampler state in the pixel shader.
	deviceContext->PSSetSamplers(0, 1, &mpSampleState);

	// Render the triangles.
	deviceContext->DrawIndexed(indexCount, 0, 0);
}
