#include "Shader.h"



CShader::CShader()
{
}


CShader::~CShader()
{
	if (mpMatrixBuffer)
	{
		mpMatrixBuffer->Release();
		mpMatrixBuffer = nullptr;
	}
}

void CShader::SetWorldMatrix(D3DXMATRIX world)
{
	mWorldMatrix = world;
	D3DXMatrixTranspose(&mWorldMatrix, &mWorldMatrix);
}

void CShader::SetViewMatrix(D3DXMATRIX view)
{
	mViewMatrix = view;
	D3DXMatrixTranspose(&mViewMatrix, &mViewMatrix);
}

void CShader::SetProjMatrix(D3DXMATRIX proj)
{
	mProjMatrix = proj;
	D3DXMatrixTranspose(&mProjMatrix, &mProjMatrix);
}

void CShader::SetViewProjMatrix(D3DXMATRIX viewProj)
{
	mViewProjMatrix = viewProj;
	D3DXMatrixTranspose(&mViewProjMatrix, &mViewProjMatrix);
}

bool CShader::SetupMatrixBuffer(ID3D11Device * device)
{
	D3D11_BUFFER_DESC matrixBufferDesc;
	HRESULT result;

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
		logger->GetInstance().WriteLine("Failed to create the buffer pointer to access the vertex shader from within the sky dome shader class.");
		return false;
	}

	return true;
}

bool CShader::SetMatrixBuffer(ID3D11DeviceContext * deviceContext, unsigned int bufferSlot, ShaderType shaderType)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	/////////////////////////////
	// Matrix buffer
	/////////////////////////////

	// Lock the matrix buffer for writing to.
	result = deviceContext->Map(mpMatrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	// If we did not successfully lock the constant buffer.
	if (FAILED(result))
	{
		// Output error message to the logs.
		logger->GetInstance().WriteLine("Failed to lock the matrix buffer before writing to it in shader class.");
		return false;
	}

	// Grab pointer to the matrix const buff.
	MatrixBufferType* matrixBufferPtr = static_cast<MatrixBufferType*>(mappedResource.pData);

	// Set data in the structure.
	matrixBufferPtr->world = mWorldMatrix;
	matrixBufferPtr->view = mViewMatrix;
	matrixBufferPtr->projection = mProjMatrix;
	matrixBufferPtr->viewProj = mViewProjMatrix;

	// Unlock the const buffer and write modifications to it.
	deviceContext->Unmap(mpMatrixBuffer, 0);

	// Pass buffer to shader
	if (shaderType == ShaderType::Vertex)
	{
		deviceContext->VSSetConstantBuffers(bufferSlot, 1, &mpMatrixBuffer);
	}
	else if (shaderType == ShaderType::Pixel)
	{
		deviceContext->PSSetConstantBuffers(bufferSlot, 1, &mpMatrixBuffer);
	}
	else if (shaderType == ShaderType::Geometry)
	{
		deviceContext->GSSetConstantBuffers(bufferSlot, 1, &mpMatrixBuffer);
	}
	else
	{
		logger->GetInstance().WriteLine("Did not know how to handle the shader type passed into set matrix buffer in shader class.");
		return false;
	}

	return true;
}
