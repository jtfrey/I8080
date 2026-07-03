
#include "I8080APU.h"
#include "I8080APUConstants.h"

typedef enum {
    kNoteEnd = 0,
    kNotePlay = 1,
    kNoteRepeat = 2,
    kNoteSkip = 3
} action_t;

typedef struct {
    uint16_t    action : 2;
    uint16_t    note_idx : 8;
    uint16_t    duration : 6;
} note_t;

typedef struct {
    note_t      melody;
    note_t      bassline;
} beat_t;

static const beat_t     ode_to_joy[] = {
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration= 2 } },
        { .melody = { .action = kNoteRepeat },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNoteRepeat  } },
          
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_B3_8BIT_IDX, .duration= 2 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNoteRepeat  } },
          
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A3_8BIT_IDX, .duration= 2 } },
        { .melody = { .action = kNoteRepeat },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_B3_8BIT_IDX, .duration= 2 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration= 2 } },
          
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E4_8BIT_IDX, .duration = 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 2 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D4_8BIT_IDX, .duration= 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G2_8BIT_IDX, .duration= 2 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D4_8BIT_IDX, .duration = 3 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G2_8BIT_IDX, .duration = 3 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
          
        { .melody = { .action = kNoteEnd },
          .bassline = { .action = kNoteEnd } },
        
    };

static const beat_t     smb_overworld[] = {
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteRepeat },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteRepeat },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteRepeat  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G5_8BIT_IDX, .duration = 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D3_8BIT_IDX, .duration = 2 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G4_8BIT_IDX, .duration = 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G3_8BIT_IDX, .duration = 2 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        
        
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C5_8BIT_IDX, .duration = 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G3_8BIT_IDX, .duration = 2 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G4_8BIT_IDX, .duration = 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E3_8BIT_IDX, .duration = 2 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_B5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_ASHARP5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_FSHARP3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E2_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_B3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_B5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C5_8BIT_IDX, .duration = 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G3_8BIT_IDX, .duration = 2 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G4_8BIT_IDX, .duration = 2 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E3_8BIT_IDX, .duration = 2 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_B5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_ASHARP5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_FSHARP3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E2_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_B3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_B5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
          
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_ASHARP5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_G3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_DSHARP5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C4_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_E5_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_GSHARP4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteRepeat } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_A4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_C4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_F3_8BIT_IDX, .duration = 1 } },
        { .melody = { .action = kNotePlay, .note_idx = I8080_APU_NOTE_D4_8BIT_IDX, .duration = 1 },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        { .melody = { .action = kNoteSkip },
          .bassline = { .action = kNoteSkip  } },
        
          
          
        { .melody = { .action = kNoteEnd },
          .bassline = { .action = kNoteEnd } }
    };

int
main()
{
    I8080APUMapperContextPtr    apu_context = I8080APUMapperContextCreate();
    const beat_t                *beats = smb_overworld;
    
    I8080APUMapperCallbacks->reset(NULL, I8080AddrRangeCreate(0, I8080APUMapperAddressRangeLength), apu_context);
    
    I8080APUMapperContextSetRegister(apu_context, kI8080APURegisterIndexMasterState, 0b11100000);
    I8080APUMapperContextSetRegister(apu_context, kI8080APURegisterIndexMasterEnvelopeLevelLo, 0x40);
    I8080APUMapperContextSetRegister(apu_context, kI8080APURegisterIndexPulse0DutyAndFreqShift, 0x07);
    I8080APUMapperContextSetRegister(apu_context, kI8080APURegisterIndexPulse0EnvelopeLevelLo, 0x20);
    I8080APUMapperContextSetRegister(apu_context, kI8080APURegisterIndexPulse1DutyAndFreqShift, 0x03);
    I8080APUMapperContextSetRegister(apu_context, kI8080APURegisterIndexPulse1EnvelopeLevelLo, 0x20);
    
    while ( true ) {
        if ( beats->melody.action == kNoteEnd && beats->bassline.action == kNoteEnd ) break;
        
        switch ( beats->melody.action ) {
            case kNoteSkip:
                break;
            case kNotePlay:
                I8080APUMapperContextSetRegister(apu_context,
                        kI8080APURegisterIndexPulse0FrequencyTimerLo, beats->melody.note_idx);
                I8080APUMapperContextSetRegister(apu_context,
                        kI8080APURegisterIndexPulse0DurationLo, beats->melody.duration);
            case kNoteRepeat:
                I8080APUMapperContextSetRegister(apu_context, kI8080APURegisterIndexPulse0State, 0b11000000);
                break;
        }
        switch ( beats->bassline.action ) {
            case kNoteSkip:
                break;
            case kNotePlay:
                I8080APUMapperContextSetRegister(apu_context,
                        kI8080APURegisterIndexPulse1FrequencyTimerLo, beats->bassline.note_idx);
                I8080APUMapperContextSetRegister(apu_context,
                        kI8080APURegisterIndexPulse1DurationLo, beats->bassline.duration);
            case kNoteRepeat:
                I8080APUMapperContextSetRegister(apu_context, kI8080APURegisterIndexPulse1State, 0b11000000);
                break;
        }
        usleep(150000);
        beats++;
    }

    return 0;
}
/*

E E F G | G F E D     | C C D E    | E- D D-E E F G    | G F E D     | C C D E    | D - C C-
C C C C | B, B, B, B, | A, A, B, C | C- G, G, -C C C C | B, B, B, B, | A, A, B, C | G, - C C-

*/