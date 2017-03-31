#include "RenderTexture.h"



CRenderTexture::CRenderTexture()
{
}


CRenderTexture::~CRenderTexture()
{
}

bool CRenderTexture::Initialise(ID3D11Device * device, int width, int height, DXGI_FORMAT format)
{
	HRESULT result;

	D3D11_TEXTURE2D_DESC textureDesc = { 0 };
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	result = device->CreateTexture2D(&textureDesc, NULL, &mpRenderTargetTexture);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the render target texture from the texture description provided in rendertexture class.");
		return false;
	}

	result = device->CreateRenderTargetView(mpRenderTargetTexture, NULL, &mpRenderTargetView);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the render target view from the render target view desc provided in rendertexture class.");
		return false;
	}

	result = device->CreateShaderResourceView(mpRenderTargetTexture, NULL, &mpShaderResourceView);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the shader resource view from the desc provided in render texture class.");
		return false;
	}
	
	logger->GetInstance().WriteLine("Successfully initialised render texture.");

	return true;
}

void CRenderTexture::Shutdown()
{
	if (mpShaderResourceView)
	{
		mpShaderResourceView->Release();
		mpShaderResourceView = nullptr;
	}

	if (mpRenderTargetView)
	{
		mpRenderTargetView->Release();
		mpRenderTargetView = nullptr;
	}

	if (mpRenderTargetTexture)
	{
		mpRenderTargetTexture->Release();
		mpRenderTargetView = nullptr;
	}
}

void CRenderTexture::SetRenderTarget(ID3D11DeviceContext * deviceContext, ID3D11DepthStencilView * depthStencilView)
{
	deviceContext->OMSetRenderTargets(1, &mpRenderTargetView, depthStencilView);
}

void CRenderTexture::ClearRenderTarget(ID3D11DeviceContext * deviceContext, ID3D11DepthStencilView * depthStencilView, float red, float green, float blue, float alpha)
{
	float colour[4];

	colour[0] = red;
	colour[1] = green;
	colour[2] = blue;
	colour[3] = alpha;

	deviceContext->ClearRenderTargetView(mpRenderTargetView, colour);

	deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

}

ID3D11ShaderResourceView * &CRenderTexture::GetShaderResourceView()
{
	return mpShaderResourceView;
}
