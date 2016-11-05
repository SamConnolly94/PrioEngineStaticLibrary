#include "Input.h"

CInput::CInput()
{
}

CInput::CInput(const CInput &)
{
}


CInput::~CInput()
{
}

void CInput::Initialise()
{
	// Set all the keys to false.
	for (int i = 0; i < 256; i++)
	{
		mKeys[i] = false;
		mWasHeldLastFrame[i] = false;
	}

	return;
}

void CInput::KeyDown(unsigned int input)
{
	// If key is pressed down, save that state in the key array.
	mKeys[input] = true;
	return;
}

void CInput::KeyUp(unsigned int input)
{
	// If a key is let go of then save it's new state in the key array.
	mKeys[input] = false;
	return;
}

bool CInput::IsKeyDown(unsigned int key)
{
	// Return the key state out of the key array.
	return mKeys[key];
}

/* If the key has been hit this frame then return true. Will not ring true if it was hit last frame as well.*/
bool CInput::KeyHit(unsigned int key)
{
	return IsKeyDown(key) && !mWasHeldLastFrame[key];
}

/* If the key has been held for multiple frames then returns true. */
bool CInput::KeyHeld(unsigned int key)
{
	// If the key isn't down, then we don't even need to bother processing this function.
	if (!IsKeyDown(key))
	{
		mWasHeldLastFrame[key] = false;
		return false;
	}

	// If the key has been pressed, but this is the first frame.
	if (IsKeyDown(key) && !mWasHeldLastFrame[key])
	{
		mWasHeldLastFrame[key] = true;
		return false;
	}

	// Return the result regarding whether key is held AND was held last frame too!
	return (IsKeyDown(key) && mWasHeldLastFrame[key]);
}

