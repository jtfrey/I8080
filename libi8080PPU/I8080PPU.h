/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 Picture Processing Unit.
 */
 
#ifndef __I8080PPU_H__
#define __I8080PPU_H__

#include "I8080Memory.h"
#include "I8080System.h"
#include "I8080CGAColor.h"

/**
 * Bits Per Pixel
 * The number of bits used to represent a color index -- a pixel.
 * With a value of 2, a pixel can have 2^2 values:  0, 1, 2, 3.
 */
#define I8080_PPU_BPP            2

/**
 * Pixels Per Byte
 * The number of pixels contained in a single byte.
 */
#define I8080_PPU_PPB           (8 / I8080_PPU_BPP)

/**
 * Colors Per Palette
 * The number of color indices in a palette.
 */
#define I8080_PPU_CPP            4

/**
 * Palette Table Dimension
 * The number of palettes in a palette table.
 */
#define I8080_PPU_PTBLDIM        8

/**
 * Tile Width
 * The number of pixels in a single tile in the x-direction.
 */
#define I8080_PPU_TILEWIDTH      8

/**
 * Tile Height
 * The number of pixels in a single tile in the y-direction.
 * Because a terminal display uses characters with a typically
 * rectangular shape (with the long axis in the y-direction)
 * pixels are higher than they are wide.
 */
#define I8080_PPU_TILEHEIGHT     4

/**
 * Bits Per Tile
 * With 32 pixels in an 8x4 tile and 2 bits per pixel, that
 * works out to 64 bits.
 */
#define I8080_PPU_BPT           (I8080_PPU_BPP * I8080_PPU_TILEWIDTH * I8080_PPU_TILEHEIGHT)

/**
 * Tile Bytes
 * The number of bytes required for /ref I8080_PPU_BPT pixels.
 */
#define I8080_PPU_TILEBYTES     ((I8080_PPU_BPT + 7) / 8)

/**
 * Tile Row Bytes
 * The number of bytes required for one row of pixels in a
 * tile.  This value is used particularly during sprite drawing
 * to cache the pixels from rows of sprites.
 */
#define I8080_PPU_TILEROWBYTES  ((I8080_PPU_TILEWIDTH * I8080_PPU_BPP + 7) / 8)

/**
 * Tile Table Dimension
 * The total number of tiles that can be held in a tile table.
 */
#define I8080_PPU_TTBLDIM       32

/**
 * Tile Map Width
 * The number of tiles horizontally in a tile map.
 */
#define I8080_PPU_TMAPWIDTH     16

/**
 * Tile Map Height
 * The number of tiles vertically in a tile map.
 */
#define I8080_PPU_TMAPHEIGHT    16

/**
 * Tile Map Dimension
 * The total number of tiles in a tile map.
 */
#define I8080_PPU_TMAPDIM       (I8080_PPU_TMAPWIDTH * I8080_PPU_TMAPHEIGHT)

/**
 * Screen Width
 * Given the tile map width and the tile width, the width of the
 * screen is the product of the two.
 */
#define I8080_PPU_SCREENWIDTH   (I8080_PPU_TMAPWIDTH * I8080_PPU_TILEWIDTH)

/**
 * Screen Height
 * Given the tile map height and the tile height, the height of the
 * screen is the product of the two.
 */
#define I8080_PPU_SCREENHEIGHT  (I8080_PPU_TMAPHEIGHT * I8080_PPU_TILEHEIGHT)

/**
 * Maximum Sprites
 * The maximum number of sprites that can be held in the sprite table.
 */
#define I8080_PPU_MAXSPRT       52

/**
 * Maximum Sprites per Line
 * The maximum number of sprites that can occupy a single scan line.
 */
#define I8080_PPU_MAXSPRTLN     16

/**
 * A color palette
 * A PPU color palette contains four color indices, referring to the
 * CGW palette that was associated with the PPU context when it was
 * created.  The indices in the \p cidx array map to pixel values —
 * for \ref I8080_PPU_BPP of 2, that's 0, 1, 2, 3, respectively.
 *
 * The color index at 0 represents the background color if this palette
 * is loaded into the PPU context's palette 0; otherwise, it represents
 * a transparent pixel.
 *
 * The structure is packed to make it easily accessed via the I8080Mem
 * interface.  Within the emulator under the defaulf configuration
 * above, the PPU context makes its array of \ref I8080_PPU_CPP palettes
 * available at an offset of 16 bytes from the base mapping address
 * (see \ref I8080PPUAddrOffsetPaletteTable).  With 8 palettes x 4 color
 * indices, if the PPU is mapped at $4000, the palettes run from $4010 
 * through $4030.
 */
typedef struct __attribute__((packed)) {
    uint8_t         cidx[I8080_PPU_CPP];        /*!< CGW palette color indices e.g.
                                                     for pixel value 0, 1, 2, 3 */
} I8080PPUPalette_t;

/**
 * A graphics tile
 * The PPU generates an image by expanding a 2D grid of tile references
 * into a 2D grid of pixels.  A solid-green 128x64 display would require
 * 8 KiB of video memory to hold that image; in the tile-based approach,
 * a single 8x4 tile (at 8 bytes) with 256 references to it is sufficient.
 * That's just 264 B.  Now imagine there are 32 unique tiles avialable:
 * with 256 bytes of tile data, that's still just 512 B for the entire
 * 128x64 pixel image.
 *
 * The array of \p pixels is arranged in row-major format, with the
 * first row of pixels occuring in the first \p I8080_PPU_TILEROWBYTES
 * bytes of the array.  Pixels are ordered from the lowest bits of each
 * byte up through the highest.  Thus:
 *
 *  Pixel idx   Coord       Mask
 *   0          (0,0)       0b00000011
 *   0          (1,0)       0b00001100
 *   0          (2,0)       0b00110000
 *   0          (3,0)       0b11000000
 *   1          (4,0)       0b00000011
 *   1          (5,0)       0b00001100
 *   1          (6,0)       0b00110000
 *   1          (7,0)       0b11000000
 *   2          (0,1)       0b00000011
 *   2          (1,1)       0b00001100
 *   2          (2,1)       0b00110000
 *   2          (3,1)       0b11000000
 *                :
 *   7          (4,3)       0b00000011
 *   7          (5,3)       0b00001100
 *   7          (6,3)       0b00110000
 *   7          (7,3)       0b11000000
 *
 * The structure is packed to make it easily accessed via the I8080Mem
 * interface.
 */
typedef struct __attribute__((packed)) {
    uint8_t         pixels[I8080_PPU_TILEBYTES];
} I8080PPUTile_t;

/**
 * Reference to a graphics tile
 * Tile maps reference a basis tile by index in the table holding them
 * and attaches a palette index that selects one of the \ref I8080_PPU_PTBLDIM
 * palettes in the PPU context.
 *
 * Since there are 32 tiles (2^5) in the table and 8 (2^3) available palettes, a
 * tile reference can be a single 8-bit value.  The tile index occurs in the
 * lowest 5 bits.
 */
typedef struct __attribute__((packed)) {
#ifdef BITS_BIG_ENDIAN
    uint8_t         plte_idx : 3;
    uint8_t         tile_idx : 5;
#else
    uint8_t         tile_idx : 5;
    uint8_t         plte_idx : 3;
#endif
} I8080PPUTileRef_t;

/**
 * I8080PPUTileRef_t struct initializer
 * Given a palette and tile index, this macro emits a static
 * initializer for a \ref I8080PPUTileRef_t struct.
 * @param PIDX          palette index
 * @param TIDX          tile index
 * @return              a \ref I8080PPUTileRef_t struct initializer
 */
#define I8080PPUTileRefMake(PIDX, TIDX)     {.plte_idx=(PIDX), .tile_idx=(TIDX)}

/**
 * Sprite display options
 * A bitmask that controls sprite drawing.  Each sprite in the PPU context's
 * table can:
 *
 *     - be enabled/disabled for rendering
 *     - have its pixels flipped in render order horizontally
 *     - have its pixels flipped in render order vertically
 *     - be rendered in front of the background rathen than behind it
 */
typedef enum __attribute__((packed)) {
    kI8080PPUSpriteOptsEnable           = 0b1,          /*!< enable this sprite for rendering */
    kI8080PPUSpriteOptsFlipX            = 0b100000,     /*!< flip pixel ordering horizontally */
    kI8080PPUSpriteOptsFlipY            = 0b1000000,    /*!< flip pixel ordering vertically */
    kI8080PPUSpriteOptsForeground       = 0b10000000    /*!< render in front of the background layer */
} I8080PPUSpriteOpts_t;

/**
 * Sprite definition
 * A sprite is defined in terms of the (x,y) coordinate of its top-left
 * corner on the screen, display options, and a reference to a tile and
 * palette.
 *
 * The structure is packed to make it easily accessed via the I8080Mem
 * interface.  Within the emulator under the defaulf configuration
 * above, the PPU context makes its array of \ref I8080_PPU_MAXSPRT
 * sprites available at an offset of 48 bytes from the base mapping address
 * (see \ref I8080PPUAddrOffsetSpriteTable).  If the PPU is mapped at $4000,
 * the sprites run from $4030 through $4100.
 */
typedef struct __attribute__((packed)) {
    int8_t                  y;
    int8_t                  x;
    I8080PPUSpriteOpts_t    options;
    I8080PPUTileRef_t       tileref;
} I8080PPUSprite_t;

/**
 * PPU mode bitmask
 * PPU register 0 controls the rendering mode in which it is
 * running.
 *
 * There are two distinct pages of tile map data.  The LSb of the
 * mode register controls from which page tile map data is drawn.
 * The \p kI8080PPUModeRenderMapFlipFreqMask bits, if set, will force the
 * PPU to flip the \p kI8080PPUModeMapSelect bit at the end of every
 * 2^((mode & 0b11110000) >> 4) rendering cycles.  Since rendering
 * occurs at 60 Hz, setting 0b00010000 will trigger a flip every
 * 2^(0b0001) = 2^1 cycles — or every other cycle.  The same map will
 * be drawn for 2 cycles, then the alternate for 2, etc.  That's a 30 Hz
 * flip rate.  For 0b00100000, every 2^2 = 4 cycles it will switch, for
 * a 15 Hz rate — a nice "lightning" effect.  For 0b01000000 = 2^4, or
 * a 3.75 Hz rate, you get a nice flashing siren effect.
 *
 * When the system is reset the PPU initializes its mode register to
 * zero — no rendering, no background, no sprites.  It is up to the
 * software to enable rendering and the desired layers.
 */
typedef enum __attribute__((packed)) {
    kI8080PPUModeMapSelect              = 0b00000001,       /*!< 0: render tile map 0, 1:render tile map 1 */
    kI8080PPUModeRenderEnable           = 0b00000010,       /*!< 0: disable rendering, 1: enable rendering */
    kI8080PPUModeRenderBackground       = 0b00000100,       /*!< 1/0 do/do not render the background tiles */
    kI8080PPUModeRenderSprites          = 0b00001000,       /*!< 1/0 do/do not render sprites */
    kI8080PPUModeRenderMapFlipFreqMask  = 0b11110000,       /*!< Masks the bits that determine the rate at
                                                                 which the map select gets automatically
                                                                 flipped */
    kI8080PPUModeInit                   = 0                 /*!< By default no rendering, no layers enabled */
} I8080PPUMode_t;

/**
 * PPU status register
 * PPU register 1 provides feedback from the rendered to the user
 * program controlling it.  This register is read-only.
 */
typedef enum __attribute__((packed)) {
    kI8080PPUStatusSpriteOverflow       = 0b01000000,       /*!< Set during a render cycle if the per-line sprite
                                                                 limit is exceeded; reset at the beginning of each
                                                                 rendering cycle */
    kI8080PPUStatusSprite0Hit           = 0b10000000        /*!< Set when an opaque pixel of sprite 0 overlaps an
                                                                 opaque pixel in the background layer; cleared at
                                                                 the beginning of each rendering cycle */
} I8080PPUStatus_t;

/**
 * PPU register indices
 * This enumeration provides the PPU register number associated with
 * each named register.  The value also indicates the register's
 * offset from the base mapping address within the emulator.
 */
typedef enum {
    kI8080PPURegisterIndexMode          = 0,    /*!< PPU mode, see \ref I8080PPUMode_t */
    kI8080PPURegisterIndexStatus        = 1,    /*!< PPU status, see \ref I8080PPUStatus_t */
    kI8080PPURegisterIndexSpriteOffset  = 2,    /*!< Begin filling sprite slots from this offset in the sprite table;
                                                     can be used to alter the precedence bias of sprite selection */
    kI8080PPURegisterIndexINTEOpcode    = 3,    /*!< if non-zero, the 8080 opcode to deliver via an INTE to the CPU
                                                     when a rendering cycle begins (to trigger non-graphics processing) */
    kI8080PPURegisterIndexIsRendering   = 4,    /*!< (Read-only) Non-zero when the PPU is rendering to video, zero otherwise; kinda like
                                                     an inverse v-blank indicator */
    kI8080PPURegisterIndexDMADest       = 5,    /*!< Direct-Memory Access copy destination:
                                                        0x00:  208-byte sprite table
                                                        0x01:  256-byte tile table
                                                        0x02:  256-byte tile map 0
                                                        0x03:  256-byte tile map 1
                                                     Set this register before setting the DMA src page register,
                                                     \p kI8080PPURegisterIndexDMASrcPage
                                                 */
    kI8080PPURegisterIndexDMASrcOffset  = 6,
    kI8080PPURegisterIndexDMASrcPage    = 7,    /*!< Memory page from which the DMA copy should be made; writing to this
                                                     register triggers the DMA to stall the CPU and perform the copy */
    kI8080PPURegisterIndexMax
} I8080PPURegisterIndex_t;

/**
 * Relative address of PPU registers
 * The 16-bytes of PPU registers can be found at this address relative to the
 * mapping base address.
 */
extern const I8080Addr_t I8080PPUAddrOffsetRegisters;

/**
 * Relative address of palette table
 * The 32-bytes of PPU palette tables can be found at this address relative to the
 * mapping base address.
 */
extern const I8080Addr_t I8080PPUAddrOffsetPaletteTable;

/**
 * Relative address of sprite table
 * The 208-bytes of PPU sprite tables can be found at this address relative to the
 * mapping base address.
 */
extern const I8080Addr_t I8080PPUAddrOffsetSpriteTable;

/**
 * Relative address of tile table
 * The 256-bytes of PPU tile table can be found at this address relative to the
 * mapping base address.
 */
extern const I8080Addr_t I8080PPUAddrOffsetTileTable;

/**
 * Relative address of tile map 0
 * The 256-bytes of PPU tile map 0 can be found at this address relative to the
 * mapping base address.
 */
extern const I8080Addr_t I8080PPUAddrOffsetTileMap0;

/**
 * Relative address of tile map 1
 * The 256-bytes of PPU tile map 1 can be found at this address relative to the
 * mapping base address.
 */
extern const I8080Addr_t I8080PPUAddrOffsetTileMap1;

/**
 * A PPU memory mapper context
 * This is the public portion of a PPU memory mapper context.  The
 * \ref I8080PPUMapperContextCreate() function should be used to create a
 * new context.
 */
typedef struct {
    I8080CGAPaletteId_t         color_palette_id;   /*!< the CGA palette to load into curses; set when the context is
                                                         created, but can be changed to take effect between a
                                                         shutdown/reset of the system */
} I8080PPUMapperContext_t;

/**
 * Type of a pointer to a PPU mapper context
 */
typedef I8080PPUMapperContext_t * I8080PPUMapperContextPtr;

I8080PPUMapperContextPtr I8080PPUMapperContextCreate(I8080SystemPtr sys8080, I8080CGAPaletteId_t color_palette_id);

uint8_t I8080PPUMapperContextGetRegister(I8080PPUMapperContextPtr ppu, I8080PPURegisterIndex_t rgstr_idx);
void I8080PPUMapperContextSetRegister(I8080PPUMapperContextPtr ppu, I8080PPURegisterIndex_t rgstr_idx, uint8_t byte);

void I8080PPUMapperContextLoadPalette(I8080PPUMapperContextPtr ppu, int plte_idx, I8080PPUPalette_t *plte_data);
void I8080PPUMapperContextLoadPalettes(I8080PPUMapperContextPtr ppu, int plte_idx, int plte_count, I8080PPUPalette_t *plte_data);

void I8080PPUMapperContextLoadSprite(I8080PPUMapperContextPtr ppu, int sprite_idx, I8080PPUSprite_t *sprite_data);
void I8080PPUMapperContextLoadSprites(I8080PPUMapperContextPtr ppu, int sprite_idx, int sprite_count, I8080PPUSprite_t *sprite_data);

void I8080PPUMapperContextLoadTile(I8080PPUMapperContextPtr ppu, int tile_idx, I8080PPUTile_t *tile_data);
void I8080PPUMapperContextLoadTiles(I8080PPUMapperContextPtr ppu, int tile_idx, int tile_count, I8080PPUTile_t *tile_data);

void I8080PPUMapperContextLoadTileMap(I8080PPUMapperContextPtr ppu, int map_idx, I8080PPUTileRef_t *tilemap_data);

/**
 * PPU callbacks
 * Address of the callbacks that implement the \ref I8080PPUMapperContext_t
 * operations.
 */
extern const I8080MemMapperCallbacks *I8080PPUMapperCallbacks;

/**
 * Length of the address space occupied by the PPU
 * Since this is a fixed constant, it's exposed as a fixed constant.
 */
extern const I8080Addr_t I8080PPUMapperAddressRangeLength;

#endif /* __I8080PPU_H__ */
