// BILLY FRONTIER ENTRY POINT
// (C) 2022 Iliyas Jorio
// This file is part of Billy Frontier. https://github.com/jorio/billyfrontier

#include <SDL.h>
#include "Pomme.h"
#include "PommeInit.h"
#include "PommeFiles.h"

#include <iostream>
#include <cstring>

#if __APPLE__
#include <libproc.h>
#include <unistd.h>
#endif

#ifdef __vita__
#include <unistd.h> 
#include <vitasdk.h>
extern "C" {
int _newlib_heap_size_user = 256 * 1024 * 1024;
void vglInitExtended(int legacy_pool_size, int width, int height, int ram_threshold, SceGxmMultisampleMode msaa);
};
#endif

extern "C"
{
	#include "game.h"
	#include "version.h"

	SDL_Window* gSDLWindow = nullptr;
	FSSpec gDataSpec;
//	CommandLineOptions gCommandLine;
	int gCurrentAntialiasingLevel;

#if 0 //_WIN32
	// Tell Windows graphics driver that we prefer running on a dedicated GPU if available
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
#endif

//	int GameMain(void);
}

static fs::path FindGameData()
{
	fs::path dataPath;

#if __APPLE__
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid_t pid = getpid();
	int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
	if (ret <= 0)
	{
		throw std::runtime_error(std::string(__func__) + ": proc_pidpath failed: " + std::string(strerror(errno)));
	}

	dataPath = pathbuf;
	dataPath = dataPath.parent_path().parent_path() / "Resources";
#elif defined(__vita__)
	dataPath = "ux0:data/BillyFrontier/Data";
#else
	dataPath = "Data";
#endif

	dataPath = dataPath.lexically_normal();

	// Set data spec -- Lets the game know where to find its asset files
	gDataSpec = Pomme::Files::HostPathToFSSpec(dataPath / "Skeletons");

	// Use application resource file
	auto applicationSpec = Pomme::Files::HostPathToFSSpec(dataPath / "System" / "Application");
	short resFileRefNum = FSpOpenResFile(&applicationSpec, fsRdPerm);

	if (resFileRefNum == -1)
	{
		throw std::runtime_error("Couldn't find the Data folder.");
	}

	UseResFile(resFileRefNum);

	return dataPath;
}

static void ParseCommandLine(int argc, char** argv)
{
	(void) argc;
	(void) argv;

#if 0
	memset(&gCommandLine, 0, sizeof(gCommandLine));
	gCommandLine.vsync = 1;

	for (int i = 1; i < argc; i++)
	{
		std::string argument = argv[i];

		if (argument == "--track")
		{
			GAME_ASSERT_MESSAGE(i + 1 < argc, "practice track # unspecified");
			gCommandLine.bootToTrack = atoi(argv[i + 1]);
			i += 1;
		}
		else if (argument == "--car")
		{
			GAME_ASSERT_MESSAGE(i + 1 < argc, "car # unspecified");
			gCommandLine.car = atoi(argv[i + 1]);
			i += 1;
		}
		else if (argument == "--stats")
			gDebugMode = 1;
		else if (argument == "--no-vsync")
			gCommandLine.vsync = 0;
		else if (argument == "--vsync")
			gCommandLine.vsync = 1;
		else if (argument == "--adaptive-vsync")
			gCommandLine.vsync = -1;
		else if (argument == "--display")
		{
			GAME_ASSERT_MESSAGE(i + 1 < argc, "display # unspecified");
			gCommandLine.display = atoi(argv[i + 1]);
			i += 1;
		}
		else if (argument == "--windowed-resolution")
		{
			GAME_ASSERT_MESSAGE(i + 2 < argc, "windowed width & height unspecified");
			gCommandLine.windowedWidth = atoi(argv[i + 1]);
			gCommandLine.windowedHeight = atoi(argv[i + 2]);
			i += 2;
		}
#if 0
		else if (argument == "--fullscreen-resolution")
		{
			GAME_ASSERT_MESSAGE(i + 2 < argc, "fullscreen width & height unspecified");
			gCommandLine.fullscreenWidth = atoi(argv[i + 1]);
			gCommandLine.fullscreenHeight = atoi(argv[i + 2]);
			i += 2;
		}
		else if (argument == "--fullscreen-refresh-rate")
		{
			GAME_ASSERT_MESSAGE(i + 1 < argc, "fullscreen refresh rate unspecified");
			gCommandLine.fullscreenRefreshRate = atoi(argv[i + 1]);
			i += 1;
		}
#endif
	}
#endif
}

static void GetInitialWindowSize(int display, int& width, int& height)
{
	const float aspectRatio = 4.0f / 3.0f;
	const float screenCoverage = 2.0f / 3.0f;

	SDL_Rect displayBounds = { .x = 0, .y = 0, .w = 640, .h = 480 };
	SDL_GetDisplayUsableBounds(display, &displayBounds);

	if (displayBounds.w > displayBounds.h)
	{
		width	= displayBounds.h * screenCoverage * aspectRatio;
		height	= displayBounds.h * screenCoverage;
	}
	else
	{
		width	= displayBounds.w * screenCoverage;
		height	= displayBounds.w * screenCoverage / aspectRatio;
	}
}

static void Boot()
{
	// Start our "machine"
	Pomme::Init();

	// Load game prefs before starting
	InitDefaultPrefs();
	LoadPrefs();

retryVideo:
	// Initialize SDL video subsystem
	if (0 != SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Couldn't initialize SDL video subsystem.");
	}

	// Create window
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	gCurrentAntialiasingLevel = gGamePrefs.antialiasingLevel;
	if (gCurrentAntialiasingLevel != 0)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1 << gCurrentAntialiasingLevel);
	}

	// Determine display
	int display = gGamePrefs.monitorNum;

#if 0
	// Display # given on CLI takes precedence over prefs
	if (gCommandLine.display != 0)
	{
		display = gCommandLine.display - 1;
		gGamePrefs.monitorNum = display;
		SavePrefs();
		printf("Will try display %d\n", display);
	}
#endif

	if (display >= SDL_GetNumVideoDisplays())
	{
		display = 0;
	}

	// Determine initial window size
	int initialWidth = 640;
	int initialHeight = 480;
#if 0
	if (gCommandLine.windowedWidth != 0 && gCommandLine.windowedHeight != 0)
	{
		// Window size given on CLI takes precedence
		initialWidth = gCommandLine.windowedWidth;
		initialHeight = gCommandLine.windowedHeight;
	}
	else
#endif
	{
		GetInitialWindowSize(display, initialWidth, initialHeight);
	}

	gSDLWindow = SDL_CreateWindow(
			"Billy Frontier " PROJECT_VERSION,
			SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
			SDL_WINDOWPOS_UNDEFINED_DISPLAY(display),
			initialWidth,
			initialHeight,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

	if (!gSDLWindow)
	{
		if (gCurrentAntialiasingLevel != 0)
		{
			printf("Couldn't create SDL window with the requested MSAA level. Retrying without MSAA...\n");

			// retry without MSAA
			gGamePrefs.antialiasingLevel = 0;
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
			goto retryVideo;
		}
		else
		{
			throw std::runtime_error("Couldn't create SDL window.");
		}
	}

	// Find path to game data folder
	fs::path dataPath = FindGameData();

	// Init joystick subsystem
	{
		SDL_Init(SDL_INIT_GAMECONTROLLER);
		auto gamecontrollerdbPath8 = (dataPath / "System" / "gamecontrollerdb.txt").u8string();
		if (-1 == SDL_GameControllerAddMappingsFromFile((const char*)gamecontrollerdbPath8.c_str()))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Billy Frontier", "Couldn't load gamecontrollerdb.txt!", gSDLWindow);
		}
	}
}

static void Shutdown()
{
	Pomme::Shutdown();

	if (gSDLWindow)
	{
		SDL_DestroyWindow(gSDLWindow);
		gSDLWindow = NULL;
	}

	SDL_Quit();
}

int main(int argc, char** argv)
{
	int				returnCode				= 0;
	std::string		finalErrorMessage		= "";
	bool			showFinalErrorMessage	= false;

#ifdef __vita__
	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
	SceCommonDialogConfigParam cmnDlgCfgParam;
	sceCommonDialogConfigParamInit(&cmnDlgCfgParam);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&cmnDlgCfgParam.language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&cmnDlgCfgParam.enterButtonAssign);
	sceCommonDialogSetConfigParam(&cmnDlgCfgParam);
	
	SDL_setenv("VITA_DISABLE_TOUCH_BACK", "1", 1);
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);
	vglInitExtended(4 * 1024 * 1024, 960, 544, 8 * 1024 * 1024, SCE_GXM_MULTISAMPLE_4X);
	chdir("ux0:data");
#endif

	try
	{
		ParseCommandLine(argc, argv);
		Boot();
		/*returnCode =*/ GameMain();
	}
	catch (Pomme::QuitRequest&)
	{
		// no-op, the game may throw this exception to shut us down cleanly
	}
#if !(_DEBUG)
	// In release builds, catch anything that might be thrown by GameMain
	// so we can show an error dialog to the user.
	catch (std::exception& ex)		// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = ex.what();
		showFinalErrorMessage = true;
	}
	catch (...)						// Last-resort catch
	{
		returnCode = 1;
		finalErrorMessage = "unknown";
		showFinalErrorMessage = true;
	}
#endif

	Shutdown();

	if (showFinalErrorMessage)
	{
		std::cerr << "Uncaught exception: " << finalErrorMessage << "\n";
		SDL_ShowSimpleMessageBox(0, "Cro-Mag Rally", finalErrorMessage.c_str(), nullptr);
	}

	return returnCode;
}
