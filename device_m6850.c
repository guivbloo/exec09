#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "types.h"
#include "device.h"
#include "device_m6850.h"
#include "cimgui.h"
#include "tinyfiledialogs.h"
#include "gui_colors.h"



#define BUFFER_SIZE 1

struct m6850_port
{
  uint8_t ctrl; /* Control Register */
  uint8_t status; /* Status Register */
  uint8_t RDR; /* Receive Data Register */
  uint8_t TDR; /* Transmit Data Register */
};

uint8_t buffer_put[BUFFER_SIZE];
uint8_t buffer_get[BUFFER_SIZE];
uint8_t index_put = 0;
uint8_t index_get = 0;

// Communication log
#define MAX_LOG_LINES 1024

struct log_entry {
    bool is_tx;
    uint8_t data;
};
static struct log_entry comm_log[MAX_LOG_LINES];
static int comm_log_count = 0;

typedef enum 
{
  AutoSendStatus_None,
  AutoSendStatus_Requested,
  AutoSendStatus_Initialized,
  AutoSendStatus_Done,
} AutoSendStatus_;

// Auto-envoi firmware
bool auto_send_enabled = false;
char trigger_char[5] = "0x01";
uint8_t trigger_byte;
char firmware_path[256] = "";
AutoSendStatus_ auto_send_status = AutoSendStatus_None;
static FILE *auto_send_fp = NULL;
int auto_send_pos = 0;
long file_size = 0;



#define TRANSMIT_DATA_REGISTER 1    
#define RECEIVE_DATA_REGISTER 1
#define CONTROL_REGISTER 0
#define STATUS_REGISTER 0

#define SER_CTL_RESET   0x03   /* CR1:0=11 - Reset device */

#define SER_STAT_READOK  0x1
#define SER_STAT_WRITEOK 0x2
#define SER_STAT_TXEMPTY 0x2


void m6850_update (struct hw_device *dev)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  if(((port->status & 0x01) == 0x00) && (index_put > 0))
  {
    /* A byte is pending to be received */
    port->status |= 0x01; /* RDRF = 1 */
    port->RDR = buffer_put[index_put-1];
    if(port->ctrl & 0x80 == 0x80) /* Receive Interrupt is enabled */
      port->status |= 0x80; /* IRQ  bit set*/
    index_put--;
  }
  if(((port->status & 0x02) == 0x00) && (index_get < BUFFER_SIZE))
  {
    /* A byte is pending to be sent */
    buffer_get[index_get] = port->TDR;
    index_get++;
    port->status |= 0x02; /* TDRE = 1 */
    if(((port->ctrl & 0x10) == 0x10) && ((port->ctrl | 0x20) == 0x20)) /* Transmit Interrupt is enabled */
      port->status |= 0x80; /* IRQ  bit set*/
  }
}

uint8_t m6850_read (struct hw_device *dev, unsigned long addr)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  switch (addr)
  {
    case STATUS_REGISTER:
    {
      return port->status;
    }
    case RECEIVE_DATA_REGISTER:
    {
      port->status &=~(0x01); /* Clear RDRF */
      if(port->ctrl & 0x80 == 0x80) /* Receive Interrupt is enabled */
        port->status &=~(0x80); /* Clear IRQ */
      return port->RDR;
    }
      
  }
}

void m6850_master_reset(struct m6850_port *port)
{
  port->status = 0x02;
  port->RDR = 0;
  port->TDR = 0;
}

void m6850_write (struct hw_device *dev, unsigned long addr, uint8_t val)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  switch (addr) 
  {
    case CONTROL_REGISTER:
    {
      port->ctrl = val;
      if (val & 0x03 == 0x03)
        m6850_master_reset(port);
      break;
    }
    case TRANSMIT_DATA_REGISTER:
    {
      port->TDR = val;
      port->status &=~(0x02); /* Clear TDRE */
      if(((port->ctrl & 0x10) == 0x10) && ((port->ctrl | 0x20) == 0x20)) /* Transmit Interrupt is enabled */
        port->status &=~(0x80); /* Clear IRQ */
      break;
    }
  }
}

void m6850_reset (struct hw_device *dev)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  port->ctrl = 0;
  m6850_master_reset(port);

}

void m6850_dump (struct hw_device *dev)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  printf("(dbg) -- M6850 registers --\n");
  printf("(dbg) CR: 0x%02X  SR: 0x%02X\n", port->ctrl, port->status);
  printf("(dbg) RDR: 0x%02X  TDR: 0x%02X\n", port->RDR, port->TDR);
}

uint8_t m6850_irq_pending(struct hw_device *dev)
{
    struct m6850_port *port = (struct m6850_port *)dev->priv;
    return port->status & 0x80;
}


void m6850_display(struct hw_device *dev, ImVec2 pos)
{
  struct m6850_port *port = (struct m6850_port *)dev->priv;
  char reg_tdr[5];
  char reg_rdr[5];
  char reg_status[5];
  char reg_ctrl[5];

  if(m6850_kbhit() > 0)
  {
    uint8_t byte = m6850_getchar();
    if (comm_log_count < MAX_LOG_LINES) 
    {
      struct log_entry *e = &comm_log[comm_log_count];
      e->is_tx = false;
      e->data = byte;
      comm_log_count++;
      if(byte == trigger_byte && auto_send_status == AutoSendStatus_Requested)
      {
        auto_send_fp = fopen(firmware_path, "rb");
        fseek(auto_send_fp, 0, SEEK_END);         // Va à la fin du fichier
        file_size = ftell(auto_send_fp);
        fseek(auto_send_fp, 0, SEEK_SET);
        auto_send_status = AutoSendStatus_Initialized;
        auto_send_pos = 0;
      }
    }
  } 
  if(auto_send_status == AutoSendStatus_Initialized)
  {
    if(m6850_putready() == 0)
    {
      int byte = fgetc(auto_send_fp);
      if (byte == EOF) 
      {
          fclose(auto_send_fp);
          auto_send_fp = NULL;
          auto_send_status = AutoSendStatus_Done;
      }
      else
      {
        m6850_putchar(byte & 0xFF);
        if (comm_log_count < MAX_LOG_LINES) 
        {
          struct log_entry *e = &comm_log[comm_log_count];
          e->is_tx = true;
          e->data = byte & 0xFF;
          comm_log_count++;
        } 
      }
      auto_send_pos++;
    }
  }


    //Caractéristiques de la fenêtre
    ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
  igSetNextWindowPos(pos, ImGuiCond_Once);
    if (igBegin("ACIA 6850", NULL, flags))
    {
        igText("Registers");
        igAlignTextToFramePadding();
        igText("TDR:"); igSameLine();
        igSetNextItemWidth(40.0f); // largeur en pixels du prochain élément
        snprintf(reg_tdr, sizeof(reg_tdr), "0x%02X", port->TDR);
        igInputText("##_TDR", reg_tdr, IM_ARRAYSIZE(reg_tdr),ImGuiInputTextFlags_ReadOnly); igSameLine();
        igAlignTextToFramePadding();
        igText("RDR:"); igSameLine();
        igSetNextItemWidth(40.0f); // largeur en pixels du prochain élément
        snprintf(reg_rdr, sizeof(reg_rdr), "0x%02X", port->RDR);
        igInputText("##_RDR", reg_rdr, IM_ARRAYSIZE(reg_rdr),ImGuiInputTextFlags_ReadOnly); igSameLine();
        if(port->RDR > 32 && port->RDR < 127)
            igText("ASCII: ('%c')", port->RDR);
        else
            igText("ASCII: .");
        igAlignTextToFramePadding();
        igText("CR:"); igSameLine();
        igSetNextItemWidth(40.0f); // largeur en pixels du prochain élément
        snprintf(reg_ctrl, sizeof(reg_ctrl), "0x%02X", port->ctrl);
        igInputText("##_CR", reg_ctrl, IM_ARRAYSIZE(reg_ctrl),ImGuiInputTextFlags_ReadOnly); igSameLine();
        igAlignTextToFramePadding();
        igText("SR:"); igSameLine( );
        igSetNextItemWidth(40.0f); // largeur en pixels du prochain élément
        snprintf(reg_status, sizeof(reg_status), "0x%02X", port->status);
        igInputText("##_SR", reg_status, IM_ARRAYSIZE(reg_status),ImGuiInputTextFlags_ReadOnly);
        igSeparator();
        igText("Control Register Bits");

/* Noms détaillés (pour tooltips) */
const char *bit_desc[8] = {
    "Clock Divide bit 0 (CR1:0)",
    "Clock Divide bit 1 (CR1:0)",
    "Word Select bit 0 (CR3:2)",
    "Word Select bit 1 (CR3:2)",
    "Receive Interrupt Enable",
    "Transmit Interrupt Enable",
    "RTS Control",
    "IRQ Enable"
};

/* Libellés abrégés sous les bits */
const char *bit_labels[8] = {" D0 "," D1 "," W2 "," W3 ","RXE ","TXE ","RTS ","IRQ "};

/* --- Tableau aligné --- */
if (igBeginTable("ctrl_bits_table", 8, ImGuiTableFlags_SizingFixedFit))
{
    /* Ligne 2 : boutons colorés (non cliquables) */
    igTableNextRow();
    igBeginDisabled(true);
    for (int bit = 7; bit >= 0; bit--) {
        igTableSetColumnIndex(7 - bit);
        bool set = (port->ctrl >> bit) & 1;
        ImVec4 col = set ? blue_hover : border_col;
        igPushStyleColorImVec4(ImGuiCol_Button, col);
        igPushStyleColorImVec4(ImGuiCol_ButtonHovered, col);
        igPushStyleColorImVec4(ImGuiCol_ButtonActive, col);
        char label[8];
        snprintf(label, sizeof(label), "%d##CR", bit);
        igButtonEx(label, (ImVec2){22, 22});
        igPopStyleColor();
        igPopStyleColor();
        igPopStyleColor();

        if (igIsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            igSetTooltip("%s", bit_desc[bit]);
        }
    }
    igEndDisabled();

    /* Ligne 3 : noms abrégés */
    igTableNextRow();
    for (int bit = 7; bit >= 0; bit--) {
        igTableSetColumnIndex(7 - bit);
        igText("%s", bit_labels[bit]);
    }

    igEndTable();
}

        igSeparator();
igText("Status Register Bits");



/* Descriptions détaillées selon la doc du 6850 */
const char *status_desc[8] = {
    "Receive Data Register Full (RDRF)",
    "Transmit Data Register Empty (TDRE)",
    "Data Carrier Detect (DCD)",
    "Clear To Send (CTS)",
    "Framing Error (FE)",
    "Overrun Error (OVRN)",
    "Parity Error (PE)",
    "Interrupt Request (IRQ)"
};

/* Libellés abrégés */
const char *status_labels[8] = {"RDRF","TDRE","DCD ","CTS "," FE ","OVRN"," PE ","IRQ "};

/* --- Tableau aligné pour Status --- */
if (igBeginTable("status_bits_table", 8, ImGuiTableFlags_SizingFixedFit))
{
    /* Ligne 2 : boutons colorés (non cliquables) */
    igTableNextRow();
    igBeginDisabled(true);
    for (int bit = 7; bit >= 0; bit--) {
        igTableSetColumnIndex(7 - bit);
        bool set = (port->status >> bit) & 1;
        ImVec4 col = set ? blue_hover : border_col;
        igPushStyleColorImVec4(ImGuiCol_Button, col);
        igPushStyleColorImVec4(ImGuiCol_ButtonHovered, col);
        igPushStyleColorImVec4(ImGuiCol_ButtonActive, col);
        char label[8];
        snprintf(label, sizeof(label), "%d##SR", bit);
        igButtonEx(label, (ImVec2){22, 22});
        igPopStyleColor();
        igPopStyleColor();
        igPopStyleColor();

        if (igIsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            igSetTooltip("%s", status_desc[bit]);
        }
    }
    igEndDisabled();

    /* Ligne 3 : noms abrégés */
    igTableNextRow();
    for (int bit = 7; bit >= 0; bit--) {
        igTableSetColumnIndex(7 - bit);
        igText("%s", status_labels[bit]);
    }

    igEndTable();
}
        igSeparator();
        igText("Communication log");

       
       

        // --- Historique des communications ---
       if (igBeginChild("comm_log", (ImVec2){0, 200}, true, ImGuiWindowFlags_HorizontalScrollbar))
{
    for (int i = 0; i < comm_log_count; i++) {
        struct log_entry *e = &comm_log[i];
        ImVec4 color = e->is_tx ? text_green : blue;

        igTextColored(color, "[%s]", e->is_tx ? "TX" : "RX");
        igSameLine();
        igText("%02X", e->data); igSameLine();
        char ascii_buf[9];
        if (e->data >= 32 && e->data <= 126) {
            snprintf(ascii_buf, sizeof(ascii_buf), "('%c')", e->data);
        } else {
            snprintf(ascii_buf, sizeof(ascii_buf), "(.)");
        } 
        igText("%s", ascii_buf);
    }

    if (igGetScrollY() >= igGetScrollMaxY() - 10.0f)
        igSetScrollHereY(1.0f);
}
igEndChild();
        if (igButton("Clear Log"))
        {
            comm_log_count = 0;
        }
        static char tx_buf[5] = "0x00";
        igAlignTextToFramePadding();
        igText("Send Byte:"); igSameLine();
        igSetNextItemWidth(40.0f); // largeur en pixels du prochain élément
        igInputText("##Send Byte", tx_buf, sizeof(tx_buf), ImGuiInputTextFlags_CharsHexadecimal);
        igSameLine();
        if (igButton("Send")) {
            int val = (int)strtol(tx_buf, NULL, 16);
            if(m6850_putready() == 0)
            {
              m6850_putchar(val & 0xFF);
              if (comm_log_count < MAX_LOG_LINES) 
              {
                struct log_entry *e = &comm_log[comm_log_count];
                e->is_tx = true;
                e->data = val & 0xFF;
                comm_log_count++;
              } 
            }
        }
        igSeparator();

        // --- Auto-send Firmware ---
        igText("Firmware Auto-Send");
        if(auto_send_status == AutoSendStatus_Done)
        {
          auto_send_enabled = false;
          firmware_path[0] = '\0';
          auto_send_status = AutoSendStatus_None;
        }
        igCheckbox("Enable Auto-Send", &auto_send_enabled);
        if(!auto_send_enabled) {
            igBeginDisabled(1); 
            auto_send_status == AutoSendStatus_None;
        }
        else
        {
          if(auto_send_status == AutoSendStatus_None)
            {
              auto_send_pos = 0;
              file_size = 0;
            }
        }
        igAlignTextToFramePadding();
        igText("Trigger byte:"); igSameLine();
        igSetNextItemWidth(40.0f); // largeur en pixels du prochain élément
        igInputText("##Trigger Char", trigger_char, sizeof(trigger_char), ImGuiInputTextFlags_None);
        sscanf(trigger_char, "0x%2X", &trigger_byte);
        if (igButton("Browse"))
        {
          const char *path = tinyfd_openFileDialog("Select .bin file", "", 0, NULL, NULL, 0);
          if (path)
          {
            strncpy(firmware_path, path, sizeof(firmware_path));
            firmware_path[sizeof(firmware_path) - 1] = '\0'; // Ensure null-termination
          }
        }
        if (auto_send_enabled && firmware_path != NULL && auto_send_status == AutoSendStatus_None)
        {
            auto_send_status = AutoSendStatus_Requested;
        }
        igSameLine();
        igInputText("##Firmware Path", firmware_path, sizeof(firmware_path), ImGuiInputTextFlags_ReadOnly);
        igText("Progress:");
        igSameLine();
        igPushStyleColorImVec4(ImGuiCol_PlotHistogram, blue);
        igProgressBar((float)auto_send_pos / (float)file_size, (ImVec2){0,0}, "");
        igPopStyleColor();
        if(!auto_send_enabled) {
            igEndDisabled(); 
          }
        igSeparator();


    }
    igEnd();

}


struct hw_class m6850_class =
  {
    .name = "m6850",
    .readonly = 0,
    .reset = m6850_reset,
    .read = m6850_read,
    .write = m6850_write,
    .update = m6850_update,
    .dump = m6850_dump,
    .check_interrupt = m6850_irq_pending,
  };


struct hw_device* m6850_create (unsigned long size)
{
  struct m6850_port *port = malloc (sizeof (struct m6850_port));
  return device_create (&m6850_class, size, port);
}

/* User functions */

uint8_t m6850_getchar()
{
  uint8_t val = 0xFF;
  if(index_get > 0)
  {
    val = buffer_get[index_get-1];
    index_get--;
  }
  return val;
}

void m6850_putchar(uint8_t val)
{
  if(index_put < BUFFER_SIZE)
  {
    buffer_put[index_put] = val;
    index_put++;
  }
}

uint8_t m6850_kbhit()
{
  return index_get;
}

/* Return 0 if there is space left */
uint8_t m6850_putready()
{
  if(index_put > BUFFER_SIZE-1)
    return 1;
  else
    return 0;
}
