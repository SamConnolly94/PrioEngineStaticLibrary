#include "SkyboxShader.h"



CSkyboxShader::CSkyboxShader()
{
	mpVertexShader = nullptr;
	mpPixelShader = nullptr;
	mpLayout = nullptr;
	mpGradientBuffer = nullptr;
}


CSkyboxShader::~CSkyboxShader()
{
}

bool CSkyboxShader::Initialise(ID3D11Device * device, HWND hwnd)
{
	bool result;

	// Initialise the vertex and pixel shaders.
	result = InitialiseShader(device, hwnd, "Shaders/Skybox.vs.hlsl", "Shaders/Skybox.ps.hlsl");
	if (!result)
	{
		return false;
	}

	return true;
}

void CSkyboxShader::Shutdown()
{
	ShutdownShader();
}

bool CSkyboxShader::Render(ID3D11DeviceContext* deviceContext, int indexCount, D3DXVECTOR4 apexColour, D3DXVECTOR4 centreColour)
{
	bool result;

	// Set the parameters which will be used for rendering.
	result = SetShaderParameters(deviceContext, apexColour, centreColour);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderShader(deviceContext, indexCount);

	return true;
}

bool CSkyboxShader::InitialiseShader(ID3D11Device * device, HWND hwnd, std::string vsFilename, std::string psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	D3D11_INPUT_ELEMENT_DESC polygonLayout[1];
	unsigned int numElements;
	D3D11_BUFFER_DESC matrixBufferDesc;
	D3D11_BUFFER_DESC gradientBufferDesc;

	// Initialise pointers in this function to null.
	errorMessage = nullptr;
	vertexShaderBuffer = nullptr;
	pixelShaderBuffer = nullptr;

	// Compile the vertex shader code.
	result = D3DX11CompileFromFile(vsFilename.c_str(), NULL, NULL, "SkyDomeVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &vertexShaderBuffer, &errorMessage, NULL);
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
	result = D3DX11CompileFromFile(psFilename.c_str(), NULL, NULL, "SkyDomePixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, NULL, &pixelShaderBuffer, &errorMessage, NULL);
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

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &mpLayout);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create polygon layout in skybox shader.");
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = nullptr;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = nullptr;

	if (!SetupMatrixBuffer(device))
	{
		logger->GetInstance().WriteLine("Failed to set up matrix buffer in skybox shader class.");
		return false;
	}

	// Set up the constant gradient buffer 
	gradientBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	gradientBufferDesc.ByteWidth = sizeof(GradientBufferType);
	gradientBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	gradientBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	gradientBufferDesc.MiscFlags = 0;
	gradientBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the pixel shader constant buffer from within this class.
	result = device->CreateBuffer(&gradientBufferDesc, NULL, &mpGradientBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the gradient buffer pointer from within the sky dome shader class.");
		return false;
	}


	return true;
}

void CSkyboxShader::ShutdownShader()
{
	if (mpGradientBuffer)
	{
		mpGradientBuffer->Release();
		mpGradientBuffer = nullptr;
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

void CSkyboxShader::OutputShaderErrorMessage(ID3D10Blob * errorMessage, HWND hwnd, std::string shaderFilename)
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

	logger->GetInstance().WriteLine("*** SHADER ERROR ***");
	logger->GetInstance().WriteLine(errStr);

	// Release the blob used to store error message data.
	errorMessage->Release();
	errorMessage = 0;

	// Output error in message box.
	MessageBox(hwnd, "Error compiling shader. Check logs for a detailed error message.", shaderFilename.c_str(), MB_OK);
}

bool CSkyboxShader::SetShaderParameters(ID3D11DeviceContext* deviceContext, D3DXVECTOR4 apexColour, D3DXVECTOR4 centreColour)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	GradientBufferType* dataPtr2;
	unsigned int bufferNumber;

	bufferNumber = 0;

	if (!SetMatrixBuffer(deviceContext, bufferNumber, ShaderType::Vertex))
	{
		logger->GetInstance().WriteLine("Failed to set the matrix buffer in skybox refract shader.");
		return false;
	}

	result = deviceContext->Map(mpGradientBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr2 = (GradientBufferType*)mappedResource.pData;

	dataPtr2->apexColour = apexColour;
	dataPtr2->centreColour = centreColour;

	// Unlock the constant buffer.
	deviceContext->Unmap(mpGradientBuffer, 0);

	// Set the position of the gradient constant buffer in the pixel shader.
	bufferNumber = 0;

	// Finally set the gradient constant buffer in the pixel shader with the updated values.
	deviceContext->PSSetConstantBuffers(bufferNumber, 1, &mpGradientBuffer);

	return true;
}

void CSkyboxShader::RenderShader(ID3D11DeviceContext * deviceContext, int indexCount)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(mpLayout);

	// Set the vertex and pixel shaders that will be used to render the triangles.
	deviceContext->VSSetShader(mpVertexShader, NULL, 0);
	deviceContext->PSSetShader(mpPixelShader, NULL, 0);

	// Render the triangle.
	deviceContext->DrawIndexed(indexCount, 0, 0);
}
