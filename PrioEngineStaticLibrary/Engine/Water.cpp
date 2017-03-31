#include "Water.h"

CWater::CWater()
{
	mpNormalMap = nullptr;
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;

	mMovement = { 0.0f, 0.0f };
	//mWaveHeight = 7.5f;
	mWaveHeight = 0.0f;
	mWaveScale = 0.6f;
	mRefractionDistortion = 20.0f;
	mReflectionDistortion = 16.0f;
	mMaxDistortionDistance = 40.0f;
	mRefractionStrength = 0.95f;
	mReflectionStrength = 0.9f;
	mWaterDepth = 6.0f;
}


CWater::~CWater()
{
}

bool CWater::Initialise(ID3D11Device* device, D3DXVECTOR3 minPoint, D3DXVECTOR3 maxPoint, unsigned int subDivisionX, unsigned int subDivisionZ, std::string normalMap, int screenWidth, int screenHeight)
{
	// Release the existing data.
	Shutdown();

	if (!InitialiseBuffers(device, minPoint, maxPoint, subDivisionX, subDivisionZ))
	{
		logger->GetInstance().WriteLine("Failed to initialise the buffers of the body of water.");
		return false;
	}

	mpNormalMap = new CTexture();
	if (!mpNormalMap->Initialise(device, "Resources/Textures/WaterNormalHeight.png"))
	{
		logger->GetInstance().WriteLine("Failed to load the normal map for water. Filename was: " + normalMap);
		return false;
	}

	///////////////////////////
	// Refraction render target
	//////////////////////////

	mpRefractionRenderTexture = new CRenderTexture();
	if (!mpRefractionRenderTexture->Initialise(device, screenWidth, screenHeight, DXGI_FORMAT_R8G8B8A8_UNORM))
	{
		logger->GetInstance().WriteLine("Failed to initialise the refraction render texture.");
		return false;
	}

	///////////////////////////
	// Reflection render target
	///////////////////////////

	mpReflectionRenderTexture = new CRenderTexture();
	if (!mpReflectionRenderTexture->Initialise(device, screenWidth, screenHeight, DXGI_FORMAT_R8G8B8A8_UNORM))
	{
		logger->GetInstance().WriteLine("Failed to initialise the reflection render texture.");
		return false;
	}

	///////////////////////////
	// Height map render target
	///////////////////////////

	mpHeightRenderTexture = new CRenderTexture();
	if (!mpHeightRenderTexture->Initialise(device, screenWidth, screenHeight, DXGI_FORMAT_R32_FLOAT))
	{
		logger->GetInstance().WriteLine("Failed to initialise the height render texture.");
		return false;
	}

	logger->GetInstance().WriteLine("Successfully loaded the texture for the water model.");

	return true;
}

void CWater::Shutdown()
{
	if (mpHeightRenderTexture)
	{
		mpHeightRenderTexture->Shutdown();
		delete mpHeightRenderTexture;
		mpHeightRenderTexture = nullptr;
	}

	if (mpRefractionRenderTexture)
	{
		mpRefractionRenderTexture->Shutdown();
		delete mpRefractionRenderTexture;
		mpRefractionRenderTexture = nullptr;
	}

	if (mpReflectionRenderTexture)
	{
		mpReflectionRenderTexture->Shutdown();
		delete mpReflectionRenderTexture;
		mpReflectionRenderTexture = nullptr;
	}

	if (mpNormalMap)
	{
		mpNormalMap->Shutdown();
		delete mpNormalMap;
		mpNormalMap = nullptr;
	}

	if (mpIndexBuffer)
	{
		mpIndexBuffer->Release();
		mpIndexBuffer = nullptr;
	}

	if (mpVertexBuffer)
	{
		mpVertexBuffer->Release();
		mpVertexBuffer = nullptr;
	}
}

void CWater::Render(ID3D11DeviceContext * deviceContext)
{
	RenderBuffers(deviceContext);
}

void CWater::Update(float frameTime)
{
	// Mess around with these numbers until you find something you like.
	mMovement.x += frameTime * 0.005f;
	mMovement.y += frameTime  * 0.007f;
}

bool CWater::InitialiseBuffers(ID3D11Device * device, D3DXVECTOR3 minPoint, D3DXVECTOR3 maxPoint, unsigned int subDivisionX, unsigned int subDivisionZ, bool uvs /* = true */, bool normals/* = true*/)
{
	unsigned int vertexSize = sizeof(VertexType);
	mNumVertices = (subDivisionX + 1) * (subDivisionZ + 1);
	float xStep = (maxPoint.x - minPoint.x) / subDivisionX;
	float zStep = (maxPoint.z - minPoint.z) / subDivisionZ;
	float uStep = 1.0f / subDivisionX;
	float vStep = 1.0f / subDivisionZ;
	D3DXVECTOR3 pt = minPoint;
	D3DXVECTOR3 normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	D3DXVECTOR2 uv = D3DXVECTOR2(0.0f, 1.0f);

	auto vertices = std::make_unique<char[]>(mNumVertices * vertexSize);

	auto vert = vertices.get();
	for (int z = 0; z <= subDivisionZ; ++z)
	{
		for (int x = 0; x <= subDivisionX; ++x)
		{
			*reinterpret_cast<D3DXVECTOR3*>(vert) = pt;
			vert += sizeof(D3DXVECTOR3);

			*reinterpret_cast<D3DXVECTOR2*>(vert) = uv;
			vert += sizeof(D3DXVECTOR2);

			pt.x += xStep;
			uv.x += uStep;

			*reinterpret_cast<D3DXVECTOR3*>(vert) = normal;
			vert += sizeof(D3DXVECTOR3);
		}
		pt.x = minPoint.x;
		pt.z += zStep;
		uv.x = 0;
		uv.y -= vStep; // V axis is opposite direction to Z
	}


	mNumIndices = subDivisionX * subDivisionZ * 6;
	int mIndexSize = 4;
	auto indices = std::make_unique<char[]>(mNumIndices * mIndexSize);

	uint32_t tlIndex = 0;
	auto index = reinterpret_cast<uint32_t*>(indices.get());

	for (int z = 0; z < subDivisionZ; ++z)
	{
		for (int x = 0; x < subDivisionX; ++x)
		{
			*index++ = tlIndex;
			*index++ = tlIndex + subDivisionX + 1;
			*index++ = tlIndex + 1;

			*index++ = tlIndex + 1;
			*index++ = tlIndex + subDivisionX + 1;
			*index++ = tlIndex + subDivisionX + 2;
			++tlIndex;
		}
		++tlIndex;
	}

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = mNumVertices * vertexSize;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = vertices.get();
	if (FAILED(device->CreateBuffer(&bufferDesc, &initData, &mpVertexBuffer)))
	{
		return false;
	}
	
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = mNumIndices * mIndexSize;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	initData.pSysMem = indices.get();
	if (FAILED(device->CreateBuffer(&bufferDesc, &initData, &mpIndexBuffer)))
	{
		return false;
	}

	return true;
}

unsigned int CWater::GetNumberOfIndices()
{
	return mNumIndices;
}

CTexture * CWater::GetNormalMap()
{
	return mpNormalMap;
}

CRenderTexture * CWater::GetRefractionTexture()
{
	return mpRefractionRenderTexture;
}

CRenderTexture * CWater::GetReflectionTexture()
{
	return mpReflectionRenderTexture;
}

CRenderTexture * CWater::GetHeightTexture()
{
	return mpHeightRenderTexture;
}

void CWater::SetRefractionRenderTarget(ID3D11DeviceContext * deviceContext, ID3D11DepthStencilView * depthStencilView)
{
	mpRefractionRenderTexture->SetRenderTarget(deviceContext, depthStencilView);
}

void CWater::SetReflectionRenderTarget(ID3D11DeviceContext * deviceContext, ID3D11DepthStencilView * depthStencilView)
{
	mpReflectionRenderTexture->SetRenderTarget(deviceContext, depthStencilView);
}

void CWater::SetHeightMapRenderTarget(ID3D11DeviceContext * deviceContext, ID3D11DepthStencilView * depthStencilView)
{
	mpHeightRenderTexture->SetRenderTarget(deviceContext, depthStencilView);
}

void CWater::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;

	// Set the vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	// Set the vertex buffer to active in the input assembler.
	deviceContext->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler.
	deviceContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Tell directx we've passed it a triangle list in the form of indices.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

D3DXVECTOR2 CWater::GetMovement()
{
	return mMovement;
}

float CWater::GetWaterMovementX()
{
	return mMovement.x;
}

float CWater::GetWaterMovementY()
{
	return mMovement.y;
}

float CWater::GetWaveHeight()
{
	return mWaveHeight;
}

float CWater::GetWaveScale()
{
	return mWaveScale;
}

float CWater::GetRefractionDistortion()
{
	return mRefractionDistortion;
}

float CWater::GetReflectionDistortion()
{
	return mReflectionDistortion;
}

float CWater::GetMaxDistortionDistance()
{
	return mMaxDistortionDistance;
}

float CWater::GetRefractionStrength()
{
	return mRefractionStrength;
}

float CWater::GetReflectionStrength()
{
	return mReflectionStrength;
}

float CWater::GetDepth()
{
	return mWaterDepth;
}

void CWater::SetMovementX(float movementX)
{
	mMovement.x = movementX;
}

void CWater::SetMovementY(float movementY)
{
	mMovement.y = movementY;
}

void CWater::SetWaveHeight(float height)
{
	mWaveHeight = height;
}

void CWater::SetWaveScale(float scale)
{
	mWaveScale = scale;
}

void CWater::SetRefractionDistortion(float refractionDistortion)
{
	mRefractionDistortion = refractionDistortion;
}

void CWater::SetReflectionDistortion(float reflectionDistortion)
{
	mReflectionDistortion = reflectionDistortion;
}

void CWater::SetRefractionStrength(float refractionStrength)
{
	mRefractionStrength = refractionStrength;
}

void CWater::SetReflectionStrength(float reflectionStrength)
{
	mReflectionStrength = reflectionStrength;
}

void CWater::SetDepth(float depth)
{
	mWaterDepth = depth;
}
