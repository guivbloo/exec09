#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "cimgui.h"
#include "sokol_imgui.h"

#include <string.h>
#include "gui_monitor.h"
#include "monitor.h"
#include "machine.h"
#include "bus_access.h"
#include "m6809.h"
#include "simulator.h"



typedef struct {
    uint64_t last_time;
    bool show_test_window;
    bool show_another_window;
    sg_pass_action pass_action;
} state_t;
static state_t state;



void gui_read_hook (absolute_address_t addr)
{
    return;
}
void gui_write_hook (absolute_address_t addr, uint8_t val)
{
    return;
}
void gui_insn_hook (void)
{
   target_addr_t pc;
   pc = m6809_get_pc ();
   command_trace_insn (pc);
}

void SetVSCodeTheme(void)
{
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    // --- Palette principale VS Code ---
    ImVec4 blue        = {0.0f, 0.478f, 0.8f, 1.0f};   // #007ACC
    ImVec4 blue_hover  = {0.0f, 0.58f, 1.0f, 1.0f};    // #0094FF
    ImVec4 bg_dark     = {0.105f, 0.105f, 0.105f, 1.0f}; // #1B1B1B
    ImVec4 bg_light    = {0.16f, 0.16f, 0.16f, 1.0f};    // #292929
    ImVec4 text_color  = {0.86f, 0.86f, 0.86f, 1.0f};    // #DBDBDB
    ImVec4 border_col  = {0.20f, 0.20f, 0.20f, 1.0f};    // #333333

    // --- Arrière-plans ---
    colors[ImGuiCol_WindowBg]          = bg_dark;
    colors[ImGuiCol_ChildBg]           = bg_dark;
    colors[ImGuiCol_PopupBg]           = bg_light;

    // --- Textes ---
    colors[ImGuiCol_Text]              = text_color;
    colors[ImGuiCol_TextDisabled]      = (ImVec4){0.5f, 0.5f, 0.5f, 1.0f};

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
            .width = 1280,
            .height = 1024,
            .window_title = "EXEC09",
            .ios_keyboard_resizes_canvas = false,
            .icon.sokol_default = true,
            .enable_clipboard = true,
            .logger.func = slog_func,
        });
}