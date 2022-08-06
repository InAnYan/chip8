/*
chip8-emulator v1.0 - CHIP-8 emulator.
Copyright(C) 2021, 2022 Ruslan Popov <ruslanpopov1512@gmail.com>

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "common.h"

#define SDL_MAIN_HANDLED

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_sdlrenderer.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "nativefiledialog/nfd.h" // WARNING: Only Windows support files are included
#include "IconFontCppHeaders/IconsFontAwesome4.h"

#include "log/logger.hpp"
#include "chip8/CHIP8.hpp"

#define START_ROM "chip8start.ch8"
#define TEXT_CMP1(cmd, txt1) (!strcmp((cmd), #txt1))
#define TEXT_CMP2(cmd, txt1, txt2) (!strcmp((cmd), #txt1) || !strcmp((cmd), #txt2))
#define TEXT_CMP3(cmd, txt1, txt2, txt3) (!strcmp((cmd), #txt1) || !strcmp((cmd), #txt2) || !strcmp((cmd), #txt3))
#define MS_PER_FRAME 10

static inline ImVec2 operator+(ImVec2 lhs, ImVec2 rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

using namespace std;

// Main vars
ofstream logFile;
SDL_Window* window;
SDL_Renderer* renderer;
ImTextureID icons;

// Window properties vars
string currentPath = START_ROM;
string windowName  = "CHIP-8 emulator: " + currentPath;
int windowWidth = 1024;
int windowHeight = 666;
float rectWidth;
float rectHeight;
ImGuiWindowFlags mainWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
			 | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
bool isShowAboutWindow = false;

// Assembler window
ImVector<char*> assemblerItems;

// Console window
ImVector<char*> consoleItems;
char inputBuffer[32];
bool scrollToBottom = false;
bool autoScroll = true;
bool reclaim_focus = false;

void clearConsole();
void executeCommand(const char* cmd);

// Drawing vars
ImDrawList* draw_list;
ImVec2 p1, p2; // Rectangle drawing
ImVec2 mainRect1, mainRect2; // Display rect
ImVec2 windowMax, windowMin; // Content region
bool filledStyle = true;
bool colorsInverted = false;

// Drawing functions
void drawMenu();
void drawDisplay();
void drawCodeWindow();
void drawCommandsWindow();
void drawStatusWindow();

// Other
inline void TextCentered(const char* text);
inline void TextRighted(const char* text);
void addTextToLog(string const& text); // Used as callback for CHIP-8 class

// Internal
CHIP8 chip8(currentPath);
int currentBreakPoint = 0;

int cyclesPerFrame = 5;
bool running, halted, step;

bool debugMode = false;
bool p_open = true;
bool doResize = true;

int firstResizeCnt = 0; // It's really needed

long msOnFrame;
chrono::time_point<chrono::steady_clock> frameBegin, frameEnd;

int  init(int argc, char** argv);
void events();
void reload(string newPath);
void showAboutWindow(bool* p_open);
void resize();
void quit();

int main(int argc, char** argv)
{
	int errCode;
	if ((errCode = init(argc, argv)) != 0) return errCode;
	
	running = true;
	p_open = true;
	halted = debugMode;
	step = false;
	logger::info("Getting into app's loop!");
	while (running)
	{
		frameBegin = std::chrono::high_resolution_clock::now();

		if(halted || cyclesPerFrame == 0 || chip8.delayTimer > 0) events();

		if (chip8.delayTimer == 0)
		{
			if (!halted) for (int i = 0; i < cyclesPerFrame; i++)
			{
				events();
				chip8.emulateCycle();
				if (chip8.getPC() == currentBreakPoint)
				{
					logger::info("At breakpoint");
					consoleItems.push_back(_strdup("At breakpoint"));
					halted = true;
				}
				chip8.lastKey = -1;
			}
			if (chip8.caughtEndlessLoop()) halted = true;
			if (step)
			{
				chip8.emulateCycle();
				if (chip8.getPC() == currentBreakPoint)
				{
					logger::info("At breakpoint");
					consoleItems.push_back(_strdup("At breakpoint"));
				}
				step = false;
			}
		}
		else 
		{
			chip8.delayTimer--;
		}

		if (chip8.soundTimer > 0)
		{
			// TODO: play sound
			chip8.soundTimer--;
		}

		ImGui_ImplSDLRenderer_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		if (isShowAboutWindow) showAboutWindow(&isShowAboutWindow);
		
		drawMenu();

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::Begin("MainWindow", &p_open, mainWindowFlags);
		
		drawDisplay();
		ImGui::SameLine();
		drawCodeWindow();

		ImGui::Spacing();

		drawCommandsWindow();
		ImGui::SameLine();
		drawStatusWindow();

		ImGui::End(); // General end

		ImGui::Render();
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
		SDL_RenderPresent(renderer);

		frameEnd = std::chrono::high_resolution_clock::now();
		msOnFrame = (long) std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameBegin).count();
		if (MS_PER_FRAME - msOnFrame > 0)
		{
			SDL_Delay(MS_PER_FRAME - msOnFrame);
		}
	}

	// Quittting
	quit();
}

int init(int argc, char** argv)
{
	vector<string> args;
	for (int i = 1; i < argc; i++)
	{
		args.push_back(string(argv[i]));
	}

	if (find(args.begin(), args.end(), "-h") != args.end() || find(args.begin(), args.end(), "--help") != args.end())
	{
		cout << "Usage: " << endl
			<< "  -h [ --help ]                         shows this message" << endl
			<< "  -d [ --debug ]                        debug mode on" << endl
			<< "  -p [ --path ] file (=chip8start.ch8)  path to ROM" << endl;
		exit(0);
	}
	if (find(args.begin(), args.end(), "-d") != args.end() || find(args.begin(), args.end(), "--debug") != args.end()) debugMode = true;
	
	auto it1 = find(args.begin(), args.end(), "-p");
	auto it2 = find(args.begin(), args.end(), "--path");
	if (it1 != args.end()) 
	{
		currentPath = *(it1 + 1);
	} 
	else if (it2 != args.end())
	{
		currentPath = *(it2 + 1);
	}

	logFile.open(logger::generateName("logs/chip8emu-gui"), ios::out);
	logger::addSink(&cout);
	logger::addSink(&logFile);

	logger::info("Starting emulator...");
	logger::warning("This is not completed version. Use it on your own risk");
	logger::debug("It must have a GUI!");
	if (debugMode) logger::debug("Debug mode on");
	logger::info("Author - Ruslan Popov");
	logger::info("Email - ruslanpopov1512@gmail.com");

	logger::debug("Initializing ImGui context...");
	ImGui::CreateContext();

	logger::debug("Initializing SDL...");
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		logger::error("Unable to init SDL: " + string(SDL_GetError()));
		return 1;
	}

	logger::debug("Initializing SDL_image...");
	if (IMG_Init(IMG_INIT_PNG) == 0)
	{
		logger::error("Unable to init SDL_image: " + string(IMG_GetError()));
		return 2;
	}

	logger::debug("Creating window...");
	window = SDL_CreateWindow(windowName.c_str(), SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (window == NULL)
	{
		logger::error("Can`t create window");
		return 3;
	}

	logger::debug("Creating renderer...");
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL)
	{
		logger::error("Can`t create renderer");
		return 4;
	}

	logger::debug("Initializing ImGui...");
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);

	logger::debug("Loading icons...");
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
	ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
	io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FA, 13.0f, &icons_config, icons_ranges);

	logger::debug("Loading CHIP-8...");
	chip8.logCallback = addTextToLog;
	if (currentPath != START_ROM)
	{
		chip8.reload(currentPath);
	}

	return 0;
}

void events()
{
	SDL_Event event;
	ImGuiIO& io = ImGui::GetIO();
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		if (event.type == SDL_QUIT)
			running = false;
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE
			&& event.window.windowID == SDL_GetWindowID(window))
			running = false;
		if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(window) 
			&& event.window.event == SDL_WINDOWEVENT_RESIZED)
			doResize = true;
		if (event.type == SDL_KEYDOWN)
		{
			switch (event.key.keysym.sym)
			{
			case SDLK_v:
				chip8.setKey(0xf);
				break;
			case SDLK_c:
				chip8.setKey(0xB);
				break;
			case SDLK_x:
				chip8.setKey(0x0);
				break;
			case SDLK_z:
				chip8.setKey(0xa);
				break;
			case SDLK_f:
				chip8.setKey(0xe);
				break;
			case SDLK_d:
				chip8.setKey(0x9);
				break;
			case SDLK_s:
				chip8.setKey(0x8);
				break;
			case SDLK_a:
				chip8.setKey(0x7);
				break;
			case SDLK_r:
				chip8.setKey(0xd);
				break;
			case SDLK_e:
				chip8.setKey(0x6);
				break;
			case SDLK_w:
				chip8.setKey(0x5);
				break;
			case SDLK_q:
				chip8.setKey(0x4);
				break;
			case SDLK_4:
				chip8.setKey(0xc);
				break;
			case SDLK_3:
				chip8.setKey(0x3);
				break;
			case SDLK_2:
				chip8.setKey(0x2);
				break;
			case SDLK_1:
				chip8.setKey(0x1);
				break;

			case SDLK_RETURN:
				if (!io.WantCaptureKeyboard)
					step = true;
				break;
			case SDLK_ESCAPE:
			case SDLK_SPACE:
				if(!io.WantCaptureKeyboard)
					executeCommand("stop");
				break;
			}
		}

		if (event.type == SDL_KEYUP)
		{
			switch (event.key.keysym.sym)
			{
			case SDLK_v:
				chip8.unsetKey(0xf);
				break;
			case SDLK_c:
				chip8.unsetKey(0xB);
				break;
			case SDLK_x:
				chip8.unsetKey(0x0);
				break;
			case SDLK_z:
				chip8.unsetKey(0xa);
				break;
			case SDLK_f:
				chip8.unsetKey(0xe);
				break;
			case SDLK_d:
				chip8.unsetKey(0x9);
				break;
			case SDLK_s:
				chip8.unsetKey(0x8);
				break;
			case SDLK_a:
				chip8.unsetKey(0x7);
				break;
			case SDLK_r:
				chip8.unsetKey(0xd);
				break;
			case SDLK_e:
				chip8.unsetKey(0x6);
				break;
			case SDLK_w:
				chip8.unsetKey(0x5);
				break;
			case SDLK_q:
				chip8.unsetKey(0x4);
				break;
			case SDLK_4:
				chip8.unsetKey(0xc);
				break;
			case SDLK_3:
				chip8.unsetKey(0x3);
				break;
			case SDLK_2:
				chip8.unsetKey(0x2);
				break;
			case SDLK_1:
				chip8.unsetKey(0x1);
				break;
			}
		}
	}
}

void drawMenu()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 6));
	ImGui::BeginMainMenuBar();
	if (ImGui::MenuItem("Open ROM", "CTRL+O"))
	{
		logger::debug("Opening new ROM...");
		nfdchar_t* outPath = NULL;
		nfdresult_t result = NFD_OpenDialog(NULL, NULL, &outPath);
		if (result == NFD_OKAY)
		{
			logger::debug("Reloading...");
			reload(string(outPath));
			delete outPath;
			logger::debug("Reload end");
		}
		else if (result == NFD_CANCEL)
			logger::debug("Canceled");
		else if (result == NFD_ERROR)
			logger::error("Something went wrong with opening new ROM: " + string(NFD_GetError()));
	}

	if (ImGui::MenuItem("Start debug", "CTRL-D"))
	{
		executeCommand("begin");
	}

	if (ImGui::MenuItem("About", "CTRL-A"))
	{
		isShowAboutWindow = true;
	}
	
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
	ImGui::SetCursorPosY(5);
	ImGui::Checkbox("Filled style", &filledStyle);
	ImGui::SetCursorPosY(5);
	ImGui::Checkbox("Invert colors", &colorsInverted);
	
	string cyclesStr = (cyclesPerFrame == 0 ? "  " : "") + to_string(cyclesPerFrame * (1000 / MS_PER_FRAME)) + " cycles/sec";
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(cyclesStr.c_str()).x;
	ImGui::SetCursorPosX(windowWidth - textWidth - 15*2 - 25);
	ImGui::SetCursorPosY(5);
	if (ImGui::Button("+", ImVec2(15, 15))) cyclesPerFrame++;
	ImGui::SameLine();
	ImGui::Text(cyclesStr.c_str());
	ImGui::SameLine();
	ImGui::SetCursorPosY(5);
	if (ImGui::Button("-", ImVec2(15, 15))) (cyclesPerFrame == 0 ? 0 : cyclesPerFrame--);
	ImGui::PopStyleVar();
	ImGui::EndMainMenuBar();
	ImGui::PopStyleVar();
}

void drawDisplay()
{
	ImGui::BeginChild("DisplayChild", ImVec2(ImGui::GetWindowContentRegionMax().x * 0.75f, ImGui::GetWindowContentRegionMax().y * 0.65f), true, ImGuiWindowFlags_MenuBar);

	// Title
	ImGui::BeginMenuBar();
	TextCentered("DISPLAY");
	ImGui::EndMenuBar();

	draw_list = ImGui::GetWindowDrawList();
	if (firstResizeCnt < 2) // Yes, it's needed
	{
		resize();
		firstResizeCnt++;
	}
	if (doResize) resize();

	draw_list->AddRectFilled(mainRect1, mainRect2, (colorsInverted ? WHITE : BLACK));
	draw_list->AddRect(mainRect1, mainRect2, IM_COL32(128, 128, 128, 255));

	// Drawing pixels
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 64; j++) {
			draw_list->AddRectFilled({ mainRect1.x + 1 + j * rectWidth, mainRect1.y + 1 + i * rectHeight },
				{ mainRect1.x + filledStyle + j * rectWidth + rectWidth, mainRect1.y + filledStyle + i * rectHeight + rectHeight },
				chip8.display(j, i) ? (colorsInverted ? BLACK : WHITE) : (colorsInverted ? WHITE : BLACK));
		}
	}

	ImGui::EndChild();
}

void drawCodeWindow()
{
	ImGui::BeginChild("CodeChild", ImVec2(ImGui::GetWindowContentRegionMax().x * 0.25f - 17, ImGui::GetWindowContentRegionMax().y * 0.65f), true, ImGuiWindowFlags_MenuBar);
	ImGui::BeginMenuBar();
	TextCentered("RAM");
	ImGui::EndMenuBar();
	
	ImGui::BeginChild("ScrollingRegionCode");

	int pc = chip8.getPC();
	static int range1 = 0x200 - 10;
	static int range2 = range1 + 42;
	if (pc+2 > range2 || pc - 2 < range1)
	{
		range1 = pc - 16;
		range2 = range1 + 42;
	}
	
	float diffY = ImGui::GetWindowContentRegionMax().y - (42+16+3) * 6;
	ImGui::Dummy(ImVec2(0, diffY/2));

	for (CHIP8::dbyte i = range1; i < range2; i += 2)
	{
		string code = CHIP8::disasmCode(chip8.getFromRam(i) * 0x100 + chip8.getFromRam(i + 1));
		string fstr = type_to_hex(i) + ": ";
		bool hasColorYellow = (i == pc);
		bool hasColorBlue = (code.rfind("jmp", 0) == 0 || code.rfind("call", 0) == 0) && !hasColorYellow;
		bool hasColorRed = (code.rfind("ret", 0) == 0) && !hasColorYellow;
		bool hasColorGreen = (code.rfind("drw", 0) == 0) && !hasColorYellow;
		ImVec2 old = ImGui::GetCursorPos();
		if (code == "ret")
		{
			ImVec2 n(old.x, old.y + 3);
			ImGui::SetCursorPos(n);
			ImGui::TextUnformatted(string(23, '_').c_str());
			ImGui::SetCursorPos(old);
		}
		ImGui::TextUnformatted(fstr.c_str());
		if (hasColorYellow) ImGui::PushStyleColor(ImGuiCol_Text, YELLOW);
		if (hasColorBlue) ImGui::PushStyleColor(ImGuiCol_Text, BLUE);
		if (hasColorRed) ImGui::PushStyleColor(ImGuiCol_Text, RED);
		if (hasColorGreen) ImGui::PushStyleColor(ImGuiCol_Text, GREEN);
		ImGui::SameLine();
		ImGui::TextUnformatted(code.c_str());
		if (hasColorYellow)
		{
			ImGui::SameLine();
			ImVec2 n(old.x, old.y);
			ImGui::SetCursorPos(n);
			ImGui::TextUnformatted("                         <-");
			ImGui::SetCursorPos(old);
			ImGui::NewLine();
		}
		if (hasColorYellow || hasColorBlue || hasColorRed || hasColorGreen) ImGui::PopStyleColor();
	}
	ImGui::EndChild();

	ImGui::EndChild();
}

void drawCommandsWindow()
{
	ImGui::BeginChild("CmdChild", ImVec2(ImGui::GetWindowContentRegionMax().x * 0.75f, ImGui::GetWindowContentRegionMax().y * 0.35f - 15), true, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::BeginMenuBar();
	TextCentered("COMMANDS");
	ImGui::EndMenuBar();

	// Log section
	const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::Selectable("Clear")) clearConsole();
		ImGui::EndPopup();
	}
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
	for (int i = 0; i < consoleItems.Size; i++)
	{
		const char* item = consoleItems[i];

		ImU32 color;
		bool has_color = false;
		if (strstr(item, "ERROR")) { color = RED; has_color = true; }
		else if (strstr(item, ">")) { color = YELLOW; has_color = true; }
		else if (strstr(item, "INFO: ")) { color = BLUE; has_color = true; }
		if (has_color)
			ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(item);
		if (has_color)
			ImGui::PopStyleColor();
	}
	if (scrollToBottom || (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
		ImGui::SetScrollHereY(1.0f);
	scrollToBottom = false;
	ImGui::PopStyleVar();
	ImGui::EndChild();

	// Input bar
	ImGui::Separator();
	scrollToBottom = false;
	autoScroll = true;
	reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue;
	draw_list->AddText(ImGui::GetCursorScreenPos() + ImVec2(0, 3), WHITE, "Input: ");
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(40, 5.5));
	if (ImGui::InputText("##", inputBuffer, IM_ARRAYSIZE(inputBuffer), input_text_flags))
	{
		char* s = inputBuffer;
		if (s[0])
			executeCommand(s);
		strcpy(s, "");
		reclaim_focus = true;
	}

	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1);

	// Control buttons #23446C
	// ImGui::Button("dummy");
	// TODO: Space
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.75,1 });
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 5));
	if (ImGui::Button(ICON_FA_PLAY, { 21,21 }))
	{
		executeCommand("continue");
	}
	ImGui::PopStyleVar();
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5,1 });
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 5));
	if (ImGui::Button(ICON_FA_PAUSE, { 21,21 }))
	{
		executeCommand("stop");
	}
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 5));
	if (ImGui::Button(ICON_FA_STOP, { 21,21 }))
	{
		executeCommand("bnc");
	}
	ImGui::PopStyleVar();
	ImGui::SameLine();
	ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 5));
	if (ImGui::Button("step", { 40, 21 }))
	{
		executeCommand("step");
	}

	ImGui::EndChild();
}

void drawStatusWindow()
{
	ImGui::BeginChild("StatusChild", ImVec2(ImGui::GetWindowContentRegionMax().x * 0.25f - 17, ImGui::GetWindowContentRegionMax().y * 0.35f - 15), true, ImGuiWindowFlags_MenuBar);
	ImGui::BeginMenuBar();
	TextCentered("CPU STATE");
	ImGui::EndMenuBar();
	ImGui::Text(chip8.regInfo().c_str());
	if (halted) ImGui::Text("\n--- STOPPED ---");
	ImGui::EndChild();
}

void reload(string newPath)
{
	currentPath = newPath;
	windowName = "CHIP-8 emulator: " + currentPath;
	SDL_SetWindowTitle(window, windowName.c_str());
	chip8.reload(newPath);
	halted = false;
	clearConsole();
}

void resize()
{
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	mainRect1 = ImGui::GetWindowContentRegionMin();
	mainRect1.x += 4;
	mainRect1.y += 31;
	windowMax = ImGui::GetWindowContentRegionMax();
	windowMin = ImGui::GetWindowContentRegionMin();

	// Main rect drawing. Actually, it's a bad idea, to make emulator window resizable
	if (windowMax.x / 2 > windowMax.y)
	{
		mainRect2.x = windowMax.y * 2 + windowMin.x;
		mainRect2.y = windowMax.y + windowMin.y + 5;
		int diffX = windowMax.x - mainRect2.x; // It should be integer
		mainRect1.x += diffX / 2 + 6;
		mainRect2.x += diffX / 2 + 6;
	}
	else
	{
		// TODO: stretching too much
		mainRect2.x = windowMax.x + windowMin.x + 3;
		mainRect2.y = windowMax.x/2 + windowMin.y;
		int diffY = windowMax.y - mainRect2.y; // It should be integer
		mainRect1.y += diffY / 2 + 16;
		mainRect2.y += diffY / 2 + 16;
	}

	rectWidth = (mainRect2.x - mainRect1.x - 2) / 64;
	rectHeight = (mainRect2.y - mainRect1.y - 2) / 32;

	doResize = false;
}

void clearConsole()
{
	for (int i = 0; i < consoleItems.Size; i++)
		free(consoleItems[i]);
	consoleItems.clear();
}

void executeCommand(const char* cmd)
{
	string userPrompt = "> ";
	userPrompt += cmd;
	consoleItems.push_back(_strdup(userPrompt.c_str()));

	if (TEXT_CMP1(cmd, exit))
	{
		logger::info("Got exit command");
		running = false;
	}
	else if (TEXT_CMP1(cmd, clear))
	{
		logger::info("Got clear command");
		clearConsole();
	}
	else if (TEXT_CMP2(cmd, h, help))
	{
		logger::info("Got help command");
		consoleItems.push_back(_strdup("UNIMPLEMENTED")); // TODO: Implement
	}
	else if (TEXT_CMP3(cmd, r, reg, info))
	{
		logger::info("Got info command");
		consoleItems.push_back(_strdup(chip8.regInfo().c_str()));
	}
	else if (TEXT_CMP2(cmd, c, continue))
	{
		logger::info("Got continue command");
		consoleItems.push_back(_strdup("Continuing..."));
		halted = false;
	}
	else if (TEXT_CMP2(cmd, b, begin))
	{
		logger::info("Got begin command");
		chip8.reload(currentPath);
		consoleItems.push_back(_strdup("At PC = 0x200"));
		halted = true;
	}
	else if (TEXT_CMP1(cmd, bnc))
	{
		logger::info("Got begin and continue command");
		chip8.refresh();
		consoleItems.push_back(_strdup("Continuing from the begining..."));
		halted = false;
	}
	else if (TEXT_CMP2(cmd, s, step))
	{
		logger::info("Got step command");
		step = true;
		chip8.resetEndlessLoop();
		consoleItems.push_back(_strdup("Step"));
	}
	else if (TEXT_CMP1(cmd, stop))
	{
		logger::info("Got stop command");
		if (!halted)
		{
			halted = true;
			consoleItems.push_back(_strdup("Stopped"));
		}
		else
		{
			halted = false;
			consoleItems.push_back(_strdup("Unstopped"));
		}
	}
	else if (!strncmp(cmd, "bp", 2) || !strncmp(cmd, "breakpoint", 10))
	{
		logger::info("Got breakpoint command");
		if (cmd[0] == 'b' && cmd[1] == 'p')
		{
			try
			{
				currentBreakPoint = stoi(string(cmd).substr(2, strlen(cmd)), nullptr, 0);
			}
			catch (invalid_argument const& e)
			{
				logger::error("Got invalid argument");
				consoleItems.push_back(_strdup("ERROR: Got invalid argument"));
				return;
			}
		}
		else 
		{
			try
			{
				currentBreakPoint = stoi(string(cmd).substr(10, strlen(cmd)), nullptr, 0);
			}
			catch (invalid_argument const& e)
			{
				logger::error("Got invalid argument");
				consoleItems.push_back(_strdup("ERROR: Got invalid argument"));
				return;
			}
		}
	}
	else if (!strncmp(cmd, "dump", 4))
	{
		logger::info("Got dump command");

		stringstream res;
		int addr;
		res << "Memory: " << endl;
		try
		{
			addr = stoi(string(cmd).substr(4, strlen(cmd)), nullptr, 0);
		}
		catch (invalid_argument const& e)
		{
			logger::error("Got invalid argument");
			consoleItems.push_back(_strdup("ERROR: Got invalid argument"));
			return;
		}
		for (int i = 0; i < 5; i++)
		{
			res << "[" << hex << setw(4) << i + addr << "]: " << (unsigned) chip8.getFromRam(i + addr) << endl;
		}

		consoleItems.push_back(_strdup(res.str().c_str()));
	}
	else
	{
		logger::info("Got unrecognized command");
		consoleItems.push_back(_strdup("ERROR: Got unrecognized command"));
	}

	scrollToBottom = true;
}

void addTextToLog(string const& text)
{
	consoleItems.push_back(_strdup(text.c_str()));
}

void quit()
{
	logger::info("Quitting...");
	clearConsole();
	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

inline void TextCentered(const char* text) {
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(text).x;

	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::Text(text);
}

inline void TextRighted(const char* text) {
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(text).x;

	ImGui::SetCursorPosX(windowWidth - textWidth);
	ImGui::Text(text);
}

void showAboutWindow(bool* p_open)
{
	ImGui::SetNextWindowSize(ImVec2(600,460));
	ImGui::Begin("About", p_open, ImGuiWindowFlags_NoResize);
	if (ImGui::BeginTabBar("AboutTabBar"))
	{
		if (ImGui::BeginTabItem("\"chip8emu\" license"))
		{
			ImGui::Text("chip8emu v0.7 - CHIP-8 emulator.\n\
Copyright(C) 2021, 2022 Ruslan Popov <ruslanpopov1512@gmail.com>\n\
\n\
The MIT License (MIT)\n\
\n\
Permission is hereby granted, free of charge, to any person obtaining a copy\n\
of this software and associated documentation files (the \"Software\"), to deal\n\
in the Software without restriction, including without limitation the rights\n\
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell\n\
copies of the Software, and to permit persons to whom the Software is\n\
furnished to do so, subject to the following conditions:\n\
\n\
The above copyright notice and this permission notice shall be included in all\n\
copies or substantial portions of the Software.\n\
\n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE\n\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n\
SOFTWARE.\n\
\n\
This software uses such libraries as:\n");
			ImGui::BulletText("\"Dear ImGui\"");
			ImGui::BulletText("\"Native File Dialog\"");
			ImGui::BulletText("\"IconFontCppHeaders\"");
			ImGui::BulletText("\"SDL\" and \"SDL_image\"");
			ImGui::Text("Those license notices provided in tabs and in \"LICENSE.txt\" file.");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("\"Dear ImGui\" license"))
		{
			ImGui::Text("The MIT License (MIT)\n\
\n\
Copyright(c) 2014 - 2022 Omar Cornut\n\
\n\
Permission is hereby granted, free of charge, to any person obtaining a copy\n\
of this software and associated documentation files (the \"Software\"), to deal\n\
in the Software without restriction, including without limitation the rights\n\
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell\n\
copies of the Software, and to permit persons to whom the Software is\n\
furnished to do so, subject to the following conditions:\n\
\n\
The above copyright notice and this permission notice shall be included in all\n\
copies or substantial portions of the Software.\n\
\n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n\
SOFTWARE.\
");
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::End();
}