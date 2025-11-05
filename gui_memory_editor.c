#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "cimgui.h"
#include "sokol_imgui.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "gui_memory_editor.h"
#include "monitor.h"
#include "machine.h"
#include "bus_access.h"
#include "m6809.h"
#include "simulator.h" 

char str0[20] = "0000";
char str1[20] = "0100";
uint16_t start_address = 0x0000;
uint16_t length = 0x0100;

void gui_memory_editor(ImVec2 pos)
{
    igSetNextWindowPos(pos, ImGuiCond_Once);
    char str[20];
    char str2[20];
    ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
    ImGuiTableFlags table_flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;

    igBegin("Memory Editor", NULL, flags); //Création de la fenêtre
    igBeginTableEx("table1", 18, table_flags, (ImVec2){0.0f, 200.0f}, 0.0f);
    ///igTableSetupColumn("AAA", ImGuiTableColumnFlags_WidthFixed);
    //igTableSetupColumn("Instruction", ImGuiTableColumnFlags_WidthStretch);
    for (int row = 0; row < length/16; row++) {
        igTableNextRowEx(ImGuiTableRowFlags_None, 0.0f);
        str2[0] = '\0';
        for (int column = 0; column < 18; column++) {
            igTableSetColumnIndex(column);
            if(column == 0)
            {
                uint16_t addr = start_address + row * 16;
                sprintf(str, "%04X:", addr);
                igText("%s", str);
            }
            else if(column < 17)
            {
                uint16_t addr = start_address + row * 16 + column - 1;
                uint8_t val = bus_read8(addr);
                sprintf(str, "%02X##%d-%d", val, row, column);
                str2[column -1] = (val >= 32 && val <= 126) ? (char)val : '.';
                str2[column -1 +1] = '\0';
                ImGuiPopupFlags popup_flags = ImGuiPopupFlags_None;
                if(igSelectable(str))
                {
                    igOpenPopup(str, popup_flags);
                }
                if (igBeginPopup(str, 0)) 
                {
                    if (igSelectable("Breakpoint on R")) 
                    {
                        printf("Setting read breakpoint at %04X\n", to_absolute(addr));
                    }
                    if (igSelectable("Breakpoint on W"))
                    {
                        printf("Setting write breakpoint at %04X\n", to_absolute(addr));
                    }
                    if (igSelectable("Breakpoint on RW"))
                    {
                        printf("Setting RW breakpoint at %04X\n", to_absolute(addr));
                    }
                    if (igSelectable("Edit Value"))
                    {
                       printf("Editing value at %04X\n", to_absolute(addr));
                    }
                    igEndPopup();
                }
            }
            else if (column == 17)
            {
                igText("%s", str2);
            }
        }
    }
    igEndTable();
    igSeparator();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("Start", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    start_address = (int)strtol(str0, NULL, 16);
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("Length", str1, IM_ARRAYSIZE(str1),0);
    length = (int)strtol(str1, NULL, 16);
    igEnd();
}