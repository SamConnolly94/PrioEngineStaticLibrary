#include "Graphics.h"

CGraphics::CGraphics()
{
	// Initialise the Direct 3D class to null.
	mpD3D = nullptr;
	mpCamera = nullptr;
	mpTriangle = nullptr;
	mpColourShader = nullptr;
	mpTextureShader = nullptr;
	mpDiffuseLightShader = nullptr;
	mpTerrainShader = nullptr;
	mWireframeEnabled = false;
	mFieldOfView = static_cast<float>(D3DX_PI / 4);
	mpText = nullptr;
	mFullScreen = false;
	mpFrustum = nullptr;
	mpSkyboxShader = nullptr;
	mpTerrain = nullptr;
	mpSkybox = nullptr;
	mpCloudPlane = nullptr;
	mpCloudShader = nullptr;
	mFrameTime = 0.0f;
	mpRain = nullptr;
	mpRainShader = nullptr;
}

CGraphics::~CGraphics()
{
}

bool CGraphics::Initialise(int screenWidth, int screenHeight, HWND hwnd)
{
	bool successful;

	mHwnd = hwnd;

	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;

	// Create the Direct3D object.
	mpD3D = new CD3D11;
	// Check for successful creation of the object.
	if (!mpD3D)
	{
		// Output failure message to the log.
		logger->GetInstance().WriteLine("Did not successfully create the D3D11 object.");
		//Don't continue with the init function any more.
		return false;
	}
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpD3D).name());

	// Initialise the D3D object.
	successful = mpD3D->Initialise(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, mFullScreen, SCREEN_DEPTH, SCREEN_NEAR);
	// If failed to init D3D, then output error.
	if (!successful)
	{
		// Write the error to the log.
		logger->GetInstance().WriteLine("Failed to initialised Direct3D.");
		// Write the error to a message box too.
		MessageBox(hwnd, "Could not initialise Direct3D.", "Error", MB_OK);
		// Do not continue with this function any more.
		return false;
	}

	// Create a colour shader now, it's necessary for terrain.
	CreateColourShader(hwnd);
	CreateTextureShaderForModel(hwnd);
	CreateTextureAndDiffuseLightShaderFromModel(hwnd);
	CreateTerrainShader(hwnd);

	mpFrustum = new CFrustum();

	mpCamera = CreateCamera();
	mpCamera->Render();

	//mpReflectionCamera = new CCamera(mScreenWidth, mScreenWidth, mFieldOfView, SCREEN_NEAR, SCREEN_DEPTH);

	/// SET UP TEXT FROM CAMERA POS.
	mpText = new CGameText();

	if (!mpText)
	{
		logger->GetInstance().WriteLine("Failed to allocate memory to the GameText var in Graphics.cpp.");
		return false;
	}

	D3DXMATRIX baseView;
	mpCamera->GetViewMatrix(baseView);
	if (!mpText->Initialise(mpD3D->GetDevice(), mpD3D->GetDeviceContext(), mHwnd, mScreenWidth, mScreenHeight, baseView))
	{
		logger->GetInstance().WriteLine("Failed to initailise the text object in graphics.cpp.");
		return false;
	}
	mBaseView = baseView;

	logger->GetInstance().WriteLine("Setting up AntTweakBar.");
	TwInit(TW_DIRECT3D11, mpD3D->GetDevice());
	TwWindowSize(mScreenWidth, mScreenHeight);
	logger->GetInstance().WriteLine("AntTweakBar successfully initialised. ");

	//////////////////////////////////////////////
	// Skybox shader

	mpSkyboxShader = new CSkyboxShader();
	if (!mpSkyboxShader)
	{
		logger->GetInstance().WriteLine("Failed to allocate memory to the skybox shader.");
		return false;
	}

	bool result = mpSkyboxShader->Initialise(mpD3D->GetDevice(), hwnd);
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to initialise the skybox shader.");
		return false;
	}

	CreateSkybox();

	D3DXVECTOR4 horizonColour = mpSkybox->GetCenterColour();

	// Set up the diffuse colour for the light.
	D3DXVECTOR4 diffuseColour = horizonColour;

	// Copy the colour of the horizon for the ambient colour.
	D3DXVECTOR4 ambientColour = horizonColour;

	ambientColour.x *= mAmbientMultiplier;
	ambientColour.y *= mAmbientMultiplier;
	ambientColour.z *= mAmbientMultiplier;
	ambientColour.w = 1.0f;

	// Set the direction of our light.
	D3DXVECTOR3 direction(0.0f, 0.0f, 1.0f);

	// Set up specular vairables for our light.
	float specularPower = 0.1f;
	D3DXVECTOR4 specularColour = ambientColour;

	mpSceneLight = CreateLight(diffuseColour, specularColour, specularPower, ambientColour, direction);


	mpWaterShader = new CWaterShader();
	if (!mpWaterShader->Initialise(mpD3D->GetDevice(), hwnd))
	{
		logger->GetInstance().WriteSubtitle("Critical Error!");
		logger->GetInstance().WriteLine("Failed to initialise the water shader, shutting down.");
		logger->GetInstance().CloseSubtitle();

		return false;
	}

	mpRefractionShader = new CReflectRefractShader();

	if (!mpRefractionShader->Initialise(mpD3D->GetDevice(), hwnd))
	{
		logger->GetInstance().WriteSubtitle("Critical Error!");
		logger->GetInstance().WriteLine("Failed to initialise the refraction shader, shutting down.");
		logger->GetInstance().CloseSubtitle();
		return false;
	}

	mpCloudPlane = new CCloudPlane();
	if (!mpCloudPlane->Initialise(mpD3D->GetDevice(), "Resources/Textures/cloud1.dds", "Resources/Textures/cloud2.dds"))
	{
		logger->GetInstance().WriteLine("Failed to initialise the cloud plane in graphics class.");
		return false;
	}

	mpCloudShader = new CCloudShader();
	if (!mpCloudShader->Initialise(mpD3D->GetDevice(), hwnd))
	{
		logger->GetInstance().WriteLine("Failed tto initialise the cloud shader in graphics class.");
		return false;
	}

	mpRain = new CRain();
	if (!mpRain->Initialise(mpD3D->GetDevice(), "Resources/Textures/raindrop.dds", 25000))
	{
		logger->GetInstance().WriteLine("Failed to initialise the rain particle emitter.");
		return false;
	}

		
	mpRainShader = new CRainShader();
	if (!mpRainShader->Initialise(mpD3D->GetDevice(), hwnd))
	{
		logger->GetInstance().WriteLine("Failed to initialise the rain shader.");
		return false;
	}

	mpCamera->SetPosition(0.0f, 50.0f, 20.0f);
	// Success!
	logger->GetInstance().WriteLine("Direct3D was successfully initialised.");
	return true;
}

void CGraphics::Shutdown()
{
	if (mpRain)
	{
		mpRain->Shutdown();
		delete mpRain;
		mpRain = nullptr;
	}

	if (mpRainShader)
	{
		mpRainShader->Shutdown();
		delete mpRainShader;
		mpRainShader = nullptr;
	}

	if (mpCloudPlane)
	{
		mpCloudPlane->Shutdown();
		delete mpCloudPlane;
		mpCloudPlane = nullptr;
	}

	if (mpCloudShader)
	{
		mpCloudShader->Shutdown();
		delete mpCloudShader;
		mpCloudShader = nullptr;
	}
	if (mpSkybox)
	{
		mpSkybox->Shutdown();
		delete mpSkybox;
		mpSkybox = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpSkybox).name());
	}

	if (mpSkyboxShader)
	{
		mpSkyboxShader->Shutdown();
		delete mpSkyboxShader;
		mpSkyboxShader = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpSkyboxShader).name());
	}

	if (mpText)
	{
		mpText->Shutdown();
		delete mpText;
		mpText = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpText).name());
	}

	for (auto image : mpUIImages)
	{
		image->Shutdown();
		delete image;
		image = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(image).name());
	}

	if (mpFrustum != nullptr)
	{
		delete mpFrustum;
	}

	mpUIImages.clear();

	if (mpDiffuseLightShader)
	{
		mpDiffuseLightShader->Shutdown();
		delete mpDiffuseLightShader;
		mpDiffuseLightShader = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpDiffuseLightShader).name());
	}

	if (mpTerrainShader)
	{
		mpTerrainShader->Shutdown();
		delete mpTerrainShader;
		mpTerrainShader = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpTerrainShader).name());
	}

	if (mpTextureShader)
	{
		mpTextureShader->Shutdown();
		delete mpTextureShader;
		mpTextureShader = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpTextureShader).name());
	}

	if (mpColourShader)
	{
		mpColourShader->Shutdown();
		delete mpColourShader;
		mpColourShader = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpColourShader).name());
	}

	// Deallocate any allocated memory on the primitives list.
	std::list<CPrimitive*>::iterator it;
	it = mpPrimitives.begin();

	while (it != mpPrimitives.end())
	{
		(*it)->Shutdown();
		delete (*it);
		(*it) = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid((*it)).name());
		it++;
	}

	while (!mpPrimitives.empty())
	{
		mpPrimitives.pop_back();
	}

	// Deallocate any allocated memroy on the mesh list.
	for (auto mesh : mpMeshes)
	{
		mesh->Shutdown();
		delete mesh;
	}
	mpMeshes.clear();

	if (mpSceneLight)
	{
		delete mpSceneLight;
		mpSceneLight = nullptr;
	}

	if (mpTerrain)
	{
		delete mpTerrain;
		mpTerrain = nullptr;
	}

	// Remove camera.

	if (mpCamera)
	{
		delete mpCamera;
		mpCamera = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpCamera).name());
	}

	if (mpWaterShader)
	{
		mpWaterShader->Shutdown();
		delete mpWaterShader;
		mpWaterShader = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpWaterShader).name());
	}

	if (mpRefractionShader)
	{
		mpRefractionShader->Shutdown();
		delete mpRefractionShader;
		mpRefractionShader = nullptr;
	}

	// If the Direct 3D object exists.
	if (mpD3D)
	{
		// Clean up the D3D object before we get rid of it.
		mpD3D->Shutdown();
		delete mpD3D;
		mpD3D = nullptr;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mpD3D).name());
		// Output message to log to let us know that this object is gone.
		logger->GetInstance().WriteLine("Direct3D object has been shutdown, deallocated and pointer set to null.");
	}

	return;
}

bool CGraphics::Frame(float updateTime)
{
	bool success;

	UpdateScene(updateTime);

	// Render the graphics scene.
	success = Render();

	// If we did not successfully render the scene.
	if (!success)
	{
		// Output a message to the log.
		logger->GetInstance().WriteLine("Failed to render the scene.");
		// Prevent the program from continuing any further.
		return false;
	}
	return true;
}

void CGraphics::UpdateScene(float updateTime)
{
	mFrameTime = updateTime;
	mRunTime += updateTime;

	mpCamera->Render();

	for (auto primitive : mpPrimitives)
	{
		primitive->UpdateMatrices();
	}

	mpCloudPlane->Update(updateTime);

	if (mpTerrain)
	{
		mpTerrain->Update(updateTime);
		mpTerrain->UpdateMatrices();

		if (!mpTerrain->GetUpdateFlag())
		{
			mpCamera->RenderReflection(mpTerrain->GetWater()->GetPosY());
		}
	}

	if (mUpdateToDayTime)
	{
		// If we completed the skybox update.
		if (mpSkybox->UpdateToDay(updateTime))
		{
			mTimeSinceLastSkyboxUpdate = 0.0f;
			mUpdateToDayTime = false;
		}

		// Update the scene directional light.
		mpSceneLight->SetDiffuseColour(mpSkybox->GetCenterColour());
		D3DXVECTOR4 ambient = mpSceneLight->GetDiffuseColour();
		ambient *= mAmbientMultiplier;
		ambient.w = 1.0f;
		mpSceneLight->SetAmbientColour(ambient);
	}
	else if (mUpdateToEveningTime)
	{
		// If we completed the skybox update.
		if (mpSkybox->UpdateToEvening(updateTime))
		{
			mTimeSinceLastSkyboxUpdate = 0.0f;
			mUpdateToEveningTime = false;
		}

		// Update the scene directional light.
		mpSceneLight->SetDiffuseColour(mpSkybox->GetCenterColour());
		D3DXVECTOR4 ambient = mpSceneLight->GetDiffuseColour();
		ambient *= mAmbientMultiplier;
		ambient.w = 1.0f;
		mpSceneLight->SetAmbientColour(ambient);
	}
	else if (mUpdateToNightTime)
	{
		// If we completed the skybox update.
		if (mpSkybox->UpdateToNight(updateTime))
		{
			mTimeSinceLastSkyboxUpdate = 0.0f;
			mUpdateToNightTime = false;
		}

		// Update the scene directional light.
		mpSceneLight->SetDiffuseColour(mpSkybox->GetCenterColour());
		D3DXVECTOR4 ambient = mpSceneLight->GetDiffuseColour();
		ambient *= mAmbientMultiplier;
		ambient.w = 1.0f;
		mpSceneLight->SetAmbientColour(ambient);
	}
	else if (mTimeSinceLastSkyboxUpdate > mSkyboxUpdateInterval && mUseTimeBasedSkybox)
	{
		// If we completed the skybox update.
		if (mpSkybox->UpdateTimeOfDay(updateTime))
		{
			mTimeSinceLastSkyboxUpdate = 0.0f;
		}

		// Update the scene directional light.
		mpSceneLight->SetDiffuseColour(mpSkybox->GetCenterColour());
		D3DXVECTOR4 ambient = mpSceneLight->GetDiffuseColour();
		ambient *= mAmbientMultiplier;
		ambient.w = 1.0f;
		mpSceneLight->SetAmbientColour(ambient);
	}

	mTimeSinceLastSkyboxUpdate += updateTime;

	mpRain->Update(updateTime);
}

bool CGraphics::Render()
{
	D3DXMATRIX viewMatrix;
	D3DXMATRIX worldMatrix;
	D3DXMATRIX projMatrix;
	D3DXMATRIX viewProj;
	D3DXMATRIX orthoMatrix;
	mpD3D->GetOrthogonalMatrix(orthoMatrix);

	// Set the back buffer as the render target
	mpD3D->SetBackBufferRenderTarget();
	mpD3D->GetDeviceContext()->ClearDepthStencilView(mpD3D->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Clear buffers so we can begin to render the scene.
	mpD3D->BeginScene(0.6f, 0.9f, 1.0f, 1.0f);

	// Get the world, view and projection matrices from the old camera and d3d objects.
	mpCamera->GetViewMatrix(viewMatrix);
	mpD3D->GetWorldMatrix(worldMatrix);
	mpD3D->GetProjectionMatrix(projMatrix);
	mpD3D->GetOrthogonalMatrix(orthoMatrix);
	mpCamera->GetViewProjMatrix(viewProj, projMatrix);

	mpFrustum->ConstructFrustum(SCREEN_DEPTH, projMatrix, viewMatrix);

	if (!RenderSkybox(worldMatrix, viewMatrix, projMatrix, viewProj))
		return false;

	if (!RenderModels(worldMatrix, viewMatrix, projMatrix, viewProj))
		return false;

	if (!RenderBitmaps(mBaseView, mBaseView, orthoMatrix, viewProj))
		return false;

	if (!RenderText(worldMatrix, mBaseView, orthoMatrix, viewProj))
		return false;

	TwDraw();

	// Present the rendered scene to the screen.
	mpD3D->EndScene();

	return true;
}

/* Renders any primitive shapes on the scene. */
bool CGraphics::RenderPrimitives(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj, D3DXMATRIX viewProj)
{
	std::list<CPrimitive*>::iterator primitivesIt;
	primitivesIt = mpPrimitives.begin();

	while (primitivesIt != mpPrimitives.end())
	{
		(*primitivesIt)->GetWorldMatrix(world);

		// put the model vertex and index buffers on the graphics pipleline to prepare them for dawing.
		(*primitivesIt)->Render(mpD3D->GetDeviceContext());

		// Render texture with no light.
		if ((*primitivesIt)->HasTexture() && !(*primitivesIt)->UseDiffuseLight())
		{
			mpTextureShader->SetViewProjMatrix(viewProj);
			if (!RenderPrimitiveWithTexture((*primitivesIt), world, view, proj))
			{
				return false;
			}
		}
		// Render texture with light.
		else if ((*primitivesIt)->HasTexture() && (*primitivesIt)->UseDiffuseLight())
		{
			// Pretty sure I broke this ages ago. Do I even care or ever want to render primitives?
			// Probably not, but not certain yet.
			// TODO: Investigate if I need this code.
			//if (!RenderPrimitiveWithTextureAndDiffuseLight((*primitivesIt), world, view, proj))
			//{
			//	return false;
			//}
		}
		// Render colour.
		else if ((*primitivesIt)->HasColour())
		{
			mpColourShader->SetViewProjMatrix(viewProj);
			if (!RenderPrimitiveWithColour((*primitivesIt), world, view, proj))
			{
				return false;
			}
		}
		primitivesIt++;
	}

	return true;
}

/* Render any meshes / instances of meshes which we have created on the scene. */
bool CGraphics::RenderMeshes(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj, D3DXMATRIX viewProj)
{
	if (mpTerrain->GetUpdateFlag())
	{
		// Skip render pass.
		logger->GetInstance().WriteLine("Updating terrain, skip the render pass.");
		return true;
	}

	mpDiffuseLightShader->SetViewMatrix(view);
	mpDiffuseLightShader->SetProjMatrix(proj);
	mpDiffuseLightShader->SetViewProjMatrix(viewProj);

	// Render any models which belong to each mesh. Do this in batches to make it faster.
	for (auto mesh : mpMeshes)
	{
		mesh->Render(mpD3D->GetDeviceContext(), mpFrustum, mpDiffuseLightShader, mpSceneLight);
	}

	return true;
}

/* Render the terrain and all areas inside of it. */
bool CGraphics::RenderTerrains(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj, D3DXMATRIX viewProj)
{
	// If we haven't actually initialised our terrain yet.
	if (!mpTerrain)
	{
		// Output a message to the logger.
		logger->GetInstance().WriteLine("Terrain has not yet been initialised, skipping render pass.");
		// Completed.
		return true;
	}


	// Update the world matrix and perform operations on the world matrix of this object.
	mpTerrain->GetWorldMatrix(world);

	mpTerrain->Render(mpD3D->GetDeviceContext());

	if (mpSceneLight)
	{
		mpTerrainShader->SetWorldMatrix(world);
		mpTerrainShader->SetViewMatrix(view);
		mpTerrainShader->SetProjMatrix(proj);
		mpTerrainShader->SetViewProjMatrix(viewProj);

		// Render the terrain area with the diffuse light shader.
		if (!mpTerrainShader->Render(mpD3D->GetDeviceContext(),
			mpTerrain->GetIndexCount(),
			mpTerrain->GetTexturesArray(),
			mpTerrain->GetNumberOfTextures(),
			mpTerrain->GetGrassTextureArray(),
			mpTerrain->GetNumberOfGrassTextures(),
			mpTerrain->GetRockTextureArray(),
			mpTerrain->GetNumberOfRockTextures(),
			mpSceneLight->GetDirection(),
			mpSceneLight->GetDiffuseColour(),
			mpSceneLight->GetAmbientColour(),
			mpTerrain->GetHighestPoint(),
			mpTerrain->GetLowestPoint(),
			mpTerrain->GetPos(),
			mpTerrain->GetSnowHeight(),
			mpTerrain->GetGrassHeight(),
			mpTerrain->GetDirtHeight(),
			mpTerrain->GetSandHeight()
		))
		{
			logger->GetInstance().WriteSubtitle("Critical error.");
			logger->GetInstance().WriteLine("Failed to render the terrain with terrain render shader.");
			logger->GetInstance().CloseSubtitle();

			// If we failed to render, return false.
			return false;
		}
	}

	// Successfully rendered the terrain.
	return true;

}

bool CGraphics::RenderSkybox(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj, D3DXMATRIX viewProj)
{
	if (!mpSkybox)
	{
		logger->GetInstance().WriteLine("No skybox exists, so skipping render pass for skybox.");
		return true;
	}

	D3DXVECTOR3 cameraPosition;

	// Get the position of the camera.
	cameraPosition = mpCamera->GetPosition();

	// Translate the sky dome to be centered around the camera position.
	D3DXMatrixTranslation(&world, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	// Turn off back face culling.
	mpD3D->TurnOffBackFaceCulling();

	// Turn off the Z buffer.
	mpD3D->DisableZBuffer();

	// Render the sky dome using the sky dome shader.
	mpSkybox->Render(mpD3D->GetDeviceContext());

	mpSkyboxShader->SetWorldMatrix(world);
	mpSkyboxShader->SetViewMatrix(view);
	mpSkyboxShader->SetProjMatrix(proj);
	mpSkyboxShader->SetViewProjMatrix(viewProj);

	mpSkyboxShader->Render(mpD3D->GetDeviceContext(), mpSkybox->GetIndexCount(), 
		mpSkybox->GetApexColor(), mpSkybox->GetCenterColour());

	// Turn back face culling back on.
	mpD3D->TurnOnBackFaceCulling();

	// Allow the clouds to additively blend with the skybox.
	mpD3D->EnableAdditiveAlphaBlending();

	// Place the cloud plane vertex / index data onto the rendering pipeline.
	mpCloudPlane->Render(mpD3D->GetDeviceContext());

	// Set shader variables before rendering clouds with the shader.
	mpCloudShader->SetBrightness(mpCloudPlane->GetBrightness());
	mpCloudShader->SetCloud1Movement(mpCloudPlane->GetMovement(0).x, mpCloudPlane->GetMovement(0).y);
	mpCloudShader->SetCloud2Movement(mpCloudPlane->GetMovement(1).x, mpCloudPlane->GetMovement(1).y);

	mpCloudShader->SetWorldMatrix(world);
	mpCloudShader->SetViewMatrix(view);
	mpCloudShader->SetProjMatrix(proj);
	mpCloudShader->SetViewProjMatrix(viewProj);
	mpCloudShader->SetCloudTexture1(mpCloudPlane->GetCloudTexture1());
	mpCloudShader->SetCloudTexture2(mpCloudPlane->GetCloudTexture2());

	// Render the clouds using vertex and pixel shaders.
	mpCloudShader->Render(mpD3D->GetDeviceContext(), mpCloudPlane->GetIndexCount());

	// Turn off alpha blending.
	mpD3D->DisableAlphaBlending();

	// Turn the Z buffer back on.
	mpD3D->EnableZBuffer();

	// Reset the world matrix.
	mpD3D->GetWorldMatrix(world);

	return true;
}

/* Renders physical entities within the scene. */
bool CGraphics::RenderModels(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj, D3DXMATRIX viewProj)
{
	if (!RenderWater(world, view, proj, viewProj))
		return false;

	if (!RenderPrimitives(world, view, proj, viewProj))
		return false;

	if (!RenderMeshes(world, view, proj, viewProj))
		return false;

	if (!RenderTerrains(world, view, proj, viewProj))
		return false;

	if (!RenderRain(world, view, proj, viewProj))
		return false;

	return true;
}

bool CGraphics::RenderText(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX ortho, D3DXMATRIX viewProj)
{
	mpD3D->DisableZBuffer();
	mpD3D->EnableAlphaBlending();

	bool result = mpText->Render(mpD3D->GetDeviceContext(), world, ortho);

	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to render text.");
		return false;
	}


	mpD3D->EnableZBuffer();
	mpD3D->DisableAlphaBlending();

	return true;
}

bool CGraphics::RenderBitmaps(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX ortho, D3DXMATRIX viewProj)
{
	mpD3D->DisableZBuffer();
	mpD3D->EnableAlphaBlending();

	bool result;

	for (auto image : mpUIImages)
	{
		result = image->Render(mpD3D->GetDeviceContext(), image->GetX(), image->GetY());

		if (!result)
		{
			logger->GetInstance().WriteLine("Failed to render.");
			return false;
		}
	}

	mpTextureShader->SetWorldMatrix(world);
	mpTextureShader->SetViewMatrix(view);
	mpTextureShader->SetProjMatrix(ortho);
	mpTextureShader->SetViewProjMatrix(viewProj);

	for (auto image : mpUIImages)
	{
		result = mpTextureShader->Render(mpD3D->GetDeviceContext(), image->GetNumberOfIndices(), image->GetTexture());

		if (!result)
		{
			logger->GetInstance().WriteLine("Failed to render with texture shader.");
			return false;
		}
	}

	mpD3D->EnableZBuffer();
	mpD3D->DisableAlphaBlending();

	return true;
}

SentenceType * CGraphics::CreateSentence(std::string text, int posX, int posY, int maxLength)
{
	return mpText->CreateSentence(mpD3D->GetDevice(), mpD3D->GetDeviceContext(), text, posX, posY, maxLength);
}

bool CGraphics::UpdateSentence(SentenceType *& sentence, std::string text, int posX, int posY, PrioEngine::RGB colour)
{
	return mpText->UpdateSentence(sentence, text, posX, posY, colour.r, colour.g, colour.b, mpD3D->GetDeviceContext());
}

bool CGraphics::RemoveSentence(SentenceType *& sentence)
{
	return mpText->RemoveSentence(sentence);
}

C2DImage * CGraphics::CreateUIImages(std::string filename, int width, int height, int posX, int posY)
{
	/// Set up image.

	C2DImage* image = new C2DImage();

	bool successful = image->Initialise(mpD3D->GetDevice(), mScreenWidth, mScreenHeight, filename, width, height);

	image->SetX(posX);
	image->SetY(posY);

	if (!successful)
	{
		logger->GetInstance().WriteLine("Failed to initialise C2DSprite in Graphics.cpp.");
		return false;
	}
	logger->GetInstance().MemoryAllocWriteLine(typeid(image).name());

	// Push image to the list.
	mpUIImages.push_back(image);

	return image;

}

bool CGraphics::RemoveUIImage(C2DImage *& element)
{
	std::list<C2DImage*>::iterator it = mpUIImages.begin();

	while (it != mpUIImages.end())
	{
		if ((*it) == element)
		{
			(*it)->Shutdown();
			delete (*it);
			logger->GetInstance().MemoryDeallocWriteLine(typeid(*it).name());
			(*it) = nullptr;
			mpUIImages.erase(it);
			element = nullptr;
			return true;
		}
	}

	return false;
}

bool CGraphics::UpdateTerrainBuffers(CTerrain *& terrain, double ** heightmap, int width, int height)
{
	return terrain->UpdateBuffers(mpD3D->GetDevice(), mpD3D->GetDeviceContext(), heightmap, width, height);
}

bool CGraphics::IsFullscreen()
{
	return mFullScreen;
}

bool CGraphics::SetFullscreen(bool isEnabled)
{
	mFullScreen = isEnabled;

	// Set to windowed mode before shutting down or swap chain throws an exception.
	if (!mpD3D->ToggleFullscreen(mFullScreen))
		return false;

	return true;
}

CSkyBox * CGraphics::CreateSkybox()
{
	CSkyBox* skybox;
	logger->GetInstance().WriteLine("Setting up skybox.");
	skybox = new CSkyBox();
	if (!skybox)
	{
		logger->GetInstance().WriteLine("Failed to allocate memory to skybox.");
		return false;
	}

	bool result = skybox->Initialise(mpD3D->GetDevice());
	if (!result)
	{
		logger->GetInstance().WriteLine("Failed to create the skybox.");
		return false;
	}

	// Deallocate the old one before copying it over.
	if (mpSkybox)
	{
		mpSkybox->Shutdown();
		delete mpSkybox;
		mpSkybox = nullptr;
		logger->GetInstance().WriteLine("Deallocating the skybox as a new one is being created, this should avoid memory leaks.");
	}

	mpSkybox = skybox;

	return skybox;
}


bool CGraphics::RenderWater(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj, D3DXMATRIX viewProj)
{
	if (mpTerrain == nullptr)
	{
		logger->GetInstance().WriteLine("Skipping water as there's nothing to render.");
		return true;
	}
	if (mpTerrain->GetWater() == nullptr)
	{
		logger->GetInstance().WriteLine("No body of water exists for this terrain yet, skipping render pass.");
		return true;
	}
	if (mpTerrain->GetUpdateFlag())
	{
		logger->GetInstance().WriteLine("Skipping render pass for water as the terrain is currently being updated.");
		return true;
	}

	bool result = true;

	// Reset the world matrix.
	mpD3D->GetWorldMatrix(world);

	/////////////////////////////////
	// Height
	////////////////////////////////

	D3DXMATRIX camWorld;
	mpWaterShader->SetWorldMatrix(world);
	mpWaterShader->SetViewMatrix(view);
	mpWaterShader->SetProjMatrix(proj);
	mpWaterShader->SetViewProjMatrix(viewProj);

	mpWaterShader->SetWaterMovement(mpTerrain->GetWater()->GetMovement());
	mpWaterShader->SetWaveHeight(mpTerrain->GetWater()->GetWaveHeight());
	mpWaterShader->SetWaveScale(mpTerrain->GetWater()->GetWaveScale());
	mpWaterShader->SetDistortion(mpTerrain->GetWater()->GetRefractionDistortion(), mpTerrain->GetWater()->GetReflectionDistortion());
	mpWaterShader->SetMaxDistortion(mpTerrain->GetWater()->GetMaxDistortionDistance());
	mpWaterShader->SetRefractionStrength(mpTerrain->GetWater()->GetRefractionStrength());
	mpWaterShader->SetReflectionStrength(mpTerrain->GetWater()->GetReflectionStrength());
	mpCamera->GetWorldMatrix(camWorld);
	mpWaterShader->SetCameraMatrix(camWorld);
	mpWaterShader->SetCameraPosition(mpCamera->GetPosition());
	mpWaterShader->SetViewportSize(mScreenWidth, mScreenHeight);
	mpWaterShader->SetLightProperties(mpSceneLight);
	mpWaterShader->SetNormalMap(mpTerrain->GetWater()->GetNormalMap());

	// Set render target to the height texture map.
	mpTerrain->GetWater()->SetHeightMapRenderTarget(mpD3D->GetDeviceContext(), mpD3D->GetDepthStencilView());
	
	mpTerrain->GetWater()->GetHeightTexture()->ClearRenderTarget(mpD3D->GetDeviceContext(), mpD3D->GetDepthStencilView(), 0.0f, 0.0f, 0.0f, 0.0f);
	//mpD3D->GetDeviceContext()->ClearDepthStencilView(mpD3D->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Render the water height map.
	mpTerrain->GetWater()->Render(mpD3D->GetDeviceContext());
	mpTerrain->GetWater()->GetWorldMatrix(world);
	mpWaterShader->SetWorldMatrix(world);
	result = mpWaterShader->RenderHeight(mpD3D->GetDeviceContext(), mpTerrain->GetWater()->GetNumberOfIndices());

	if (!result)
	{
		return false;
	}

	// Two requirements for refraction and refleciton are a light and terrain.
	if (mpTerrain && mpSceneLight)
	{
		//////////////////////////////
		// Refraction
		//////////////////////////////

		mpD3D->GetWorldMatrix(world);
		// Reset the terrain world matrix
		mpTerrain->GetWorldMatrix(world);

		
		mpTerrain->GetWater()->SetRefractionRenderTarget(mpD3D->GetDeviceContext(), mpD3D->GetDepthStencilView());
		//mpD3D->GetDeviceContext()->ClearDepthStencilView(mpD3D->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Place our refract / reflect properties into the refract reflect shader.
		mpRefractionShader->SetWorldMatrix(world);
		mpRefractionShader->SetViewMatrix(view);
		mpRefractionShader->SetProjMatrix(proj);
		mpRefractionShader->SetViewProjMatrix(viewProj);
		mpRefractionShader->SetLightProperties(mpSceneLight);
		mpRefractionShader->SetViewportProperties(mScreenWidth, mScreenHeight);
		mpRefractionShader->SetTerrainAreaProperties(mpTerrain->GetSnowHeight(), mpTerrain->GetGrassHeight(), mpTerrain->GetDirtHeight(), mpTerrain->GetSandHeight());
		mpRefractionShader->SetPositioningProperties(mpTerrain->GetPosY(), mpTerrain->GetWater()->GetPosY());
		mpRefractionShader->SetWaterHeightmap(mpTerrain->GetWater()->GetHeightTexture()->GetShaderResourceView());
		mpRefractionShader->SetDirtTextureArray(mpTerrain->GetTexturesArray());
		mpRefractionShader->SetGrassTextureArray(mpTerrain->GetGrassTextureArray());
		mpRefractionShader->SetPatchMap(mpTerrain->GetPatchMap());
		mpRefractionShader->SetRockTexture(mpTerrain->GetRockTextureArray());

		mpTerrain->Render(mpD3D->GetDeviceContext()); 
		mpTerrain->GetWater()->GetRefractionTexture()->ClearRenderTarget(mpD3D->GetDeviceContext(), mpD3D->GetDepthStencilView(), mpSceneLight->GetDiffuseColour().x, mpSceneLight->GetDiffuseColour().y, mpSceneLight->GetDiffuseColour().z, 1.0f);
		result = mpRefractionShader->RefractionRender(mpD3D->GetDeviceContext(), mpTerrain->GetIndexCount());

		if (!result)
		{
			logger->GetInstance().WriteLine("Failed to render the refraction shader for water. ");
			return false;
		}

		/////////////////////////////////
		// Reflection
		/////////////////////////////////

			//mpCamera->GetReflectionViewMatrix(view);
			// Render vertices using reflection shader.
			mpTerrain->GetWater()->GetReflectionTexture()->ClearRenderTarget(mpD3D->GetDeviceContext(), mpD3D->GetDepthStencilView(), mpSceneLight->GetDiffuseColour().x, mpSceneLight->GetDiffuseColour().y, mpSceneLight->GetDiffuseColour().z, 1.0f);

			mpTerrain->GetWater()->SetReflectionRenderTarget(mpD3D->GetDeviceContext(), mpD3D->GetDepthStencilView());
			mpD3D->GetDeviceContext()->ClearDepthStencilView(mpD3D->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

			mpCamera->GetReflectionView(view);
			mpRefractionShader->SetWorldMatrix(world);
			mpRefractionShader->SetViewMatrix(view);
			mpRefractionShader->SetProjMatrix(proj);
			mpRefractionShader->SetViewProjMatrix(view * proj);

			mpD3D->TurnOffBackFaceCulling();

			/////////////////////////////
			// SKYBOX
			////////////////////////////
			mpD3D->DisableZBuffer();
			mpD3D->GetWorldMatrix(world);
			// Translate the sky dome to be centered around the camera position.
			D3DXMatrixTranslation(&world, mpCamera->GetPosition().x, mpCamera->GetPosition().y, mpCamera->GetPosition().z);
			// Allow the clouds to additively blend with the skybox.
			mpD3D->EnableAdditiveAlphaBlending();

			// Place the cloud plane vertex / index data onto the rendering pipeline.
			mpCloudPlane->Render(mpD3D->GetDeviceContext());

			// Set shader variables before rendering clouds with the shader.
			mpCloudShader->SetBrightness(mpCloudPlane->GetBrightness());
			mpCloudShader->SetCloud1Movement(mpCloudPlane->GetMovement(0).x, mpCloudPlane->GetMovement(0).y);
			mpCloudShader->SetCloud2Movement(mpCloudPlane->GetMovement(1).x, mpCloudPlane->GetMovement(1).y);
			mpCloudShader->SetWorldMatrix(world);
			mpCloudShader->SetViewMatrix(view);
			mpCloudShader->SetProjMatrix(proj);
			mpCloudShader->SetViewProjMatrix(view * proj);
			mpCloudShader->SetCloudTexture1(mpCloudPlane->GetCloudTexture1());
			mpCloudShader->SetCloudTexture2(mpCloudPlane->GetCloudTexture2());

			// Render the clouds using vertex and pixel shaders.
			mpCloudShader->Render(mpD3D->GetDeviceContext(), mpCloudPlane->GetIndexCount());

			// Turn off alpha blending.
			mpD3D->DisableAlphaBlending();
			mpD3D->EnableZBuffer();

			////////////////////////////
			// Terrain
			////////////////////////////

			RenderTerrains(world, view, proj, view * proj);
			//// Reset the terrain world matrix
			//mpTerrain->GetWorldMatrix(world);

			///* Render the reflection of the terrain. */

			//// Place vertices onto render pipeline.
			//mpTerrain->Render(mpD3D->GetDeviceContext());

			//result = mpRefractionShader->ReflectionRender(mpD3D->GetDeviceContext(), mpTerrain->GetIndexCount());
			//if (!result)
			//{
			//	logger->GetInstance().WriteLine("Failed to render the reflection shader for water. ");
			//	return false;
			//}

			/* Render the reflection of models within the scene. */

			// Render any models which belong to each mesh. Do this in batches to make it faster.
			mpDiffuseLightShader->SetViewMatrix(view);
			mpDiffuseLightShader->SetProjMatrix(proj);
			mpDiffuseLightShader->SetViewMatrix(view * proj);
			if (mpTerrain->GetUpdateFlag())
			{
				// Skip render pass.
				logger->GetInstance().WriteLine("Updating terrain, skip the render pass.");
			}
			else
			{
				for (auto mesh : mpMeshes)
				{
					mesh->Render(mpD3D->GetDeviceContext(), mpFrustum, mpDiffuseLightShader, mpSceneLight);
				}
			}
			mpD3D->TurnOnBackFaceCulling();

		
	}

	/////////////////////////////////
	// Water Model
	/////////////////////////////////

	mpD3D->SetBackBufferRenderTarget();
	mpD3D->GetDeviceContext()->ClearDepthStencilView(mpD3D->GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	mpTerrain->GetWater()->Render(mpD3D->GetDeviceContext());

	//mpWaterShader->SetNormalMap(mpWater->GetNormalMap());
	mpWaterShader->SetRefractionMap(mpTerrain->GetWater()->GetRefractionTexture()->GetShaderResourceView());
	mpWaterShader->SetReflectionMap(mpTerrain->GetWater()->GetReflectionTexture()->GetShaderResourceView());

	result = mpWaterShader->RenderSurface(mpD3D->GetDeviceContext(), mpTerrain->GetWater()->GetNumberOfIndices());


	if (!result)
	{
		return false;
	}

	return true;
}

bool CGraphics::RenderRain(D3DXMATRIX world, D3DXMATRIX view, D3DXMATRIX proj, D3DXMATRIX viewProj)
{
	mpD3D->GetWorldMatrix(world);

	mpRain->SetEmitterPos(D3DXVECTOR3(0.0f, 40.0f, 100.0f));
	mpRain->SetEmitterDir(D3DXVECTOR3(0.0f, 1.0f, 0.0f));

	mpRainShader->SetCameraWorldPosition(mpCamera->GetPosition());
	mpRainShader->SetEmitterWorldDirection(mpRain->GetEmitterDir());
	mpRainShader->SetEmitterWorldPosition(mpRain->GetEmitterPos());
	mpRainShader->SetFrameTime(mFrameTime);
	mpRainShader->SetGameTime(mpRain->GetAge());
	mpRainShader->SetGravityAcceleration(-9.81f);
	mpRainShader->SetWorldMatrix(world);
	mpRainShader->SetProjMatrix(proj);
	mpRainShader->SetViewMatrix(view);
	mpRainShader->SetViewProjMatrix(viewProj);
	mpRainShader->SetRainTexture(mpRain->GetRainTexture());
	mpRainShader->SetFirstRun(mpRain->GetIsFirstRun());
	mpRainShader->SetWindX(0.0f);
	mpRainShader->SetWindZ(0.0f);

	mpRainShader->SetWorldMatrix(world);
	mpRainShader->SetViewMatrix(view);
	mpRainShader->SetProjMatrix(proj);
	mpRainShader->SetViewProjMatrix(viewProj);

	////////////////////
	// Update
	////////////////////

	mpD3D->TurnOffBackFaceCulling();
	mpD3D->SetDepthState(false, false, false);

	mpRain->UpdateRender(mpD3D->GetDeviceContext());
	mpRainShader->SetRandomTexture(mpRain->GetRandomTexture());
	mpRainShader->SetFirstRun(mpRain->GetIsFirstRun());
	mpRainShader->UpdateRender(mpD3D->GetDeviceContext());
	mpRain->SetFirstRun(false);

	////////////////////
	// Draw
	////////////////////

	mpRain->Render(mpD3D->GetDeviceContext());
	mpD3D->SetDepthState(true, false, false);
	mpRainShader->Render(mpD3D->GetDeviceContext());

	mpD3D->TurnOnBackFaceCulling();

	return true;
}

bool CGraphics::RenderPrimitiveWithColour(CPrimitive* model, D3DMATRIX worldMatrix, D3DMATRIX viewMatrix, D3DMATRIX projMatrix)
{
	bool success = false;

	// Render the model using the colour shader.
	mpColourShader->SetWorldMatrix(worldMatrix);
	mpColourShader->SetViewMatrix(viewMatrix);
	mpColourShader->SetProjMatrix(projMatrix);
	success = mpColourShader->Render(mpD3D->GetDeviceContext(), model->GetIndex());
	if (!success)
	{
		logger->GetInstance().WriteLine("Failed to render the model using the colour shader object.");
		return false;
	}

	return true;
}

bool CGraphics::RenderPrimitiveWithTexture(CPrimitive* model, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix, D3DXMATRIX projMatrix)
{
	bool success = false;

	// Attempt to render the model with the texture specified.
	mpTextureShader->SetWorldMatrix(worldMatrix);
	mpTextureShader->SetViewMatrix(viewMatrix);
	mpTextureShader->SetProjMatrix(projMatrix);
	success = mpTextureShader->Render(mpD3D->GetDeviceContext(), model->GetIndex(), model->GetTexture());

	if (!success)
	{
		logger->GetInstance().WriteLine("Failed to render the model using the texture shader in graphics.cpp.");
		return false;
	}

	return true;
}

CPrimitive* CGraphics::CreatePrimitive(std::string TextureFilename, PrioEngine::Primitives shape)
{
	CPrimitive* model;
	bool successful;

	switch (shape)
	{
	case PrioEngine::Primitives::cube:
		model = new CCube(TextureFilename);
		logger->GetInstance().MemoryAllocWriteLine(typeid(model).name());
		break;
	case PrioEngine::Primitives::triangle:
		model = new CTriangle(TextureFilename);
		logger->GetInstance().MemoryAllocWriteLine(typeid(model).name());
		break;
	default:
		return nullptr;
	}

	if (!model)
	{
		logger->GetInstance().WriteLine("Failed to create the model object");
		return nullptr;
	}

	// Initialise the model object.
	successful = model->Initialise(mpD3D->GetDevice());
	if (!successful)
	{
		logger->GetInstance().WriteLine("*** ERROR! *** Could not initialise the model object");
		MessageBox(mHwnd, "Could not initialise the model object. ", "Error", MB_OK);
		return nullptr;
	}

	if (model->HasTexture() && model->UseDiffuseLight())
	{
		if (!CreateTextureAndDiffuseLightShaderFromModel(mHwnd))
			return nullptr;
	}
	else if (model->HasTexture())
	{
		if (!CreateTextureShaderForModel(mHwnd))
			return nullptr;
	}
	// Place any created models onto the list for the engine to track.
	mpPrimitives.push_back(model);

	return model;
}

CPrimitive* CGraphics::CreatePrimitive(std::string TextureFilename, bool useLighting, PrioEngine::Primitives shape)
{
	CPrimitive* model;
	bool successful;

	switch (shape)
	{
	case PrioEngine::Primitives::cube:
		model = new CCube(TextureFilename, useLighting);
		break;
	case PrioEngine::Primitives::triangle:
		model = new CTriangle(TextureFilename, useLighting);
		break;
	default:
		return nullptr;
	}


	logger->GetInstance().MemoryAllocWriteLine(typeid(model).name());
	if (!model)
	{
		logger->GetInstance().WriteLine("Failed to create the model object");
		return nullptr;
	}

	// Initialise the model object.
	successful = model->Initialise(mpD3D->GetDevice());
	if (!successful)
	{
		logger->GetInstance().WriteLine("*** ERROR! *** Could not initialise the model object");
		MessageBox(mHwnd, "Could not initialise the model object. ", "Error", MB_OK);
		return nullptr;
	}

	if (model->HasTexture() && model->UseDiffuseLight())
	{
		if (!CreateTextureAndDiffuseLightShaderFromModel(mHwnd))
			return nullptr;
	}
	else if (model->HasTexture())
	{
		if (!CreateTextureShaderForModel(mHwnd))
			return nullptr;
	}
	// Place any created models onto the list for the engine to track.
	mpPrimitives.push_back(model);

	return model;
}

CPrimitive* CGraphics::CreatePrimitive(PrioEngine::RGBA colour, PrioEngine::Primitives shape)
{
	CPrimitive* model;
	bool successful;

	switch (shape)
	{
	case PrioEngine::Primitives::cube:
		model = new CCube(colour);
		logger->GetInstance().MemoryAllocWriteLine(typeid(model).name());
		break;
	case PrioEngine::Primitives::triangle:
		model = new CTriangle(colour);
		logger->GetInstance().MemoryAllocWriteLine(typeid(model).name());
		break;
	default:
		return nullptr;
	}

	if (!model)
	{
		logger->GetInstance().WriteLine("Failed to create the model object");
		return nullptr;
	}

	// Initialise the model object.
	successful = model->Initialise(mpD3D->GetDevice());
	if (!successful)
	{
		logger->GetInstance().WriteLine("*** ERROR! *** Could not initialise the model object");
		MessageBox(mHwnd, "Could not initialise the model object. ", "Error", MB_OK);
		return nullptr;
	}

	if (!CreateColourShader(mHwnd))
		return nullptr;

	// Place any created models onto the list for the engine to track.
	mpPrimitives.push_back(model);

	return model;
}

bool CGraphics::CreateTextureAndDiffuseLightShaderFromModel(HWND hwnd)
{
	if (mpDiffuseLightShader == nullptr)
	{
		bool successful;

		// Create texture shader.
		mpDiffuseLightShader = new CDiffuseLightShader();
		logger->GetInstance().MemoryAllocWriteLine(typeid(mpDiffuseLightShader).name());
		if (!mpDiffuseLightShader)
		{
			logger->GetInstance().WriteLine("Failed to create the texture shader object in graphics.cpp.");
			return false;
		}

		// Initialise the texture shader object.
		successful = mpDiffuseLightShader->Initialise(mpD3D->GetDevice(), hwnd);
		if (!successful)
		{
			logger->GetInstance().WriteLine("Failed to initialise the texture shader object in graphics.cpp.");
			MessageBox(hwnd, "Could not initialise the texture shader object.", "Error", MB_OK);
			return false;
		}

	}
	return true;
}

bool CGraphics::CreateTerrainShader(HWND hwnd)
{
	if (mpTerrainShader == nullptr)
	{
		bool successful;

		// Create texture shader.
		mpTerrainShader = new CTerrainShader();
		logger->GetInstance().MemoryAllocWriteLine(typeid(mpTerrainShader).name());
		if (!mpTerrainShader)
		{
			logger->GetInstance().WriteLine("Failed to create the texture shader object in graphics.cpp.");
			return false;
		}

		// Initialise the texture shader object.
		successful = mpTerrainShader->Initialise(mpD3D->GetDevice(), hwnd);
		if (!successful)
		{
			logger->GetInstance().WriteLine("Failed to initialise the mpTerrainShader shader object in graphics.cpp.");
			MessageBox(hwnd, "Could not initialise the mpTerrainShader shader object.", "Error", MB_OK);
			return false;
		}

	}
	return true;
}


bool CGraphics::CreateTextureShaderForModel(HWND hwnd)
{
	if (mpTextureShader == nullptr)
	{
		bool successful;

		// Create texture shader.
		mpTextureShader = new CTextureShader();
		logger->GetInstance().MemoryAllocWriteLine(typeid(mpTextureShader).name());
		if (!mpTextureShader)
		{
			logger->GetInstance().WriteLine("Failed to create the texture shader object in graphics.cpp.");
			return false;
		}

		// Initialise the texture shader object.
		successful = mpTextureShader->Initialise(mpD3D->GetDevice(), hwnd);
		if (!successful)
		{
			logger->GetInstance().WriteLine("Failed to initialise the texture shader object in graphics.cpp.");
			MessageBox(hwnd, "Could not initialise the texture shader object.", "Error", MB_OK);
			return false;
		}

	}
	return true;
}

bool CGraphics::CreateColourShader(HWND hwnd)
{
	if (mpColourShader == nullptr)
	{
		bool successful;

		// Create the colour shader object.
		mpColourShader = new CColourShader();
		if (!mpColourShader)
		{
			logger->GetInstance().WriteLine("Failed to create the colour shader object.");
			return false;
		}
		logger->GetInstance().MemoryAllocWriteLine(typeid(mpColourShader).name());

		// Initialise the colour shader object.
		successful = mpColourShader->Initialise(mpD3D->GetDevice(), hwnd);
		if (!successful)
		{
			logger->GetInstance().WriteLine("*** ERROR! *** Could not initialise the colour shader object");
			MessageBox(hwnd, "Could not initialise the colour shader object. ", "Error", MB_OK);
			return false;
		}

	}
	return true;
}

bool CGraphics::RemovePrimitive(CPrimitive* &model)
{
	std::list<CPrimitive*>::iterator it;
	it = mpPrimitives.begin();

	while (it != mpPrimitives.end())
	{
		if ((*it) == model)
		{
			model->Shutdown();
			delete model;
			(*it) = nullptr;
			logger->GetInstance().MemoryDeallocWriteLine(typeid((*it)).name());
			mpPrimitives.erase(it);
			model = nullptr;
			return true;
		}

		it++;
	}

	logger->GetInstance().WriteLine("Failed to find model to delete.");
	return false;
}

CMesh* CGraphics::LoadMesh(std::string filename, float radius)
{
	// Allocate the mesh memory.
	CMesh* mesh = new CMesh(mpD3D->GetDevice());

	logger->GetInstance().MemoryAllocWriteLine(typeid(mesh).name());

	// If we failed to load the mesh, then delete the object and return a nullptr.
	if (!mesh->LoadMesh(filename, radius))
	{
		// Deallocate memory.
		delete mesh;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(mesh).name());
		logger->GetInstance().WriteLine("Failed to load the mesh with name " + filename);
		return nullptr;
	}

	// Push the pointer onto a member variable list so that we don't lose it.
	mpMeshes.push_back(mesh);

	return mesh;
}

/* Deletes any allocated memory to a mesh and removes it from the list. */
bool CGraphics::RemoveMesh(CMesh *& mesh)
{
	// Define an iterator.
	std::list<CMesh*>::iterator it;
	// Initialise it to the start of the meshes list.
	it = mpMeshes.begin();

	// Iterate through the list.
	while (it != mpMeshes.end())
	{
		// If the current position of our list is the same as the mesh parameter passed in.
		if ((*it) == mesh)
		{
			mesh->Shutdown();

			// Deallocate memory.
			delete mesh;
			// Reset the pointer to be null.
			(*it) = nullptr;
			// Output the memory deallocation message to the memory log.
			logger->GetInstance().MemoryDeallocWriteLine(typeid((*it)).name());
			// Erase this element off of the list.
			mpMeshes.erase(it);
			// Set the value of the parameter to be null as well, so we have NO pointers to this area of memory any more.
			mesh = nullptr;
			// Complete! Return success.
			return true;
		}
		// Increment the iterator.
		it++;
	}

	// If we got to this point, the mesh that was passed in was not found on the list. Output failure message to the log.
	logger->GetInstance().WriteLine("Failed to find mesh to delete.");

	// Return failure.
	return false;
}

CTerrain * CGraphics::CreateTerrain(std::string mapFile)
{
	if (mpTerrain)
	{
		logger->GetInstance().WriteLine("Found a previously initialised instance of terrain, deleting it now. It will be reinitialised without memory leaks.");
		delete mpTerrain;
		mpTerrain = nullptr;
	}

	CTerrain* terrain = new CTerrain(mpD3D->GetDevice(), mScreenWidth, mScreenHeight);
	logger->GetInstance().WriteLine("Created terrain from the graphics object.");
	mpTerrain = terrain;

	// Check a map file was actually passed in.
	if (mapFile != "")
	{
		// Attempt to load the height map passed in.
		if (!terrain->LoadHeightMapFromFile(mapFile))
		{
			logger->GetInstance().WriteLine("Failed to load height map with name: " + mapFile);
		}
	}
	else
	{
		logger->GetInstance().WriteLine("No map file was passed in, not attempting to load.");
	}

	// Initialise the terrain.
	terrain->CreateTerrain(mpD3D->GetDevice());

	return terrain;
}

CTerrain * CGraphics::CreateTerrain(double ** heightMap, int mapWidth, int mapHeight)
{
	if (mpTerrain)
	{
		logger->GetInstance().WriteLine("Found a previously initialised instance of terrain, deleting it now. It will be reinitialised without memory leaks.");
		delete mpTerrain;
		mpTerrain = nullptr;
	}

	CTerrain* terrain = new CTerrain(mpD3D->GetDevice(), mScreenWidth, mScreenHeight);
	logger->GetInstance().WriteLine("Created terrain from the graphics object.");
	mpTerrain = terrain;

	// Loading height map
	terrain->SetWidth(mapWidth);
	terrain->SetHeight(mapHeight);
	terrain->LoadHeightMap(heightMap);

	// Initialise the terrain.
	terrain->CreateTerrain(mpD3D->GetDevice());

	return terrain;
}

/* Create an instance of a light and return a pointer to it. */
CLight * CGraphics::CreateLight(D3DXVECTOR4 diffuseColour, D3DXVECTOR4 specularColour, float specularPower, D3DXVECTOR4 ambientColour, D3DXVECTOR3 direction)
{
	CLight* light = new CLight();
	if (!light)
	{
		// Output error string to the message log.
		logger->GetInstance().WriteLine("Failed to create the light object. ");
		return nullptr;
	}
	logger->GetInstance().MemoryAllocWriteLine(typeid(light).name());

	// Set the colour to our colour variable passed in.
	light->SetDiffuseColour(diffuseColour);
	light->SetAmbientColour(ambientColour);
	light->SetDirection(direction);
	light->SetSpecularColour(specularColour);
	light->SetSpecularPower(specularPower);

	// Output success message.
	logger->GetInstance().WriteLine("Light successfully created.");

	// Returns a pointer to the light.
	return light;
}

void CGraphics::EnableTimeBasedSkybox(bool enabled)
{
	mUseTimeBasedSkybox = enabled;
}

bool CGraphics::GetTimeBasedSkyboxEnabled()
{
	return mUseTimeBasedSkybox;
}

void CGraphics::SetSkyboxUpdateInterval(float interval)
{
	mSkyboxUpdateInterval = interval;
}

float CGraphics::GetSkyboxUpdateInterval()
{
	return mSkyboxUpdateInterval;
}

void CGraphics::SetDayTime()
{
	mUpdateToDayTime = true;
	mUpdateToEveningTime = false;
	mUpdateToNightTime = false;
}

void CGraphics::SetNightTime()
{
	mUpdateToDayTime = false;
	mUpdateToEveningTime = false;
	mUpdateToNightTime = true;
}

void CGraphics::SetEveningTime()
{
	mUpdateToDayTime = false;
	mUpdateToEveningTime = true;
	mUpdateToNightTime = false;
}

bool CGraphics::IsDayTime()
{
	return mpSkybox->IsDayTime();
}

bool CGraphics::IsNightTime()
{
	return mpSkybox->IsNightTime();
}

bool CGraphics::IsEveningTime()
{
	return mpSkybox->IsEveningTime();
}

CCamera* CGraphics::CreateCamera()
{
	// Create the camera.
	mpCamera = new CCamera(mScreenWidth, mScreenWidth, mFieldOfView, SCREEN_NEAR, SCREEN_DEPTH);
	if (!mpCamera)
	{
		logger->GetInstance().WriteLine("Failed to create the camera for DirectX.");
		return nullptr;
	}
	logger->GetInstance().MemoryAllocWriteLine(typeid(mpCamera).name());

	// Set the initial camera position.
	mpCamera->SetPosition(0.0f, 0.0f, -1.0f);

	return mpCamera;
}

void CGraphics::SetCameraPos(float x, float y, float z)
{
	mpCamera->SetPosition(x, y, z);
}

void CGraphics::ToggleWireframe()
{
	mWireframeEnabled = !mWireframeEnabled;

	if (!mWireframeEnabled)
	{
		mpD3D->EnableSolidFill();
	}
	else
	{
		mpD3D->EnableWireframeFill();
	}
}
