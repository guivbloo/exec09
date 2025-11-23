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
#include "device_tms9918.h"
#include "cimgui.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_imgui.h"

#define FB_WIDTH 256
#define FB_HEIGHT 192

#define TMS_NUM_REGISTERS 8
#define GRAPHICS_NUM_COLS         32
#define TEXT_NUM_COLS             40
#define TEXT_CHAR_WIDTH            6
#define TEXT_PADDING_PX            8



#define STATUS_INT              0x80
#define STATUS_5S               0x40
#define STATUS_COL              0x20

#define VRAM_SIZE           (1 << 14) /* 16KB */
#define VRAM_MASK     (VRAM_SIZE - 1) /* 0x3fff */

#define TMS_R0_MODE_GRAPHICS_II 0x02
#define TMS_R0_EXT_VDP_ENABLE   0x01

#define TMS_R1_DISP_ACTIVE      0x40
#define TMS_R1_INT_ENABLE       0x20
#define TMS_R1_MODE_MULTICOLOR  0x08
#define TMS_R1_MODE_TEXT        0x10
#define TMS_R1_SPRITE_16        0x02
#define TMS_R1_SPRITE_MAG2      0x01

#define MAX_SPRITES               32

#define SPRITE_ATTR_Y              0
#define SPRITE_ATTR_X              1
#define SPRITE_ATTR_NAME           2
#define SPRITE_ATTR_COLOR          3
#define SPRITE_ATTR_BYTES          4
#define LAST_SPRITE_YPOS        0xD0
#define MAX_SCANLINE_SPRITES       4

#define PATTERN_BYTES              8
#define GFXI_COLOR_GROUP_SIZE      8
#define GRAPHICS_CHAR_WIDTH        8


typedef enum
{
  TMS_MODE_GRAPHICS_I,
  TMS_MODE_GRAPHICS_II,
  TMS_MODE_TEXT,
  TMS_MODE_MULTICOLOR,
} tms9918_mode_t;

typedef enum
{
  TMS_REG_0 = 0,
  TMS_REG_1,
  TMS_REG_2,
  TMS_REG_3,
  TMS_REG_4,
  TMS_REG_5,
  TMS_REG_6,
  TMS_REG_7,
  TMS_REG_NAME_TABLE        = TMS_REG_2,
  TMS_REG_COLOR_TABLE       = TMS_REG_3,
  TMS_REG_PATTERN_TABLE     = TMS_REG_4,
  TMS_REG_SPRITE_ATTR_TABLE = TMS_REG_5,
  TMS_REG_SPRITE_PATT_TABLE = TMS_REG_6,
  TMS_REG_FG_BG_COLOR       = TMS_REG_7,
} tms9918_register_t;

typedef enum
{
  TMS_TRANSPARENT = 0,
  TMS_BLACK,
  TMS_MED_GREEN,
  TMS_LT_GREEN,
  TMS_DK_BLUE,
  TMS_LT_BLUE,
  TMS_DK_RED,
  TMS_CYAN,
  TMS_MED_RED,
  TMS_LT_RED,
  TMS_DK_YELLOW,
  TMS_LT_YELLOW,
  TMS_DK_GREEN,
  TMS_MAGENTA,
  TMS_GREY,
  TMS_WHITE,
} tms9918_color_t;

struct tms9918_port
{
    /* status register (read-only) */
    uint8_t status_reg;
    /* register write stage (0 or 1) */
    uint8_t regWriteStage;
    /* write only registers*/
    uint8_t write_reg[TMS_NUM_REGISTERS];
    /* buffered value */
    uint8_t readAheadBuffer;
    /* current address for cpu access (auto-increments) */
    uint16_t currentAddress;
    /* video ram */
    uint8_t vram[VRAM_SIZE];
    /* holds first stage of write to address/register port */
    uint8_t regWriteStage0Value;
    /* current display mode */
    tms9918_mode_t mode;
    uint8_t rowSpriteBits[FB_WIDTH]; /* collision mask */
};


static uint32_t framebuffer[FB_WIDTH * FB_HEIGHT];
static sg_image fb_texture;
sg_image_data img_data;
sg_view view;

static ImTextureRef imtexref(ImTextureID tex_id) 
{
    return (ImTextureRef){ ._TexID = tex_id };
}


static tms9918_mode_t tms9918_mode(struct tms9918_port *tms9918)
{
  if (tms9918->write_reg[TMS_REG_0] & TMS_R0_MODE_GRAPHICS_II)
  {
    return TMS_MODE_GRAPHICS_II;
  }
  /* MC and TEX bits 3 and 4. Shift to bits 0 and 1 to determine a value (0, 1 or 2) */
  switch ((tms9918->write_reg[TMS_REG_1] & (TMS_R1_MODE_MULTICOLOR | TMS_R1_MODE_TEXT)) >> 3)
  {
    case 0:
      return TMS_MODE_GRAPHICS_I;

    case 1:
      return TMS_MODE_MULTICOLOR;

    case 2:
      return TMS_MODE_TEXT;
  }
  return TMS_MODE_GRAPHICS_I;
}

/* Function:  tmsSpriteSize
 * ----------------------------------------
 * sprite size (8 or 16)
 */
static uint8_t tms9918_spriteSize(struct tms9918_port *tms9918 )
{
  return tms9918->write_reg[TMS_REG_1] & TMS_R1_SPRITE_16 ? 16 : 8;
}

/* Function:  tmsSpriteMagnification
 * ----------------------------------------
 * sprite size (0 = 1x, 1 = 2x)
 */
static bool tms9918_spriteMag(struct tms9918_port *tms9918)
{
  return tms9918->write_reg[TMS_REG_1] & TMS_R1_SPRITE_MAG2;
} 

/* Function:  tmsNameTableAddr
 * ----------------------------------------
 * name table base address
 */
static uint16_t tms9918_nameTableAddr(struct tms9918_port *tms9918)
{
  return (tms9918->write_reg[TMS_REG_NAME_TABLE] & 0x0f) << 10;
}

/* Function:  tmsColorTableAddr
 * ----------------------------------------
 * color table base address
 */
static uint16_t tms9918_colorTableAddr(struct tms9918_port *tms9918)
{
  const uint8_t mask = (tms9918->mode == TMS_MODE_GRAPHICS_II) ? 0x80 : 0xff;
  return (tms9918->write_reg[TMS_REG_COLOR_TABLE] & mask) << 6;
}

/* Function:  tms9918_patternTableAddr
 * ----------------------------------------
 * pattern table base address
 */
static uint16_t tms9918_patternTableAddr(struct tms9918_port *tms9918)
{
  const uint8_t mask = (tms9918->mode == TMS_MODE_GRAPHICS_II) ? 0x04 : 0x07;
  return (tms9918->write_reg[TMS_REG_PATTERN_TABLE] & mask) << 11;
}

/* Function:  tms9918_spriteAttrTableAddr
 * ----------------------------------------
 * sprite attribute table base address
 */
static uint16_t tms9918_spriteAttrTableAddr(struct tms9918_port *tms9918)
{
  return (tms9918->write_reg[TMS_REG_SPRITE_ATTR_TABLE] & 0x7f) << 7;
}

/* Function:  tms9918_spritePatternTableAddr
 * ----------------------------------------
 * sprite pattern table base address
 */
static uint16_t tms9918_spritePatternTableAddr(struct tms9918_port *tms9918)
{
  return (tms9918->write_reg[TMS_REG_SPRITE_PATT_TABLE] & 0x07) << 11;
}



/* Function:  tms9918_mainBgColor
 * ----------------------------------------
 * background color
 */
static tms9918_color_t tms9918_mainBgColor(struct tms9918_port *tms9918)
{
  return tms9918->write_reg[TMS_REG_FG_BG_COLOR] & 0x0f;
}

/* Function:  tmsFgColor
 * ----------------------------------------
 * foreground color
 */
static tms9918_color_t tms9918_mainFgColor(struct tms9918_port *tms9918)
{
  const tms9918_color_t c = (tms9918_color_t)(tms9918->write_reg[TMS_REG_FG_BG_COLOR] >> 4);
  return c == TMS_TRANSPARENT ? tms9918_mainBgColor(tms9918) : c;
}

/* Function:  tmsFgColor
 * ----------------------------------------
 * foreground color
 */
static  tms9918_color_t tms9918_fgColor(struct tms9918_port *tms9918, uint8_t colorByte)
{
  const tms9918_color_t c = (tms9918_color_t)(colorByte >> 4);
  return c == TMS_TRANSPARENT ? tms9918_mainBgColor(tms9918) : c;
}

/* Function:  tmsBgColor
 * ----------------------------------------
 * background color
 */
static tms9918_color_t tms9918_bgColor(struct tms9918_port *tms9918, uint8_t colorByte)
{
  const tms9918_color_t c = (tms9918_color_t)(colorByte & 0x0f);
  return c == TMS_TRANSPARENT ? tms9918_mainBgColor(tms9918) : c;
}

/* Function:  tms9918_outputSprites
 * ----------------------------------------
 * Output Sprites to a scanline
 */
static void (tms9918_outputSprites)(struct tms9918_port *tms9918, uint8_t y, uint8_t pixels[FB_WIDTH])
{
  const bool spriteMag = tms9918_spriteMag(tms9918);
  const bool sprite16 = tms9918_spriteSize(tms9918) == 16;
  const uint8_t spriteSize = tms9918_spriteSize(tms9918);
  const uint8_t spriteSizePx = spriteSize * (spriteMag + 1);
  const uint16_t spriteAttrTableAddr = tms9918_spriteAttrTableAddr(tms9918);
  const uint16_t spritePatternAddr = tms9918_spritePatternTableAddr(tms9918);

  uint8_t spritesShown = 0;

  if (y == 0)
  {
    tms9918->status_reg = 0;
  }

  uint8_t* spriteAttr = tms9918->vram + spriteAttrTableAddr;
  for (uint8_t spriteIdx = 0; spriteIdx < MAX_SPRITES; ++spriteIdx)
  {
    int16_t yPos = spriteAttr[SPRITE_ATTR_Y];

    /* stop processing when yPos == LAST_SPRITE_YPOS */
    if (yPos == LAST_SPRITE_YPOS)
    {
      if ((tms9918->status_reg & STATUS_5S) == 0)
      {
        tms9918->status_reg |= spriteIdx;
      }
      break;
    }

    /* check if sprite position is in the -31 to 0 range and move back to top */
    if (yPos > 0xe0)
    {
      yPos -= 256;
    }

    /* first row is YPOS -1 (0xff). 2nd row is YPOS 0 */
    yPos += 1;

    int16_t pattRow = y - yPos;
    if (spriteMag)
    {
      pattRow >>= 1;  // this needs to be a shift because -1 / 2 becomes 0. Bad.
    }

    /* check if sprite is visible on this line */
    if (pattRow < 0 || pattRow >= spriteSize)
    {
      spriteAttr += SPRITE_ATTR_BYTES;
      continue;
    }

    if (spritesShown == 0)
    {
      int* rsbInt = (int*)tms9918->rowSpriteBits;
      int* end = rsbInt + sizeof(tms9918->rowSpriteBits) / sizeof(int);

      while (rsbInt < end)
      {
        *rsbInt++ = 0;
      }
    }

    const uint8_t spriteColor = spriteAttr[SPRITE_ATTR_COLOR] & 0x0f;

    /* have we exceeded the scanline sprite limit? */
    if (++spritesShown > MAX_SCANLINE_SPRITES)
    {
      if ((tms9918->status_reg & STATUS_5S) == 0)
      {
        tms9918->status_reg |= STATUS_5S | spriteIdx;
      }
      break;
    }

    /* sprite is visible on this line */
    const uint8_t pattIdx = spriteAttr[SPRITE_ATTR_NAME];
    const uint16_t pattOffset = spritePatternAddr + pattIdx * PATTERN_BYTES + (uint16_t)pattRow;

    const int16_t earlyClockOffset = (spriteAttr[SPRITE_ATTR_COLOR] & 0x80) ? -32 : 0;
    const int16_t xPos = (int16_t)(spriteAttr[SPRITE_ATTR_X]) + earlyClockOffset;

    int8_t pattByte = tms9918->vram[pattOffset];
    uint8_t screenBit = 0, pattBit = 0;

    int16_t endXPos = xPos + spriteSizePx;
    if (endXPos >= FB_WIDTH)
    {
      endXPos = FB_WIDTH;
    }

    for (int16_t screenX = xPos; screenX < endXPos; ++screenX, ++screenBit)
    {
      if (screenX >= 0)
      {
        if (pattByte < 0)
        {
          if (spriteColor != TMS_TRANSPARENT && tms9918->rowSpriteBits[screenX] < 2)
          {
            pixels[screenX] = spriteColor;
          }

          /* we still process transparent sprites, since
             they're used in 5S and collision checks */
          if (tms9918->rowSpriteBits[screenX])
          {
            tms9918->status_reg |= STATUS_COL;
          }
          else
          {
            tms9918->rowSpriteBits[screenX] = spriteColor + 1;
          }
        }
      }

      /* next pattern bit if non-magnified or if odd screen bit */
      if (!spriteMag || (screenBit & 0x01))
      {
        pattByte <<= 1;
        if (++pattBit == GRAPHICS_CHAR_WIDTH && sprite16) /* from A -> C or B -> D of large sprite */
        {
          pattBit = 0;
          pattByte = tms9918->vram[pattOffset + PATTERN_BYTES * 2];
        }
      }
    }
    spriteAttr += SPRITE_ATTR_BYTES;
  }
}

/* Function:  tms9918_graphicsIScanLine
 * ----------------------------------------
 * generate a Graphics I mode scanline
 */
static void tms9918_graphicsIScanLine(struct tms9918_port *tms9918, uint8_t y, uint8_t pixels[FB_WIDTH])
{
  const uint8_t tileY = y >> 3;   /* which name table row (0 - 23) */
  const uint8_t pattRow = y & 0x07;  /* which pattern row (0 - 7) */

  /* address in name table at the start of this row */
  const uint16_t rowNamesAddr = tms9918_nameTableAddr(tms9918) + tileY * GRAPHICS_NUM_COLS;

  const uint8_t* patternTable = tms9918->vram + tms9918_patternTableAddr(tms9918);
  const uint8_t* colorTable = tms9918->vram + tms9918_colorTableAddr(tms9918);

  /* iterate over each tile in this row */
  for (uint8_t tileX = 0; tileX < GRAPHICS_NUM_COLS; ++tileX)
  {
    const uint8_t pattIdx = tms9918->vram[rowNamesAddr + tileX];
    uint8_t pattByte = patternTable[pattIdx * PATTERN_BYTES + pattRow];
    const uint8_t colorByte = colorTable[pattIdx / GFXI_COLOR_GROUP_SIZE];

    const uint8_t fgColor = tms9918_fgColor(tms9918, colorByte);
    const uint8_t bgColor = tms9918_bgColor(tms9918, colorByte);

    /* iterate over each bit of this pattern byte */
    for (uint8_t pattBit = 0; pattBit < GRAPHICS_CHAR_WIDTH; ++pattBit)
    {
      const bool pixelBit = pattByte & 0x80;
      *(pixels++) = pixelBit ? fgColor : bgColor;
      pattByte <<= 1;
    }
  }

  tms9918_outputSprites(tms9918, y, pixels - FB_WIDTH);
}

/* Function:  tms9918_graphicsIIScanLine
 * ----------------------------------------
 * generate a Graphics II mode scanline
 */
static void tms9918_graphicsIIScanLine(struct tms9918_port *tms9918, uint8_t y, uint8_t pixels[FB_WIDTH])
{
  const uint8_t tileY = y >> 3;   /* which name table row (0 - 23) */
  const uint8_t pattRow = y & 0x07;  /* which pattern row (0 - 7) */

  /* address in name table at the start of this row */
  const uint16_t rowNamesAddr = tms9918_nameTableAddr(tms9918) + tileY * GRAPHICS_NUM_COLS;

  /* the datasheet says the lower bits of the color and pattern tables must
     be all 1's for graphics II mode. however, the lowest 2 bits of the
     pattern address are used to determine if pages 2 & 3 come from page 0
     or not. Similarly, the lowest 6 bits of the color table register are
     used as an and mask with the nametable  index */
  const uint8_t nameMask = ((tms9918->write_reg[TMS_REG_COLOR_TABLE] & 0x7f) << 3) | 0x07;

  const uint16_t pageThird = ((tileY & 0x18) >> 3)
    & (tms9918->write_reg[TMS_REG_PATTERN_TABLE] & 0x03); /* which page? 0-2 */
  const uint16_t pageOffset = pageThird << 11; /* offset (0, 0x800 or 0x1000) */

  const uint8_t* patternTable = tms9918->vram + tms9918_patternTableAddr(tms9918) + pageOffset;
  const uint8_t* colorTable = tms9918->vram + tms9918_colorTableAddr(tms9918) + (pageOffset
    & ((tms9918->write_reg[TMS_REG_COLOR_TABLE] & 0x60) << 6));

  /* iterate over each tile in this row */
  for (uint8_t tileX = 0; tileX < GRAPHICS_NUM_COLS; ++tileX)
  {
    uint8_t pattIdx = tms9918->vram[rowNamesAddr + tileX] & nameMask;

    const size_t pattRowOffset = pattIdx * PATTERN_BYTES + pattRow;
    const uint8_t pattByte = patternTable[pattRowOffset];
    const uint8_t colorByte = colorTable[pattRowOffset];

    const tms9918_color_t fgColor = tms9918_fgColor(tms9918, colorByte);
    const tms9918_color_t bgColor = tms9918_bgColor(tms9918, colorByte);

    /* iterate over each bit of this pattern byte */
    for (uint8_t pattBit = 0; pattBit < GRAPHICS_CHAR_WIDTH; ++pattBit)
    {
      const bool pixelBit = (pattByte << pattBit) & 0x80;
      pixels[tileX * GRAPHICS_CHAR_WIDTH + pattBit] = (uint8_t)(pixelBit ? fgColor : bgColor);
    }
  }

 tms9918_outputSprites(tms9918, y, pixels);
}

/* Function:  tms9918_textScanLine
 * ----------------------------------------
 * generate a Text mode scanline
 */
static void tms9918_textScanLine(struct tms9918_port *tms9918, uint8_t y, uint8_t pixels[FB_WIDTH])
{
  const uint8_t tileY = y >> 3;   /* which name table row (0 - 23) */
  const uint8_t pattRow = y & 0x07;  /* which pattern row (0 - 7) */

  /* address in name table at the start of this row */
  const uint16_t rowNamesAddr = tms9918_nameTableAddr(tms9918) + tileY * TEXT_NUM_COLS;
  const uint8_t* patternTable = tms9918->vram + tms9918_patternTableAddr(tms9918);

  const tms9918_color_t bgColor = tms9918_mainBgColor(tms9918);
  const tms9918_color_t fgColor = tms9918_mainFgColor(tms9918);

  /* fill the first and last 8 pixels with bg color */
  memset(pixels, bgColor, TEXT_PADDING_PX);
  memset(pixels + FB_WIDTH - TEXT_PADDING_PX, bgColor, TEXT_PADDING_PX);

  for (uint8_t tileX = 0; tileX < TEXT_NUM_COLS; ++tileX)
  {
    const uint8_t pattIdx = tms9918->vram[rowNamesAddr + tileX];
    const uint8_t pattByte = patternTable[pattIdx * PATTERN_BYTES + pattRow];

    for (uint8_t pattBit = 0; pattBit < TEXT_CHAR_WIDTH; ++pattBit)
    {
      bool pixelBit = (pattByte << pattBit) & 0x80;
      pixels[TEXT_PADDING_PX + tileX * TEXT_CHAR_WIDTH + pattBit] = (uint8_t)(pixelBit ? fgColor : bgColor);
    }
  }
}

/* Function:  vrEmuTms9918MulticolorScanLine
 * ----------------------------------------
 * generate a Multicolor mode scanline
 */
static void tms9918_multicolorScanLine(struct tms9918_port *tms9918, uint8_t y, uint8_t pixels[FB_WIDTH])
{
  const uint8_t tileY = y >> 3;
  const uint8_t pattRow = ((y / 4) & 0x01) + (tileY & 0x03) * 2;

  const uint16_t namesAddr = tms9918_nameTableAddr(tms9918) + tileY * GRAPHICS_NUM_COLS;
  const uint8_t* patternTable = tms9918->vram + tms9918_patternTableAddr(tms9918);

  for (uint8_t tileX = 0; tileX < GRAPHICS_NUM_COLS; ++tileX)
  {
    const uint8_t pattIdx = tms9918->vram[namesAddr + tileX];
    const uint8_t colorByte = patternTable[pattIdx * PATTERN_BYTES + pattRow];

    memset(pixels + tileX * 8, tms9918_fgColor(tms9918, colorByte), 4);
    memset(pixels + tileX * 8 + 4, tms9918_bgColor(tms9918, colorByte), 4);
  }
  tms9918_outputSprites(tms9918, y, pixels);
}

/* Function:  tms9918_displayEnabled
  * ----------------------------------------
  * check BLANK flag
  */
bool tms9918_displayEnabled(struct tms9918_port *tms9918)
{
  return tms9918->write_reg[TMS_REG_1] & TMS_R1_DISP_ACTIVE;
}

/* Function:  tms9918_scanLine
 * ----------------------------------------
 * generate a scanline
 */
void tms9918_scanLine(struct tms9918_port *tms9918, uint8_t y, uint8_t pixels[FB_WIDTH])
{
  if (!tms9918_displayEnabled(tms9918) || y >= FB_HEIGHT)
  {
    memset(pixels, tms9918_mainBgColor(tms9918), FB_WIDTH);
    return;
  }

  switch (tms9918->mode)
  {
    case TMS_MODE_GRAPHICS_I:
      tms9918_graphicsIScanLine(tms9918, y, pixels);
      break;

    case TMS_MODE_GRAPHICS_II:
      tms9918_graphicsIIScanLine(tms9918, y, pixels);
      break;

    case TMS_MODE_TEXT:
      tms9918_textScanLine(tms9918, y, pixels);
      break;

    case TMS_MODE_MULTICOLOR:
      tms9918_multicolorScanLine(tms9918, y, pixels);
      break;
  }

  if (y == FB_HEIGHT - 1 && (tms9918->write_reg[1] & TMS_R1_INT_ENABLE))
  {
    tms9918->status_reg |= STATUS_INT;
  }
}

/* Function:  tms9918_regValue
 * ----------------------------------------
 * return a reigister value
 */
uint8_t tms9918_regValue(struct tms9918_port *tms9918, tms9918_register_t reg)
{
  if (tms9918 == NULL)
    return 0;

  return tms9918->write_reg[reg & 0x07];
}

/* Function:  tms9918_writeRegValue
 * ----------------------------------------
 * write a register value
 */
void tms9918_writeRegValue(struct tms9918_port *tms9918, tms9918_register_t reg, uint8_t value)
{
  if (tms9918 != NULL)
  {
    tms9918->write_reg[reg & 0x07] = value;
    tms9918->mode = tms9918_mode(tms9918);
  }
}

/* Function:  tms9918_vramValue
 * ----------------------------------------
 * return a value from vram
 */
uint8_t tms9918_vramValue(struct tms9918_port *tms9918, uint16_t addr)
{
  return tms9918->vram[addr & VRAM_MASK];
}

/* Function:  tms9918_displayMode
  * --------------------
  * current display mode
  */

tms9918_mode_t tms9918_displayMode(struct tms9918_port *tms9918)
{
  return tms9918->mode;
}


static void generate_test_pattern(void)
{
    // Palette TMS9918 (ARGB)
    static const uint32_t TMS9918_PALETTE[16] = {
    0xFF000000, 0xFF000000, 0xFF21C842, 0xFF5EDC78,
    0xFF5455ED, 0xFF7D76FC, 0xFFD4524D, 0xFF42EBF5,
    0xFFFC5554, 0xFFFF7978, 0xFFD4C154, 0xFFE6CE80,
    0xFF21B03B, 0xFFC95BBA, 0xFFCCCCCC, 0xFFFFFFFF
};
    static int frame = 0;
    frame++;

    // Taille TMS
    const int tiles_x = 32;
    const int tiles_y = 24;
    const int tile_w = 8;
    const int tile_h = 8;

    // Génère un fond de tiles
    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {

            // Couleurs style TMS9918 (pseudo-aléatoires mais stables)
            uint8_t fg = (tx + ty * 3) % 16;
            uint8_t bg = (tx * 5 + ty * 2) % 16;

            // Pattern simple qui bouge doucement (illusion d’animation)
            for (int y = 0; y < tile_h; y++) {
                for (int x = 0; x < tile_w; x++) {

                    int px = tx * tile_w + x;
                    int py = ty * tile_h + y;

                    uint8_t bit = ((x + y + frame / 5 + tx) % 2) ? 1 : 0;

                    uint32_t color = bit ? 
                        TMS9918_PALETTE[fg] :
                        TMS9918_PALETTE[bg];

                    framebuffer[py * FB_WIDTH + px] = color;
                }
            }
        }
    }

    // --- SPRITE ANIMÉ FAÇON TMS9918 ---
    static int sx = 0;
    static int sy = 80;

    sx = (sx + 1) % (FB_WIDTH - 16);

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int px = sx + x;
            int py = sy + y;
            framebuffer[py * FB_WIDTH + px] = TMS9918_PALETTE[8]; // rouge vif
        }
    }
}


void tms9918_update (struct hw_device *dev);
void tms9918_display (struct hw_device *dev, ImVec2 pos)
{
  generate_test_pattern();
  img_data.mip_levels[0].ptr = framebuffer;
  img_data.mip_levels[0].size = FB_WIDTH * FB_HEIGHT * sizeof(uint32_t);
  sg_update_image(fb_texture, &img_data);
  igSetNextWindowPos(pos, ImGuiCond_Once);
  ImGuiWindowFlags_ flags = ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoMove;
  igBegin("TMS9918 Video", NULL, flags);
  ImTextureID img_id = simgui_imtextureid(view);
  igImageEx(imtexref(img_id), (ImVec2){FB_WIDTH * 2, FB_HEIGHT * 2}, (ImVec2){0,1}, (ImVec2){1, 0});
  igEnd();
}

void tms9918_display_init(void) 
{
  fb_texture = sg_make_image(&(sg_image_desc){
        .width = FB_WIDTH,
        .height = FB_HEIGHT,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .label = "framebuffer",
        .usage.dynamic_update = true,
    });
    view = sg_make_view(&(sg_view_desc){
        .texture.image = fb_texture,
    });
}


uint8_t tms9918_read (struct hw_device *dev, unsigned long addr)
{
    struct tms9918_port *tms9918 = (struct tms9918_port *)dev->priv;
    switch(addr)
    {
      case 0: /* mode = 0, read data*/
      {
          tms9918->regWriteStage = 0;
          uint8_t currentValue = tms9918->readAheadBuffer;
          tms9918->readAheadBuffer = tms9918->vram[(tms9918->currentAddress++) & VRAM_MASK];
          return currentValue;
      }
      case 1: /* mode = 1, read status register*/
      {
          const uint8_t tmsStatus = tms9918->status_reg;
          tms9918->status_reg = 0;
          tms9918->regWriteStage = 0;
          return tmsStatus;
      }
    }
}

void tms9918_reset(struct hw_device *dev)
{
    struct tms9918_port *tms9918 = (struct tms9918_port *)dev->priv;
    tms9918->regWriteStage0Value = 0;
    tms9918->currentAddress = 0;
    tms9918->regWriteStage = 0;
    tms9918->status_reg = 0;
    tms9918->readAheadBuffer = 0;
    memset(tms9918->write_reg, 0, sizeof(tms9918->write_reg));

    /* ram intentionally left in unknown state */

    tms9918->mode = tms9918_mode(tms9918);
}

void tms9918_write (struct hw_device *dev, unsigned long addr, uint8_t val)
{
  struct tms9918_port *tms9918 = (struct tms9918_port *)dev->priv;
  switch(addr)
  {
    case 0: /* mode = 0, write data to VRAM*/
      tms9918->regWriteStage = 0;
      tms9918->readAheadBuffer = val;
      tms9918->vram[(tms9918->currentAddress++) & VRAM_MASK] = val;
      break;
    case 1: /* mode = 1, write an address*/
      if (tms9918->regWriteStage == 0)
      {
        /* first stage byte - either an address LSB or a register value */
        tms9918->regWriteStage0Value = val;
        tms9918->regWriteStage = 1;
      }
      else
      {
        /* second stage byte - either an address MSB or a register select */
        if (val & 0x80) /* register */
        {
          tms9918->write_reg[val & 0x07] = tms9918->regWriteStage0Value;
          tms9918->mode = tms9918_mode(tms9918);
        }
        else /* address */
        {
          tms9918->currentAddress = tms9918->regWriteStage0Value | ((val & 0x3f) << 8);
          if ((val & 0x40) == 0)
          {
            tms9918->readAheadBuffer = tms9918->vram[(tms9918->currentAddress++) & VRAM_MASK];
          }
        }
        tms9918->regWriteStage = 0;
      }
      break;
    default:
      /* should not happen */ 
  }
}


uint8_t tms9918_irq_pending(struct hw_device *dev);
void tms9918_dump (struct hw_device *dev);



struct hw_class tms9918_class =
  {
    .name = "tms9918",
    .readonly = 0,
    .reset = tms9918_reset,
    .read = tms9918_read,
    .write = NULL,
    .update = NULL,
    .dump = NULL,
    .check_interrupt = NULL,
  };


struct hw_device* tms9918_create (unsigned long size)
{
  struct tms9918_port *port = malloc (sizeof (struct tms9918_port));
  return device_create (&tms9918_class, size, port);
}

