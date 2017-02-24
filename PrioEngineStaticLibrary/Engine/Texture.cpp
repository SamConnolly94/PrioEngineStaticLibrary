#include "Texture.h"


CTexture::CTexture()
{
	mpTexture = nullptr;
}


CTexture::~CTexture()
{
}

/* Load in a texture. */
bool CTexture::Initialise(ID3D11Device * device, std::string filename)
{
	mFilename = filename;
	HRESULT result;

	// Load in the texture.
	result = D3DX11CreateShaderResourceViewFromFile(device, filename.c_str(), NULL, NULL, &mpTexture, NULL);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to load the texture '" + filename + "'in Texture.cpp.");
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

std::wstring CTexture::s2ws(const std::string str)
{
	int len;
	int slength = (int)str.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}
