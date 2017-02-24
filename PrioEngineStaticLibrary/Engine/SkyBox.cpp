#include "SkyBox.h"

CSkyBox::CSkyBox()
{
	mpModel = nullptr;
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
}


CSkyBox::~CSkyBox()
{
}

bool CSkyBox::Initialise(ID3D11Device * device, D3DXVECTOR4 ambientColour)
{
	if (!LoadSkyBoxModel( "Resources/Models/Sphere.fbx"))
	{
		logger->GetInstance().WriteLine("Failed to load the sky box model in initialisation function of skybox.");
		return false;
	}

	if (!InitialiseBuffers(device))
	{
		logger->GetInstance().WriteLine("Failed to initialise the buffers for skybox. ");
		return false;
	}

	mApexColour = D3DXVECTOR4{ 0.0f, 0.15f, 0.66f, 1.0f };
	mCentreColour = ambientColour;

	return true;
}

void CSkyBox::Shutdown()
{
	ReleaseBuffers();

	ReleaseSkyBoxModel();
}

bool CSkyBox::Render(ID3D11DeviceContext * deviceContext)
{
	RenderBuffers(deviceContext);
	return true;
}

void CSkyBox::ReleaseBuffers()
{
	if (mpVertexBuffer)
	{
		mpVertexBuffer->Release();
		mpVertexBuffer = nullptr;
	}

	if (mpIndexBuffer)
	{
		mpIndexBuffer->Release();
		mpIndexBuffer = nullptr;
	}
}

void CSkyBox::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);

	deviceContext->IASetIndexBuffer(mpIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

int CSkyBox::GetIndexCount()
{
	return mIndexCount;
}

D3DXVECTOR4 CSkyBox::GetApexColor()
{
	return mApexColour;
}

D3DXVECTOR4 CSkyBox::GetCenterColor()
{
	return mCentreColour;
}

bool CSkyBox::InitialiseBuffers(ID3D11Device * device)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc;
	D3D11_BUFFER_DESC indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData;
	D3D11_SUBRESOURCE_DATA indexData;
	HRESULT result;

	vertices = new VertexType[mVertexCount];
	for (unsigned int i = 0; i < mVertexCount; i++)
	{
		vertices[i].position = mpVerticesList[i];
	}

	indices = new unsigned long[mIndexCount];
	for (unsigned int i = 0; i < mIndexCount; i++)
	{
		indices[i] = mpIndicesList[i];
	}

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * mVertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &mpVertexBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the vertex buffer for the skybox.");
		return false;
	}

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * mIndexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&indexBufferDesc, &indexData, &mpIndexBuffer);
	if (FAILED(result))
	{
		logger->GetInstance().WriteLine("Failed to create the index buffeer for the skybox.");
		return false;
	}

	delete[] vertices;
	vertices = nullptr;
	delete[] indices;
	indices = nullptr;

	return true;
}

bool CSkyBox::LoadSkyBoxModel(char * modelName)
{
	// Grab the mesh object for the last mesh we loaded.

	Assimp::Importer importer;
	const std::string name = modelName;
	logger->GetInstance().WriteLine("Attempting to open " + name + " using Assimp.");

	// Read in the file, store this mesh in the scene.
	const aiScene* scene = importer.ReadFile(modelName,
		aiProcess_ConvertToLeftHanded |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_SortByPType);

	// If scene hasn't been initialised then something has gone wrong!
	if (!scene)
	{
		logger->GetInstance().WriteLine(importer.GetErrorString());
		logger->GetInstance().WriteLine("Failed to create scene.");
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

	logger->GetInstance().WriteLine("Successfully initialised our arrays for mesh '" + name + "'. ");

	// Close off this section in the log.
	logger->GetInstance().CloseSubtitle();

	mVertexCount = mpVerticesList.size();
	mIndexCount = mpIndicesList.size();
	// Success!
	return true;
}

void CSkyBox::ReleaseSkyBoxModel()
{
}
