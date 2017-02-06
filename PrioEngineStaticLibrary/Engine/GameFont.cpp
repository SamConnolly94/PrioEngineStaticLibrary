#include "GameFont.h"



CGameFont::CGameFont()
{
	mpFont = nullptr;
	mpTexture = nullptr;
}


CGameFont::~CGameFont()
{
}

bool CGameFont::Initialise(ID3D11Device * device, char * fontDataFile, WCHAR * fontTexture)
{
	bool result = false; 

	// Load the text file containing any data about the font.
	result = LoadFontData(fontDataFile);

	// If we failed to load the font data text file.
	if (!result)
	{
		// Output a message informing the user to the log.
		gLogger->WriteLine("Failed to load the font data file.");
		// Tell whatever called this that we failed.
		return false;
	}

	// Load the texture file which has our font plotted on it.
	result = LoadTextureFile(device, fontTexture);

	// If we failed to load the font texture file.
	if (!result)
	{
		// Output a message informing the user to the log.
		gLogger->WriteLine("Failed to load the texture for a font.");
		// Tell whatever called this that we failed.
		return false;
	}

	// Successfully loaded everything.
	return true;
}

void CGameFont::Shutdown()
{	
	// Release the texture which contains the each character belonging to the font.
	ReleaseTexture();

	// Release the data text file.
	ReleaseFontData();
}

ID3D11ShaderResourceView * CGameFont::GetTexture()
{
	return mpTexture->GetTexture();
}

void CGameFont::BuildVertexArray(void * vertices, std::string text, float xPos, float yPos)
{
	VertexType* vertexPtr;
	int numLetters;
	int index;
	int letter;
	
	// Cast the vertices into our VertexType struct.
	vertexPtr = static_cast<VertexType*>(vertices);

	// Get the number of letters in the sentence we want to draw.
	numLetters = static_cast<int>(std::strlen(text.c_str()));

	// Initialise the index of a vertex array.
	index = 0;
	const float fontHeight = 40.0f;

	// Draw each letter.
	for (int i = 0; i < numLetters; i++)
	{
		letter = ((int)text[i]) - 32;

		// If the letter is a space then just move over three pixels.
		if (letter == 0)
		{
			xPos = xPos + 6.0f;
		}
		else
		{
			// First triangle in quad.
			vertexPtr[index].position = D3DXVECTOR3(xPos, yPos, 0.0f);  // Top left.
			vertexPtr[index].texture = D3DXVECTOR2(mpFont[letter].left, 0.0f);
			index++;

			vertexPtr[index].position = D3DXVECTOR3((xPos + mpFont[letter].size), (yPos - fontHeight), 0.0f);  // Bottom right.
			vertexPtr[index].texture = D3DXVECTOR2(mpFont[letter].right, 1.0f);
			index++;

			vertexPtr[index].position = D3DXVECTOR3(xPos, (yPos - fontHeight), 0.0f);  // Bottom left.
			vertexPtr[index].texture = D3DXVECTOR2(mpFont[letter].left, 1.0f);
			index++;

			// Second triangle in quad.
			vertexPtr[index].position = D3DXVECTOR3(xPos, yPos, 0.0f);  // Top left.
			vertexPtr[index].texture = D3DXVECTOR2(mpFont[letter].left, 0.0f);
			index++;

			vertexPtr[index].position = D3DXVECTOR3(xPos + mpFont[letter].size, yPos, 0.0f);  // Top right.
			vertexPtr[index].texture = D3DXVECTOR2(mpFont[letter].right, 0.0f);
			index++;

			vertexPtr[index].position = D3DXVECTOR3((xPos + mpFont[letter].size), (yPos - fontHeight), 0.0f);  // Bottom right.
			vertexPtr[index].texture = D3DXVECTOR2(mpFont[letter].right, 1.0f);
			index++;

			// Update the x location for drawing by the size of the letter and one pixel.
			xPos = xPos + mpFont[letter].size + 1.0f;
		}
	}
}

bool CGameFont::LoadFontData(char * dataFile)
{
	std::ifstream inFile;
	char temp;

	inFile.open(dataFile);

	if (!inFile.is_open())
	{
		gLogger->WriteLine("Could not open the input file in GameFont.cpp.");
		return false;
	}

	// Allocate memory to our temp array for the font.
	mpFont = new FontType[95];
	// Write allocation message to memory log.
	gLogger->MemoryAllocWriteLine(typeid(mpFont).name());
	
	// Check if we initalised our dynamic array for the font.
	if (!mpFont)
	{
		// Write the failure message to the log.
		gLogger->WriteLine("Failed to allocate memory to the mpFont array in GameFont.cpp.");
		// Tell whatever called this we failed.
		return false;
	}

	// Read in the 95 used ascii characters for text.
	for (int i = 0; i<95; i++)
	{
		inFile.get(temp);
		while (temp != ' ')
		{
			inFile.get(temp);
		}
		inFile.get(temp);
		while (temp != ' ')
		{
			inFile.get(temp);
		}

		inFile >> mpFont[i].left;
		inFile >> mpFont[i].right;
		inFile >> mpFont[i].size;
	}

	// Close the file, done with it now.
	inFile.close();

	// Success!
	return true;
}

void CGameFont::ReleaseFontData()
{
	// If memory has been allocated to the font array.
	if (mpFont)
	{
		// Deallocate the memory.
		delete[] mpFont;
		// Write to memory log.
		gLogger->MemoryDeallocWriteLine(typeid(mpFont).name());
		// Set the font to be nullptr so it can't be used again.
		mpFont = nullptr;
	}
}

bool CGameFont::LoadTextureFile(ID3D11Device * device, WCHAR * fontTexture)
{
	bool result = false;

	// Create the texture object.
	mpTexture = new CTexture();

	// Check if we successfully allocated memory to the texture.
	if (!mpTexture)
	{
		// Write the failure message to the log.
		gLogger->WriteLine("Failed to allocate memory to mpTexture in GameFont.cpp.");
		// Tell whatever called this that it failed.
		return false;
	}
	// Write the memory allocation message to the memory log.
	gLogger->MemoryAllocWriteLine(typeid(mpTexture).name());
	
	// Intialise the texture which we allocated memory too.
	result = mpTexture->Initialise(device, fontTexture);

	// Check if we initialised our texture.
	if (!result)
	{
		// Output error message to the logger.
		gLogger->WriteLine("Failed to intialise mpTexture in GameFont.cpp.");
		// Tell whatever called this we failed.
		return false;
	}

	// Success!
	return true;
}

void CGameFont::ReleaseTexture()
{
	// If memory was allocated to the texture.
	if (mpTexture)
	{
		// Release resources held by the texture object.
		mpTexture->Shutdown();
		// Deallocate memory from the texture object.
		delete mpTexture;
		// Write deallocation message to the memory log.
		gLogger->MemoryDeallocWriteLine(typeid(mpTexture).name());
		// Set to nullptr so it can't be used.
		mpTexture = nullptr;
	}
}
