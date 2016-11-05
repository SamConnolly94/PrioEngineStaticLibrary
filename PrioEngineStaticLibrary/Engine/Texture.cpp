#include "Texture.h"



CTexture::CTexture()
{
	mpTexture = nullptr;
}


CTexture::~CTexture()
{
}

/* Load in a texture. */
bool CTexture::Initialise(ID3D11Device * device, WCHAR * filename)
{
	HRESULT result;

	// Load in the texture.
	result = D3DX11CreateShaderResourceViewFromFile(device, filename, NULL, NULL, &mpTexture, NULL);

	if (FAILED(result))
	{
		mpLogger->GetLogger().WriteLine("Failed to load the texture in Texture.cpp.");
		return false;
	}

	return true;
}

/* Deallocates memory and cleans up after object. */
void CTexture::Shutdown()
{
	// Let go of the sample texture.
	mpTexture->Release();
	mpTexture = nullptr;
}

/* Return the texture which is loaded. */
ID3D11ShaderResourceView * CTexture::GetTexture()
{
	return mpTexture;
}
