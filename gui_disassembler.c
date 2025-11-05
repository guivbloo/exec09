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
#include "gui_disassembler.h"
#include "monitor.h"
#include "machine.h"
#include "bus_access.h"
#include "m6809.h"
#include "simulator.h" 

#define MAX_INSTR_DISPLAY 16

uint16_t dis_start_address = 0xFF00;
uint16_t dis_length = 0x0020;

void gui_disassembler()
{
    char str[20];
    char str0[20] = "FF00";
    char str1[20] = "0020";
        ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
    ImGuiTableFlags table_flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;
   igBegin("Disassembler", NULL, flags); //Création de la fenêtre
    igBeginTableEx("table1", 3, table_flags, (ImVec2){0.0f, 200.0f}, 0.0f);
    igTableSetupColumn("AAA", ImGuiTableColumnFlags_WidthFixed);
    igTableSetupColumn("Instruction", ImGuiTableColumnFlags_WidthStretch);
    //int size = 0;
    for (int size = 0; size < dis_length;) 
    {
        igTableNextRowEx(ImGuiTableRowFlags_None, 0.0f);
        for (int column = 0; column < 2; column++) 
        {
            igTableSetColumnIndex(column);
            if(column == 0)
                igSmallButton("B");
            else
            {
                char buff[64];  
                size += monitor_display_insn (to_absolute(dis_start_address+size), buff);
                igText("%s", buff);
            }
        }
    }
    igEndTable();
    igSeparator();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("Start", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    dis_start_address = (int)strtol(str0, NULL, 16);
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("Length", str1, IM_ARRAYSIZE(str1),0);
    dis_length = (int)strtol(str1, NULL, 16);
    igEnd();
}