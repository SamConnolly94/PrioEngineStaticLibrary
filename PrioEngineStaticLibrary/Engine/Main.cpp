#include "Engine.h"
#include "PrioEngineVars.h"

// Declaration of functions used to run game itself.
void GameLoop(CEngine* &engine);
void Control(CEngine* &engine, CCamera* cam, CTerrain* grid, float frameTime);

CLogger* logger;

// Main
int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Enable run time memory check while running in debug.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	// Start the game engine.
	CEngine* PrioEngine;
	bool result;

	// Create the engine object.
	PrioEngine = new CEngine();
	// If failed to create the engine.
	if (!PrioEngine)
	{
		// Write a message to the log to let the user know we couldn't create the engine object.
		logger->GetInstance().WriteLine("Could not create the engine object.");
		// Return 0, we're saying we're okay, implement error codes in future versions maybe? 
		return 0;
	}
	logger->GetInstance().MemoryAllocWriteLine(typeid(PrioEngine).name());

	// Set up the engine.
	result = PrioEngine->Initialise("Prio Engine");
	// If we successfully initialised the game engine
	if (result)
	{
		// Start the game loop.
		GameLoop(PrioEngine);
	}

	// Shutdown and release the engine.
	PrioEngine->Shutdown();
	delete PrioEngine;
	logger->GetInstance().MemoryDeallocWriteLine(typeid(PrioEngine).name());
	PrioEngine = nullptr;
	logger->GetInstance().Shutdown();

	// The singleton logger will cause a memory leak. Don't worry about it. Should be no more than 64 bytes taken by it though, more likely will only take 48 bytes.
	_CrtDumpMemoryLeaks();

	return 0;
}

/* Controls any gameplay and things that should happen when we play the game. */
void GameLoop(CEngine* &engine)
{
	const int frameTimePosX = 10;
	const int frameTimePosY = 10;
	const float FPSPosX = 10.0f;
	const float FPSPosY = 50.0f;

	// Constants.
	const float kRotationSpeed = 100.0f;
	const float kMovementSpeed = 1.0f;

	// Variables
	float frameTime;
	CCamera* myCam;
	myCam = engine->GetMainCamera();

	CTerrain* terrain = engine->CreateTerrain("Default.map");

	SentenceType* frametimeText = engine->CreateText("Frametime: ", frameTimePosX, frameTimePosY, 32);
	SentenceType* FPSText = engine->CreateText("FPS: ", static_cast<int>(FPSPosX), static_cast<int>(FPSPosY), 32);

	// Start the game timer running.
	engine->StartTimer();

	const float kTextUpdateInterval = 0.2f;
	float timeSinceTextUpdate = kTextUpdateInterval;

	// Process anything which should happen in the game here.
	while (engine->IsRunning())
	{
		// Get hold of the time it took to draw the last frame.
		frameTime = engine->GetFrameTime();

		// Process any keys pressed this frame.
		Control(engine, myCam, terrain, frameTime);

		// Update the text on our game.
		if (timeSinceTextUpdate >= kTextUpdateInterval)
		{
			engine->UpdateText(frametimeText, "FrameTime: " + std::to_string(frameTime), frameTimePosX, frameTimePosY, { 1.0f, 1.0f, 0.0f });
			engine->UpdateText(FPSText, "FPS: " + std::to_string(1.0f / frameTime), static_cast<int>(FPSPosX), static_cast<int>(FPSPosY), { 1.0f, 1.0f, 0.0f });
			timeSinceTextUpdate = 0.0f;
		}
		timeSinceTextUpdate += frameTime;
	}
}

/* Control any user input here, must be called in every tick of the game loop. */
void Control(CEngine* &engine, CCamera* cam, CTerrain* grid, float frameTime)
{
	const float kMoveSpeed = 25.0f;
	const float kCamRotationSpeed = 100.0f;
	
	/// Camera control.
	// Move backwards
	if (engine->KeyHeld(PrioEngine::Key::kS))
	{
		cam->MoveLocalZ(-kMoveSpeed * frameTime);
	}
	// Move Forwards
	else if (engine->KeyHeld(PrioEngine::Key::kW))
	{
		cam->MoveLocalZ(kMoveSpeed * frameTime);
	}
	// Move Left
	if (engine->KeyHeld(PrioEngine::Key::kA))
	{
		cam->MoveLocalX(-kMoveSpeed * frameTime);
	}
	// Move Right
	else if (engine->KeyHeld(PrioEngine::Key::kD))
	{
		cam->MoveLocalX(kMoveSpeed * frameTime);
	}

	// Rotate left
	if (engine->KeyHeld(PrioEngine::Key::kLeft))
	{
		cam->RotateY(-kCamRotationSpeed * frameTime);
	}
	// Rotate right.
	else if (engine->KeyHeld(PrioEngine::Key::kRight))
	{
		cam->RotateY(kCamRotationSpeed * frameTime);
	}
	// Rotate upwards.
	if (engine->KeyHeld(PrioEngine::Key::kUp))
	{
		cam->RotateX(-kCamRotationSpeed * frameTime);
	}
	// Rotate downwards.
	else if (engine->KeyHeld(PrioEngine::Key::kDown))
	{
		cam->RotateX(kCamRotationSpeed * frameTime);
	}

	/// User controls.

	// If the user hits escape.
	if (engine->KeyHit(PrioEngine::Key::kEscape))
	{
		engine->Stop();
	}

	// If the user hits F1.
	if (engine->KeyHit(PrioEngine::Key::kF1))
	{
		engine->ToggleWireframe();
	}
	else if (engine->KeyHit(PrioEngine::Key::kF2))
	{
		engine->ToggleFullscreen(PrioEngine::Key::kF2);
	}
}