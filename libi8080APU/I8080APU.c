/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the Audio Processing Unit (APU) API.
 */

#include "I8080APU.h"
#include "I8080APUConstants.h"

#include <AudioToolbox/AudioToolbox.h>

#define I8080_APU_BUFFER_COUNT  2
#define I8080_APU_BUFFER_SIZE   2048
#define I8080_APU_SAMPLE_RATE   44100

/**
 * Duration timer state
 * Fields used to track the duration of a channel's emitting of
 * samples for a given configuration.
 */
typedef struct {
    uint16_t        length;                 /*!< the number of 1/60 s periods to sustain the sampling */
    bool            is_fixed_duration;      /*!< false = indefinite sustain, true = fixed length */
} I8080APUDurationState_t;

/**
 * Duty cycle state
 * Fields used to track the duty cycle waveform profile.
 */
typedef struct {
    uint16_t        tick;                   /*!< updated on each 44100 Hz cycle */
    uint16_t        length;                 /*!< clock the duty cycle waveform once the \p tick reaches this value */
    uint16_t        waveform_profile;       /*!< the selected 16-bit waveform profile; e.g. for 50% duty cycle, 0x7F80 */
    uint16_t        waveform_bit;           /*!< the active bit position in the waveform profile; starts at 0x8000 and shifts
                                                 right on each clock, returning to 0x8000 once it reaches 0x0000 */
} I8080APUDutyCycleState_t;

/**
 * Envelope (volume) state
 * Fields used to track the envelope (volume).
 */
typedef struct {
    uint16_t        tick;                   /*!< updated on each 44100 Hz cycle */
    uint16_t        length;                 /*!< for variable volume, clock the envelope once the \p tick reaches this value */
    uint16_t        level;                  /*!< the current level (volume) */
    bool            is_const;               /*!< false = \p level decreases over time, true = \p level is a constant */
    bool            is_looped;              /*!< false = variable \p level remains at zero, true = \p level is restored to INT16_MAX once
                                                 it hits zero */
} I8080APUEnvelopeState_t;

/**
 * Frequency timer state
 * Fields used to track the frequency timer for a channel.  The
 * sweep fields are included.
 */
typedef struct {
    uint16_t        tick;                   /*!< updated on each 44100 Hz cycle */
    uint16_t        length;                 /*!< clock a single wavelength of the selected frequency once the \p tick reaches this value */
    uint8_t         sweep_tick;             /*!< updated on each pass of one wavelength of the selected frequency */
    uint8_t         sweep_shift;            /*!< low-nibble = number of bit positions to right-shift \p length to get the delta, bit 7
                                                 indicates sign on the delta (1 = negative, 0 = positve) */
    uint8_t         sweep_period;           /*!< clock a change to \p length after this many wavelengths of the current frequency have been generated */
    bool            is_swept;               /*!< false = constant \p length, true = variable \p length (sweep) */
} I8080APUFrequencyState_t;

/**
 * Waveform sequence state
 * Fields used to track a 32-sample waveform profile that varies from
 * [-15,+15].  Used for the triangle and sawtooth channels.
 */
typedef struct {
    const int16_t   *sequence;              /*!< pointer to the 32-sample sequence */
    uint8_t         step;                   /*!< current sampling index within the sequence */
} I8080APUSequenceState_t;

/**
 * Noise state
 * Fields used to track the state of a noise channel.
 *
 * The PRNG works just like the shift-register noise channel in the
 * NES.  From the starting seed 15-bit pattern in \p shift_register,
 * the value is shifted right by one bit position and bit 14 is replaced
 * with the XOR of the departing bit 0 with another bit position (dependent
 * on the \p is_short_loop flag).  Unlike the NES, where the shift_register
 * is always seeded with a constant 0x1, this implementation seeds the C
 * library's "better random number generator" with the current Unix epoch
 * timestamp and generates an actual random number.  The NES's constant
 * starting point yields predictable 93- or 32767-value non-repeating
 * sequences.  This implementation's use of a random seed will yield same-sized
 * sequences but the loop may be delayed by one value from the initial
 * shift-register value (e.g. 1 unique + 93 cycle).
 */
typedef struct {
    uint16_t        tick;                   /*!< updated on each 44100 Hz cycle */
    uint16_t        shift_register;         /*!< 16-bit value that accrues pseudo-randomness as the noise channel is sampled */
    uint8_t         period;                 /*!< clock a shift-register update once the \p tick reaches this value */
    bool            is_short_loop;          /*!< true = select the 93-element random cycle, false = select the 32767-element cycle */
} I8080APUNoiseState_t;

/**
 * Pulse channel state
 * Each pulse channel maintains duration, envelope, frequency, and duty-cycle
 * state for its sample generation.
 */
typedef struct {
    bool                        is_enabled; /*!< false = mute this channel, true = produce samples */
    I8080APUDurationState_t     duration;   /*!< duration state */
    I8080APUEnvelopeState_t     envelope;   /*!< envelope state */
    I8080APUFrequencyState_t    frequency;  /*!< frequency state (including sweep) */
    I8080APUDutyCycleState_t    duty_cycle; /*!< duty-cycle state */
} I8080APUPulseChannelState_t;

/**
 * Triangle channel state
 * Each triangle channel maintains duration, envelope, frequency, and sequence
 * state for its sample generation.
 */
typedef struct {
    bool                        is_enabled; /*!< false = mute this channel, true = produce samples */
    I8080APUDurationState_t     duration;   /*!< duration state */
    I8080APUEnvelopeState_t     envelope;   /*!< envelope state */
    I8080APUFrequencyState_t    frequency;  /*!< frequency state (including sweep) */
    I8080APUSequenceState_t     sequence;   /*!< sequence state */
} I8080APUTriangleChannelState_t;

/**
 * Noise channel state
 * Each noise channel maintains duration, envelope, and PRNG state for its
 * sample generation.
 */
typedef struct {
    bool                        is_enabled; /*!< false = mute this channel, true = produce samples */
    I8080APUDurationState_t     duration;   /*!< duration state */
    I8080APUEnvelopeState_t     envelope;   /*!< envelope state */
    I8080APUNoiseState_t        noise;      /*!< noise (PRNG) state */
} I8080APUNoiseChannelState_t;

/**
 * DPCM channel state
 * Each DPCM channel pre-buffers samples from system memory, processing those
 * samples one bit at a time to update a cumulative sample value (1=sample+1, 0=sample-1).
 */
typedef struct {
    bool                        is_enabled; /*!< false = mute this channel, true = produce samples */
    bool                        is_looped;  /*!< false = play once, true = replay indefinitely */
    I8080MemRef                 sysmem;     /*!< reference to the system memory to fetch samples */
    I8080Addr_t                 addr;       /*!< the configured sample data address */
    I8080Addr_t                 nbytes;     /*!< the configured sample data byte length */
    I8080Addr_t                 cur_addr;   /*!< address of next sample */
    I8080Addr_t                 cur_nbytes; /*!< number of bytes of sample data remaining */
    uint8_t                     samples[16];/*!< pre-fetched sample buffer */
    uint8_t                     *sample_ptr;/*!< pointer to in-scope sample being processed */
    uint8_t                     *sample_end;/*!< pointer to the byte beyond the last buffered sample */
    uint8_t                     sample_mask;/*!< bitmask used to isolate the next sample bit; starts at 0x1 and shifts left; when
                                                 it reaches zero, the next sample byte is selected and \p sample_mask is reset to 0x1 */
    int16_t                     sample;     /*!< the current sample value */
    uint16_t                    tick;       /*!< updated on each 44100 Hz cycle */
    uint16_t                    length;     /*!< clock the next sample value once the \p tick reaches this value; the
                                                 value of the rate register + 1 */
} I8080APUDPCMChannelState_t;

/**
 * APU state
 * The APU state includes a global frame tick (count of 1/60 s periods), the
 * maximum output volume level, and state for each of its constituent
 * generator channels.
 */
typedef struct {
    uint16_t                        frame_tick;     /*!< tick counter that rolls over every 1/60 s (735 samples) */
    uint16_t                        max_level;      /*!< the volume limiter for the mixed audio */
    
    I8080APUPulseChannelState_t     pulse_0;        /*!< pulse channel 0 state */
    I8080APUPulseChannelState_t     pulse_1;        /*!< pulse channel 1 state */
    I8080APUTriangleChannelState_t  triangle_0;     /*!< triangle channel 0 state */
    I8080APUTriangleChannelState_t  triangle_1;     /*!< triangle channel 1 state */
    I8080APUNoiseChannelState_t     noise;          /*!< noise channel state */
    I8080APUDPCMChannelState_t      dpcm;           /*!< delta PCM channel state */
} I8080APUState_t;

/**
 * I8080 memory mapper context for the APU
 * Instances of this data structure are used to interface the APU with the memory
 * space of the emulator and the audio generators.
 */
typedef struct {
    I8080APURegisters_t         rgstrs;             /*!< the memory-mapped registers */
    I8080APUState_t             apu_state;          /*!< the APU state */

    AudioQueueRef               audio_queue;        /*!< the CoreAudio sampling queue object */
    AudioQueueBufferRef         audio_buffers[I8080_APU_BUFFER_COUNT];  /*!< the CoreAudio sample buffers to be used when generating audio */
} I8080APUMapperPrivateContext_t;

//

#define I8080_APU_MASTER_RGSTRS_OFFSET      offsetof(I8080APUMapperPrivateContext_t, rgstrs.master)
#define I8080_APU_PULSE_0_RGSTRS_OFFSET     offsetof(I8080APUMapperPrivateContext_t, rgstrs.pulse_0)
#define I8080_APU_PULSE_1_RGSTRS_OFFSET     offsetof(I8080APUMapperPrivateContext_t, rgstrs.pulse_1)
#define I8080_APU_TRIANGLE_0_RGSTRS_OFFSET  offsetof(I8080APUMapperPrivateContext_t, rgstrs.triangle_0)
#define I8080_APU_TRIANGLE_1_RGSTRS_OFFSET  offsetof(I8080APUMapperPrivateContext_t, rgstrs.triangle_1)
#define I8080_APU_NOISE_RGSTRS_OFFSET       offsetof(I8080APUMapperPrivateContext_t, rgstrs.noise)
#define I8080_APU_DPCM_RGSTRS_OFFSET        offsetof(I8080APUMapperPrivateContext_t, rgstrs.dpcm)

#define I8080_APU_MASTER_RGSTRS_ADDR        0
#define I8080_APU_PULSE_0_RGSTRS_ADDR       (I8080_APU_PULSE_0_RGSTRS_OFFSET - I8080_APU_MASTER_RGSTRS_OFFSET)
#define I8080_APU_PULSE_1_RGSTRS_ADDR       (I8080_APU_PULSE_1_RGSTRS_OFFSET - I8080_APU_MASTER_RGSTRS_OFFSET)
#define I8080_APU_TRIANGLE_0_RGSTRS_ADDR    (I8080_APU_TRIANGLE_0_RGSTRS_OFFSET - I8080_APU_MASTER_RGSTRS_OFFSET)
#define I8080_APU_TRIANGLE_1_RGSTRS_ADDR    (I8080_APU_TRIANGLE_1_RGSTRS_OFFSET - I8080_APU_MASTER_RGSTRS_OFFSET)
#define I8080_APU_NOISE_RGSTRS_ADDR         (I8080_APU_NOISE_RGSTRS_OFFSET - I8080_APU_MASTER_RGSTRS_OFFSET)
#define I8080_APU_DPCM_RGSTRS_ADDR          (I8080_APU_DPCM_RGSTRS_OFFSET - I8080_APU_MASTER_RGSTRS_OFFSET)

#define I8080_APU_END_ADDR                  sizeof(I8080APURegisters_t)

const I8080Addr_t I8080APUAddrOffsetMasterRegisters = I8080_APU_MASTER_RGSTRS_ADDR;
const I8080Addr_t I8080APUAddrOffsetPulse0Registers = I8080_APU_PULSE_0_RGSTRS_ADDR;
const I8080Addr_t I8080APUAddrOffsetPulse1Registers = I8080_APU_PULSE_1_RGSTRS_ADDR;
const I8080Addr_t I8080APUAddrOffsetTriangle0Registers = I8080_APU_TRIANGLE_0_RGSTRS_ADDR;
const I8080Addr_t I8080APUAddrOffsetTriangle1Registers = I8080_APU_TRIANGLE_1_RGSTRS_ADDR;
const I8080Addr_t I8080APUAddrOffsetNoiseRegisters = I8080_APU_NOISE_RGSTRS_ADDR;
const I8080Addr_t I8080APUAddrOffsetDPCMRegisters = I8080_APU_DPCM_RGSTRS_ADDR;

//

static const uint16_t I8080APUDutySequenceProfiles[12] = {
                        0b0100000000000000,     /*!<  6.25%  */
                        0b0110000000000000,     /*!< 12.50%  */
                        0b0111000000000000,     /*!< 18.75%  */
                        0b0111100000000000,     /*!< 25.00%  */
                        0b0111110000000000,     /*!< 31.25%  */
                        0b0111111000000000,     /*!< 37.50%  */
                        0b0111111100000000,     /*!< 43.75%  */
                        0b0111111110000000,     /*!< 50.00%  */
                        0b0111111111000000,     /*!< 56.25%  */
                        0b0111111111100000,     /*!< 62.50%  */
                        0b0111111111110000,     /*!< 68.75%  */
                        0b0111111111111000      /*!< 75.00%  */
                    };

static const int16_t I8080APUTriangleSteps[32] = {
                    15,  13,  11,   9,   7,   5,   3,   1,
                    -1,  -3,  -5,  -7,  -9, -11, -13, -15,
                   -15, -13, -11,  -9,  -7,  -5,  -3,  -1,
                     1,   3,   5,   7,   9,  11,  13,  15 };

static const int16_t I8080APUSawtoothSteps[32] = {
                    15,  14,  13,  12,  11,  10,   9,   8,
                     7,   6,   5,   4,   3,   2,   1,   0,
                    -1,  -2,  -3,  -4,  -5,  -6,  -7,  -8,
                    -9, -10, -11, -12, -13, -14, -15,  15 };

static const uint16_t I8080APUNoisePeriods[16] = {
    2, 3, 7, 19, 23, 29, 37, 41, 47, 53, 59, 67, 71, 79, 89, 97
};

//

static inline
void
I8080APUPulseChannelInit(
    I8080APUPulseChannelState_t         *ch,
    I8080APUPulseChannelRegisters_t     *rgstrs,
    I8080APUMasterRegisters_t           *master
)
{
    if ( (ch->is_enabled = rgstrs->is_enabled) ) {
        ch->envelope.tick = 0;
        if ( (ch->envelope.is_const = rgstrs->is_envlp_const) ) {
            ch->envelope.level = ( master->is_8bit_envelope ? 
                                        (rgstrs->envlp_level_lo << 8) :
                                        (rgstrs->envlp_level_lo | (rgstrs->envlp_level_hi << 8))
                                    );
        } else {
            ch->envelope.level = 0x4000;
            ch->envelope.is_looped = rgstrs->is_envlp_looped;
            ch->envelope.length = ( master->is_8bit_envelope ? 
                                        (rgstrs->envlp_level_lo) :
                                        (rgstrs->envlp_level_lo | (rgstrs->envlp_level_hi << 8))
                                    );
        }
        
        ch->frequency.tick = 0;
        ch->frequency.length = master->is_8bit_frequency ? 
                I8080APU8BitFreqTimerMap[rgstrs->freq_timer_lo] :
                (rgstrs->freq_timer_lo | (rgstrs->freq_timer_hi << 8));
        if ( (ch->frequency.is_swept = rgstrs->is_freq_swept) ) {
            ch->frequency.sweep_tick = 0;
            ch->frequency.sweep_shift = rgstrs->freq_shift;
            ch->frequency.sweep_period = rgstrs->freq_sweep_period;
        }
        
        ch->duration.length = master->is_8bit_duration ?
                (rgstrs->duration_lo * 7 + rgstrs->duration_lo / 2) :
                (rgstrs->duration_lo | (rgstrs->duration_hi << 8));
        ch->duration.is_fixed_duration = (ch->duration.length != 0);
        
        ch->duty_cycle.tick = 0;
        ch->duty_cycle.length = ch->frequency.length >> 4;
        ch->duty_cycle.waveform_profile = I8080APUDutySequenceProfiles[rgstrs->duty_cycle];
        ch->duty_cycle.waveform_bit = 0x8000;
    }
}

static inline
SInt16
I8080APUPulseChannelSample(
    I8080APUPulseChannelState_t *ch
)
{
    SInt16                      sample;
    
    /* Update the envelope if necessary: */
    if ( ! ch->envelope.is_const ) {
        if ( ++ch->envelope.tick == ch->envelope.length ) {
            ch->envelope.tick = 0;
            if ( ch->envelope.level < 0x20 ) {
                ch->envelope.level = 0;
            } else {
                ch->envelope.level -= 0x20;
            }
            if ( ! ch->envelope.level && ch->envelope.is_looped ) {
                ch->envelope.level = 0x4000;
            }
        }
    }
    if ( (sample = ch->envelope.level) ) {
        /* Update the frequency counter: */
        if ( (ch->frequency.tick += 0x10) >= ch->frequency.length ) {
            ch->frequency.tick -= ch->frequency.length;
            if ( ch->frequency.is_swept && (++ch->frequency.sweep_tick == (ch->frequency.sweep_period & 0x7F)) ) {
                int32_t             delta = ch->frequency.length >> ch->frequency.sweep_shift;
                
                ch->frequency.sweep_tick = 0;
                if ( ch->frequency.sweep_period & 0x80 ) delta = -delta;
                delta += ch->frequency.length;
                if ( delta < 0x80 ) delta = 0x80;
                else if ( delta > INT16_MAX ) delta = INT16_MAX;
                ch->frequency.length = delta;
                ch->duty_cycle.length = delta / 16;
            }
        }
        /* Update the duty cycle: */
        if ( (ch->duty_cycle.tick += 0x10) >= ch->duty_cycle.length ) {
            ch->duty_cycle.tick -= ch->duty_cycle.length;
            if ( (ch->duty_cycle.waveform_bit >>= 1) == 0 ) ch->duty_cycle.waveform_bit = 0x8000;
        }
        /* Mute the sample if the waveform is low: */
        if ( (ch->duty_cycle.waveform_profile & ch->duty_cycle.waveform_bit) == 0 ) sample = (sample ^ 0xFFFF);
    }
    return sample;
}

//

static inline
void
I8080APUTriangleChannelInit(
    I8080APUTriangleChannelState_t      *ch,
    I8080APUTriangleChannelRegisters_t  *rgstrs,
    I8080APUMasterRegisters_t           *master
)
{
    if ( (ch->is_enabled = rgstrs->is_enabled) ) {
        ch->envelope.tick = 0;
        if ( (ch->envelope.is_const = rgstrs->is_envlp_const) ) {
            ch->envelope.level = ( master->is_8bit_envelope ? 
                                        (rgstrs->envlp_level_lo << 8) :
                                        (rgstrs->envlp_level_lo | (rgstrs->envlp_level_hi << 8))
                                    );
        } else {
            ch->envelope.level = 0x4000;
            ch->envelope.is_looped = rgstrs->is_envlp_looped;
            ch->envelope.length = ( master->is_8bit_envelope ? 
                                        (rgstrs->envlp_level_lo) :
                                        (rgstrs->envlp_level_lo | (rgstrs->envlp_level_hi << 8))
                                    );
        }
        
        ch->frequency.tick = 0;
        ch->frequency.length = ( master->is_8bit_frequency ? 
                I8080APU8BitFreqTimerMap[rgstrs->freq_timer_lo] :
                (rgstrs->freq_timer_lo | (rgstrs->freq_timer_hi << 8)) ) >> 5;
        
        ch->duration.length = master->is_8bit_duration ?
                (rgstrs->duration_lo * 7 + rgstrs->duration_lo / 2) :
                (rgstrs->duration_lo | (rgstrs->duration_hi << 8));
        ch->duration.is_fixed_duration = (ch->duration.length != 0);
        
        ch->sequence.sequence = rgstrs->is_sawtooth ? I8080APUSawtoothSteps : I8080APUTriangleSteps;
        ch->sequence.step = 0;
    }
}

static inline
SInt16
I8080APUTriangleChannelSample(
    I8080APUTriangleChannelState_t  *ch
)
{
    SInt32                          sample;
    
    /* Update the envelope if necessary: */
    if ( ! ch->envelope.is_const ) {
        if ( ++ch->envelope.tick == ch->envelope.length ) {
            ch->envelope.tick = 0;
            if ( ch->envelope.level < 0x20 ) {
                ch->envelope.level = 0;
            } else {
                ch->envelope.level -= 0x20;
            }
            if ( ! ch->envelope.level && ch->envelope.is_looped ) {
                ch->envelope.level = 0x4000;
            }
        }
    }
    if ( (sample = ch->envelope.level) ) {
        /* Update the frequency counter: */
        if ( (ch->frequency.tick += 0x10) >= ch->frequency.length ) {
            ch->frequency.tick -= ch->frequency.length;
            ch->sequence.step = (ch->sequence.step + 1) % 32;
        }
        /* Generate sample: */
        sample = (sample * ch->sequence.sequence[ch->sequence.step]) / 15;
    }
    return (SInt16)sample;
}

//

static inline
void
I8080APUNoiseChannelInit(
    I8080APUNoiseChannelState_t         *ch,
    I8080APUNoiseChannelRegisters_t     *rgstrs,
    I8080APUMasterRegisters_t           *master
)
{
    if ( (ch->is_enabled = rgstrs->is_enabled) ) {
        ch->envelope.tick = 0;
        if ( (ch->envelope.is_const = rgstrs->is_envlp_const) ) {
            ch->envelope.level = ( master->is_8bit_envelope ? 
                                        (rgstrs->envlp_level_lo << 8) :
                                        (rgstrs->envlp_level_lo | (rgstrs->envlp_level_hi << 8))
                                    );
        } else {
            ch->envelope.level = 0x4000;
            ch->envelope.is_looped = rgstrs->is_envlp_looped;
            ch->envelope.length = ( master->is_8bit_envelope ? 
                                        (rgstrs->envlp_level_lo) :
                                        (rgstrs->envlp_level_lo | (rgstrs->envlp_level_hi << 8))
                                    );
        }
        
        ch->duration.length = master->is_8bit_duration ?
                (rgstrs->duration_lo * 7 + rgstrs->duration_lo / 2) :
                (rgstrs->duration_lo | (rgstrs->duration_hi << 8));
        ch->duration.is_fixed_duration = (ch->duration.length != 0);
        
        ch->noise.period = I8080APUNoisePeriods[rgstrs->period];
        ch->noise.is_short_loop = rgstrs->is_short_loop;
        srandom(time(NULL));
        ch->noise.shift_register = random() & INT16_MAX;
        ch->noise.tick = 0;
    }
}

static inline
SInt16
I8080APUNoiseChannelSample(
    I8080APUNoiseChannelState_t     *ch
)
{
    SInt16                          sample;

    /* Update the envelope if necessary: */
    if ( ! ch->envelope.is_const ) {
        if ( ++ch->envelope.tick == ch->envelope.length ) {
            ch->envelope.tick = 0;
            if ( ch->envelope.level < 0x20 ) {
                ch->envelope.level = 0;
            } else {
                ch->envelope.level -= 0x20;
            }
            if ( ! ch->envelope.level && ch->envelope.is_looped ) {
                ch->envelope.level = 0x4000;
            }
        }
    }
    if ( (sample = ch->envelope.level) ) {
        if ( ++ch->noise.tick == ch->noise.period ) {
            bool            bit0 = ch->noise.shift_register & 0x1;
            bool            tgt = 0x1 & ((ch->noise.is_short_loop) ?
                                        (ch->noise.shift_register >> 6) :
                                        (ch->noise.shift_register >> 1));
            ch->noise.shift_register = (ch->noise.shift_register >> 1) | ((bit0 ^ tgt) << 14);
            ch->noise.tick = 0;
        }
        if ( ch->noise.shift_register & 0x1 ) sample = 0;
    }
    return sample;
}

//

static inline
int
I8080APUDPCMChannelPreFetch(
    I8080APUDPCMChannelState_t          *ch
)
{
    int                                 n_fetch = 0;
    
    ch->sample_mask = 0x1;
    ch->sample_ptr = ch->sample_end = ch->samples;
    while ( (n_fetch < 16) && ch->cur_nbytes ) *ch->sample_end++ = I8080MemRead(ch->sysmem, ch->cur_addr++), ch->cur_nbytes--, n_fetch++;
    return n_fetch;
}

static inline
void
I8080APUDPCMChannelInit(
    I8080APUDPCMChannelState_t          *ch,
    I8080APUDPCMChannelRegisters_t      *rgstrs,
    I8080APUMasterRegisters_t           *master
)
{
    if ( (ch->is_enabled = rgstrs->is_enabled) ) {
        ch->is_looped = rgstrs->is_looped;
        ch->tick = 0;
        ch->sample = 0;
        ch->length = 1 + rgstrs->rate;
        ch->cur_addr = ch->addr = rgstrs->address_lo | (rgstrs->address_hi << 8);
        ch->cur_nbytes = ch->nbytes = rgstrs->length_lo | (rgstrs->length_hi << 8);
        I8080APUDPCMChannelPreFetch(ch);
    }
}

static inline
SInt16
I8080APUDPCMChannelSample(
    I8080APUDPCMChannelState_t      *ch
)
{
    SInt16                          sample;
    
    if ( ++ch->tick == ch->length ) {
        if ( *ch->sample_ptr & ch->sample_mask ) ch->sample++;
        else ch->sample--;
        ch->tick = 0;
        
        if ( (ch->sample_mask <<= 1) == 0 ) {
            if ( (++ch->sample_ptr == ch->sample_end) ) {
                /* Prefetch next set of sample bytes; if nothing is left, that's the end */
                if ( I8080APUDPCMChannelPreFetch(ch) == 0 ) {
                    if ( ch->is_looped ) {
                        /* reset the sample sequence */
                        ch->cur_addr = ch->addr, ch->cur_nbytes = ch->nbytes;
                        ch->sample = 0;
                        I8080APUDPCMChannelPreFetch(ch);
                    } else {
                        /* mute the channel */
                        ch->is_enabled = false;
                    }
                }
            } else {
                /* Next byte in prefetch buffer */
                ch->sample_mask = 0x1;
            }
        }
    }
    sample = ch->sample;
    if ( sample > 16 ) sample = 16;
    else if ( sample < -17 ) sample = -17;
    return sample * 0x400;
}
   
//

static inline
void
I8080APUInit(
    I8080APUState_t                     *apu,
    I8080APURegisters_t                 *rgstrs
)
{
    apu->frame_tick = 0;
    apu->max_level = INT16_MAX & ( rgstrs->master.is_8bit_envelope ?
                            (rgstrs->master.master_level_lo << 8) :
                            (rgstrs->master.master_level_lo | (rgstrs->master.master_level_hi << 8))
                        );
    I8080APUPulseChannelInit(&apu->pulse_0, &rgstrs->pulse_0, &rgstrs->master);
    I8080APUPulseChannelInit(&apu->pulse_1, &rgstrs->pulse_1, &rgstrs->master);
    I8080APUTriangleChannelInit(&apu->triangle_0, &rgstrs->triangle_0, &rgstrs->master);
    I8080APUTriangleChannelInit(&apu->triangle_1, &rgstrs->triangle_1, &rgstrs->master);
    I8080APUNoiseChannelInit(&apu->noise, &rgstrs->noise, &rgstrs->master);
    I8080APUDPCMChannelInit(&apu->dpcm, &rgstrs->dpcm, &rgstrs->master);
}

static inline
SInt16
I8080APUSample(
    I8080APUState_t     *apu
)
{
    SInt64              mixed_sample = 0;
    
    if ( ++apu->frame_tick == (44100 / 60) ) {
        apu->frame_tick = 0;
        if ( apu->pulse_0.is_enabled && apu->pulse_0.duration.is_fixed_duration && (--apu->pulse_0.duration.length == 0) ) apu->pulse_0.is_enabled = false;
        if ( apu->pulse_1.is_enabled && apu->pulse_1.duration.is_fixed_duration && (--apu->pulse_1.duration.length == 0) ) apu->pulse_1.is_enabled = false;
        if ( apu->triangle_0.is_enabled && apu->triangle_0.duration.is_fixed_duration && (--apu->triangle_0.duration.length == 0) ) apu->triangle_0.is_enabled = false;
        if ( apu->triangle_1.is_enabled && apu->triangle_1.duration.is_fixed_duration && (--apu->triangle_1.duration.length == 0) ) apu->triangle_1.is_enabled = false;
        if ( apu->noise.is_enabled && apu->noise.duration.is_fixed_duration && (--apu->noise.duration.length == 0) ) apu->noise.is_enabled = false;
    }
    if ( apu->pulse_0.is_enabled && apu->pulse_0.envelope.level && (apu->pulse_0.frequency.length >= 8) ) mixed_sample += I8080APUPulseChannelSample(&apu->pulse_0);
    if ( apu->pulse_1.is_enabled && apu->pulse_1.envelope.level && (apu->pulse_1.frequency.length >= 8) ) mixed_sample += I8080APUPulseChannelSample(&apu->pulse_1);
    if ( apu->triangle_0.is_enabled && apu->triangle_0.envelope.level && (apu->triangle_0.frequency.length >= 8) ) mixed_sample += I8080APUTriangleChannelSample(&apu->triangle_0);
    if ( apu->triangle_1.is_enabled && apu->triangle_1.envelope.level && (apu->triangle_1.frequency.length >= 8) ) mixed_sample += I8080APUTriangleChannelSample(&apu->triangle_1);
    if ( apu->noise.is_enabled && apu->noise.envelope.level ) mixed_sample += I8080APUNoiseChannelSample(&apu->noise);
    if ( apu->dpcm.is_enabled ) mixed_sample += I8080APUDPCMChannelSample(&apu->dpcm);
    if ( mixed_sample ) {
        mixed_sample = ((mixed_sample * apu->max_level) / 6) / INT16_MAX;
        if ( mixed_sample < -apu->max_level ) mixed_sample = -apu->max_level;
        else if ( mixed_sample > apu->max_level ) mixed_sample = apu->max_level;
    }
    return (SInt16)mixed_sample;
}

//

void
I8080APUAudioQueueInitBuffer(
    AudioQueueRef           queue,
    AudioQueueBufferRef     buffer
)
{
    memset(buffer->mAudioData, 0, buffer->mAudioDataBytesCapacity);
    buffer->mAudioDataByteSize = buffer->mAudioDataBytesCapacity;
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

//

void
I8080APUAudioQueueFillBuffer(
    void                    *context,
    AudioQueueRef           queue,
    AudioQueueBufferRef     buffer
)
{
    I8080APUState_t         *apu_state = (I8080APUState_t*)context;
    SInt16                  *samples = (SInt16*)buffer->mAudioData;
    UInt32                  n_ticks = buffer->mAudioDataBytesCapacity / sizeof(SInt16);

    while ( n_ticks-- ) *samples++ = I8080APUSample(apu_state);
    buffer->mAudioDataByteSize = buffer->mAudioDataBytesCapacity;
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

//

static
bool
I8080APUContextReset(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080APUMapperPrivateContext_t *apu = (I8080APUMapperPrivateContext_t*)context;
    
    memset(&apu->rgstrs, 0, sizeof(apu->rgstrs));
    memset(&apu->apu_state, 0, sizeof(apu->apu_state));
    apu->apu_state.dpcm.sysmem = mem;
    if ( ! apu->audio_queue ) {
        int                         buffer_idx;
        AudioStreamBasicDescription audio_format = {
                                                .mSampleRate = 44100.0,
                                                .mFormatID = kAudioFormatLinearPCM,
                                                .mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked,
                                                .mFramesPerPacket = 1,
                                                .mChannelsPerFrame = 1,
                                                .mBitsPerChannel = 16,
                                                .mBytesPerPacket = 2,
                                                .mBytesPerFrame = 2
                                            };
        AudioQueueNewOutput(&audio_format,
            I8080APUAudioQueueFillBuffer,
            &apu->apu_state,
            NULL,
            NULL,
            0,
            &apu->audio_queue
        );
        for ( buffer_idx = 0; buffer_idx < I8080_APU_BUFFER_COUNT; buffer_idx++ ) {
            AudioQueueAllocateBuffer(apu->audio_queue, I8080_APU_BUFFER_SIZE, &apu->audio_buffers[buffer_idx]);
            I8080APUAudioQueueInitBuffer(apu->audio_queue, apu->audio_buffers[buffer_idx]);
        }
    }
    AudioQueueStart(apu->audio_queue, NULL);
    return true;
}

//
    
static
void
I8080APUContextShutdown(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080APUMapperPrivateContext_t *apu = (I8080APUMapperPrivateContext_t*)context;
    
    if ( ! apu->audio_queue ) {
        // The sample buffers will be destroyed, as well:
        AudioQueueStop(apu->audio_queue, true);
        AudioQueueDispose(apu->audio_queue, true);
        apu->audio_queue = NULL;
    }
}

//

static
bool
I8080APUContextRead(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             *byte,
    const void          *context
)
{
    I8080APUMapperPrivateContext_t *apu = (I8080APUMapperPrivateContext_t*)context;
    I8080Addr_t             apu_addr = addr - range.base;
    
    if ( apu_addr < I8080_APU_END_ADDR ) {
        *byte = *(((uint8_t*)&apu->rgstrs) + apu_addr);
        return true;
    }
    return false;
}

//

static
bool
I8080APUContextWrite(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             byte,
    const void          *context
)
{
    I8080APUMapperPrivateContext_t *apu = (I8080APUMapperPrivateContext_t*)context;
    I8080Addr_t             apu_addr = addr - range.base;
    
    /* Register? */
    if ( apu_addr < I8080_APU_END_ADDR ) {
        *(((uint8_t*)&apu->rgstrs) + apu_addr) = byte;
        switch ( apu_addr ) {
            case I8080_APU_MASTER_RGSTRS_ADDR:
                /* A change to the master register triggers a reload of all channels: */
                I8080APUInit(&apu->apu_state, &apu->rgstrs);
                break;
            case I8080_APU_MASTER_RGSTRS_ADDR+1:
            case I8080_APU_MASTER_RGSTRS_ADDR+2:
                apu->apu_state.max_level = INT16_MAX & ( apu->rgstrs.master.is_8bit_envelope ?
                        (apu->rgstrs.master.master_level_lo << 8) :
                        (apu->rgstrs.master.master_level_lo | (apu->rgstrs.master.master_level_hi << 8))
                    );
                break;
            case I8080_APU_PULSE_0_RGSTRS_ADDR:
                /* Reload pulse 0: */
                I8080APUPulseChannelInit(&apu->apu_state.pulse_0, &apu->rgstrs.pulse_0, &apu->rgstrs.master);
                break;
            case I8080_APU_PULSE_1_RGSTRS_ADDR:
                /* Reload pulse 1: */
                I8080APUPulseChannelInit(&apu->apu_state.pulse_1, &apu->rgstrs.pulse_1, &apu->rgstrs.master);
                break;
            case I8080_APU_TRIANGLE_0_RGSTRS_ADDR:
                /* Reload triangle 0: */
                I8080APUTriangleChannelInit(&apu->apu_state.triangle_0, &apu->rgstrs.triangle_0, &apu->rgstrs.master);
                break;
            case I8080_APU_TRIANGLE_1_RGSTRS_ADDR:
                /* Reload triangle 1: */
                I8080APUTriangleChannelInit(&apu->apu_state.triangle_1, &apu->rgstrs.triangle_1, &apu->rgstrs.master);
                break;
            case I8080_APU_NOISE_RGSTRS_ADDR:
                /* Reload noise: */
                I8080APUNoiseChannelInit(&apu->apu_state.noise, &apu->rgstrs.noise, &apu->rgstrs.master);
                break;
            case I8080_APU_DPCM_RGSTRS_ADDR:
                /* Reload DPCM: */
                I8080APUDPCMChannelInit(&apu->apu_state.dpcm, &apu->rgstrs.dpcm, &apu->rgstrs.master);
                break;
        }
        return true;
    }
    return false;
}

//

const I8080MemMapperCallbacks __I8080APUMapperCallbacks = {
            .mapper_name = "audio-processing-unit",
            .rewrite_addr = NULL,
            .read = I8080APUContextRead,
            .write = I8080APUContextWrite,
            .reset = I8080APUContextReset,
            .shutdown = I8080APUContextShutdown
        };
const I8080MemMapperCallbacks *I8080APUMapperCallbacks = &__I8080APUMapperCallbacks;

const I8080Addr_t I8080APUMapperAddressRangeLength = sizeof(((I8080APUMapperPrivateContext_t*)NULL)->rgstrs);

//

I8080APUMapperContextPtr
I8080APUMapperContextCreate()
{
    I8080APUMapperPrivateContext_t  *new_apu = (I8080APUMapperPrivateContext_t*)calloc(1, sizeof(I8080APUMapperPrivateContext_t));
    
    return (I8080APUMapperContextPtr)new_apu;
}

//

uint8_t
I8080APUMapperContextGetRegister(
    I8080APUMapperContextPtr    apu,
    I8080APURegisterIndex_t     rgstr_idx
)
{
    uint8_t                     byte = 0;
    if ( rgstr_idx < kI8080APURegisterIndexMax ) I8080APUContextRead(NULL, I8080AddrRangeCreate(0, I8080APUMapperAddressRangeLength), rgstr_idx, &byte, apu);
    return byte;
}

//

void
I8080APUMapperContextSetRegister(
    I8080APUMapperContextPtr    apu,
    I8080APURegisterIndex_t     rgstr_idx,
    uint8_t                     byte
)
{
    if ( rgstr_idx < kI8080APURegisterIndexMax ) I8080APUContextWrite(NULL, I8080AddrRangeCreate(0, I8080APUMapperAddressRangeLength), rgstr_idx, byte, apu);
}
