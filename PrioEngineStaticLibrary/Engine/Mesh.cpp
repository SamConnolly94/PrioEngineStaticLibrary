#include "Mesh.h"

CMesh::CMesh(ID3D11Device* device)
{
	// Initialise our counter variables to the default values.
	mVertexCount = 0;
	mIndexCount = 0;

	mpDevice = device;
}

CMesh::~CMesh()
{
}

void CMesh::Shutdown()
{
	for (unsigned int i = 0; i < mNumberOfSubMeshes; i++)
	{
		for (unsigned int t = 0; t < mNumberOfTextures; t++)
		{
			if (mSubMeshMaterials[i].mTextures[t] != nullptr)
			{
				mSubMeshMaterials[i].mTextures[t]->Release();
				mSubMeshMaterials[i].mTextures[t] = nullptr;
			}
		}
	}
	delete[] mpSubMeshes;
	delete[] mSubMeshMaterials;

	for (auto model : mpModels)
	{
		delete model;
		logger->GetInstance().MemoryDeallocWriteLine(typeid(model).name());
	}
	mpModels.clear();

}

/* Load data from file into our mesh object. */
bool CMesh::LoadMesh(std::string filename)
{
	// If no file name was passed in, then output an error into the log and return false.
	if (filename.empty() || filename == "")
	{
		logger->GetInstance().WriteLine("You need to pass in a file name to load a model.");
		return false;
	}

	// Stash our filename for this mesh away as a member variable, it may come in handy in future.
	mFilename = filename;

	logger->GetInstance().WriteLine("Loading " + mFilename + " file using assimp model loader.");
	bool result = LoadAssimpModel(filename);
	return result;
}

void CMesh::Render(ID3D11DeviceContext* context, CDiffuseLightShader* shader, D3DXMATRIX &view, D3DXMATRIX &proj, std::list<CLight*>lights)
{
	for (auto model : mpModels)
	{
		model->UpdateMatrices();

		for (unsigned int subMeshCount = 0; subMeshCount < mNumberOfSubMeshes; subMeshCount++)
		{
			// Prepare the buffers for rendering.
			model->RenderBuffers(context, subMeshCount, mpSubMeshes[subMeshCount].vertexBuffer, mpSubMeshes[subMeshCount].indexBuffer, sizeof(VertexType) );

			// Get the textures.

			bool useAlpha = mSubMeshMaterials[mpSubMeshes[subMeshCount].materialIndex].mTextures[1] != NULL? true : false;
			bool useSpecular = mSubMeshMaterials[mpSubMeshes[subMeshCount].materialIndex].mTextures[2] != NULL ? true : false;
			shader->UpdateMapBuffer(context, useAlpha, useSpecular);

			for (auto light : lights)
			{
				// Pass over the textures for rendering.
				if (!shader->Render(context, mpSubMeshes[subMeshCount].numberOfIndices,
					model->GetWorldMatrix(), view, proj,
					mSubMeshMaterials[mpSubMeshes[subMeshCount].materialIndex].mTextures, mNumberOfTextures,
					light->GetDirection(), light->GetDiffuseColour(), light->GetAmbientColour()))
				{
					logger->GetInstance().WriteLine("Failed to render the mesh model.");
				}

			}
		}
	}
}

/* Create an instance of this mesh.
@Returns CModel* ptr
 */
CModel* CMesh::CreateModel()
{
	// Allocate memory to a model.
	CModel* model = new CModel();

	// Check the model has had space allocated to it.
	if (model == nullptr)
	{
		logger->GetInstance().WriteLine("Failed to allocate space to model. ");
		return nullptr;
	}
	
	// Stick our models on a list to prevent losing the pointers.
	mpModels.push_back(model);

	//return model;
	return model;
}

/* Load a model using our assimp vertex manager.
@Returns bool Success*/
bool CMesh::LoadAssimpModel(std::string filename)
{
	// Grab the mesh object for the last mesh we loaded.

	Assimp::Importer importer;
	const std::string name = filename;
	logger->GetInstance().WriteLine("Attempting to open " + name + " using Assimp.");

	// Read in the file, store this mesh in the scene.
	const aiScene* scene = importer.ReadFile(filename, aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_SortByPType);

	// If scene hasn't been initialised then something has gone wrong!
	if (!scene)
	{
		logger->GetInstance().WriteLine(importer.GetErrorString());
		logger->GetInstance().WriteLine("Failed to create scene.");
		return false;
	}

	mNumberOfSubMeshes = scene->mNumMeshes;

	int rootNode = 1;

	// Allocate memory to the sub mesh.
	mSubMeshMaterials = new MaterialType[mNumberOfSubMeshes];

	///////////////////////////////////////////////
	// Define sub mesh materials.
	///////////////////////////////////////////////

	for (unsigned int materialCount = 0; materialCount < mNumberOfSubMeshes; materialCount++)
	{
		// Default the textures to be a nullptr.
		for (int textureCount = 0; textureCount < mNumberOfTextures; textureCount++)
		{
			mSubMeshMaterials[materialCount].mTextures[textureCount] = nullptr;
		}

		aiMaterial* material = scene->mMaterials[materialCount];
		aiString diffuseMapName;
		aiString alphaMapName;
		aiString specularMapName;
		HRESULT result;
		std::string sDir = "Resources/Textures/";

		///////////////////////////////
		// Diffuse map
		if (material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseMapName) == AI_SUCCESS)
		{
			std::string sTextureName = diffuseMapName.C_Str();
			std::string sFullPath = sDir + sTextureName;

			// Load in the texture.
			result = D3DX11CreateShaderResourceViewFromFile(mpDevice, sFullPath.c_str(), NULL, NULL, &mSubMeshMaterials[materialCount].mTextures[0], NULL);

			if (FAILED(result))
			{
				logger->GetInstance().WriteLine("Failed to load the diffuse map for texture " + sFullPath );
				return false;
			}
			logger->GetInstance().WriteLine("Successfully loaded diffuse map named '" + sFullPath + "'.");
		}


		////////////////////////////////////
		// Alpha map.
		if (scene->mMaterials[materialCount]->GetTexture(aiTextureType_OPACITY, 0, &alphaMapName) == AI_SUCCESS)
		{
			std::string sTextureName = alphaMapName.C_Str();
			std::string sFullPath = sDir + sTextureName;

			// Load in the texture.
			result = D3DX11CreateShaderResourceViewFromFile(mpDevice, sFullPath.c_str(), NULL, NULL, &mSubMeshMaterials[materialCount].mTextures[1], NULL);

			if (FAILED(result))
			{
				logger->GetInstance().WriteLine("Failed to load the alpha map " + sFullPath);
				return false;
			}
			logger->GetInstance().WriteLine("Successfully loaded alpha map named '" + sFullPath + "'.");
		}

		////////////////////////
		// Specular map.
		if (scene->mMaterials[materialCount]->GetTexture(aiTextureType_SPECULAR, 0, &specularMapName) == AI_SUCCESS)
		{
			std::string sTextureName = specularMapName.C_Str();
			std::string sFullPath = sDir + sTextureName;

			// Load in the texture.
			result = D3DX11CreateShaderResourceViewFromFile(mpDevice, sFullPath.c_str(), NULL, NULL, &mSubMeshMaterials[materialCount].mTextures[2], NULL);

			if (FAILED(result))
			{
				logger->GetInstance().WriteLine("Failed to load the specular map " + sFullPath);
				return false;
			}
			logger->GetInstance().WriteLine("Successfully loaded specular map named '" + sFullPath + "'.");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Define sub meshes.
	/////////////////////////////////////////////////////////////////////////

	mpSubMeshes = new SubMesh[mNumberOfSubMeshes];

	for (unsigned int meshCount = 0; meshCount < mNumberOfSubMeshes; meshCount++)
	{
		if (!CreateSubmesh(*scene->mMeshes[meshCount], &mpSubMeshes[meshCount]))
			return false;
	}

	logger->GetInstance().WriteLine("Successfully initialised our arrays for mesh '" + mFilename + "'. ");


	// Close off this section in the log.
	logger->GetInstance().CloseSubtitle();

	// Success!
	return true;
}

bool CMesh::CreateSubmesh(const aiMesh& mesh, SubMesh* subMesh)
{
	VertexType* vertices = new VertexType[mesh.mNumVertices];
	const int kNumberOfIndicesInFace = 3;
	int numberOfIndices = mesh.mNumFaces * kNumberOfIndicesInFace;
	unsigned int* indices = new unsigned int[numberOfIndices];

	for (unsigned int vertex = 0; vertex < mesh.mNumVertices; vertex++)
	{
		vertices[vertex].position = { mesh.mVertices[vertex].x, mesh.mVertices[vertex].y, mesh.mVertices[vertex].z };
		vertices[vertex].uv = { mesh.mTextureCoords[0][vertex].x , mesh.mTextureCoords[0][vertex].y };
		vertices[vertex].normal = { mesh.mNormals[vertex].x, mesh.mNormals[vertex].y, mesh.mNormals[vertex].z };
	}

	int index = 0;
	for (unsigned int i = 0; i < mesh.mNumFaces; i++)
	{
		if (mesh.mFaces[i].mNumIndices == 3)
		{
			indices[index] = mesh.mFaces[i].mIndices[0];
			index++;
			indices[index] = mesh.mFaces[i].mIndices[1];
			index++;
			indices[index] = mesh.mFaces[i].mIndices[2];
			index++;
		}
	}
	
	subMesh->faces = mesh.mFaces;
	subMesh->numberOfVertices = mesh.mNumVertices;
	subMesh->numberOfIndices = mesh.mNumFaces * kNumberOfIndicesInFace;
	subMesh->materialIndex = mesh.mMaterialIndex;

	int offset = 0;

	if (mesh.HasPositions())
	{
		offset += 12;
	}
	if (mesh.HasNormals())
	{
		offset += 12;
	}
	if (mesh.HasTangentsAndBitangents())
	{
		offset += 24;
	}
	if (mesh.HasTextureCoords(0))
	{
		offset += 8;
	}
	if (mesh.HasVertexColors(0))
	{
		offset += 4;
	}

	HRESULT result;

	D3D11_BUFFER_DESC bufferDesc;
	D3D11_SUBRESOURCE_DATA initData;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = offset * subMesh->numberOfVertices;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	initData.pSysMem = &vertices[0];
	result = mpDevice->CreateBuffer(&bufferDesc, &initData, &subMesh->vertexBuffer);

	if (FAILED(result))
	{
		logger->WriteLine("Failed to create the vertex buffer for mesh.");
		return false;
	}

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(unsigned int) * subMesh->numberOfIndices;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	initData.pSysMem = &indices[0];

	result = mpDevice->CreateBuffer(&bufferDesc, &initData, &subMesh->indexBuffer);
	if (FAILED(result))
	{
		logger->WriteLine("Failed to create the vertex buffer for mesh.");
		return false;
	}

	delete[] vertices;
	vertices = nullptr;
	delete[] indices;
	indices = nullptr;

	return true;
}