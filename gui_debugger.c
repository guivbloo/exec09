#include "cimgui.h"
#include "gui_debugger.h"
#include "monitor.h"
#include "m6809.h"
#include "machine.h"
#include "simulator.h"
#include "bus_access.h"
#include "types.h"

#define MAX_HISTORY_DISPLAY 9

BOOLEAN run = false;

extern unsigned int trace_offset;
extern target_addr_t trace_buffer[MAX_TRACE];

void gui_debugger(ImVec2 pos)
{
    static float f = 0.0f;
    UINT8 reg_cc;
    char reg_x_str[7];
    char reg_x_ptr_str[7];
    char reg_y_str[7];
    char reg_y_ptr_str[7];
    char reg_pc_str[7];
    char reg_pc_ptr_str[7];    
    char reg_u_str[7];
    char reg_u_ptr_str[7];
    char reg_s_str[7];
    char reg_s_ptr_str[7];   
    char reg_dp_str[7];
    char reg_d_ptr_str[7]; 
    char reg_a_str[5];
    char reg_b_str[5];
    char flags_reg[9] = "        \0";

    ImVec4 color_active = {0.2f, 0.6f, 1.0f, 1.0f};  // bleu clair quand active
    ImVec4 color_inactive = {0.0f, 0.0f, 0.0f, 1.0f}; // noir quand inactive
    //Caractéristiques de la fenêtre
    ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
    if(run == true)
    {
        sim_run();
    }
    igSetNextWindowPos(pos, ImGuiCond_Once);
    igBegin("Debugger", NULL, flags); //Création de la fenêtre
    igAlignTextToFramePadding();
    igText("X:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_x_str, sizeof(reg_x_str), "0x%04X", m6809_get_x());
    igInputText("##_X", reg_x_str, IM_ARRAYSIZE(reg_x_str),ImGuiInputTextFlags_ReadOnly );igSameLine();
    igText("[X]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_x_ptr_str, sizeof(reg_x_ptr_str), "0x%04X", bus_read16(m6809_get_x()));
    igInputText("##_[X]", reg_x_ptr_str, IM_ARRAYSIZE(reg_x_ptr_str),ImGuiInputTextFlags_ReadOnly); igSameLine();
    igText("  Y:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_y_str, sizeof(reg_y_str), "0x%04X", m6809_get_y());
    igInputText("##_Y", reg_y_str, IM_ARRAYSIZE(reg_y_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igText("[Y]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_y_ptr_str, sizeof(reg_y_ptr_str), "0x%04X", bus_read16(m6809_get_y()));
    igInputText("##_[Y]", reg_y_ptr_str, IM_ARRAYSIZE(reg_y_ptr_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igText("PC:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_pc_str, sizeof(reg_pc_str), "0x%04X", m6809_get_pc());
    igInputText("##_PC", reg_pc_str, IM_ARRAYSIZE(reg_pc_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igText("[PC]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_pc_ptr_str, sizeof(reg_pc_ptr_str), "0x%04X", bus_read16(m6809_get_pc()));
    igInputText("##_[PC]", reg_pc_ptr_str, IM_ARRAYSIZE(reg_pc_ptr_str),ImGuiInputTextFlags_ReadOnly);
    igAlignTextToFramePadding();
    igText("U:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_u_str, sizeof(reg_u_str), "0x%04X", m6809_get_u());
    igInputText("##_U", reg_u_str, IM_ARRAYSIZE(reg_u_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igText("[U]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_u_ptr_str, sizeof(reg_u_ptr_str), "0x%04X", bus_read16(m6809_get_u()));
    igInputText("##_[U]", reg_u_ptr_str, IM_ARRAYSIZE(reg_u_ptr_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igAlignTextToFramePadding();
    igText("  S:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_s_str, sizeof(reg_s_str), "0x%04X", m6809_get_s());
    igInputText("##_S", reg_s_str, IM_ARRAYSIZE(reg_s_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igText("[S]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_s_ptr_str, sizeof(reg_s_ptr_str), "0x%04X", bus_read16(m6809_get_s()));
    igInputText("##_[S]", reg_s_ptr_str, IM_ARRAYSIZE(reg_s_ptr_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igAlignTextToFramePadding();
    igText("DP:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_dp_str, sizeof(reg_dp_str), "0x%04X", m6809_get_dp());
    igInputText("##_DP", reg_dp_str, IM_ARRAYSIZE(reg_dp_str),ImGuiInputTextFlags_ReadOnly);
    igAlignTextToFramePadding();
    igText("A:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_a_str, sizeof(reg_a_str), "0x%02X", m6809_get_a());
    igInputText("##_A", reg_a_str, IM_ARRAYSIZE(reg_a_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igText("  B:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_b_str, sizeof(reg_b_str), "0x%02X", m6809_get_b());
    igInputText("##_B", reg_b_str, IM_ARRAYSIZE(reg_b_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igText("[D]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    snprintf(reg_d_ptr_str, sizeof(reg_d_ptr_str), "0x%04X", bus_read16(m6809_get_d()));
    igInputText("##_[D]", reg_d_ptr_str, IM_ARRAYSIZE(reg_d_ptr_str),ImGuiInputTextFlags_ReadOnly);igSameLine();
    igText(" CC:"); igSameLine();
    if (m6809_get_cc() & C_FLAG) flags_reg[0] = 'C';
    if (m6809_get_cc() & V_FLAG) flags_reg[1] = 'V';
    if (m6809_get_cc() & Z_FLAG) flags_reg[2] = 'Z';
    if (m6809_get_cc() & N_FLAG) flags_reg[3] = 'N';
    if (m6809_get_cc() & I_FLAG) flags_reg[4] = 'I';
    if (m6809_get_cc() & H_FLAG) flags_reg[5] = 'H';
    if (m6809_get_cc() & F_FLAG) flags_reg[6] = 'F';
    if (m6809_get_cc() & E_FLAG) flags_reg[7] = 'E';
    igTextColored(color_active, flags_reg);
    igSeparator();
    igAlignTextToFramePadding();
    igSetNextItemWidth(60.0f); 
    igButton("Next") ; igSameLine();
    igSetNextItemWidth(60.0f); 
    igButton("Step"); igSameLine();
    if (igIsItemClicked())
    {
        sim_run();
    }
    igSetNextItemWidth(60.0f); 
    if(igButton("Continue"))
    {
        run = true;
    }
    igSameLine();
    igSetNextItemWidth(120.0f);
    if (igButton("Break"))
    {
        run = false;
    }
    igSameLine();
    igSetNextItemWidth(120.0f);
    if(igButton("Reset"))
    {
        run = false;
        machine_reset();
    }
    //igNewLine();

    //Tableau des instructions récentes
    ImGuiTableFlags table_flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;
    igBeginTableEx("table1", 2, table_flags, (ImVec2){0.0f, 200.0f}, 0.0f);
    igTableSetupColumn("AAA", ImGuiTableColumnFlags_WidthFixed);
    igTableSetupColumn("Instruction", ImGuiTableColumnFlags_WidthStretch);
    unsigned int off = (trace_offset + 1 - MAX_HISTORY_DISPLAY) % MAX_TRACE;
    for (int row = 0; row < MAX_HISTORY_DISPLAY + 10; row++) {
        igTableNextRowEx(ImGuiTableRowFlags_None, 0.0f);
        for (int column = 0; column < 2; column++) {
            igTableSetColumnIndex(column);
            if(column == 0)
                igSmallButton("B");
            else 
            {
                if (row < MAX_HISTORY_DISPLAY - 1)
                {
                    char buf[256];
                    target_addr_t pc = trace_buffer[off];
                    absolute_address_t addr = to_absolute (pc);
                    monitor_display_insn (addr, buf);
                    igTextDisabled("%s", buf); 
                    off = (off + 1) % MAX_TRACE;
                }
                else if (row == MAX_HISTORY_DISPLAY - 1)
                {
                    char buf[256];
                    monitor_display_pc_content(buf);
                    igTextColored(color_active, "%s", buf);
                }
                else
                {
                    absolute_address_t ad = to_absolute(m6809_get_pc());
                    char buf[64];
                    char buff[128];
                    int size = 0;
                    for(int i=0;i<row - (MAX_HISTORY_DISPLAY -1); i++)
                    {
                        size += dasm(buf, ad + size);
                    }   
                    monitor_display_insn (ad+size, buff);
                    igText("%s", buff);
                }   
            }
        }
    }
    igEndTable();
    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
    igEnd();
}