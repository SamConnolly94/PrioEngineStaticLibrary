#include "Mesh.h"

CMesh::CMesh(ID3D11Device* device, HWND hwnd, PrioEngine::ShaderType shaderType)
{
	// Initialise our counter variables to the default values.
	mVertexCount = 0;
	mIndexCount = 0;

	// Store the handle to our main window.
	mHwnd = hwnd;

	// Stash away a pointer to our device.
	mpDevice = device;

	// Set the pointer to our texture to be null, we can use this for checks to see if we're using a texture or not later.
	mpTexture = nullptr;

	// Default the shader type to colour, we can change this later on circumstantially.
	if (shaderType != NULL)
	{
		mShaderType = shaderType;
	}
	else
	{
		mShaderType = PrioEngine::ShaderType::Colour;
	}
}

CMesh::~CMesh()
{

	// If we still have pointers to models.
	while (!mpModels.empty())
	{
		// Delete allocated memory from the back of our array.
		delete (mpModels.back());
		gLogger->MemoryDeallocWriteLine(typeid(mpModels.back()).name());
		// Pop the model off of the list.
		mpModels.pop_back();
	}

	// If the directional light shader has been allocated memory.
	if (mpDirectionalLightShader)
	{
		// Deallocate memory.
		delete mpDirectionalLightShader;
		// Write the deallocation message to the memory log.
		gLogger->MemoryDeallocWriteLine(typeid(mpDirectionalLightShader).name());
		// Set the directional light shader pointer to be default value.
		mpDirectionalLightShader = nullptr;
	}

	if (mpColourShader)
	{
		// Deallocate memory.
		delete mpColourShader;
		// Write the deallocation message to the memory log.
		gLogger->MemoryDeallocWriteLine(typeid(mpColourShader).name());
		// Set the colour shader pointer to be default value.
		mpColourShader = nullptr;
	}

	if (mpTextureShader)
	{
		// Deallocate memory.
		delete mpTextureShader;
		// Write the deallocation message to the memory log.
		gLogger->MemoryDeallocWriteLine(typeid(mpTextureShader).name());
		// Set the mpTextureShader shader pointer to be default value.
		mpTextureShader = nullptr;
	}

	if (mpSpecularShader)
	{
		// Deallocate memory.
		delete mpSpecularShader;
		// Write the deallocation message to the memory log.
		gLogger->MemoryDeallocWriteLine(typeid(mpSpecularShader).name());
		// Set the mpTextureShader shader pointer to be default value.
		mpSpecularShader = nullptr;
	}
		
	// If the texture has been allocated memory.
	if (mpTexture)
	{
		// Deallocate memory.
		delete mpTexture;
		// Output the deallocation message to the log.
		gLogger->MemoryDeallocWriteLine(typeid(mpTexture).name());
		// Set the texture pointer to be a default value.
		mpTexture = nullptr;
	}
}

/* Load data from file into our mesh object. */
bool CMesh::LoadMesh(char* filename, WCHAR* textureName)
{
	// If no file name was passed in, then output an error into the log and return false.
	if (filename == NULL || filename == "")
	{
		gLogger->WriteLine("You need to pass in a file name to load a model.");
		return false;
	}

	// Check if a texture was passed in.
	if (textureName == NULL || textureName == L"")
	{
		gLogger->WriteLine("You did not pass in a texture file name with a mesh named " + static_cast<std::string>(filename) + ", will load with solid black colour.");
	}
	else
	{
		mpTexture = new CTexture();
		gLogger->MemoryAllocWriteLine(typeid(mpTexture).name());
		mpTexture->Initialise(mpDevice, textureName);
		if (mShaderType == PrioEngine::ShaderType::Colour)
		{
			mShaderType = PrioEngine::ShaderType::Diffuse;
		}
	}

	// Stash our filename for this mesh away as a member variable, it may come in handy in future.
	mFilename = filename;

	// Extract the file extension.
	std::size_t extensionLocation = mFilename.find_last_of(".");
	mFileExtension = mFilename.substr(extensionLocation, mFilename.length());

	// Allocate memory to one of our shaders, depending on whether a texture was loaded in or not.
	if (mShaderType == PrioEngine::ShaderType::Diffuse)
	{
		// Initialise our directional light shader.
		mpDirectionalLightShader = new CDiffuseLightShader();
		gLogger->MemoryAllocWriteLine(typeid(mpDirectionalLightShader).name());
		// If the directional light shader is not successfully initialised.
		if (!mpDirectionalLightShader->Initialise(mpDevice, mHwnd))
		{
			// Output failure message to the log.
			gLogger->WriteLine("Failed to initialise the directional light shader in mesh object.");
		}
	}
	// Allocate memory to one of our shaders, depending on whether a texture was loaded in or not.
	else if (mShaderType == PrioEngine::ShaderType::Specular)
	{
		// Initialise our directional light shader.
		mpSpecularShader = new CSpecularLightingShader();
		gLogger->MemoryAllocWriteLine(typeid(mpSpecularShader).name());
		// If the directional light shader is not successfully initialised.
		if (!mpSpecularShader->Initialise(mpDevice, mHwnd))
		{
			// Output failure message to the log.
			gLogger->WriteLine("Failed to initialise the specular light shader in mesh object.");
		}
	}
	else if (mShaderType == PrioEngine::ShaderType::Texture)
	{
		// Allocate memory to the texture shader.
		mpTextureShader = new CTextureShader();
		gLogger->MemoryAllocWriteLine(typeid(mpTextureShader).name());
		// If the texture shader is not successfully initialised.
		if (!mpTextureShader->Initialise(mpDevice, mHwnd))
		{
			// output failure message to the log.
			gLogger->WriteLine("Failed to initialise the colour shader in mesh object.");
		}
	}
	else if (mShaderType == PrioEngine::ShaderType::Colour)
	{
		// Allocate memory to the colour shader.
		mpColourShader = new CColourShader();
		gLogger->MemoryAllocWriteLine(typeid(mpColourShader).name());
		// If the colour shader is not successfully initialised.
		if (!mpColourShader->Initialise(mpDevice, mHwnd))
		{
			// output failure message to the log.
			gLogger->WriteLine("Failed to initialise the colour shader in mesh object.");
		}
	}

	// Check what extension we are trying to load.
	if (mFileExtension == ".sam")
	{
		gLogger->WriteLine("Loading .sam file using Prio Engines built in model loader.");
		return LoadSam();
	}
	else
	{
		gLogger->WriteLine("Loading " + mFileExtension + " file using assimp model loader.");
		return LoadAssimpModel(filename);
	}
		
	// Output error message to the log.
	gLogger->WriteLine("You have tried to load an unsupported file type as a mesh. The file name was: '" + mFilename + "'.");
	// Return failure.
	return false;
}

void CMesh::Render(ID3D11DeviceContext* context, D3DXMATRIX &view, D3DXMATRIX &proj, std::list<CLight*>lights)
{
	std::list<CModel*>::iterator it = mpModels.begin();
	std::list<CLight*>::iterator lightIt = lights.begin();

	while (it != mpModels.end())
	{
		(*it)->UpdateMatrices();

		(*it)->RenderBuffers(context);

		while (lightIt != lights.end())
		{
			if (mShaderType == PrioEngine::ShaderType::Diffuse)
			{
				// Our number of indices isn't quite accurate, we stash indicies away in vector 3's as we should always be creating a triangle. 
				if (!mpDirectionalLightShader->Render(context, (*it)->GetNumberOfIndices(), (*it)->GetWorldMatrix(), view, proj, mpTexture->GetTexture(), (*lightIt)->GetDirection(), (*lightIt)->GetDiffuseColour(), (*lightIt)->GetAmbientColour()))
				{
					gLogger->WriteLine("Failed to render the mesh model.");
				}
				lightIt++;
			}
			else if (mShaderType == PrioEngine::ShaderType::Specular)
			{
				D3DXVECTOR3 lightPos = (*lightIt)->GetPos();
				// Our number of indices isn't quite accurate, we stash indicies away in vector 3's as we should always be creating a triangle. 
				if (!mpSpecularShader->Render(context, (*it)->GetNumberOfIndices(), (*it)->GetWorldMatrix(), view, proj, mpTexture->GetTexture(), (*lightIt)->GetDirection(), (*lightIt)->GetDiffuseColour(), (*lightIt)->GetAmbientColour(), lightPos, (*lightIt)->GetSpecularColour(), (*lightIt)->GetSpecularPower()))
				{
					gLogger->WriteLine("Failed to render the mesh model.");
				}
				lightIt++;
			}
			else if (mShaderType == PrioEngine::ShaderType::Texture)
			{
				// Our number of indices isn't quite accurate, we stash indicies away in vector 3's as we should always be creating a triangle. 
				if (!mpTextureShader->Render(context, (*it)->GetNumberOfIndices(), (*it)->GetWorldMatrix(), view, proj, mpTexture->GetTexture()))
				{
					gLogger->WriteLine("Failed to render the mesh model.");
				}
				lightIt++;
			}
			else if (mShaderType == PrioEngine::ShaderType::Colour)
			{
				// Our number of indices isn't quite accurate, we stash indicies away in vector 3's as we should always be creating a triangle. 
				if (!mpColourShader->Render(context, (*it)->GetNumberOfIndices(), (*it)->GetWorldMatrix(), view, proj))
				{
					gLogger->WriteLine("Failed to render the mesh model.");
				}
				lightIt++;
			}
			else
			{
				gLogger->WriteLine("Failed to find any available shader to render the instance of mesh in mesh.cpp Render function.");
			}
		}

		it++;
	}
}

/* Create an instance of this mesh. 
@Returns CModel* ptr
 */
CModel* CMesh::CreateModel()
{
	// Allocate memory to a model.
	CModel* model;
	// Create a variable equal to a vertex type.
	PrioEngine::ShaderType vt;
	// Initialise the vertex type.
	if (mShaderType == PrioEngine::ShaderType::Diffuse)
	{
		vt = PrioEngine::ShaderType::Diffuse;
	}
	else if (mShaderType == PrioEngine::ShaderType::Texture)
	{
		vt = PrioEngine::ShaderType::Texture;
	}
	else if (mShaderType == PrioEngine::ShaderType::Colour)
	{
		vt = PrioEngine::ShaderType::Colour;
	}
	else if (mShaderType == PrioEngine::ShaderType::Specular)
	{
		vt = PrioEngine::ShaderType::Specular;
	}
	else
	{
		// Failed to discover what vertex type should be used, return a nullptr
		return nullptr;
	}

	model = new CModel(mpDevice, vt);

	// Write an allocation message to our memory log.
	gLogger->MemoryAllocWriteLine(typeid(model).name());

	// Check the model has had space allocated to it.
	if (model == nullptr)
	{
		gLogger->WriteLine("Failed to allocate space to model. ");
		return nullptr;
	}

	model->SetNumberOfVertices(mpVerticesList.size());
	model->SetNumberOfIndices(mpIndicesList.size());

	// If we're using diffuse light.
	if (mpDirectionalLightShader || mpSpecularShader)
	{
		model->SetGeometry(mpVerticesList, mpIndicesList, mpUVList, mpNormalsList);
	}
	else if (mpTextureShader)
	{
		model->SetGeometry(mpVerticesList, mpIndicesList, mpUVList);
	}
	else if (mpColourShader)
	{
		model->SetGeometry(mpVerticesList, mpIndicesList, mpVertexColourList);
	}
	else
	{
		gLogger->WriteLine("Failed to find a valid shader being used when creating instance of mesh. ");
		return nullptr;
	}
	// Stick our models on a list to prevent losing the pointers.
	mpModels.push_back(model);

	//return model;
	return model;
}

/* Load a model using our assimp vertex manager. 
@Returns bool Success*/
bool CMesh::LoadAssimpModel(char* filename)
{
	// Grab the mesh object for the last mesh we loaded.

	Assimp::Importer importer;
	const std::string name = filename;
	gLogger->WriteLine("Attempting to open " + name + " using Assimp.");

	// Read in the file, store this mesh in the scene.
	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_ConvertToLeftHanded |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_SortByPType);


	// If scene hasn't been initialised then something has gone wrong!
	if (!scene)
	{
		gLogger->WriteLine(importer.GetErrorString());
		gLogger->WriteLine("Failed to create scene.");
		return nullptr;
	}

	// Iterate through all our meshes to be loaded.
	for (unsigned int meshCount = 0; meshCount < scene->mNumMeshes; meshCount++)
	{
		// Load the current mesh.
		const aiMesh& mesh = *scene->mMeshes[meshCount];
		// Store info about the mesh.
		unsigned int numFaces = mesh.mNumFaces;
		unsigned int numVertices = mesh.mNumVertices;

		for (unsigned int vertexCount = 0; vertexCount < numVertices; vertexCount++)
		{
			// Parse a singular vertex info.

			const aiVector3D& vertexCoords = mesh.mVertices[vertexCount];

			// Load the vertex info into our array.
			mpVerticesList.push_back(D3DXVECTOR3(vertexCoords.x, vertexCoords.y, vertexCoords.z));

			// Parse information on the UV of a singular vertex.
			const aiVector3D& textureCoords = mesh.mTextureCoords[0][vertexCount];

			// Only need to parse the U and V channels.
			mpUVList.push_back(D3DXVECTOR2(textureCoords.x, textureCoords.y));

			// Load the normals data of this singular vertex.
			const aiVector3D& normals = mesh.mNormals[vertexCount];

			// Store this normals data in our array.
			mpNormalsList.push_back(D3DXVECTOR3(normals.x, normals.y, normals.z));

			// Set the default colour to black just in case no texture can be loaded.
			mpVertexColourList.push_back(D3DXVECTOR4(PrioEngine::Colours::black.r, PrioEngine::Colours::black.g, PrioEngine::Colours::black.b, PrioEngine::Colours::black.a));
		}

		for (unsigned int faceCount = 0; faceCount < numFaces; faceCount++)
		{
			const aiFace& face = mesh.mFaces[faceCount];

			for (unsigned int indexCount = 0; indexCount < face.mNumIndices; indexCount++)
			{
				mpIndicesList.push_back(face.mIndices[indexCount]);
			}
		}
	}

	gLogger->WriteLine("Successfully initialised our arrays for mesh '" + mFilename + "'. ");


	// Close off this section in the log.
	gLogger->CloseSubtitle();

	// Success!
	return true;
}

bool CMesh::LoadSam()
{
	// Will find the size that our array should be.
	GetSizes();

	// Will populate the new arrays we have created.
	InitialiseArrays();

	return true;
}

/* Will find how big an array should be depending on the object file. */
bool CMesh::GetSizes()
{
	// Define an input stream to open the mesh file in.
	std::ifstream inFile;

	int vertexIndex = 0;
	int texcoordIndex = 0;
	int normalIndex = 0;
	int faceIndex = 0;

	// Open the mesh file.
	inFile.open(mFilename);

	// Check if the file has been successfully opened.
	if (!inFile.is_open())
	{
		// Output error message to logs.
		gLogger->WriteLine("Failed to find file of name: '" + mFilename + "'.");
		// Return failure.
		return false;
	}

	char ch;
	// Read the first character into our ch var.
	inFile.get(ch);
	// Iterate through the rest of the file.
	while (!inFile.eof())
	{
		// Skip any lines with comments on them.
		while (ch == '#')
		{
			while (ch != '\n' && !inFile.eof())
			{
				inFile.get(ch);
			}
			inFile.get(ch);
		}

		// If this line represents a vertex.
		if (ch == 'v')
		{
			inFile.get(ch);
			if (ch == ' ')
			{
				mVertexCount++;
			}
		}
		else if (ch == 'i')
		{
			inFile.get(ch);
			if (ch == ' ')
			{
				mIndexCount += 3;
			}
		}

		// Read the first character of the next line.
		inFile.get(ch);

		// Perform a check at the end of a loop, this will raise any boolean flags ready for the loop to do a logic test.
		inFile.eof();
	}

	// Finished with the file now, close it.
	inFile.close();

	return true;
}

/* Populate buffers with geometry data. */
bool CMesh::InitialiseArrays()
{
	// Define an input stream to open the mesh file in.
	std::ifstream inFile;

	int vertexIndex = 0;
	int indiceIndex = 0;

	// Open the mesh file.
	inFile.open(mFilename);

	// Check if the file has been successfully opened.
	if (!inFile.is_open())
	{
		// Output error message to logs.
		gLogger->WriteLine("Failed to find file of name: '" + mFilename + "'.");
		// Return failure.
		return false;
	}

	char ch;
	// Read the first character into our ch var.
	inFile.get(ch);
	// Iterate through the rest of the file.
	while (!inFile.eof())
	{
		// Skip any lines with comments on them.
		while (ch == '#')
		{
			while (ch != '\n' && !inFile.eof())
			{
				inFile.get(ch);
			}
			inFile.get(ch);
		}

		// If this line represents a vertex.
		if (ch == 'v')
		{
			inFile.get(ch);
			if (ch == ' ')
			{
				float x, y, z;
				// Read in each value.
				/*inFile >> mpVertices[vertexIndex].x >> mpVertices[vertexIndex].y >> mpVertices[vertexIndex].z;*/
				inFile >> x >> y >> z;
				mpVerticesList.push_back(D3DXVECTOR3(x, y, z));
				// Increment our index.
				vertexIndex++;
			}
			
		}
		else if (ch == 'i')
		{
			inFile.get(ch);
			if (ch == ' ')
			{
				// If memory has been allocated to the mpMatrices variable.
				unsigned long index1, index2, index3;
				// Read in each value.
				inFile >> index1 >> index2 >> index3;

				mpIndicesList.push_back(index1);
				mpIndicesList.push_back(index2);
				mpIndicesList.push_back(index3);

			}
		}

		// Read the first character of the next line.
		inFile.get(ch);

		// Perform a check at the end of a loop, this will raise any boolean flags ready for the loop to do a logic test.

	}

	// Finished with the file now, close it.
	inFile.close();

	return true;
}
