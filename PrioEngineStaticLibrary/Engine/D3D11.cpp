#include "D3D11.h"

CD3D11::CD3D11()
{
	mGraphicsCardName = "";
	mpAlphaBlendingStateDisabled = nullptr;
	mpAlphaBlendingStateEnabled = nullptr;	
	mpRasterStateNoCulling = nullptr;
	mpAdditiveAlphaBlendingStateEnabled = nullptr;
	mpDepthState = nullptr;
}


CD3D11::~CD3D11()
{
}

/* Setup of Direct3D for DirectX 11. */
bool CD3D11::Initialise(int screenWidth, int screenHeight, bool vsync, HWND hwnd, bool fullscreen, float screenDepth, float screenNear)
{
	HRESULT result;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes;
	unsigned int refRateNumerator;
	unsigned int refRateDenominator;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthEnabledStencilBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilBufferDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	D3D11_RASTERIZER_DESC noCullRasterDesc;
	D3D11_VIEWPORT viewport;
	D3D11_BLEND_DESC blendStateDesc;

	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;
	mFullscreen = fullscreen;

	// Store the vsync setting.
	mVsyncEnabled = vsync;

	// Create a Direct X Graphics interface
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	// If we did not successfully create a Direct X graphics interface.
	if (FAILED(result))
	{
		// Output failure message to log.
		logger->GetInstance().WriteLine("Failed to create a Direct X Graphics Interface Factory.");
		// Stop!
		return false;
	}

	// Create an adapter from the factory for the graphics card we will use.
	result = factory->EnumAdapters(0, &adapter);
	// Check to see if we successfully created an adapter for our graphics card.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to create an adapter for our graphics card.");
		// Stop!
		return false;
	}

	// Set our adapter's output variable to our primary display monitor.
	result = adapter->EnumOutputs(0, &adapterOutput);
	// Check that we have successfully initialised our adapters output variable.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to initialise adapter outputs. This means we could not find the primary monitor the graphics card is connected to.");
		// Stop!
		return false;
	}

	// Get the number of modes that will fit our display format for the monitor.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	// Check that we successfully acquired the number of modes.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Did not successfully acquire the number of modes that our monitor supports.");
		// Stop!
		return false;
	}

	// Store all the possible display modes for our graphics card and monitor combo.
	displayModeList = new DXGI_MODE_DESC[numModes];
	// Check that we successfully stored any display modes.
	if (!displayModeList)
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to store any display modes for this graphics card and monitor.");
		// Stop!
		return false;
	}

	logger->GetInstance().MemoryAllocWriteLine(typeid(displayModeList).name());

	// Fill the display mode list struct.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	// Check if we were successfull populating the display mode list structure.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to populate the display mode list structure.");
		// Stop!
		return false;
	}

	// Iterate through display modes to find one which matches screen width and height. When found store the refresh rate.
	for (unsigned int i = 0; i < numModes; i++)
	{
		// If width is the same as our screen.
		if (displayModeList[i].Width == (unsigned int) screenWidth)
		{
			// If height is the same as our screen.
			if (displayModeList[i].Height == (unsigned int)screenHeight)
			{
				// Store numerator for the refresh rate of this monitor.
				refRateNumerator = displayModeList[i].RefreshRate.Numerator;
				// Store denominator for the refresh rate of this monitor.
				refRateDenominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	// Get the graphics card (or adapter) description.
	result = adapter->GetDesc(&adapterDesc);
	// Check that we were successful retrieving a description of our graphics card.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to retrieve the description of the graphics card / adapter.");
		// Stop!
		return false;
	}

	// Store how much VRAM we found on the graphics card.
	mGraphicsCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	/// Convert the name of the graphics card to a string from WCHAR* and store it.
	WCHAR* graphicsCardDescWCHAR = adapterDesc.Description;
	std::wstring graphicsCardDescWS(graphicsCardDescWCHAR);
	std::string graphicsCardDescStr(graphicsCardDescWS.begin(), graphicsCardDescWS.end());
	mGraphicsCardDescription = graphicsCardDescStr;
	

	// If we did not manage to parse the graphics card name from the descriptor.
	if (mGraphicsCardDescription == "")
	{
		// Output the error message to the log.
		logger->GetInstance().WriteLine("Could not parse the name of the graphics card from the descriptor.");
		// Stop!
		return false;
	}
	
	mGraphicsCardName = mGraphicsCardDescription;
	

	// Output the name of the graphics card to the log.
	logger->GetInstance().WriteLine("Got the name of the graphics card: '" + mGraphicsCardName + "'" );

	// Release the structures and interfaces we used to get monitor refresh rate.
	delete [] displayModeList;
	displayModeList = NULL;
	logger->GetInstance().MemoryDeallocWriteLine(typeid(displayModeList).name());

	// Release the adapter output (monitor).
	adapterOutput->Release();
	adapterOutput = NULL;

	// Release the adapter (graphics card).
	adapter->Release();
	adapter = NULL;

	// Release the factory.
	factory->Release();
	factory = NULL;

	/* Create swap chain descriptor. */
	CreateSwapChainDesc(hwnd, swapChainDesc, refRateNumerator, refRateDenominator);
	if (!CreateSwapChain(featureLevel, swapChainDesc))
	{
		return false;
	}

	// Grab pointer to the back buffer.
	result = mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	// If we did not successfully grab the pointer to the back buffer.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Could not get the pointer to the back buffer.");
		// Stop!
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = mpDevice->CreateRenderTargetView(backBufferPtr, NULL, &mpRenderTargetView);
	// If we did not successfully create the render target view.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to create the render target view.");
		// Stop!
		return false;
	}

	// Release the pointer to the back buffer.
	backBufferPtr->Release();
	backBufferPtr = nullptr;

	/* Create depth buffer. */
	if (!CreateDepthBuffer(depthBufferDesc))
	{
		return false;
	}

	/* Create depth stencil buffer. */

	if (!CreateDepthStencilBuffer(depthEnabledStencilBufferDesc))
	{
		return false;
	}

	if (!CreateDepthDisabledStencilState(depthDisabledStencilBufferDesc))
	{
		return false;
	}

	// Put the depth stencil buffer into effect!
	mpDeviceContext->OMSetDepthStencilState(mpDepthEnabledStencilState, 1);

	/* Create the depth stencil view. */

	if (!CreateDepthStencilView(depthStencilViewDesc))
	{
		return false;
	}

	/* Direct X Graphics Options. */

	if (!InitRasterizer(rasterDesc))
	{
		return false;
	}

	// Setup a raster description which turns off back face culling.
	noCullRasterDesc.AntialiasedLineEnable = false;
	noCullRasterDesc.CullMode = D3D11_CULL_NONE;
	noCullRasterDesc.DepthBias = 0;
	noCullRasterDesc.DepthBiasClamp = 0.0f;
	noCullRasterDesc.DepthClipEnable = true;
	noCullRasterDesc.FillMode = D3D11_FILL_SOLID;
	noCullRasterDesc.FrontCounterClockwise = false;
	noCullRasterDesc.MultisampleEnable = false;
	noCullRasterDesc.ScissorEnable = false;
	noCullRasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the no culling rasterizer state.
	result = mpDevice->CreateRasterizerState(&noCullRasterDesc, &mpRasterStateNoCulling);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the rasterizer state with no culling.");
		return false;
	}

	/* Set up the viewport for Direct3D to clip map space coordinates. */

	// Setup the viewport for rendering.
	InitViewport(viewport);

	/* Create projection matrix. */

	// Setup the projection matrix.
	CreateProjMatrix(screenDepth, screenNear);

	/* Create world matrix. */

	// Initialise the world matrix to the identity matrix.d
	D3DXMatrixIdentity(&mWorldMatrix);

	/* Create orthographic projection matrix. */

	// Create an orthographic projection matrix for 2D rendering of things like interfaces and sprites.
	D3DXMatrixOrthoLH(&mOrthoMatrix, (float)screenWidth, (float)screenHeight, screenNear, screenDepth);

	 // Create the state using the blend state description.
	 // Clear memory 
	 ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));

	 // Set up the descriptor for blend state. Look over second year notes if you've forgotten what this does.
	 blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	 blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	 blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	 blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	 blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	 blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	 blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	 blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;


	 result = mpDevice->CreateBlendState(&blendStateDesc, &mpAlphaBlendingStateEnabled);
	 if (FAILED(result))
	 {
		 logger->GetInstance().WriteLine("Failed to create the alpha blending state enabled from descriptor.");
		 return false;
	 }

	 // Alter to disable alpha blending.
	 blendStateDesc.RenderTarget[0].BlendEnable = FALSE;

	 result = mpDevice->CreateBlendState(&blendStateDesc, &mpAlphaBlendingStateDisabled);
	 if (FAILED(result))
	 {
		 logger->GetInstance().WriteLine("Failed to create the alpha blending state disabled from descriptor.");
		 return false;
	 }
	 //////////////////////////////
	 // Additive alpha blending.
	 //////////////////////////////

	 // Set up the descriptor for blend state. Look over second year notes if you've forgotten what this does.
	 blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	 blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	 blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;;
	 blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	 blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	 blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	 blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	 blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0f;


	 result = mpDevice->CreateBlendState(&blendStateDesc, &mpAdditiveAlphaBlendingStateEnabled);
	 if (FAILED(result))
	 {
		 logger->GetInstance().WriteLine("Failed to create the additive alpha blending state enabled from descriptor.");
		 return false;
	 }

	// Success! We have successfully setup DirectX.
	return true;
}

/* Clean up function. Should be called when the application is closing. Always call from parent cleanup function, NOWHERE ELSE. */
void CD3D11::Shutdown()
{
	logger->GetInstance().WriteLine("DirectX Shutdown Function Initialised.");

	if (mpDepthState != nullptr)
	{
		mpDepthState->Release();
		mpDepthState = nullptr;
	}

	// If swap chain has been intialised.
	if (mpSwapChain)
	{
		// Set to windowed mode before shutting down or swap chain throws an exception.
		mpSwapChain->SetFullscreenState(false, NULL);
		// Output change in window to log.
		logger->GetInstance().WriteLine("Set to windowed mode.");
	}

	// If blending state initialised.
	if (mpAlphaBlendingStateEnabled)
	{
		mpAlphaBlendingStateEnabled->Release();
		mpAlphaBlendingStateEnabled = nullptr;
	}

	if (mpAdditiveAlphaBlendingStateEnabled)
	{
		mpAdditiveAlphaBlendingStateEnabled->Release();
		mpAdditiveAlphaBlendingStateEnabled = nullptr;
	}

	// If blending state initialised.
	if (mpAlphaBlendingStateDisabled)
	{
		mpAlphaBlendingStateDisabled->Release();
		mpAlphaBlendingStateDisabled = nullptr;
	}

	// If rasterizer state has been initialised.
	if (mpRasterizerState)
	{
		// Release the rasterizer.
		mpRasterizerState->Release();
		mpRasterizerState = nullptr;
	}

	if (mpRasterStateNoCulling)
	{
		mpRasterStateNoCulling->Release();
		mpRasterStateNoCulling = nullptr;
	}

	// If depth stencil view has been initialised.
	if (mpDepthStencilView)
	{
		// Release the depth stencil view.
		mpDepthStencilView->Release();
		mpDepthStencilView = nullptr;
	}

	// If depth stencil state has been initialised.
	if (mpDepthEnabledStencilState)
	{
		// Release the depth stencil state.
		mpDepthEnabledStencilState->Release();
		mpDepthEnabledStencilState = nullptr;
	}

	if (mpDepthDisabledStencilState)
	{
		mpDepthDisabledStencilState->Release();
		mpDepthDisabledStencilState = nullptr;
	}

	// If depth stencil buffer has been initialised.
	if (mpDepthStencilBuffer)
	{
		// Release the depth stencil buffer.
		mpDepthStencilBuffer->Release();
		mpDepthStencilBuffer = nullptr;
	}

	// If render target view has been initialised.
	if (mpRenderTargetView)
	{
		// Release the render target view.
		mpRenderTargetView->Release();
		mpRenderTargetView = nullptr;
	}

	TwTerminate();

	// If device context has been initialised.
	if (mpDeviceContext)
	{
		// Release the device context.
		mpDeviceContext->Release();
		mpDeviceContext = nullptr;
	}

	// If device has been initialised.
	if (mpDevice)
	{
		// Release the device.
		mpDevice->Release();
		mpDevice = nullptr;
	}

	// If swap chain has been initialised.
	if (mpSwapChain)
	{
		// Release the swap chain.
		mpSwapChain->Release();
		mpSwapChain = nullptr;
	}

	// Output success message to log.
	logger->GetInstance().WriteLine("Successfully cleaned up anything related to directX used for this program.");

	// Complete! Return.
	return;
}

void CD3D11::BeginScene(float red, float green, float blue, float alpha)
{
	float colour[4];

	// Setup the colour to clear the buffer to.
	colour[0] = red;
	colour[1] = green;
	colour[2] = blue;
	colour[3] = alpha;

	// Clear the back buffer.
	mpDeviceContext->ClearRenderTargetView(mpRenderTargetView, colour);

	// Clear the depth buffer.
	mpDeviceContext->ClearDepthStencilView(mpDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	return;
}

void CD3D11::EndScene()
{
	// Present the back buffer to the screen after rendering as the rendering has completed now.
	if (mVsyncEnabled)
	{
		// Lock screen to refresh rate.
		mpSwapChain->Present(1, 0);
	}
	else
	{
		// Present as soon as it's ready.
		mpSwapChain->Present(0, 0);
	}

	return;
}

ID3D11Device * CD3D11::GetDevice()
{
	return mpDevice;
}

ID3D11DeviceContext * CD3D11::GetDeviceContext()
{
	return mpDeviceContext;
}

void CD3D11::GetProjectionMatrix(D3DMATRIX & projMatrix)
{
	projMatrix = mProjectionMatrix;
	return;
}

void CD3D11::GetWorldMatrix(D3DMATRIX & worldMatrix)
{
	worldMatrix = mWorldMatrix;
	return;
}

void CD3D11::GetOrthogonalMatrix(D3DMATRIX & orthogMatrix)
{
	orthogMatrix = mOrthoMatrix;
	return;
}

void CD3D11::EnableWireframeFill()
{
	HRESULT	result;

	mpRasterizerState->Release();

	D3D11_RASTERIZER_DESC rasterDesc;

	// Set up the description for the raster which dictates how many polygons are drawn.
	rasterDesc.AntialiasedLineEnable = false;
	//rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the previously defined description.
	result = mpDevice->CreateRasterizerState(&rasterDesc, &mpRasterizerState);
	// If we failed to create the rasterizer.
	if (FAILED(result))
	{
		// Log the error message
		logger->GetInstance().WriteLine("Failed to create the rasterizer from the description provided.");
	}

	// Set the rasterizer state.
	mpDeviceContext->RSSetState(mpRasterizerState);
	logger->GetInstance().WriteLine("Rasterizer state changed to use wireframe fill.");
}

void CD3D11::EnableSolidFill()
{
	HRESULT result;
	mpRasterizerState->Release();

	D3D11_RASTERIZER_DESC rasterDesc;

	// Set up the description for the raster which dictates how many polygons are drawn.
	rasterDesc.AntialiasedLineEnable = false;
	//rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the previously defined description.
	result = mpDevice->CreateRasterizerState(&rasterDesc, &mpRasterizerState);
	// If we failed to create the rasterizer.
	if (FAILED(result))
	{
		// Log the error message
		logger->GetInstance().WriteLine("Failed to create the rasterizer from the description provided.");
	}

	// Set the rasterizer state.
	mpDeviceContext->RSSetState(mpRasterizerState);

	logger->GetInstance().WriteLine("Rasterizer state changed to use solid fill.");
}

void CD3D11::EnableAlphaBlending()
{
	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	mpDeviceContext->OMSetBlendState(mpAlphaBlendingStateEnabled, blendFactor, 0xffffffff);
}

void CD3D11::DisableAlphaBlending()
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mpDeviceContext->OMSetBlendState(mpAlphaBlendingStateDisabled, blendFactor, 0xffffffff);
}

void CD3D11::EnableAdditiveAlphaBlending()
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mpDeviceContext->OMSetBlendState(mpAdditiveAlphaBlendingStateEnabled, blendFactor, 0xffffffff);
}

void CD3D11::DisableZBuffer()
{
	// Create the stencil buffer from the descriptor.
	mpDeviceContext->OMSetDepthStencilState(mpDepthDisabledStencilState, 1);
}

void CD3D11::EnableZBuffer()
{
	// Create the stencil buffer from the descriptor.
	mpDeviceContext->OMSetDepthStencilState(mpDepthEnabledStencilState, 1);
}

/* Gets information about the graphics card that DirectX is using. */
//void CD3D11::GetGraphicsCardInfo(char * cardName, int & memory)
//{
//	strcpy_s(cardName, STRING_NUMBER_OF_BITS, mGraphicsCardDescription);
//	memory = mGraphicsCardMemory;
//	return;
//}

/* Creates a swaph and chain from the descriptor passed in, this will switch the back and front buffers depending on what is ready to be displayed.*/
void CD3D11::CreateSwapChainDesc(HWND hwnd, DXGI_SWAP_CHAIN_DESC& swapChainDesc, int refRateNumerator, int refRateDenominator)
{
	/* Create swap chain. */

	// Initialise the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = mScreenWidth;
	swapChainDesc.BufferDesc.Height = mScreenHeight;

	// Set 32 bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (mVsyncEnabled)
	{
		// Set the refresh rate numerator to what we detected from the monitor adapter earlier.
		swapChainDesc.BufferDesc.RefreshRate.Numerator = refRateNumerator;
		// Set the refresh rate denominator to what we detected from the monitor adapter earlier.
		swapChainDesc.BufferDesc.RefreshRate.Denominator = refRateDenominator;
	}
	else
	{
		// Set the refresh rate to be uncapped by using 0 for the numerator and 1 for the denominator.
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the render target for the buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn off multisampling.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set fullscreen or windowed.
	if (mFullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Clean the back buffer after the contents are presented.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set any of the advanced flags.
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
}

/* Creates the swap and chain which is used to switch between the front and back buffers, allowing one to present and one to render. */
bool CD3D11::CreateSwapChain(D3D_FEATURE_LEVEL & featureLevel, DXGI_SWAP_CHAIN_DESC & swapChainDesc)
{
	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create swap chain from the descriptor we have set.
	HRESULT result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL /*D3D11_CREATE_DEVICE_DEBUG*/, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &mpSwapChain, &mpDevice, NULL, &mpDeviceContext);

	// If we did not successfully create the device and swap chain.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Could not successfully create the device and swap chain.");
		// Stop!
		return false;
	}

	return true;
}

/* Initialises a depth buffer descriptor which is passed in by reference, and creates the depth buffer from that descriptor. */
bool CD3D11::CreateDepthBuffer(D3D11_TEXTURE2D_DESC& depthBufferDesc)
{
	HRESULT result;

	// Initialise the description of the debpth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = mScreenWidth;
	depthBufferDesc.Height = mScreenHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// Create depth buffer according to our description.
	result = mpDevice->CreateTexture2D(&depthBufferDesc, NULL, &mpDepthStencilBuffer);
	// If we did not successfully create the depth buffer.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to create the depth buffer from the description given.");
		// Stop!
		return false;
	}

	return true;
}

/* Initialises a depth stencil view descriptor which is passed in by reference, and creates it. */
bool CD3D11::CreateDepthStencilView(D3D11_DEPTH_STENCIL_VIEW_DESC& depthStencilViewDesc)
{
	HRESULT result;
	// Initialise the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	

	// Create the depth stencil view.
	result = mpDevice->CreateDepthStencilView(mpDepthStencilBuffer, &depthStencilViewDesc, &mpDepthStencilView);
	// If we did not successfully create the depth stencil view.
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to create the depth stencil view.");
		// Stop!
		return false;
	}

	ID3D11RenderTargetView* nullRenderTarget = nullptr;
	GetDeviceContext()->OMSetRenderTargets(1, &nullRenderTarget, nullptr);

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	mpDeviceContext->OMSetRenderTargets(1, &mpRenderTargetView, mpDepthStencilView);

	return true;
}

/* Initialises a depth stencil buffer descriptor which is passed in as a by reference parameter. */
bool CD3D11::CreateDepthStencilBuffer(D3D11_DEPTH_STENCIL_DESC& depthStencilBufferDesc)
{
	HRESULT result;

	// Initialise the description of the stencil state.
	depthStencilBufferDesc = {};

	// Set up the description of the stencil state.
	depthStencilBufferDesc.DepthEnable = true;
	depthStencilBufferDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilBufferDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilBufferDesc.StencilEnable = true;
	depthStencilBufferDesc.StencilReadMask = 0xFF;
	depthStencilBufferDesc.StencilWriteMask = 0xFF;

	// Stencil operations when pixel is front-facing.
	depthStencilBufferDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilBufferDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilBufferDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilBufferDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back facing.
	depthStencilBufferDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilBufferDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilBufferDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilBufferDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the stencil buffer from the descriptor.
	result = mpDevice->CreateDepthStencilState(&depthStencilBufferDesc, &mpDepthEnabledStencilState);
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to create the depth stencil buffer from the descriptor provided.");
		// Stop!
		return false;
	}

	return true;
}

bool CD3D11::CreateDepthDisabledStencilState(D3D11_DEPTH_STENCIL_DESC& depthStencilBufferDesc)
{
	HRESULT result;
	// Clear the second depth stencil state before setting the parameters.
	ZeroMemory(&depthStencilBufferDesc, sizeof(depthStencilBufferDesc));

	// Now create a second depth stencil state which turns off the Z buffer for 2D rendering.  The only difference is 
	// that DepthEnable is set to false, all other parameters are the same as the other depth stencil state.
	depthStencilBufferDesc.DepthEnable = false;
	depthStencilBufferDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilBufferDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilBufferDesc.StencilEnable = true;
	depthStencilBufferDesc.StencilReadMask = 0xFF;
	depthStencilBufferDesc.StencilWriteMask = 0xFF;
	depthStencilBufferDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilBufferDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilBufferDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilBufferDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilBufferDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilBufferDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilBufferDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilBufferDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the stencil buffer from the descriptor.
	result = mpDevice->CreateDepthStencilState(&depthStencilBufferDesc, &mpDepthDisabledStencilState);
	if (FAILED(result))
	{
		// Log the error message.
		logger->GetInstance().WriteLine("Failed to create the depth stencil buffer from the descriptor provided.");
		// Stop!
		return false;
	}

	return true;
}

/* Initialises a rasterizer descriptor which is passed in by reference, and attempts to create the rasterizer. */
bool CD3D11::InitRasterizer(D3D11_RASTERIZER_DESC& rasterDesc)
{
	HRESULT result;

	// Set up the description for the raster which dictates how many polygons are drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the previously defined description.
	result = mpDevice->CreateRasterizerState(&rasterDesc, &mpRasterizerState);
	// If we failed to create the rasterizer.
	if (FAILED(result))
	{
		// Log the error message
		logger->GetInstance().WriteLine("Failed to create the rasterizer from the description provided.");
		// Stop!
		return false;
	}

	// Set the rasterizer state.
	mpDeviceContext->RSSetState(mpRasterizerState);

	return true;
}

/* Initialises a viewport which is passed in by reference. */
void CD3D11::InitViewport(D3D11_VIEWPORT& viewport)
{
	// Setup the viewport for rendering.
	viewport.Width = static_cast<float>(mScreenWidth);
	viewport.Height = static_cast<float>(mScreenHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	// Create the viewport.
	mpDeviceContext->RSSetViewports(1, &viewport);
}

/* Creates the projection matrix, this allows us to view in 3D on a 2D monitor by mapping what should be projected onto the screen. */
void CD3D11::CreateProjMatrix(float screenDepth, float screenNear)
{
	float fieldOfView;
	float screenAspect;

	// Setup the projection matrix.
	fieldOfView = static_cast<float>(D3DX_PI / 4.0f);
	screenAspect = static_cast<float>(mScreenWidth) / static_cast<float>(mScreenHeight);

	// Create the projection matrix for 3D rendering.
	D3DXMatrixPerspectiveFovLH(&mProjectionMatrix, fieldOfView, screenAspect, screenNear, screenDepth);
}

void CD3D11::SetDepthState(bool depth, bool stencil, bool depthWrite)
{
	if (mpDepthState != nullptr)
	{
		mpDepthState->Release();
		mpDepthState = nullptr;
	}

	D3D11_DEPTH_STENCIL_DESC depthStateDesc;

	ZeroMemory(&depthStateDesc, sizeof(depthStateDesc));
	depthStateDesc.DepthEnable = depth;
	depthStateDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStateDesc.DepthWriteMask = depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStateDesc.StencilEnable = stencil;
	mpDevice->CreateDepthStencilState(&depthStateDesc, &mpDepthState);
	mpDeviceContext->OMSetDepthStencilState(mpDepthState, 1);

}

bool CD3D11::ToggleFullscreen(bool fullscreenEnabled)
{
	HRESULT result;
	result = mpSwapChain->SetFullscreenState(fullscreenEnabled, NULL);

	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to swap to fullscreen using the swapchain in D3D11.cpp.");
		return false;
	}

	return true;
}

void CD3D11::TurnOnBackFaceCulling()
{
	mpDeviceContext->RSSetState(mpRasterizerState);
}

void CD3D11::TurnOffBackFaceCulling()
{
	mpDeviceContext->RSSetState(mpRasterStateNoCulling);
}

ID3D11DepthStencilView * CD3D11::GetDepthStencilView()
{
	return mpDepthStencilView;
}

void CD3D11::SetBackBufferRenderTarget()
{
	mpDeviceContext->OMSetRenderTargets(1, &mpRenderTargetView, mpDepthStencilView);
}
