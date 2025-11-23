#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "cimgui.h"
#include "sokol_imgui.h"
#include "m6809.h"
#include <string.h>
#include <stdlib.h>
#include "gui_monitor.h"
#include "monitor.h"
#include "machine.h"
#include "bus_access.h"
#include "simulator.h"
#include "types.h"
#include "gui_colors.h"
#include "device_tms9918.h"

#define MAX_HISTORY_DISPLAY 9
#define MAX_NEXT_INST_DISPLAY 20






BOOLEAN run = false;


char str0[20] = "0000";
char str1[20] = "0100";
uint16_t start_address = 0x0000;
uint16_t length = 0x0100;

extern unsigned int trace_offset;
extern target_addr_t trace_buffer[MAX_TRACE];

extern lst_line_t lst_lines[MAX_LST_LINES];
extern int lst_line_count;

typedef struct {
    bool active;
    absolute_address_t addr;
} watchpoint_state_t;
static watchpoint_state_t watchpoint_state;

typedef struct {
    uint64_t last_time;
    bool show_test_window;
    bool show_another_window;
    sg_pass_action pass_action;
} state_t;
static state_t state;

void gui_debugger(ImVec2 pos);
void gui_memory_editor(ImVec2 pos);
void gui_specific_features(ImVec2 pos);
void gui_cpu_display(ImVec2 pos);

void gui_read_hook (absolute_address_t addr)
{   
    breakpoint_t *br = brkfind_by_addr (addr);
    if (br && br->enabled && br->on_read)
    {      
        printf("state true\n");
        watchpoint_state.active = true;
        watchpoint_state.addr = addr;
        if(monitor_breakpoint_hit (br) == true)
        {
            run = false;
        } 
    }
    else
    {
        watchpoint_state.active = false;   
    }
}

void gui_write_hook (absolute_address_t addr, uint8_t val)
{
    return;
}

BOOLEAN gui_insn_hook (void)
{
   target_addr_t pc;
   pc = m6809_get_pc ();
    absolute_address_t abspc = to_absolute (pc);
    breakpoint_t *br = brkfind_by_addr (abspc);
    if(run == true)
    {
        if (br && br->enabled && br->on_execute)
        {
            if (br->keep_running == 0)
            {
                run = false;
                brk_enable(br, 0);
                return TRUE;
            }
        }
        else if(br && br->on_execute)
        {
                brk_enable(br, 1);
        }
    }
    command_trace_insn (pc);
    return FALSE;
}

void SetVSCodeTheme(void)
{
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;



    // --- Arrière-plans ---
    colors[ImGuiCol_WindowBg]          = bg_dark;
    colors[ImGuiCol_ChildBg]           = bg_dark;
    colors[ImGuiCol_PopupBg]           = bg_light;

    // --- Textes ---
    colors[ImGuiCol_Text]              = text_color;
    colors[ImGuiCol_TextDisabled]      = textdisabled_color;

    // --- Barres de titre ---
    colors[ImGuiCol_TitleBg]           = bg_dark;
    colors[ImGuiCol_TitleBgActive]     = blue;
    colors[ImGuiCol_TitleBgCollapsed]  = bg_dark;

    // --- Bordures ---
    colors[ImGuiCol_Border]            = border_col;
    colors[ImGuiCol_BorderShadow]      = (ImVec4){0.0f, 0.0f, 0.0f, 0.0f};

    // --- Widgets (boutons, cases, etc.) ---
    colors[ImGuiCol_Button]            = bg_light;
    colors[ImGuiCol_ButtonHovered]     = blue_hover;
    colors[ImGuiCol_ButtonActive]      = blue;
    colors[ImGuiCol_FrameBg]           = bg_light;
    colors[ImGuiCol_FrameBgHovered]    = blue_hover;
    colors[ImGuiCol_FrameBgActive]     = blue;

    // --- Scrollbar & sliders ---
    colors[ImGuiCol_ScrollbarBg]       = bg_dark;
    colors[ImGuiCol_ScrollbarGrab]     = (ImVec4){0.35f, 0.35f, 0.35f, 1.0f};
    colors[ImGuiCol_ScrollbarGrabHovered] = blue_hover;
    colors[ImGuiCol_ScrollbarGrabActive]  = blue;

    colors[ImGuiCol_SliderGrab]        = blue;
    colors[ImGuiCol_SliderGrabActive]  = blue_hover;

    // --- Onglets ---
    colors[ImGuiCol_Tab]               = bg_light;
    colors[ImGuiCol_TabHovered]        = blue_hover;
    colors[ImGuiCol_TabActive]         = blue;
    colors[ImGuiCol_TabUnfocused]      = bg_dark;
    colors[ImGuiCol_TabUnfocusedActive]= bg_light;

    // --- Séparateurs / menus ---
    colors[ImGuiCol_Separator]         = border_col;
    colors[ImGuiCol_SeparatorHovered]  = blue_hover;
    colors[ImGuiCol_SeparatorActive]   = blue;
    colors[ImGuiCol_MenuBarBg]         = bg_light;

    // --- Navigation & focus ---
    colors[ImGuiCol_NavHighlight]      = blue;
    colors[ImGuiCol_NavWindowingHighlight] = blue_hover;
    colors[ImGuiCol_NavWindowingDimBg] = (ImVec4){0.2f, 0.2f, 0.2f, 0.4f};

    // --- Autres ---
    colors[ImGuiCol_CheckMark]         = blue;
    colors[ImGuiCol_Header]            = bg_light;
    colors[ImGuiCol_HeaderHovered]     = blue_hover;
    colors[ImGuiCol_HeaderActive]      = blue;
    colors[ImGuiCol_ResizeGrip]        = bg_light;
    colors[ImGuiCol_ResizeGripHovered] = blue_hover;
    colors[ImGuiCol_ResizeGripActive]  = blue;

    // --- Style ---
    style->FrameRounding = 3.0f;
    style->WindowRounding = 5.0f;
    style->GrabRounding = 3.0f;
    style->TabRounding = 3.0f;
    style->FrameBorderSize = 1.0f;
    style->WindowBorderSize = 1.0f;
    style->ScrollbarSize = 14.0f;
}


static void init(void) {
    // setup sokol-gfx, sokol-time and sokol-imgui
    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    // use sokol-imgui with all default-options (we're not doing
    // multi-sampled rendering or using non-default pixel formats)
    simgui_setup(&(simgui_desc_t){
        .logger.func = slog_func,
    });

    /* initialize application state */
    state = (state_t) {
        .show_test_window = true,
        .pass_action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = { 0.9019f, 0.8862f, 0.8549f, 1.0f }
            }
        }
    };
    tms9918_display_init();
    SetVSCodeTheme();
}

static void frame(void) {
    const int width = sapp_width();
    const int height = sapp_height();


    simgui_new_frame(&(simgui_frame_desc_t){
        .width = width,
        .height = height,
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale()
    });

    if (igBeginMainMenuBar()) {
              if (igBeginMenu("File")) {
                   if (igMenuItem("Create")) { 
                   }
                   if (igMenuItem("Open")) { 
                   }
                   if (igMenuItem("Save")) {
                   }
                   if (igMenuItem("Save as..")) { 
                    }
             igEndMenu();
             }
             igEndMainMenuBar();
        }

    gui_debugger((ImVec2){ 1.0f, 20.0f });
    gui_memory_editor((ImVec2){ 1.0f, 261.0f });
    gui_specific_features((ImVec2){ 1.0f, 522.0f });
    machine_display();
    // the sokol_gfx draw pass
    sg_begin_pass(&(sg_pass){ .action = state.pass_action, .swapchain = sglue_swapchain() });
    simgui_render();
    sg_end_pass();
    sg_commit();
}

static void cleanup(void) {
    simgui_shutdown();
    sg_shutdown();
}

static void input(const sapp_event* event) {
    simgui_handle_event(event);
}

void gui_monitor_init()
{
    monitor_init();
    bus_read_hook = gui_read_hook;
    bus_write_hook = gui_write_hook;
    m6809_insn_hook = gui_insn_hook;
}

void gui_monitor_run()
{
    sapp_run(&(sapp_desc) {
            .init_cb = init,
            .frame_cb = frame,
            .cleanup_cb = cleanup,
            .event_cb = input,
            .width = 1368,
            .height = 1200,
            .window_title = "EXEC09",
            .ios_keyboard_resizes_canvas = false,
            .icon.sokol_default = true,
            .enable_clipboard = true,
            .logger.func = slog_func,
        });
}

void gui_debugger(ImVec2 pos)
{
    static float f = 0.0f;
    UINT8 reg_cc;
    static target_addr_t last_pc = 0;
    static bool scroll_to_pc = true;
    /* Couleurs */
    ImVec4 on_color  = {0.0f, 0.58f, 1.0f, 1.0f}; 
    ImVec4 off_color = {0.25f, 0.25f, 0.25f, 1.0f};
    ImVec4 color_active = {0.2f, 0.6f, 1.0f, 1.0f};  // bleu clair quand active
    ImVec4 color_inactive = {0.0f, 0.0f, 0.0f, 1.0f}; // noir quand inactive
    //Caractéristiques de la fenêtre
    ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
    if(run == true)
    {
        uint16_t i = 0;
        do
        {
            i += sim_run();
        } while (run == true && i < 32000);  
    }
    igSetNextWindowPos(pos, ImGuiCond_Once);
    igSetNextWindowSize((ImVec2){540.0f, 240.0f}, ImGuiCond_Once);
    igBegin("Debugger", NULL, flags); //Création de la fenêtre
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

    //Tableau des instructions récentes
    ImGuiTableFlags table_flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;
    igBeginTableEx("table1", 4, table_flags, (ImVec2){0.0f, 180.0f}, 0.0f);
    igTableSetupColumn("But", ImGuiTableColumnFlags_WidthFixed);
    igTableSetupColumn("Adr", ImGuiTableColumnFlags_WidthFixed);
    igTableSetupColumn("Ins", ImGuiTableColumnFlags_WidthFixed);
    igTableSetupColumn("Asm", ImGuiTableColumnFlags_WidthStretch);
    for (int row = 0; row < lst_line_count; row++) {
        igTableNextRowEx(ImGuiTableRowFlags_None, 0.0f);
        char buf[256];
        char button_name[16];
        int set;
        target_addr_t pc = lst_lines[row].addr;
        target_addr_t current_pc = m6809_get_pc();
        if (current_pc != last_pc) {
            scroll_to_pc = true;
            last_pc = current_pc;
        }
        igTableSetColumnIndex(0);
        breakpoint_t* bk = brkfind_by_addr (to_absolute(pc));
        if(bk && bk->used == 1)
            set = 1;
        else    
            set = 0;
        ImVec4 col = set ? on_color : off_color;
        igPushStyleColorImVec4(ImGuiCol_Button, col);
        igPushStyleColorImVec4(ImGuiCol_ButtonHovered, col);
        igPushStyleColorImVec4(ImGuiCol_ButtonActive, col);
        snprintf(button_name, sizeof(button_name), "B##_%d", row);
        if(lst_lines[row].hex[0] != '\0')
        {
            igSmallButton(button_name);
            if (igIsItemClicked())
            {
                if(set == 1)
                {
                    brkfree (bk);
                }
                else
                {
                    monitor_breakpoint_add(pc);
                }
            }
        }   
        igPopStyleColor();
        igPopStyleColor();
        igPopStyleColor();
        igTableSetColumnIndex(1);
        if(pc == current_pc && lst_lines[row].hex[0] != '\0')
        {
            igTextColored(color_active, "%04X", lst_lines[row].addr);
        }
        else if(lst_lines[row].hex[0] != '\0')
        {
            igText("%04X", lst_lines[row].addr);
        }
        igTableSetColumnIndex(2);
        if(pc == current_pc && lst_lines[row].hex[0] != '\0')
        {
            igTextColored(color_active, "%s", lst_lines[row].hex);
            if(scroll_to_pc) {
                igSetScrollHereY(0.5f);
                scroll_to_pc = false;
            }
        }
        else
        {
            igText("%s", lst_lines[row].hex);
        }
        igTableSetColumnIndex(3);
        if(pc == current_pc && lst_lines[row].hex[0] != '\0')
        {
            igTextColored(color_active, "%s", lst_lines[row].source);
        }
        else
        {
            igText("%s", lst_lines[row].source);
        }
    }
    igEndTable();
//    igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
    igEnd();
}

void gui_memory_editor(ImVec2 pos)
{
    igSetNextWindowPos(pos, ImGuiCond_Once);
    igSetNextWindowSize((ImVec2){540.0f, 260.0f}, ImGuiCond_Once);
    char str[20];
    char str2[20];
    char str3[20];
    char str4[20];
    ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
    ImGuiTableFlags table_flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;

    igBegin("Memory Viewer", NULL, flags); //Création de la fenêtre
    igBeginTableEx("table1", 18, table_flags, (ImVec2){526.0f, 190.0f}, 0.0f);
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
                uint8_t val = bus_read8_abs(to_absolute(addr));
                sprintf(str, "%02X##%d-%d", val, row, column);
                str2[column -1] = (val >= 32 && val <= 126) ? (char)val : '.';
                str2[column -1 +1] = '\0';
                ImGuiPopupFlags popup_flags = ImGuiPopupFlags_None;
                breakpoint_t* bk = brkfind_by_addr (to_absolute(addr));
                if(bk && bk->used == 1 && bk->on_execute == 0)
                {
                    if(watchpoint_state.active == true && bk->addr == watchpoint_state.addr)
                    {
                        igPushStyleColorImVec4(ImGuiCol_HeaderHovered, text_orange);

                    }
                    igSelectableEx(str, 1, ImGuiSelectableFlags_Highlight, (ImVec2){0.0f, 0.0f});
                    if(watchpoint_state.active == true && bk->addr == watchpoint_state.addr)
                    {
                        igPopStyleColor();    
                    }
                }
                else  
                {  
                    igSelectable(str);
                    //igSetNextItemWidth(20.0f);
                    //igInputText(str4, str3, IM_ARRAYSIZE(str3), ImGuiInputTextFlags_ReadOnly);
                }
                if(igIsItemClicked())
                {
                    igOpenPopup(str, popup_flags);
                }
                if (igBeginPopup(str, 0)) 
                {
                    if(bk && bk->used == 1 && bk->on_execute == 0)
                    {
                        if (igSelectable("Delete Watchpoint"))
                        {
                            brkfree (bk);
                        }
                        if (igSelectable("Edit Value"))
                        {
                            printf("Editing value at %04X\n", to_absolute(addr));
                        }
                    }
                    else
                    {
                        if (igSelectable("Watchpoint on R")) 
                        {
                            monitor_watchpoint_add(addr, 1, 0);
                            printf("Setting read breakpoint at %04X\n", to_absolute(addr));
                        }
                        if (igSelectable("Watchpoint on W"))
                        {
                            monitor_watchpoint_add(addr, 0, 1);
                            printf("Setting write breakpoint at %04X\n", to_absolute(addr));
                        }
                        if (igSelectable("Watchpoint on RW"))
                        {
                            monitor_watchpoint_add(addr, 1, 1);

                            printf("Setting RW breakpoint at %04X\n", to_absolute(addr));
                        }
                        if (igSelectable("Edit Value"))
                        {
                        printf("Editing value at %04X\n", to_absolute(addr));
                        }
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
    igAlignTextToFramePadding();
    igText("Start:");igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##Start", str0, IM_ARRAYSIZE(str0),0);igSameLine();
    start_address = (int)strtol(str0, NULL, 16);
    igText("Length:");igSameLine();
    igSetNextItemWidth(60.0f); // largeur en pixels du prochain élément
    igInputText("##Length", str1, IM_ARRAYSIZE(str1),0);
    length = (int)strtol(str1, NULL, 16);
    igEnd();
}

void gui_specific_features(ImVec2 pos)
{
    igSetNextWindowPos(pos, ImGuiCond_Once);
    igSetNextWindowSize((ImVec2){540.0f, 182.0f}, ImGuiCond_Once);
    ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
    char str[20];
    igBegin("Specific features", NULL, flags); //Création de la fenêtre
    //Tab breakpoints
    igBeginTabBar(" ", ImGuiTabBarFlags_None);
    if (igBeginTabItem("Breakpoint", NULL, ImGuiTabItemFlags_None))
    {
        //igText("Breakpoint(s) used: %d / %d", monitor_breakpoint_used(), MAX_BREAKS);
        ImGuiTableFlags table_flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;
        igBeginTableEx("table1", 5, table_flags, (ImVec2){270.0f, 122.0f}, 0.0f);
        igTableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed);
        igTableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
        igTableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed);
        igTableSetupColumn("Mode", ImGuiTableColumnFlags_WidthFixed);
        igTableSetupColumn(" ", 0);
        igTableHeadersRow();
        for(int i=0;i<MAX_BREAKS;i++)
        {
            breakpoint_t *bk = brkfind_by_id(i);
            if(bk->used == 1)
            {
                igTableNextRow();
                igTableSetColumnIndex(0);
                if(bk->enabled == 1)
                    igText("Active");
                else
                    igText("Inactive");
                igTableSetColumnIndex(1);
                if(bk->on_execute == 1)
                    igText("BP");
                else
                    igText("WP");
                igTableSetColumnIndex(2);
                igText("0x%04lX", bk->addr_target);
                igTableSetColumnIndex(3);
                if(bk->on_execute == 1)
                    igText("--");
                else if(bk->on_read == 1 && bk->on_write == 1)
                    igText("RW");
                else if(bk->on_read == 1 && bk->on_write == 0)
                    igText("R-");            
                else if(bk->on_read == 0 && bk->on_write == 1)
                    igText("-W");
                igTableSetColumnIndex(4);
                sprintf(str, "Delete##%d", i);
                igSmallButton(str);
                if(igIsItemClicked())
                {
                    brkfree(bk);
                }
            }
        }
        igEndTable();
        igEndTabItem();
    }
    if (igBeginTabItem("Trace", NULL, ImGuiTabItemFlags_None))
    {
        extern unsigned int trace_offset;
        extern target_addr_t trace_buffer[MAX_TRACE];
        char buf[256];
        ImGuiTableFlags table_flags = ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersOuterV;
        igBeginTableEx("trace_table", 1, table_flags, (ImVec2){520.0f, 122.0f}, 0.0f);
        igTableSetupColumn("Asm", ImGuiTableColumnFlags_WidthStretch);
        // Affiche les instructions dans l'ordre d'exécution (plus anciennes en haut)
        for (unsigned int i = 0; i < MAX_TRACE; i++) {
            unsigned int idx = (trace_offset + i) % MAX_TRACE;
            target_addr_t addr = trace_buffer[idx];
            if (addr == 0) continue; // Ignore les cases vides

            monitor_display_insn (addr, buf);
            igTableNextRowEx(ImGuiTableRowFlags_None, 0.0f);
            igTableSetColumnIndex(0);
            igText("%s", buf); 
            igSetScrollHereY(0.5f);
        }
        igEndTable();
        igEndTabItem();
    }
    if( igBeginTabItem("Stack", NULL, ImGuiTabItemFlags_None))
    {
        igText("Not implemented yet.");
        igEndTabItem();
    }
    if( igBeginTabItem("Memory mapping", NULL, ImGuiTabItemFlags_None))
    {
        igText("Not implemented yet.");
        igEndTabItem();
    }
    if( igBeginTabItem("Variables", NULL, ImGuiTabItemFlags_None))
    {
        igText("Not implemented yet.");
        igEndTabItem();
    }
    if( igBeginTabItem("Measure", NULL, ImGuiTabItemFlags_None))
    {
        igText("Not implemented yet.");
        igEndTabItem();
    }
    igEndTabBar();
    igEnd();
}


