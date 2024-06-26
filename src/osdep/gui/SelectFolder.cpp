#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <guisan.hpp>
#include <guisan/sdl.hpp>
#include "SelectorEntry.hpp"

#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "gui_handling.h"

#include "options.h"
#include "inputdevice.h"
#include "amiberry_gfx.h"
#include "amiberry_input.h"

enum
{
	DIALOG_WIDTH = 520,
	DIALOG_HEIGHT = 600
};

std::string volName;
static bool dialogResult = false;
static bool dialogFinished = false;
static std::string workingDir;

static gcn::Window* wndSelectFolder;
static gcn::Button* cmdOK;
static gcn::Button* cmdCancel;
static gcn::ListBox* lstFolders;
static gcn::ScrollArea* scrAreaFolders;
static gcn::TextField* txtCurrent;


class FolderRequesterButtonActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		if (actionEvent.getSource() == cmdOK)
		{
			dialogResult = true;
		}
		dialogFinished = true;
	}
};
static FolderRequesterButtonActionListener* folderButtonActionListener;

class SelectDirListModel : public gcn::ListModel
{
	std::vector<std::string> dirs;

public:
	SelectDirListModel(const std::string& path)
	{
		changeDir(path);
	}

	int getNumberOfElements() override
	{
		return static_cast<int>(dirs.size());
	}

	void add(const std::string& elem) override
	{
		dirs.push_back(elem);
	}

	void clear() override
	{
		dirs.clear();
	}
	
	std::string getElementAt(int i) override
	{
		if (i < 0 || i >= getNumberOfElements())
			return "---";
		return dirs[i];
	}

	void changeDir(const std::string& path)
	{
		read_directory(path, &dirs, nullptr);
		if (std::find(dirs.begin(), dirs.end(), "..") == dirs.end())
			dirs.insert(dirs.begin(), "..");
	}
};

static SelectDirListModel dirList(".");

static void checkfoldername(const std::string& current)
{
	char actualpath [MAX_DPATH];
	DIR* dir;

	if ((dir = opendir(current.c_str())))
	{
		dirList = current;
		auto* const ptr = realpath(current.c_str(), actualpath);
		workingDir = std::string(ptr);
		closedir(dir);
	}
	else
	{
		workingDir = start_path_data;
		dirList = workingDir;
	}
	txtCurrent->setText(workingDir);
}

class ListBoxActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		std::string foldername = "";

		const auto selected_item = lstFolders->getSelected();
		foldername = workingDir.append("/").append(dirList.getElementAt(selected_item));

		volName = dirList.getElementAt(selected_item);
		checkfoldername(foldername);
	}
};

static ListBoxActionListener* listBoxActionListener;

class EditDirPathActionListener : public gcn::ActionListener
{
public:
	void action(const gcn::ActionEvent& actionEvent) override
	{
		checkfoldername(txtCurrent->getText());
	}
};
static EditDirPathActionListener* editDirPathActionListener;

static void InitSelectFolder(const std::string& title)
{
	wndSelectFolder = new gcn::Window("Load");
	wndSelectFolder->setSize(DIALOG_WIDTH, DIALOG_HEIGHT);
	wndSelectFolder->setPosition((GUI_WIDTH - DIALOG_WIDTH) / 2, (GUI_HEIGHT - DIALOG_HEIGHT) / 2);
	wndSelectFolder->setBaseColor(gui_baseCol);
	wndSelectFolder->setCaption(title);
	wndSelectFolder->setTitleBarHeight(TITLEBAR_HEIGHT);

	folderButtonActionListener = new FolderRequesterButtonActionListener();

	cmdOK = new gcn::Button("Ok");
	cmdOK->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdOK->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - 2 * BUTTON_WIDTH - DISTANCE_NEXT_X,
					   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdOK->setBaseColor(gui_baseCol);
	cmdOK->addActionListener(folderButtonActionListener);

	cmdCancel = new gcn::Button("Cancel");
	cmdCancel->setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
	cmdCancel->setPosition(DIALOG_WIDTH - DISTANCE_BORDER - BUTTON_WIDTH,
						   DIALOG_HEIGHT - 2 * DISTANCE_BORDER - BUTTON_HEIGHT - 10);
	cmdCancel->setBaseColor(gui_baseCol);
	cmdCancel->addActionListener(folderButtonActionListener);

	txtCurrent = new gcn::TextField();
	txtCurrent->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, TEXTFIELD_HEIGHT);
	txtCurrent->setPosition(DISTANCE_BORDER, 10);
	txtCurrent->setBaseColor(gui_baseCol);
	txtCurrent->setBackgroundColor(colTextboxBackground);
	txtCurrent->setEnabled(true);
	editDirPathActionListener = new EditDirPathActionListener();
	txtCurrent->addActionListener(editDirPathActionListener);

	listBoxActionListener = new ListBoxActionListener();

	lstFolders = new gcn::ListBox(&dirList);
	lstFolders->setSize(DIALOG_WIDTH - 45, DIALOG_HEIGHT - 108);
	lstFolders->setBaseColor(gui_baseCol);
	lstFolders->setBackgroundColor(colTextboxBackground);
	lstFolders->setSelectionColor(gui_selection_color);
	lstFolders->setWrappingEnabled(true);
	lstFolders->addActionListener(listBoxActionListener);

	scrAreaFolders = new gcn::ScrollArea(lstFolders);
	scrAreaFolders->setBorderSize(1);
	scrAreaFolders->setPosition(DISTANCE_BORDER, 10 + TEXTFIELD_HEIGHT + 10);
	scrAreaFolders->setSize(DIALOG_WIDTH - 2 * DISTANCE_BORDER - 4, DIALOG_HEIGHT - 128);
	scrAreaFolders->setScrollbarWidth(SCROLLBAR_WIDTH);
	scrAreaFolders->setBaseColor(gui_baseCol);
	scrAreaFolders->setBackgroundColor(colTextboxBackground);
	scrAreaFolders->setSelectionColor(gui_selection_color);
	scrAreaFolders->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_AUTO);
	scrAreaFolders->setVerticalScrollPolicy(gcn::ScrollArea::SHOW_ALWAYS);

	wndSelectFolder->add(cmdOK);
	wndSelectFolder->add(cmdCancel);
	wndSelectFolder->add(txtCurrent);
	wndSelectFolder->add(scrAreaFolders);

	gui_top->add(wndSelectFolder);

	wndSelectFolder->requestModalFocus();
	focus_bug_workaround(wndSelectFolder);
	lstFolders->requestFocus();
	lstFolders->setSelected(0);
}

static void ExitSelectFolder()
{
	wndSelectFolder->releaseModalFocus();
	gui_top->remove(wndSelectFolder);

	delete cmdOK;
	delete cmdCancel;
	delete folderButtonActionListener;

	delete txtCurrent;
	delete lstFolders;
	delete scrAreaFolders;
	delete listBoxActionListener;

	delete wndSelectFolder;
}

static void navigate_right()
{
	const auto* const focusHdl = gui_top->_getFocusHandler();
	const auto* const activeWidget = focusHdl->getFocused();
	if (activeWidget == lstFolders)
		cmdOK->requestFocus();
	else if (activeWidget == cmdCancel)
		lstFolders->requestFocus();
	else if (activeWidget == cmdOK)
		cmdCancel->requestFocus();
}

static void navigate_left()
{
	const auto* const focusHdl = gui_top->_getFocusHandler();
	const auto* const activeWidget = focusHdl->getFocused();
	if (activeWidget == lstFolders)
		cmdCancel->requestFocus();
	else if (activeWidget == cmdCancel)
		cmdOK->requestFocus();
	else if (activeWidget == cmdOK)
		lstFolders->requestFocus();
}

static void SelectFolderLoop()
{
	const AmigaMonitor* mon = &AMonitors[0];

	bool got_event = false;
	SDL_Event event;
	SDL_Event touch_event;
	bool nav_left, nav_right;
	while (SDL_PollEvent(&event))
	{
		nav_left = nav_right = false;
		switch (event.type)
		{
		case SDL_KEYDOWN:
			got_event = handle_keydown(event, dialogFinished, nav_left, nav_right);
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYHATMOTION:
			if (gui_joystick)
			{
				got_event = handle_joybutton(&di_joystick[0], dialogFinished, nav_left, nav_right);
			}
			break;

		case SDL_JOYAXISMOTION:
			if (gui_joystick)
			{
				got_event = handle_joyaxis(event, nav_left, nav_right);
			}
			break;

		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			got_event = handle_finger(event, touch_event);
			gui_input->pushInput(touch_event);
			break;

		case SDL_MOUSEWHEEL:
			got_event = handle_mousewheel(event);
			break;

		default:
			got_event = true;
			break;
		}

		if (nav_left)
			navigate_left();
		else if (nav_right)
			navigate_right();

		//-------------------------------------------------
		// Send event to guisan-controls
		//-------------------------------------------------
		gui_input->pushInput(event);
	}

	if (got_event)
	{
		// Now we let the Gui object perform its logic.
		uae_gui->logic();
		SDL_RenderClear(mon->sdl_renderer);
		// Now we let the Gui object draw itself.
		uae_gui->draw();
		// Finally we update the screen.
		update_gui_screen();
	}
}

std::string SelectFolder(const std::string& title, std::string value)
{
	const AmigaMonitor* mon = &AMonitors[0];

	dialogResult = false;
	dialogFinished = false;

	InitSelectFolder(title);
	checkfoldername(value);

	// Prepare the screen once
	uae_gui->logic();

	SDL_RenderClear(mon->sdl_renderer);

	uae_gui->draw();
	update_gui_screen();

	while (!dialogFinished)
	{
		const auto start = SDL_GetPerformanceCounter();
		SelectFolderLoop();
		cap_fps(start);
	}

	ExitSelectFolder();
	if (dialogResult)
		value = workingDir;
	else
		value = "";

	return value;
}
