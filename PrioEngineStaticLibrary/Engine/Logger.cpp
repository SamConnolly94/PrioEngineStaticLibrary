#include "Logger.h"

// Constructor.
CLogger::CLogger()
{
	mLoggingEnabled = false;
	mLogFile.open(mDebugLogName);

	mMemoryLogFile.open(mMemoryLogName);

	// Check to see if logging was successfully enabled or not.
#ifdef _LOGGING_ENABLED
	mLoggingEnabled = true;

	if (mLogFile.is_open())
	{
		// Set our private boolean flag which toggles logging to on.
		mLoggingEnabled = true;

		// Initialise our line number to be 0.
		mLineNumber = 0;

		// Output a message to the log.
		WriteLine("Successfully opened " + mDebugLogName);
	}
	else
	{
		// Set our private boolean flag which toggles logging to off.
		mLoggingEnabled = false;

		// Output error message to user.
		MessageBox(NULL, "Could not open the debug log to write to it, make sure you haven't left it open. The program will continue to run but without logging.", "Could not open debug log!", MB_OK);

	}

	if (mMemoryLogFile.is_open() && mLoggingEnabled)
	{
		mMemoryLogLineNumber = 0;

		WriteLine("Successfully opened " + mMemoryLogName);
	}
	else
	{
		MessageBox(NULL, "Could not open the memory log to write to it, make sure you haven't left it open. The program will continue to run but without logging.", "Could not open memory log!", MB_OK);
	}

#endif

}

// Destructor.
void CLogger::Shutdown()
{
	// Analyse the memory allocated / deallocated messages and check for anything missing.
	//MemoryAnalysis();

	// Close the memory dump log.
	mMemoryLogFile.close();
	// Close the debug log.
	mLogFile.close();

	// Reset any static variables we used to null.
	mLineNumber = NULL;
	mMemoryLogLineNumber = NULL;
}

/**  Write a piece of text to the debug log and add a new line.. */
void CLogger::WriteLine(std::string text)
{
	if (mLoggingEnabled)
	{
		// Increment our line number.
		mLineNumber++;

		// Check that the log file we want to write to is open and available.
		if (mLogFile.is_open()) 
		{
			mLogFile << std::setfill('0') << std::setw(5) << mLineNumber << "  " << text << std::endl;
		}
		else 
		{
			std::string errMsg = "Log is not open or cannot be written to so the WriteLine of the logger has failed. ";
			std::string msgTitle = "Log not open!";
			MessageBox(0, errMsg.c_str(), msgTitle.c_str(), MB_OK);
		}
	}
}


/*  Write a piece of text to the debug log and add a new line. Use typid(var).name() and pass it in as a variable. */
void CLogger::MemoryAllocWriteLine(std::string name)
{
	if (mLoggingEnabled)
	{
		// Increment our line number.
		mMemoryLogLineNumber++;

		// Check that the log file we want to write to is open and available.
		if (mMemoryLogFile.is_open())
		{
			mMemoryLogFile << std::setfill('0') << std::setw(5) << mMemoryLogLineNumber << " Variable of type " << name << " was allocated memory." << std::endl;
		}
		else
		{
			MessageBox(0, "Memory log is not open or cannot be written to so the WriteLine of the logger has failed. ", "Memory log not opened!", MB_OK);
		}
	}
}

/**  Write a piece of text to the debug log and add a new line.. */
void CLogger::MemoryDeallocWriteLine(std::string name)
{
	if (mLoggingEnabled)
	{
		// Increment our line number.
		mMemoryLogLineNumber++;

		// Check that the log file we want to write to is open and available.
		if (mMemoryLogFile.is_open())
		{
			mMemoryLogFile << std::setfill('0') << std::setw(5) << mMemoryLogLineNumber << " Variable of type " << name.c_str() << " memory was deallocated." << std::endl;
		}
		else
		{
			std::string errMsg = "Memory log is not open or cannot be written to so the WriteLine of the logger has failed. ";
			std::string errTitle = "Memory log not opened!";
			MessageBox(0, errMsg.c_str(), errTitle.c_str(), MB_OK);
		}
	}
}

/* Writes a new section to our log, helps with clarity and ease of reading. */
void CLogger::WriteSubtitle(std::string name)
{
	WriteLine("");
	WriteLine(k256Astericks);
	WriteLine(name.c_str());
	WriteLine(k256Astericks);
	WriteLine("");
}

/* Closes the section to our log ready to start a new one.*/
void CLogger::CloseSubtitle()
{
	WriteLine(k256Astericks);
	WriteLine("");
}

/* This will analyse the memory logs and tell you what has been allocated memory but never been deallocated. Should only be run at the end of the program. */
void CLogger::MemoryAnalysis()
{
	enum MemoryType
	{
		Allocated,
		Deallocated
	};

	struct MemoryBlock
	{
		std::string name;
		std::string classType;
		MemoryType memoryType;
	};

	std::list<MemoryBlock*> allocList;
	std::list<MemoryBlock*> deallocList;

	std::ifstream inFile;

	inFile.open(mMemoryLogName);
	const std::string allocMessage = "Variable of type ";

	std::string line = "";
	while (std::getline(inFile, line))
	{
		MemoryBlock* block = new MemoryBlock();
		//MemoryAllocWriteLine(typeid(block).name());

		std::string variableName = "";
		size_t strStartPos = 0;
		size_t strOffset = 0;

		if (line != "")
		{
			// Find where our allocation message starts.
			strStartPos = line.find(allocMessage);
			// Find where our allocation message ends.
			strOffset = strStartPos + allocMessage.length();

			// Find the keyword (class / struct typically) which follows our allocation message.
			block->classType = line.substr(strOffset, line.find(' ', strOffset) - strOffset);

			// Find the start point of the keyword.
			strStartPos = line.find(block->classType);
			// Set the offset amount by this.
			strOffset = strStartPos + block->classType.length();
			// Find the end of the variable type name.
			size_t endPoint = static_cast<int>(line.find('\\*'));
			// get the name of the variable type.
			block->name = line.substr(strOffset + 1, endPoint - strOffset);

			strStartPos = line.find(block->name);
			strOffset = strStartPos + block->name.length();
			size_t found = line.find("deallocated.");
			if (found != std::string::npos)
			{
				block->memoryType = Deallocated;
				deallocList.push_back(block);
			}
			else
			{
				found = line.find("allocated ");
				if (found != std::string::npos)
				{
					block->memoryType = Allocated;
					allocList.push_back(block);
				}
				else
				{
					//WriteLine("Something went wrong when gathering data for memory analysis, do not rely on this!");
					delete block;
				}
			}
		}

		inFile.eof();
	}

	std::list<MemoryBlock*>::iterator allocIt = allocList.begin();
	std::list<MemoryBlock*>::iterator deallocIt = deallocList.begin();
	while (allocIt != allocList.end())
	{
		bool foundPair = false;
		deallocIt = deallocList.begin();

		// Iterate through the deallocated memory list.
		while (deallocIt != deallocList.end() && !foundPair)
		{
			// If all the current things match then remove from dealloc list and break.
			if ((*allocIt)->name != "FOUND" &&
				((*allocIt)->name == (*deallocIt)->name && (*allocIt)->classType == (*deallocIt)->classType))
			{
				// Remove from the list.
				(*deallocIt)->name = "FOUND";
				(*deallocIt)->classType = "FOUND";

				// Set some flags on the allocation info.
				(*allocIt)->name = "FOUND";
				(*allocIt)->classType = "FOUND";

				// Don't continue iterating through.
				foundPair = true;
			}
			else
			{
				// Look at the next element on the dealloc list.
				deallocIt++;
			}
		}
		deallocIt = deallocList.begin();
		allocIt++;
	}

	WriteSubtitle("Memory dump summary.");

	for (auto memBlock : allocList)
	{
		if (allocList.back()->name != "FOUND")
		{
			WriteLine("Memory was given to " + allocList.back()->classType + " " + allocList.back()->name + " but never deallocated.");
		}
		delete memBlock;
	}

	allocList.clear();

	for (auto memBlock : deallocList)
	{
		delete memBlock;
	}
	deallocList.clear();

	WriteLine("");
	WriteLine(k256Astericks);
	WriteLine("Memory dump summary complete.");
	WriteLine(k256Astericks);
	return;
}
