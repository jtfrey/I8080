/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 Picture Processing Unit.
 */

#include "I8080PPU.h"
#include "I8080Logging.h"
#include "I8080Timing.h"
#include <stddef.h>
#include <pthread.h>

#define I8080_PPU_MAXCOLOR      64

typedef union __attribute__((packed)) {
    uint8_t                 R[16];
    struct {
        I8080PPUMode_t      ppu_mode;           /*!< PPU operational mode settings */
        I8080PPUMode_t      ppu_status;         /*!< (Read-only) PPU status byte */
        uint8_t             sprite_offset;      /*!< start filling active sprite slots from this index in the sprite table */
        uint8_t             inte_opcode;        /*!< opcode to deliver on an INTE to the CPU when rendering completes */
        uint8_t             is_rendering;       /*!< (Read-only) non-zero when the PPU is rendering to video, zero otherwise */
        uint8_t             dma_dst;            /*!< which page of the PPU memory should receive the DMA copy */
        uint8_t             dma_src_offset;
        uint8_t             dma_src_page;       /*!< which page of system memory should be copied to the PPU */
    };
} I8080PPURegisters_t;

typedef enum {
    kI8080PPUControlIsInShutdown = 0b1,
    kI8080PPUControlIsCursesInited = 0b10,
    kI8080PPUControlIsThreadInited = 0b100
} I8080PPUControl_t;

typedef struct {
    int8_t                  y;
    int8_t                  x;
    uint8_t                 sprite_idx;
    uint8_t                 plte_idx;
    I8080PPUSpriteOpts_t    options;
    uint8_t                 pixels[I8080_PPU_TILEROWBYTES];
} I8080PPUSpriteSlot_t;

typedef struct {
    I8080PPUMapperContext_t     public;
    struct {
        I8080PPURegisters_t     rgstrs;                         /*  16 B */
        I8080PPUPalette_t       ptbl[I8080_PPU_PTBLDIM];        /*  32 B */
        I8080PPUSprite_t        sprites[I8080_PPU_MAXSPRT];     /* 208 B */
        I8080PPUTile_t          ttbl[I8080_PPU_TTBLDIM];        /* 256 B */
        I8080PPUTileRef_t       map[2][I8080_PPU_TMAPDIM];      /* 512 B */
    } mapped;
    I8080SystemPtr              sys8080;
    I8080PPUControl_t           cntrl;
    I8080Microseconds           t_last_frame;
    pthread_t                   render_thread;
    pthread_cond_t              render_cond;
    pthread_mutex_t             render_lock;
    short                       saved_colors[4 * I8080_PPU_MAXCOLOR];
    WINDOW                      *wndw;
    int                         w, h;
    int                         x, y;
} I8080PPUMapperPrivateContext_t;

//

#define I8080_PPU_RGSTRS_OFFSET     offsetof(I8080PPUMapperPrivateContext_t, mapped.rgstrs)
#define I8080_PPU_PTBL_OFFSET       offsetof(I8080PPUMapperPrivateContext_t, mapped.ptbl)
#define I8080_PPU_SPRITES_OFFSET    offsetof(I8080PPUMapperPrivateContext_t, mapped.sprites)
#define I8080_PPU_TTBL_OFFSET       offsetof(I8080PPUMapperPrivateContext_t, mapped.ttbl)
#define I8080_PPU_MAP0_OFFSET       offsetof(I8080PPUMapperPrivateContext_t, mapped.map[0])
#define I8080_PPU_MAP1_OFFSET       offsetof(I8080PPUMapperPrivateContext_t, mapped.map[1])

#define I8080_PPU_RGSTRS_ADDR       0
#define I8080_PPU_PTBL_ADDR         (I8080_PPU_PTBL_OFFSET - I8080_PPU_RGSTRS_OFFSET)
#define I8080_PPU_SPRITES_ADDR      (I8080_PPU_SPRITES_OFFSET - I8080_PPU_RGSTRS_OFFSET)
#define I8080_PPU_TTBL_ADDR         (I8080_PPU_TTBL_OFFSET - I8080_PPU_RGSTRS_OFFSET)
#define I8080_PPU_MAP0_ADDR         (I8080_PPU_MAP0_OFFSET - I8080_PPU_RGSTRS_OFFSET)
#define I8080_PPU_MAP1_ADDR         (I8080_PPU_MAP1_OFFSET - I8080_PPU_RGSTRS_OFFSET)

#define I8080_PPU_END_ADDR          (I8080_PPU_MAP0_ADDR + sizeof(I8080PPUTileRef_t[2][I8080_PPU_TMAPDIM]))

const I8080Addr_t I8080PPUAddrOffsetRegisters = I8080_PPU_RGSTRS_ADDR;
const I8080Addr_t I8080PPUAddrOffsetPaletteTable = I8080_PPU_PTBL_ADDR;
const I8080Addr_t I8080PPUAddrOffsetSpriteTable = I8080_PPU_SPRITES_ADDR;
const I8080Addr_t I8080PPUAddrOffsetTileTable = I8080_PPU_TTBL_ADDR;
const I8080Addr_t I8080PPUAddrOffsetTileMap0 = I8080_PPU_MAP0_ADDR;
const I8080Addr_t I8080PPUAddrOffsetTileMap1 =I8080_PPU_MAP1_ADDR;

//

static inline
void
I8080PPUSetTimespecWithMicroseconds(
    I8080Microseconds   us,
    struct timespec     *ts
)
{
    I8080Microseconds       whole;
    
    us = modf(1e-6 * us, &whole);
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_nsec += trunc(1e9 * us);
    if ( ts->tv_nsec >= 1e9 ) {
        // pthread_cond_timedwait() complains if the tv_nsec exceeds 1 s:
        ts->tv_nsec -= 1e9;
        ts->tv_sec++;
    }
    ts->tv_sec += trunc(whole);
}

static
int
I8080PPUFillSpriteSlots(
    I8080PPUSpriteSlot_t    *slots,
    I8080PPUSprite_t        *sprites,
    int                     sprite_index,
    I8080PPUTile_t          *ttbl,
    int                     y,
    bool                    *did_overflow
)
{
    int                     nslots = I8080_PPU_MAXSPRTLN,
                            start_sprite_index = sprite_index;
    I8080PPUSprite_t        *spriteptr = sprites + sprite_index;
    
    *did_overflow = false;
    memset(slots, 0, sizeof(I8080PPUSpriteSlot_t) * I8080_PPU_MAXSPRTLN);
    while ( 1 ) {
        if ( (spriteptr->options & kI8080PPUSpriteOptsEnable) ) {
            // In scope?
            int             ry = y - spriteptr->y;
            
            if ( ry > 0 && ry <= I8080_PPU_TILEHEIGHT ) {
                if ( nslots ) {
                    int         dst_idx, src_idx, x;
                    uint8_t     *pixel_ptr = &ttbl[spriteptr->tileref.tile_idx].pixels[I8080_PPU_TILEROWBYTES * (
                                    (spriteptr->options & kI8080PPUSpriteOptsFlipY) ? (I8080_PPU_TILEHEIGHT - ry) : (ry - 1))
                                ];
                    slots->y = spriteptr->y;
                    x = slots->x = spriteptr->x;
                    slots->options = spriteptr->options;
                    slots->sprite_idx = sprite_index;
                    slots->plte_idx = spriteptr->tileref.plte_idx;
                    if ( spriteptr->options & kI8080PPUSpriteOptsFlipX ) {
                        dst_idx=0, src_idx=I8080_PPU_TILEROWBYTES;
                        while ( src_idx-- > 0 ) {
                            uint8_t         pixel = pixel_ptr[src_idx];
                            if ( x < 0 ) pixel <<= I8080_PPU_BPP, x++;
                            if ( x < 0 ) pixel <<= I8080_PPU_BPP, x++;
                            if ( x < 0 ) pixel <<= I8080_PPU_BPP, x++;
                            if ( x < 0 ) pixel <<= I8080_PPU_BPP, x++;
                            slots->pixels[dst_idx++] = pixel;
                        }
                    } else {
                        dst_idx=0, src_idx=0;
                        while ( src_idx < I8080_PPU_TILEROWBYTES ) {
                            uint8_t         pixel = pixel_ptr[src_idx++];
                            if ( x < 0 ) pixel >>= I8080_PPU_BPP, x++;
                            if ( x < 0 ) pixel >>= I8080_PPU_BPP, x++;
                            if ( x < 0 ) pixel >>= I8080_PPU_BPP, x++;
                            if ( x < 0 ) pixel >>= I8080_PPU_BPP, x++;
                            slots->pixels[dst_idx++] = pixel;
                        }
                    }
                    slots++;
                    nslots--;
                } else {
                    *did_overflow = true;
                    break;
                }
            }
        }
        if ( ++sprite_index == I8080_PPU_MAXSPRT ) {
            spriteptr = sprites;
            sprite_index = 0;
        } else {
            spriteptr++;
        }
        if ( sprite_index == start_sprite_index ) break;
    }
    return I8080_PPU_MAXSPRTLN - nslots;
}

static inline
void
I8080PPUSpritePixel(
    I8080PPUSpriteSlot_t    *slots,
    int                     nslots,
    int                     x,
    int                     *bg_cidx,
    int                     *bg_plte,
    int                     *fg_cidx,
    int                     *fg_plte,
    int                     *sprite_idx
)
{
    bool                    found_bg = false,
                            found_fg = false;
    
    while ( nslots && ! (found_bg && found_fg) ) {
        if ( slots->x <= 0 && slots->x > -I8080_PPU_TILEWIDTH ) {
            // This sprite is in scope, return a pixel:
            uint8_t         pixel_idx = (-slots->x / I8080_PPU_PPB);
            int             cidx;
            
            if ( slots->options & kI8080PPUSpriteOptsFlipX ) {
                cidx = (slots->pixels[pixel_idx] >> (8 - I8080_PPU_BPP)) & ((1 << I8080_PPU_BPP) - 1);
                slots->pixels[pixel_idx] <<= I8080_PPU_BPP;
            } else {
                cidx = slots->pixels[pixel_idx] & ((1 << I8080_PPU_BPP) - 1);
                slots->pixels[pixel_idx] >>= I8080_PPU_BPP;
            }
            if ( cidx ) {
                if ( slots->options & kI8080PPUSpriteOptsForeground ) {
                    *fg_cidx = cidx;
                    *fg_plte = slots->plte_idx;
                    *sprite_idx = slots->sprite_idx;
                    found_fg = true;
                } else {
                    *bg_cidx = cidx;
                    *bg_plte = slots->plte_idx;
                    if ( ! found_fg ) *sprite_idx = slots->sprite_idx;
                    found_bg = true;
                }
            }
        }
        slots->x--, slots++, nslots--;
    }
    while ( nslots ) slots->x--, slots++, nslots--;
}

static
void*
I8080PPURenderThread(
    void        *arg
)
{
    I8080PPUMapperPrivateContext_t  *ppu = (I8080PPUMapperPrivateContext_t*)arg;    
    uint64_t                        frame_count = 0;
    bool                            was_rendering;
    bool                            has_background = false;
    
    // Grab the lock:
    pthread_mutex_lock(&ppu->render_lock);
    ppu->t_last_frame = 0;
    was_rendering = (ppu->mapped.rgstrs.ppu_mode & kI8080PPUModeRenderEnable) == kI8080PPUModeRenderEnable;
    if ( was_rendering ) {
        wbkgd(ppu->wndw, ' '|COLOR_PAIR(1 + ppu->mapped.ptbl[0].cidx[0]));
        has_background = true;
    }
    
    while ( (ppu->cntrl & kI8080PPUControlIsInShutdown) == 0 ) {
        I8080Microseconds   now = I8080MicrosecondsMakeNow();
        struct timespec     ts;
        
        if ( now > ppu->t_last_frame ) {
            chtype                  blank = ' ' | COLOR_PAIR(1 + ppu->mapped.ptbl[0].cidx[0]);
            chtype                  row[ppu->w];
            int                     x, y;
            int                     i, j, k;
            
            ppu->mapped.rgstrs.is_rendering = 0xFF;
            pthread_mutex_unlock(&ppu->render_lock);
            
            // Interrupt?
            if ( ppu->sys8080 && ppu->mapped.rgstrs.inte_opcode ) I8080SystemRaiseInterrupt(ppu->sys8080, ppu->mapped.rgstrs.inte_opcode);
            
            // Set background color and prepare for refresh:
            //redrawwin(ppu->wndw);
            
            // Initialize the status register:
            ppu->mapped.rgstrs.ppu_status = ppu->mapped.rgstrs.ppu_mode & kI8080PPUModeMapSelect;
            
            if ( ppu->mapped.rgstrs.ppu_mode & kI8080PPUModeRenderEnable ) {
                if ( ppu->mapped.rgstrs.ppu_mode & kI8080PPUModeRenderBackground ) {
                    I8080PPUTileRef_t       *tilemap;
                    //
                    // We will need the tile map regardless of whether or not
                    // sprites are enabled:
                    //
                    if ( ! was_rendering ) {
                        was_rendering = true;
                    }
                    if ( ! has_background ) {
                        wbkgd(ppu->wndw, ' '|COLOR_PAIR(1 + ppu->mapped.ptbl[0].cidx[0]));
                        has_background = true;
                    }
                    tilemap = &ppu->mapped.map[(ppu->mapped.rgstrs.ppu_mode & kI8080PPUModeMapSelect)][0];
                    if ( ppu->mapped.rgstrs.ppu_mode & kI8080PPUModeRenderSprites ) {
                        I8080PPUSpriteSlot_t    spriteslots[I8080_PPU_MAXSPRTLN];
                        
                        for ( j = 0, y = 0; j < I8080_PPU_TMAPHEIGHT; j++ ) {
                            uint8_t             *tiles[I8080_PPU_TMAPWIDTH];
                            I8080PPUPalette_t   *palettes[I8080_PPU_TMAPWIDTH];
                            
                            for ( i = 0; i < I8080_PPU_TMAPWIDTH; i++, tilemap++ ) {
                                tiles[i] = &ppu->mapped.ttbl[tilemap->tile_idx].pixels[0];
                                palettes[i] = &ppu->mapped.ptbl[tilemap->plte_idx];
                            }
                            for ( i = 0; i < I8080_PPU_TILEHEIGHT; i++, y++ ) {
                                chtype      *rp = row;
                                bool        sprite_overflow;
                                int         nsprites;
                                
                                // Load sprites:
                                nsprites = I8080PPUFillSpriteSlots(
                                                spriteslots,
                                                ppu->mapped.sprites,
                                                ppu->mapped.rgstrs.sprite_offset,
                                                ppu->mapped.ttbl,
                                                y,
                                                &sprite_overflow);
                                if ( sprite_overflow ) ppu->mapped.rgstrs.ppu_status |= kI8080PPUStatusSpriteOverflow;
                                for ( x = 0, k = 0; x < ppu->w; k++ ) {
                                    int         bpt = I8080_PPU_TILEROWBYTES;
                                    
                                    while ( bpt-- ) {
                                        int         ppb = I8080_PPU_PPB;
                                        uint8_t     pixels = *tiles[k]++;
                                        
                                        while ( ppb-- ) {
                                            int     background_cidx = pixels & ((1 << I8080_PPU_BPP) - 1);
                                            int     sprite_bg_cidx = 0, sprite_bg_plte,
                                                    sprite_fg_cidx = 0, sprite_fg_plte,
                                                    sprite_idx = -1;
                                            chtype  pixel;
                                            
                                            // Sprite check:
                                            I8080PPUSpritePixel(spriteslots, nsprites, x,
                                                                &sprite_bg_cidx, &sprite_bg_plte,
                                                                &sprite_fg_cidx, &sprite_fg_plte,
                                                                &sprite_idx);
                                            
                                            if ( sprite_fg_cidx ) {
                                                pixel = ' ' | COLOR_PAIR(1 + ppu->mapped.ptbl[sprite_fg_plte].cidx[sprite_fg_cidx]);
                                                if ( sprite_idx == 0 && background_cidx ) {
                                                    ppu->mapped.rgstrs.ppu_status |= kI8080PPUStatusSprite0Hit;
                                                }
                                            } else {
                                                
                                                if ( ! background_cidx && sprite_bg_cidx ) {
                                                    pixel = ' ' | COLOR_PAIR(1 + ppu->mapped.ptbl[sprite_bg_plte].cidx[sprite_bg_cidx]);
                                                } else {
                                                    pixel = background_cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[background_cidx])) : blank;
                                                }
                                            }
                                            *rp++ = pixel;
                                            
                                            // Shift next background pixel into place:
                                            pixels >>= I8080_PPU_BPP;
                                            x++;
                                        }
                                    }
                                }
                                mvwaddchnstr(ppu->wndw, y, 0, row, ppu->w);
                            }
                        }
                    } else {
                        //
                        // Only rendering the background layer
                        //
                        for ( j = 0, y = 0; j < I8080_PPU_TMAPHEIGHT; j++ ) {
                            uint8_t             *tiles[I8080_PPU_TMAPWIDTH];
                            I8080PPUPalette_t   *palettes[I8080_PPU_TMAPWIDTH];
                            
                            for ( i = 0; i < I8080_PPU_TMAPWIDTH; i++, tilemap++ ) {
                                tiles[i] = &ppu->mapped.ttbl[tilemap->tile_idx].pixels[0];
                                palettes[i] = &ppu->mapped.ptbl[tilemap->plte_idx];
                            }
                            for ( i = 0; i < I8080_PPU_TILEHEIGHT; i++, y++ ) {
                                chtype      *rp = row;
                                
                                for ( x = 0, k = 0; x < ppu->w; x += I8080_PPU_TILEWIDTH, k++ ) {
                                    uint8_t     pixels = *tiles[k]++;
                                    int         cidx;
                                    
                                    cidx = pixels & 0x3; pixels >>= 2;
                                    *rp++ = cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[cidx])) : blank;
                                    cidx = pixels & 0x3; pixels >>= 2;
                                    *rp++ = cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[cidx])) : blank;
                                    cidx = pixels & 0x3; pixels >>= 2;
                                    *rp++ = cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[cidx])) : blank;
                                    cidx = pixels & 0x3;
                                    *rp++ = cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[cidx])) : blank;
                                    
                                    pixels = *tiles[k]++;
                                    cidx = pixels & 0x3; pixels >>= 2;
                                    *rp++ = cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[cidx])) : blank;
                                    cidx = pixels & 0x3; pixels >>= 2;
                                    *rp++ = cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[cidx])) : blank;
                                    cidx = pixels & 0x3; pixels >>= 2;
                                    *rp++ = cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[cidx])) : blank;
                                    cidx = pixels & 0x3;
                                    *rp++ = cidx ? (' ' | COLOR_PAIR(1 + palettes[k]->cidx[cidx])) : blank;
                                }
                                mvwaddchnstr(ppu->wndw, y, 0, row, ppu->w);
                            }
                        }
                    }
                } else if ( ppu->mapped.rgstrs.ppu_mode & kI8080PPUModeRenderSprites ) {
                    I8080PPUSpriteSlot_t    spriteslots[I8080_PPU_MAXSPRTLN];
                    
                    if ( ! was_rendering ) {
                        was_rendering = true;
                    }
                    if ( has_background ) {
                        wbkgd(ppu->wndw, ' '|COLOR_PAIR(0));
                        has_background = false;
                    }
                    
                    for ( j = 0, y = 0; j < I8080_PPU_TMAPHEIGHT; j++ ) {
                        for ( i = 0; i < I8080_PPU_TILEHEIGHT; i++, y++ ) {
                            chtype      *rp = row;
                            bool        sprite_overflow;
                            int         nsprites;
                            
                            // Load sprites:
                            nsprites = I8080PPUFillSpriteSlots(
                                            spriteslots,
                                            ppu->mapped.sprites,
                                            ppu->mapped.rgstrs.sprite_offset,
                                            ppu->mapped.ttbl,
                                            y,
                                            &sprite_overflow);
                            if ( sprite_overflow ) ppu->mapped.rgstrs.ppu_status |= kI8080PPUStatusSpriteOverflow;
                            for ( x = 0; x < ppu->w; x++ ) {    
                                int     sprite_bg_cidx = 0, sprite_bg_plte,
                                        sprite_fg_cidx = 0, sprite_fg_plte,
                                        sprite_idx;
                                chtype  pixel;
                                
                                // Sprite check:
                                I8080PPUSpritePixel(spriteslots, nsprites, x,
                                                    &sprite_bg_cidx, &sprite_bg_plte,
                                                    &sprite_fg_cidx, &sprite_fg_plte,
                                                    &sprite_idx);
                                        
                                if ( sprite_fg_cidx ) {
                                    pixel = ' ' | COLOR_PAIR(1 + ppu->mapped.ptbl[sprite_fg_plte].cidx[sprite_fg_cidx]);
                                } else if ( sprite_bg_cidx ) {
                                    pixel = ' ' | COLOR_PAIR(1 + ppu->mapped.ptbl[sprite_bg_plte].cidx[sprite_bg_cidx]);
                                } else {
                                    pixel = ' ' | A_NORMAL;
                                }
                                *rp++ = pixel;
                            }
                            mvwaddchnstr(ppu->wndw, y, 0, row, ppu->w);
                        }
                    }
                }
            } else if ( was_rendering ) {
                wbkgd(ppu->wndw, ' '|COLOR_PAIR(0));
                has_background = false;
                was_rendering = false;
                wclear(ppu->wndw);
            }
            wnoutrefresh(ppu->wndw);
            doupdate();
            pthread_mutex_lock(&ppu->render_lock);
            ppu->mapped.rgstrs.is_rendering = 0x00;
            ppu->t_last_frame = now;
            frame_count++;
            
            uint64_t            freq = (ppu->mapped.rgstrs.ppu_mode & kI8080PPUModeRenderMapFlipFreqMask);
            if ( freq && ((frame_count % (1ULL << (freq >> 4))) == 0) ) {
                ppu->mapped.rgstrs.ppu_mode ^= kI8080PPUModeMapSelect;
            }
        }
        /* f = 60 Hz = 60 / s => t = (1/60) s = (1e6 µs/s)(1/60)s = (1/6)e5 µs */
        I8080PPUSetTimespecWithMicroseconds(1e6/60.0 - (I8080MicrosecondsMakeNow() - now), &ts);
        pthread_cond_timedwait(&ppu->render_cond, &ppu->render_lock, &ts);
    }
    pthread_mutex_unlock(&ppu->render_lock);
    return NULL;
}

//

static
bool
I8080PPUContextReset(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080PPUMapperPrivateContext_t *ppu = (I8080PPUMapperPrivateContext_t*)context;
    I8080CGAPalettePtr          curses_palette;
    int                         h, w, x, y;
    
    if ( (ppu->cntrl & kI8080PPUControlIsCursesInited) == 0 ) {
        // Turn the display on:
        initscr();
        start_color();
        noecho();
        cbreak();
        curs_set(0);
        if ( ! has_colors() ) {
            endwin();
            CRITICAL("A color terminal is needed.");
            exit(EINVAL);
        }    
        getmaxyx(stdscr, h, w);
        if ( h < I8080_PPU_SCREENHEIGHT ) {
            endwin();
            CRITICAL("A terminal height of at least %d is required.", I8080_PPU_SCREENHEIGHT);
            exit(EINVAL);
        }
        if ( w < I8080_PPU_SCREENWIDTH ) {
            endwin();
            CRITICAL("A terminal width of at least %d is required.", I8080_PPU_SCREENWIDTH);
            exit(EINVAL);
        }
        x = (w - I8080_PPU_SCREENWIDTH) / 2, y = (h - I8080_PPU_SCREENHEIGHT) / 2;
        ppu->wndw = newwin(I8080_PPU_SCREENHEIGHT, I8080_PPU_SCREENWIDTH, y, x);
        if ( ! ppu->wndw ) {
            endwin();
            CRITICAL("Failed creating PPU window.");
            exit(EINVAL);
        }
        ppu->x = x, ppu->y = y, ppu->w = I8080_PPU_SCREENWIDTH, ppu->h = I8080_PPU_SCREENHEIGHT;
        keypad(ppu->wndw, TRUE);
        nodelay(ppu->wndw, TRUE);
        clearok(ppu->wndw, FALSE);
        leaveok(ppu->wndw, TRUE);
        scrollok(ppu->wndw, FALSE);
        immedok(ppu->wndw, FALSE);
        curs_set(0);
        
        // Load the color palette:
        curses_palette = I8080CGAPalettes[ppu->public.color_palette_id];
        h = 0; w = 1;
        x = (curses_palette->n_colors > I8080_PPU_MAXCOLOR) ? I8080_PPU_MAXCOLOR : curses_palette->n_colors;
        while ( h < x ) {
            short               sr = 1000 * ((double)curses_palette->colors[h].r / 255.0),
                                sg = 1000 * ((double)curses_palette->colors[h].g / 255.0),
                                sb = 1000 * ((double)curses_palette->colors[h].b / 255.0);
            short               *s = &ppu->saved_colors[4 * h];
        
            *s = 1;
            color_content(w, s + 1, s + 2, s + 3);
            init_color(w, sr, sg, sb);
            init_pair(w, 0, w);
            h++, w++;
        }
        ppu->cntrl |= kI8080PPUControlIsCursesInited;
    }
    wclear(ppu->wndw);
    clear();
    wrefresh(ppu->wndw);
    refresh();
    
    pthread_cond_init(&ppu->render_cond, NULL);
    pthread_mutex_init(&ppu->render_lock, NULL);
    pthread_mutex_lock(&ppu->render_lock);
    if ( pthread_create(&ppu->render_thread , NULL, I8080PPURenderThread, ppu) != 0 ) {
        endwin();
        CRITICAL("Unable to launch PPU rendering thread.");
        exit(EINVAL);
    }
    ppu->cntrl |= kI8080PPUControlIsThreadInited;
    memset(&ppu->mapped, 0, sizeof(ppu->mapped));
    ppu->mapped.rgstrs.ppu_mode = kI8080PPUModeInit;
    pthread_mutex_unlock(&ppu->render_lock);
    return true;
}

//

static
void
I8080PPUContextShutdown(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080PPUMapperPrivateContext_t *ppu = (I8080PPUMapperPrivateContext_t*)context;
    int                     i;
    
    if ( (ppu->cntrl & kI8080PPUControlIsThreadInited) ) {
        pthread_mutex_lock(&ppu->render_lock);
        ppu->cntrl |= kI8080PPUControlIsInShutdown;
        pthread_cond_signal(&ppu->render_cond);
        pthread_mutex_unlock(&ppu->render_lock);
        pthread_join(ppu->render_thread, NULL);
        pthread_detach(ppu->render_thread);
        pthread_mutex_destroy(&ppu->render_lock);
        pthread_cond_destroy(&ppu->render_cond);
        ppu->cntrl &= ~kI8080PPUControlIsThreadInited;
    }    
    if ( (ppu->cntrl & kI8080PPUControlIsCursesInited) ) {
        for ( i = 0; i < I8080_PPU_MAXCOLOR; i++ ) {
            short               *s = &ppu->saved_colors[4 * i];
            
            if ( *s ) init_color(i + 1, *(s + 1), *(s + 2), *(s + 3));
        }
        nodelay(ppu->wndw, FALSE);
        delwin(ppu->wndw);
        endwin();
        ppu->cntrl &= ~kI8080PPUControlIsCursesInited;
    }
}

//

static
bool
I8080PPUContextRead(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             *byte,
    const void          *context
)
{
    I8080PPUMapperPrivateContext_t *ppu = (I8080PPUMapperPrivateContext_t*)context;
    I8080Addr_t             ppu_addr = addr - range.base;
    
    if ( ppu_addr < I8080_PPU_END_ADDR ) {
        *byte = *(((uint8_t*)&ppu->mapped) + ppu_addr);
        return true;
    }
    return false;
}

//

static
bool
I8080PPUContextWrite(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             byte,
    const void          *context
)
{
    I8080PPUMapperPrivateContext_t *ppu = (I8080PPUMapperPrivateContext_t*)context;
    I8080Addr_t             ppu_addr = addr - range.base;
    
    /* Register? */
    if ( ppu_addr < I8080_PPU_END_ADDR ) {
        switch ( ppu_addr ) {
            case I8080_PPU_RGSTRS_ADDR + kI8080PPURegisterIndexIsRendering:
                // Read-only:
                return true;
            case I8080_PPU_RGSTRS_ADDR + kI8080PPURegisterIndexDMASrcPage: {
                // DMA transfer:
                if ( ppu->sys8080 ) {
                    I8080Addr_t     src;
                    uint8_t         *dst;
                    
                    pthread_mutex_lock(&ppu->render_lock);
                    ppu->mapped.rgstrs.dma_src_page = byte;
                    
                    I8080SystemSetStall(ppu->sys8080, true);
                    src = ((I8080Addr_t)byte << 8) | ppu->mapped.rgstrs.dma_src_offset;
                    switch ( ppu->mapped.rgstrs.dma_dst ) {
                        case 0:{
                            int         count = sizeof(ppu->mapped.sprites);
                            
                            dst = (uint8_t*)ppu->mapped.sprites;
                            while ( count-- ) *dst++ = I8080MemRead(ppu->sys8080->sysmem, src++);
                            break;
                        }
                        case 1:
                        case 2:
                        case 3: {
                            int         count = 256;
                            
                            dst = ((uint8_t*)&ppu->mapped) + ((uint16_t)ppu->mapped.rgstrs.dma_dst << 8);
                            while ( count-- ) *dst++ = I8080MemRead(ppu->sys8080->sysmem, src++);
                            break;
                        }
                    }
                    I8080SystemSetStall(ppu->sys8080, false);
                    pthread_mutex_unlock(&ppu->render_lock);
                }
                break;
            }
            default:
                break;
        }
        *(((uint8_t*)&ppu->mapped) + ppu_addr) = byte;
        return true;
    }
    return false;
}

//

const I8080MemMapperCallbacks __I8080PPUMapperCallbacks = {
            .mapper_name = "curses-ppu-adapter",
            .rewrite_addr = NULL,
            .read = I8080PPUContextRead,
            .write = I8080PPUContextWrite,
            .reset = I8080PPUContextReset,
            .shutdown = I8080PPUContextShutdown
        };
const I8080MemMapperCallbacks *I8080PPUMapperCallbacks = &__I8080PPUMapperCallbacks;

const I8080Addr_t I8080PPUMapperAddressRangeLength = sizeof(((I8080PPUMapperPrivateContext_t*)NULL)->mapped);

//

I8080PPUMapperContextPtr
I8080PPUMapperContextCreate(
    I8080SystemPtr          sys8080,
    I8080CGAPaletteId_t     color_palette_id
)
{
    I8080PPUMapperPrivateContext_t  *new_ppu = (I8080PPUMapperPrivateContext_t*)calloc(1, sizeof(I8080PPUMapperPrivateContext_t));
    
    if ( new_ppu ) {
        new_ppu->sys8080 = sys8080;
        new_ppu->public.color_palette_id = (color_palette_id >= kI8080CGAPaletteIdMax) ? 
                                            (kI8080CGAPaletteIdMax - 1) : ((color_palette_id < 0) ? 0 :
                                                color_palette_id);
    }
    return (I8080PPUMapperContextPtr)new_ppu;
}

//

uint8_t
I8080PPUMapperContextGetRegister(
    I8080PPUMapperContextPtr    ppu,
    I8080PPURegisterIndex_t     rgstr_idx
)
{
    uint8_t                     byte = 0;
    if ( rgstr_idx < 16 ) I8080PPUContextRead(NULL, I8080AddrRangeCreate(0, I8080PPUMapperAddressRangeLength), rgstr_idx, &byte, ppu);
    return byte;
}

//

void
I8080PPUMapperContextSetRegister(
    I8080PPUMapperContextPtr    ppu,
    I8080PPURegisterIndex_t     rgstr_idx,
    uint8_t                     byte
)
{
    if ( rgstr_idx < 16 ) I8080PPUContextWrite(NULL, I8080AddrRangeCreate(0, I8080PPUMapperAddressRangeLength), rgstr_idx, byte, ppu);
}

//

void
I8080PPUMapperContextLoadPalette(
    I8080PPUMapperContextPtr    ppu,
    int                         plte_idx,
    I8080PPUPalette_t           *plte_data
)
{
    I8080PPUMapperPrivateContext_t  *PPU = (I8080PPUMapperPrivateContext_t*)ppu;
    
    PPU->mapped.ptbl[plte_idx %= I8080_PPU_PTBLDIM] = *plte_data;
}

//

void
I8080PPUMapperContextLoadPalettes(
    I8080PPUMapperContextPtr    ppu,
    int                         plte_idx,
    int                         plte_count,
    I8080PPUPalette_t           *plte_data
)
{
    I8080PPUMapperPrivateContext_t  *PPU = (I8080PPUMapperPrivateContext_t*)ppu;
    
    plte_idx %= I8080_PPU_PTBLDIM;
    while ( plte_count-- ) {
        PPU->mapped.ptbl[plte_idx] = *plte_data;
        plte_idx = (plte_idx + 1) % I8080_PPU_PTBLDIM;
        plte_data++;
    }
}

//

void
I8080PPUMapperContextLoadTile(
    I8080PPUMapperContextPtr    ppu,
    int                         tile_idx,
    I8080PPUTile_t              *tile_data
)
{
    I8080PPUMapperPrivateContext_t  *PPU = (I8080PPUMapperPrivateContext_t*)ppu;
    
    PPU->mapped.ttbl[tile_idx % I8080_PPU_TTBLDIM] = *tile_data;
}

//

void
I8080PPUMapperContextLoadTiles(
    I8080PPUMapperContextPtr    ppu,
    int                         tile_idx,
    int                         tile_count,
    I8080PPUTile_t              *tile_data
)
{
    I8080PPUMapperPrivateContext_t  *PPU = (I8080PPUMapperPrivateContext_t*)ppu;
    
    tile_idx %= I8080_PPU_TTBLDIM;
    while ( tile_count--  ) {
        PPU->mapped.ttbl[tile_idx] = *tile_data++;
        tile_idx = (tile_idx + 1) % I8080_PPU_TTBLDIM;
    }
}

//

void
I8080PPUMapperContextLoadSprite(
    I8080PPUMapperContextPtr    ppu,
    int                         sprite_idx,
    I8080PPUSprite_t            *sprite_data
)
{
    I8080PPUMapperPrivateContext_t  *PPU = (I8080PPUMapperPrivateContext_t*)ppu;
    
    PPU->mapped.sprites[sprite_idx % I8080_PPU_MAXSPRT] = *sprite_data;
}

//

void
I8080PPUMapperContextLoadSprites(
    I8080PPUMapperContextPtr    ppu,
    int                         sprite_idx,
    int                         sprite_count,
    I8080PPUSprite_t            *sprite_data
)
{
    I8080PPUMapperPrivateContext_t  *PPU = (I8080PPUMapperPrivateContext_t*)ppu;
    
    sprite_idx %= I8080_PPU_MAXSPRT;
    while ( sprite_count-- ) {
        PPU->mapped.sprites[sprite_idx] = *sprite_data++;
        sprite_idx = (sprite_idx + 1) % I8080_PPU_MAXSPRT;
    }
}

//

void
I8080PPUMapperContextLoadTileMap(
    I8080PPUMapperContextPtr    ppu,
    int                         map_idx,
    I8080PPUTileRef_t           *tilemap_data
)
{
    I8080PPUMapperPrivateContext_t  *PPU = (I8080PPUMapperPrivateContext_t*)ppu;
    int                             i;
    
    switch ( map_idx ) {
    
        case 0:
        case 1:
            memcpy(&PPU->mapped.map[map_idx][0], tilemap_data, sizeof(PPU->mapped.map[map_idx]));
            break;
    
    }
}
