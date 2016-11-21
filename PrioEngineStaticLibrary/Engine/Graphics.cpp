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
	mWireframeEnabled = false;
	mFieldOfView = static_cast<float>(D3DX_PI / 4);
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
		gLogger->WriteLine("Did not successfully create the D3D11 object.");
		//Don't continue with the init function any more.
		return false;
	}
	gLogger->MemoryAllocWriteLine(typeid(mpD3D).name());

	// Initialise the D3D object.
	successful = mpD3D->Initialise(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	// If failed to init D3D, then output error.
	if (!successful)
	{
		// Write the error to the log.
		gLogger->WriteLine("Failed to initialised Direct3D.");
		// Write the error to a message box too.
		MessageBox(hwnd, L"Could not initialise Direct3D.", L"Error", MB_OK);
		// Do not continue with this function any more.
		return false;
	}

	// Create a colour shader now, it's necessary for terrain.
	CreateColourShader(hwnd);

	// Success!
	gLogger->WriteLine("Direct3D was successfully initialised.");
	return true;
}

void CGraphics::Shutdown()
{

	if (mpDiffuseLightShader)
	{
		mpDiffuseLightShader->Shutdown();
		delete mpDiffuseLightShader;
		mpDiffuseLightShader = nullptr;
		gLogger->MemoryDeallocWriteLine(typeid(mpDiffuseLightShader).name());
	}

	if (mpTextureShader)
	{
		mpTextureShader->Shutdown();
		delete mpTextureShader;
		mpTextureShader = nullptr;
		gLogger->MemoryDeallocWriteLine(typeid(mpTextureShader).name());
	}

	if (mpColourShader)
	{
		mpColourShader->Shutdown();
		delete mpColourShader;
		mpColourShader = nullptr;
		gLogger->MemoryDeallocWriteLine(typeid(mpColourShader).name());
	}

	// Deallocate any allocated memory on the primitives list.
	std::list<CPrimitive*>::iterator it;
	it = mpPrimitives.begin();

	while (it != mpPrimitives.end())
	{
		(*it)->Shutdown();
		delete (*it);
		(*it) = nullptr;
		gLogger->MemoryDeallocWriteLine(typeid((*it)).name());
		it++;
	}

	while (!mpPrimitives.empty())
	{
		mpPrimitives.pop_back();
	}

	// Deallocate any allocated memroy on the mesh list.
	std::list<CMesh*>::iterator meshIt;
	meshIt = mpMeshes.begin();

	while (meshIt != mpMeshes.end())
	{
		delete (*meshIt);
		(*meshIt) = nullptr;
		gLogger->MemoryDeallocWriteLine(typeid((*meshIt)).name());
		meshIt++;
	}

	while (!mpMeshes.empty())
	{
		mpMeshes.pop_back();
	}

	// Deallocate any memory on the lights list.
	std::list<CLight*>::iterator lightIt;
	lightIt = mpLights.begin();

	while (lightIt != mpLights.end())
	{
		delete (*lightIt);
		(*lightIt) = nullptr;
		gLogger->MemoryDeallocWriteLine(typeid(*lightIt).name());
		lightIt++;
	}

	while (!mpLights.empty())
	{
		mpLights.pop_back();
	}

	// Deallocate memory on the terrain list.

	std::list<CTerrainGrid*>::iterator terrainIt;
	terrainIt = mpTerrainGrids.begin();
	while (terrainIt != mpTerrainGrids.end())
	{
		delete (*terrainIt);
		(*terrainIt) = nullptr;
		terrainIt++;
	}

	while (!mpTerrainGrids.empty())
	{
		mpTerrainGrids.pop_back();
	}

	// Remove camera.

	if (mpCamera)
	{
		delete mpCamera;
		mpCamera = nullptr;
		gLogger->MemoryDeallocWriteLine(typeid(mpCamera).name());
	}

	// If the Direct 3D object exists.
	if (mpD3D)
	{
		// Clean up the D3D object before we get rid of it.
		mpD3D->Shutdown();
		delete mpD3D;
		mpD3D = nullptr;
		gLogger->MemoryDeallocWriteLine(typeid(mpD3D).name());
		// Output message to log to let us know that this object is gone.
		gLogger->WriteLine("Direct3D object has been shutdown, deallocated and pointer set to null.");
	}

	return;
}

bool CGraphics::Frame()
{
	bool success;

	// Render the graphics scene.
	success = Render();

	// If we did not successfully render the scene.
	if (!success)
	{
		// Output a message to the log.
		gLogger->WriteLine("Failed to render the scene.");
		// Prevent the program from continuing any further.
		return false;
	}
	return true;
}

bool CGraphics::Render()
{
	D3DXMATRIX viewMatrix;
	D3DXMATRIX worldMatrix;
	D3DXMATRIX projMatrix;

	// Clear buffers so we can begin to render the scene.
	mpD3D->BeginScene(0.6f, 0.9f, 1.0f, 1.0f);

	// Generate view matrix based on cameras current position.
	mpCamera->Render();

	// Get the world, view and projection matrices from the old camera and d3d objects.
	mpCamera->GetViewMatrix(viewMatrix);
	mpD3D->GetWorldMatrix(worldMatrix);
	mpD3D->GetProjectionMatrix(projMatrix);


	// Render model using texture shader.
	if (!RenderModels(viewMatrix, worldMatrix, projMatrix))
		return false;

	// Present the rendered scene to the screen.
	mpD3D->EndScene();

	return true;
}

bool CGraphics::RenderModels(D3DXMATRIX view, D3DXMATRIX world, D3DXMATRIX proj)
{
	std::list<CPrimitive*>::iterator it;
	it = mpPrimitives.begin();
	
	D3DXMATRIX modelWorld;
	// Define three matrices to hold x, y and z rotations.
	D3DXMATRIX rotX;
	D3DXMATRIX rotY;
	D3DXMATRIX rotZ;

	while (it != mpPrimitives.end())
	{
		D3DXMatrixTranslation(&modelWorld, (*it)->GetPosX(), (*it)->GetPosY(), (*it)->GetPosZ());

		// Use Direct X to rotate the matrices and pass the matrix after rotation back into the rotation matrix we defined.
		D3DXMatrixRotationX(&rotX, (*it)->GetRotationX());
		D3DXMatrixRotationY(&rotY, (*it)->GetRotationY());
		D3DXMatrixRotationZ(&rotZ, (*it)->GetRotationZ());
		world = modelWorld * rotX * rotY * rotZ;

		// put the model vertex and index buffers on the graphics pipleline to prepare them for dawing.
		(*it)->Render(mpD3D->GetDeviceContext());

		// Render texture with no light.
		if ((*it)->HasTexture() && !(*it)->UseDiffuseLight())
		{
			if (!RenderPrimitiveWithTexture((*it), world, view, proj))
			{
				return false;
			}
		}
		// Render texture with light.
		else if ((*it)->HasTexture() && (*it)->UseDiffuseLight())
		{
			if (!RenderPrimitiveWithTextureAndDiffuseLight((*it), world, view, proj))
			{
				return false;
			}
		}
		// Render colour.
		else if ((*it)->HasColour())
		{
			if (!RenderPrimitiveWithColour((*it), world, view, proj))
			{
				return false;
			}
		}
		it++;
	}


	std::list<CMesh*>::iterator meshIt = mpMeshes.begin();

	// Render any models which belong to each mesh. Do this in batches to make it faster.
	D3DXVECTOR3 camPos = mpCamera->GetPosition();
	while (meshIt != mpMeshes.end())
	{
		(*meshIt)->SetCameraPos(camPos);
		(*meshIt)->Render(mpD3D->GetDeviceContext(), view, proj, mpLights);
		meshIt++;
	}

	// Render any terrains.

	std::list<CTerrainGrid*>::iterator terrainIt = mpTerrainGrids.begin();

	while (terrainIt != mpTerrainGrids.end())
	{
		(*terrainIt)->Render(mpD3D->GetDeviceContext());

		D3DXMatrixTranslation(&modelWorld, (*terrainIt)->GetPosX(), (*terrainIt)->GetPosY(), (*terrainIt)->GetPosZ());

		// Use Direct X to rotate the matrices and pass the matrix after rotation back into the rotation matrix we defined.
		D3DXMatrixRotationX(&rotX, (*terrainIt)->GetRotationX());
		D3DXMatrixRotationY(&rotY, (*terrainIt)->GetRotationY());
		D3DXMatrixRotationZ(&rotZ, (*terrainIt)->GetRotationZ());
		world = modelWorld * rotX * rotY * rotZ;

		// Render the terrain model using the colour shader.
		mpColourShader->Render(mpD3D->GetDeviceContext(), (*terrainIt)->GetIndexCount(), world, view, proj);
		terrainIt++;
	}

	return true;
}

bool CGraphics::RenderPrimitiveWithTextureAndDiffuseLight(CPrimitive* model, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix, D3DXMATRIX projMatrix)
{
	bool success = false;

	// Attempt to render the model with the texture specified.
	std::list<CLight*>::iterator it;
	it = mpLights.begin();

	// Render each diffuse light in the list.
	do
	{
		success = mpDiffuseLightShader->Render(mpD3D->GetDeviceContext(), model->GetIndex(), worldMatrix, viewMatrix, projMatrix, model->GetTexture(),(*it)->GetDirection(), (*it)->GetDiffuseColour(), (*it)->GetAmbientColour());
		it++;
	} while (it != mpLights.end());

	// If we did not successfully render.
	if (!success)
	{
		gLogger->WriteLine("Failed to render the model using the texture shader in graphics.cpp.");
		return false;
	}

	return true;
}

bool CGraphics::RenderPrimitiveWithColour(CPrimitive* model, D3DMATRIX worldMatrix, D3DMATRIX viewMatrix, D3DMATRIX projMatrix)
{
	bool success = false;

	// Render the model using the colour shader.
	success = mpColourShader->Render(mpD3D->GetDeviceContext(), model->GetIndex(), worldMatrix, viewMatrix, projMatrix);
	if (!success)
	{
		gLogger->WriteLine("Failed to render the model using the colour shader object.");
		return false;
	}
	
	return true;
}

bool CGraphics::RenderPrimitiveWithTexture(CPrimitive* model, D3DXMATRIX worldMatrix, D3DXMATRIX viewMatrix, D3DXMATRIX projMatrix)
{
	bool success = false;

	// Attempt to render the model with the texture specified.
	success = mpTextureShader->Render(mpD3D->GetDeviceContext(), model->GetIndex(), worldMatrix, viewMatrix, projMatrix, model->GetTexture());

	if (!success)
	{
		gLogger->WriteLine("Failed to render the model using the texture shader in graphics.cpp.");
		return false;
	}

	return true;
}

CPrimitive* CGraphics::CreatePrimitive(WCHAR* TextureFilename, PrioEngine::Primitives shape)
{
	CPrimitive* model;
	bool successful;

	switch (shape)
	{
	case PrioEngine::Primitives::cube:
		model = new CCube(TextureFilename);
		gLogger->MemoryAllocWriteLine(typeid(model).name());
		break;
	case PrioEngine::Primitives::triangle:
		model = new CTriangle(TextureFilename);
		gLogger->MemoryAllocWriteLine(typeid(model).name());
		break;
	default:
		return nullptr;
	}

	if (!model)
	{
		gLogger->WriteLine("Failed to create the model object");
		return nullptr;
	}

	// Initialise the model object.
	successful = model->Initialise(mpD3D->GetDevice());
	if (!successful)
	{
		gLogger->WriteLine("*** ERROR! *** Could not initialise the model object");
		MessageBox(mHwnd, L"Could not initialise the model object. ", L"Error", MB_OK);
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

CPrimitive* CGraphics::CreatePrimitive(WCHAR* TextureFilename, bool useLighting, PrioEngine::Primitives shape)
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


	gLogger->MemoryAllocWriteLine(typeid(model).name());
	if (!model)
	{
		gLogger->WriteLine("Failed to create the model object");
		return nullptr;
	}

	// Initialise the model object.
	successful = model->Initialise(mpD3D->GetDevice());
	if (!successful)
	{
		gLogger->WriteLine("*** ERROR! *** Could not initialise the model object");
		MessageBox(mHwnd, L"Could not initialise the model object. ", L"Error", MB_OK);
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
		gLogger->MemoryAllocWriteLine(typeid(model).name());
		break;
	case PrioEngine::Primitives::triangle:
		model = new CTriangle(colour);
		gLogger->MemoryAllocWriteLine(typeid(model).name());
		break;
	default:
		return nullptr;
	}

	if (!model)
	{
		gLogger->WriteLine("Failed to create the model object");
		return nullptr;
	}

	// Initialise the model object.
	successful = model->Initialise(mpD3D->GetDevice());
	if (!successful)
	{
		gLogger->WriteLine("*** ERROR! *** Could not initialise the model object");
		MessageBox(mHwnd, L"Could not initialise the model object. ", L"Error", MB_OK);
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
		gLogger->MemoryAllocWriteLine(typeid(mpDiffuseLightShader).name());
		if (!mpDiffuseLightShader)
		{
			gLogger->WriteLine("Failed to create the texture shader object in graphics.cpp.");
			return false;
		}

		// Initialise the texture shader object.
		successful = mpDiffuseLightShader->Initialise(mpD3D->GetDevice(), hwnd);
		if (!successful)
		{
			gLogger->WriteLine("Failed to initialise the texture shader object in graphics.cpp.");
			MessageBox(hwnd, L"Could not initialise the texture shader object.", L"Error", MB_OK);
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
		gLogger->MemoryAllocWriteLine(typeid(mpTextureShader).name());
		if (!mpTextureShader)
		{
			gLogger->WriteLine("Failed to create the texture shader object in graphics.cpp.");
			return false;
		}

		// Initialise the texture shader object.
		successful = mpTextureShader->Initialise(mpD3D->GetDevice(), hwnd);
		if (!successful)
		{
			gLogger->WriteLine("Failed to initialise the texture shader object in graphics.cpp.");
			MessageBox(hwnd, L"Could not initialise the texture shader object.", L"Error", MB_OK);
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
			gLogger->WriteLine("Failed to create the colour shader object.");
			return false;
		}
		gLogger->MemoryAllocWriteLine(typeid(mpColourShader).name());

		// Initialise the colour shader object.
		successful = mpColourShader->Initialise(mpD3D->GetDevice(), hwnd);
		if (!successful)
		{
			gLogger->WriteLine("*** ERROR! *** Could not initialise the colour shader object");
			MessageBox(hwnd, L"Could not initialise the colour shader object. ", L"Error", MB_OK);
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
			gLogger->MemoryDeallocWriteLine(typeid((*it)).name());
			mpPrimitives.erase(it);
			model = nullptr;
			return true;
		}

		it++;
	}

	gLogger->WriteLine("Failed to find model to delete.");
	return false;
}

CMesh* CGraphics::LoadMesh(char * filename, WCHAR* textureFilename)
{
	// Allocate the mesh memory.
	CMesh* mesh;
	if (textureFilename != L"" && textureFilename != NULL)
	{
		mesh = new CMesh(mpD3D->GetDevice(), mHwnd, PrioEngine::ShaderType::Diffuse);
	}
	else
	{
		mesh = new CMesh(mpD3D->GetDevice(), mHwnd, PrioEngine::ShaderType::Colour);
	}

	gLogger->MemoryAllocWriteLine(typeid(mesh).name());

	// If we failed to load the mesh, then delete the object and return a nullptr.
	if (!mesh->LoadMesh(filename, textureFilename))
	{
		// Deallocate memory.
		delete mesh;
		gLogger->MemoryDeallocWriteLine(typeid(mesh).name());
		return nullptr;
	}

	// Push the pointer onto a member variable list so that we don't lose it.
	mpMeshes.push_back(mesh);

	return mesh;
}

CMesh* CGraphics::LoadMesh(char * filename, WCHAR* textureFilename, PrioEngine::ShaderType shaderType)
{
	// Allocate the mesh memory.
	CMesh* mesh = new CMesh(mpD3D->GetDevice(), mHwnd, shaderType);
	gLogger->MemoryAllocWriteLine(typeid(mesh).name());

	// If we failed to load the mesh, then delete the object and return a nullptr.
	if (!mesh->LoadMesh(filename, textureFilename))
	{
		// Deallocate memory.
		delete mesh;
		gLogger->MemoryDeallocWriteLine(typeid(mesh).name());
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
			// Deallocate memory.
			delete mesh;
			// Reset the pointer to be null.
			(*it) = nullptr;
			// Output the memory deallocation message to the memory log.
			gLogger->MemoryDeallocWriteLine(typeid((*it)).name());
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
	gLogger->WriteLine("Failed to find mesh to delete.");

	// Return failure.
	return false;
}

CTerrainGrid * CGraphics::CreateTerrainGrid()
{
	CTerrainGrid* terrain = new CTerrainGrid(mpD3D->GetDevice());
	//terrain->Initialise(mpD3D->GetDevice());
	gLogger->WriteLine("Retrieved an instance of terrain but it will need to be initialised before use, if not we're going to attempt to render something which simply does not exist. Use terrainObj->CreateGrid() to create it. ");
	mpTerrainGrids.push_back(terrain);
	return terrain;
}

/* Create an instance of a light and return a pointer to it. */
CLight * CGraphics::CreateLight(D3DXVECTOR4 diffuseColour, D3DXVECTOR4 ambientColour)
{
	CLight* light = new CLight();
	if (!light)
	{
		// Output error string to the message log.
		gLogger->WriteLine("Failed to create the light object. ");
		return nullptr;
	}
	gLogger->MemoryAllocWriteLine(typeid(light).name());

	// Set the colour to our colour variable passed in.
	light->SetDiffuseColour(diffuseColour);
	light->SetAmbientColour(ambientColour);
	light->SetDirection({ 1.0f, 1.0f, 1.0f });
	
	// Stick the new instance on a list so we can track it, we can then ensure there are no memory leaks.
	mpLights.push_back(light);

	// Output success message.
	gLogger->WriteLine("Light successfully created.");

	// Returns a pointer to the light.
	return light;
}

/* Searches for the light pointer which has been created by the engine, and attempts to remove it.
@Return Successfully removed light (Bool) */
bool CGraphics::RemoveLight(CLight *& light)
{
	// Define an iterator.
	std::list<CLight*>::iterator it;
	// Initialise it to the start of the lights list.
	it = mpLights.begin();

	// Iterate through the list.
	while (it != mpLights.end())
	{
		// If the current position of our list is the same as the light parameter passed in.
		if ((*it) == light)
		{
			// Deallocate memory.
			delete light;
			// Reset the pointer to be null.
			(*it) = nullptr;
			// Output the memory deallocation message to the memory log.
			gLogger->MemoryDeallocWriteLine(typeid((*it)).name());
			// Erase this element off of the list.
			mpLights.erase(it);
			// Set the value of the parameter to be null as well, so we have NO pointers to this area of memory any more.
			light = nullptr;
			// Complete! Return success.
			return true;
		}
		// Increment the iterator.
		it++;
	}

	// If we got to this point, the light that was passed in was not found on the list. Output failure message to the log.
	gLogger->WriteLine("Failed to find light to delete.");

	// Return failure.
	return false;
}

CCamera* CGraphics::CreateCamera()
{
	// Create the camera.
	mpCamera = new CCamera(mScreenWidth, mScreenWidth, mFieldOfView, SCREEN_NEAR, SCREEN_DEPTH);
	if (!mpCamera)
	{
		gLogger->WriteLine("Failed to create the camera for DirectX.");
		return nullptr;
	}
	gLogger->MemoryAllocWriteLine(typeid(mpCamera).name());

	// Set the initial camera position.
	mpCamera->SetPosition(0.0f, 0.0f, 0.0f);

	return mpCamera;
}

void CGraphics::SetCameraPos(float x, float y, float z)
{
	mpCamera->SetPosition(x, y, z);
}

void CGraphics::ToggleWireframe()
{
	if (!mWireframeEnabled)
	{
		mpD3D->EnableSolidFill();
	}
	else
	{
		mpD3D->EnableWireframeFill();
	}

	mWireframeEnabled = !mWireframeEnabled;
}
