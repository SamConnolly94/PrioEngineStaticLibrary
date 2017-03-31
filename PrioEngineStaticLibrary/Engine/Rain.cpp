#include "Rain.h"



CRain::CRain()
{
	mpRandomResourceView = nullptr;
}


CRain::~CRain()
{
}

bool CRain::Initialise(ID3D11Device* device, std::string rainTexture, unsigned int numberOfParticles)
{
	//Shutdown();
	
	mFirstRun = true;
	mNumberOfParticles = numberOfParticles;
	mEmitterDirection = D3DXVECTOR3(0.0f, 1.0f, 0.0f);

	mpRainDropTexture = new CTexture();
	if (!mpRainDropTexture->Initialise(device, rainTexture))
	{
		logger->GetInstance().WriteLine("Failed to initialise the texture with name " + rainTexture + " in rain object.");
		return false;
	}
	
	mpRandomResourceView = CreateRandomTexture(device);
	if (mpRandomResourceView == nullptr)
	{
		logger->GetInstance().WriteLine("Failed to create the random texture in rain object.");
		return false;
	}

	if (!InitialiseBuffers(device))
	{
		logger->GetInstance().WriteLine("Failed to initialise the buffers for rain object.");
		return false;
	}

	return true;
}

bool CRain::InitialiseBuffers(ID3D11Device * device)
{
	D3D11_BUFFER_DESC vertexBufferDesc;
	HRESULT result;

	// Set up the descriptor of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Define our particle.
	VertexType particle;

	D3D11_SUBRESOURCE_DATA vertexData;
	particle.Age = 0.0f;
	particle.Type = 0;
	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = &particle;

	// Create the vertex buffer.
	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &mpInitialBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the initial vertex buffer from the buffer description in Rain class.");
		return false;
	}

	// Modify the vertex buffer desc to accomodate for multiple particles.
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * mNumberOfParticles;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	// Define a vertex buffer without any initial data.
	result = device->CreateBuffer(&vertexBufferDesc, 0, &mpDrawBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the draw vertex buffer from the buffer description in Rain class.");
		return false;
	}

	// Define a vertex buffer without any initial data.
	result = device->CreateBuffer(&vertexBufferDesc, 0, &mpStreamBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the stream buffer from the buffer description in Rain class.");
		return false;
	}

	return true;
}

void CRain::Shutdown()
{
	ShutdownBuffers();

	if (mpRainDropTexture)
	{
		mpRainDropTexture->Shutdown();
		delete mpRainDropTexture;
		mpRainDropTexture = nullptr;
	}

	if (mpRandomResourceView)
	{
		mpRandomResourceView->Release();
		mpRandomResourceView = nullptr;
	}
}

void CRain::Update(float updateTime)
{
	mAge += updateTime;
}

void CRain::UpdateRender(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set the vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;
	
	// Tell directx we've passed it a point list.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	if (mFirstRun)
	{
		// Set the vertex buffer to active in the input assembler.
		deviceContext->IASetVertexBuffers(0, 1, &mpInitialBuffer, &stride, &offset);
	}
	else
	{
		// Set the vertex buffer to active in the input assembler.
		deviceContext->IASetVertexBuffers(0, 1, &mpDrawBuffer, &stride, &offset);
	}

	// Set the stream buffer render target.
	deviceContext->SOSetTargets(1, &mpStreamBuffer, &offset);
}

void CRain::Render(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set the vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	//Unbind the vertex buffer
	ID3D11Buffer* buffer = 0;
	deviceContext->SOSetTargets(1, &buffer, &offset);

	std::swap(mpDrawBuffer, mpStreamBuffer);

	// Tell directx we've passed it a point list.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	deviceContext->IASetVertexBuffers(0, 1, &mpDrawBuffer, &stride, &offset);
}

void CRain::ShutdownBuffers()
{
	if (mpDrawBuffer)
	{
		mpDrawBuffer->Release();
		mpDrawBuffer = nullptr;
	}

	if (mpInitialBuffer)
	{
		mpInitialBuffer->Release();
		mpInitialBuffer = nullptr;
	}

	if (mpStreamBuffer)
	{
		mpStreamBuffer->Release();
		mpStreamBuffer = nullptr;
	}
}

ID3D11ShaderResourceView* CRain::CreateRandomTexture(ID3D11Device* device)
{
	D3DXVECTOR4 randomValues[1024];

	for (int i = 0; i < 1024; i++)
	{
		randomValues[i].x = ((((float)rand() - (float)rand()) / RAND_MAX));
		randomValues[i].y = ((((float)rand() - (float)rand()) / RAND_MAX));
		randomValues[i].z = ((((float)rand() - (float)rand()) / RAND_MAX));
		randomValues[i].w = ((((float)rand() - (float)rand()) / RAND_MAX));
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024 * sizeof(D3DXVECTOR4);
	initData.SysMemSlicePitch = 0;

	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	ID3D11Texture1D* randomTex = 0;
	device->CreateTexture1D(&texDesc, &initData, &randomTex);

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* randomTexture = 0;
	device->CreateShaderResourceView(randomTex, &viewDesc, &randomTexture);
	randomTex->Release();

	return randomTexture;
}

void CRain::SetNumberOfParticles(unsigned int numParticles)
{
	mNumberOfParticles = numParticles;
}

void CRain::SetFirstRun(bool firstRun)
{
	mFirstRun = firstRun;
}

void CRain::SetAge(float age)
{
	mAge = age;
}

void CRain::SetGameTime(float gameTime)
{
	mGameTime = gameTime;
}

void CRain::SetEmitterPos(D3DXVECTOR3 pos)
{
	mEmiterPosition = pos;
}

void CRain::SetEmitterDir(D3DXVECTOR3 dir)
{
	mEmitterDirection = dir;
}

unsigned int CRain::GetNumberOfParticles()
{
	return mNumberOfParticles;
}

bool CRain::GetIsFirstRun()
{
	return mFirstRun;
}

float CRain::GetAge()
{
	return mAge;
}

float CRain::GetGameTime()
{
	return mGameTime;
}

D3DXVECTOR3 CRain::GetEmitterPos()
{
	return mEmiterPosition;
}

D3DXVECTOR3 CRain::GetEmitterDir()
{
	return mEmitterDirection;
}

ID3D11ShaderResourceView * CRain::GetRainTexture()
{
	return mpRainDropTexture->GetTexture();
}

ID3D11ShaderResourceView * CRain::GetRandomTexture()
{
	return mpRandomResourceView;
}
