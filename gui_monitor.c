#include "../dcimgui/src/cimgui.h"
#include "gui_monitor.h"


void gui_monitor_init()
{
    // Initialisation code for the GUI monitor
    igCreateContext(NULL);
    ImGuiIO* io = igGetIO();
    io->DisplaySize.x = 1920;
    io->DisplaySize.y = 1080;
    igStyleColorsDark(NULL);
    // Additional setup can be done here
}

void gui_monitor_run()
{
    // Main loop for the GUI monitor
    while (1) {
        igNewFrame();

        // GUI rendering code goes here
        igBegin("Monitor", NULL, 0);
        igText("Welcome to the GUI Monitor!");
        igEnd();

        igRender();
        // Platform-specific rendering code goes here
    }
}