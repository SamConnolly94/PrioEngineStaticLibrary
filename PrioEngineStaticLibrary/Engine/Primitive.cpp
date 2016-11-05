#include "Primitive.h"

/*
* @PARAM filename - The location of the texture and it's file name and extension from the executable file.
*/
CPrimitive::CPrimitive(WCHAR* filename)
{
	// Initialise all variables to be null.
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
	mpTexture = nullptr;
	mpTextureFilename = nullptr;
	ResetColour();
	mUseDiffuseLighting = false;

	// Store the filename for later use.
	mpTextureFilename = filename;

	mpVertexManager = nullptr;
}

CPrimitive::CPrimitive(WCHAR* filename, bool useLighting)
{
	// Initialise all variables to be null.
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
	mpTexture = nullptr;
	mpTextureFilename = nullptr;
	ResetColour();

	// Set the flag to use diffuse lighting on our texture.
	mUseDiffuseLighting = useLighting;

	// Store the filename for later use.
	mpTextureFilename = filename;

	mpVertexManager = nullptr;

}

CPrimitive::CPrimitive(PrioEngine::RGBA colour)
{
	mpVertexBuffer = nullptr;
	mpIndexBuffer = nullptr;
	mpTexture = nullptr;
	mpTextureFilename = nullptr;
	mUseDiffuseLighting = false;
	ResetColour();

	// Store the colour which we have passed in.
	mColour = colour;

	mpVertexManager = nullptr;

	mpVertexManager->SetColour(colour);
}


CPrimitive::~CPrimitive()
{
	if (mpVertexManager)
	{
		delete mpVertexManager;
		mpVertexManager = nullptr;
		mpLogger->GetLogger().MemoryDeallocWriteLine(typeid(mpVertexManager).name());
	}
}

void CPrimitive::Shutdown()
{
	// Release the model texture.
	ReleaseTexture();

	// Release vertex and index buffers.
	ShutdownBuffers();
}

void CPrimitive::Render(ID3D11DeviceContext * deviceContext)
{
	// Place the vertex and index bufdfers onto the pipeline so they are ready for drawing.
	RenderBuffers(deviceContext);
}

/* Retrieves the number of index's in the model, required for the pixel shader when using colour. */
int CPrimitive::GetIndex()
{
	return mIndexCount;
}

ID3D11ShaderResourceView * CPrimitive::GetTexture()
{
	return mpTexture->GetTexture();
}

int CPrimitive::GetNumberOfIndices()
{
	// Find what shape we're using.
	switch (mShape)
	{
		// Cube
	case PrioEngine::Primitives::cube:
		return PrioEngine::Cube::kNumOfIndices;
		// Triangle
	case PrioEngine::Primitives::triangle:
		return PrioEngine::Triangle::kNumOfIndices;
	};

	return 0;
}

int CPrimitive::GetNumberOfVertices()
{
	// Find what shape we're using.
	switch (mShape)
	{
		// Cube
	case PrioEngine::Primitives::cube:
		return PrioEngine::Cube::kNumOfVertices;
		// Triangle
	case PrioEngine::Primitives::triangle:
		return PrioEngine::Triangle::kNumOfVertices;
	};

	return 0;
}

void CPrimitive::LoadIndiceData(unsigned long* &indices)
{
	switch (mShape)
	{
	case PrioEngine::Primitives::cube:
		for (int i = 0; i < PrioEngine::Cube::kNumOfIndices; i++)
		{
			indices[i] = PrioEngine::Cube::indices[i];
		}
		return;
	case PrioEngine::Primitives::triangle:
		for (int i = 0; i < PrioEngine::Triangle::kNumOfIndices; i++)
		{
			indices[i] = PrioEngine::Triangle::indices[i];
		}
		return;
	}
}

/* Release vertex and index buffers. */
void CPrimitive::ShutdownBuffers()
{
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

/* Place the vertex and index bufdfers onto the pipeline so they are ready for drawing. */
void CPrimitive::RenderBuffers(ID3D11DeviceContext * deviceContext)
{
	mpVertexManager->RenderBuffers(deviceContext, mpIndexBuffer);
}

bool CPrimitive::LoadTexture(ID3D11Device * device)
{
	bool result;

	// Create the texture object.
	mpTexture = new CTexture();

	if (!mpTexture)
	{
		mpLogger->GetLogger().WriteLine("Failed to create the texture object.");
		return false;
	}

	mpLogger->GetLogger().MemoryAllocWriteLine(typeid(mpTexture).name());

	// Initialise the texture object.
	result = mpTexture->Initialise(device, mpTextureFilename);

	if (!result)
	{
		mpLogger->GetLogger().WriteLine("Failed to initialise the texture object. ");
	}

	return true;
}

void CPrimitive::ReleaseTexture()
{
	// Release the texture object.
	if (mpTexture)
	{
		mpTexture->Shutdown();
		delete mpTexture;
		mpTexture = nullptr;
		mpLogger->GetLogger().MemoryDeallocWriteLine(typeid(mpTexture).name());
	}

	if (mpTextureFilename)
	{
		mpTextureFilename = nullptr;
	}
}

void CPrimitive::ResetColour()
{
	// Initialise everything to more a value which is not the same as null, 0.0f is the same as null and can cause issues.
	mColour.r = -1.0f;
	mColour.g = -1.0f;
	mColour.b = -1.0f;
	mColour.a = -1.0f;
}

bool CPrimitive::HasTexture()
{
	return mpTexture != nullptr;
}

bool CPrimitive::HasColour()
{
	return mColour.r > -1.0f && mColour.g > -1.0f && mColour.b > -1.0f;
}

bool CPrimitive::UseDiffuseLight()
{
	return mUseDiffuseLighting;
}


