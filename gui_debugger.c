#include "cimgui.h"




void gui_debugger(ImVec2 pos)
{
// 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    static float f = 0.0f;
    static char str0[7] = "0x0000";
    static char str1[5] = "0x00";

    //Modification de l'apparence des fenêtres
    //ImGuiStyle* style = igGetStyle();
    //style->Colors[ImGuiCol_TitleBgActive] = (ImVec4){0.2f, 0.5f, 1.0f, 1.0f}; // bleu clair
    //style->WindowBorderSize = 2.0f; 
    //style->Colors[ImGuiCol_Border] = color_inactive;
    //style->Colors[ImGuiCol_WindowBg] = (ImVec4){0.1f, 0.1f, 0.1f, 1.0f}; // gris foncé

    //Caractéristiques de la fenêtre
    ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
    //igPushStyleColorImVec4(ImGuiCol_WindowBg, (ImVec4){0.1f, 0.1f, 0.1f, 1.0f});
    igSetNextWindowPos(pos, ImGuiCond_Once);
    igBegin("Debugger", NULL, flags); //Création de la fenêtre

    //igSameLineEx(0.0f, -1.0f);
    igAlignTextToFramePadding();
    igText("X:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igText("[X]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0); igSameLine();
    igText("  Y:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igText("[Y]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igText("PC:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igText("[PC]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);

    igAlignTextToFramePadding();
    igText("U:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igText("[U]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igAlignTextToFramePadding();
    igText("  S:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igText("[S]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igAlignTextToFramePadding();
    igText("DP:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);
    igAlignTextToFramePadding();
    igText("A:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str1, IM_ARRAYSIZE(str1),0);igSameLine();
    igText("  B:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str1, IM_ARRAYSIZE(str1),0);igSameLine();
    igText("[D]:"); igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    igText(" CC:"); igSameLine();
    igTextDisabled("NZVC");igSameLine();
    igTextColored(color_active, "IHFE");
    igSeparator();
    igAlignTextToFramePadding();
    igSetNextItemWidth(60.0f); 
    igButton("Next") ; igSameLine();
    igSetNextItemWidth(60.0f); 
    igButton("Step"); igSameLine();
    if (igIsItemClicked())
    {
        // Action à effectuer lorsque le bouton est cliqué
        sim_run();
        refresh = TRUE;
    }
    igSetNextItemWidth(60.0f); 
    igButton("Continue");igSameLine();
    igSetNextItemWidth(120.0f);
    igButton("Break");igSameLine();
    igNewLine();

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