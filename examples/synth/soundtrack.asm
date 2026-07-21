
TRK_0:          DW      TRK_0_LEAD
                DW      TRK_0_CHORD
                DW      TRK_0_BASS
                DW      TRK_0_PRCSN

TRK_0_LEAD:     DB      MUSEQ_TRK_CMD_NXC_N
                DW      CLIP_LEAD_0
                DB      2
                        
                DB      MUSEQ_TRK_CMD_NXC_1
                DW      CLIP_LEAD_1
                
                DB      MUSEQ_TRK_CMD_JMP
                DW      TRK_0_LEAD
                
TRK_0_CHORD:    DB      MUSEQ_TRK_CMD_NXC_N
                DW      CLIP_CHORD_0
                DB      2
                        
                DB      MUSEQ_TRK_CMD_NXC_1
                DW      CLIP_CHORD_1
                
                DB      MUSEQ_TRK_CMD_JMP
                DW      TRK_0_CHORD
                
TRK_0_BASS:     DB      MUSEQ_TRK_CMD_NXC_N
                DW      CLIP_BASS_0
                DB      2
                        
                DB      MUSEQ_TRK_CMD_NXC_1
                DW      CLIP_BASS_1
                
                DB      MUSEQ_TRK_CMD_JMP
                DW      TRK_0_BASS

TRK_0_PRCSN:    DB      MUSEQ_TRK_CMD_NXC_N
                DW      CLIP_BLNK_MEASR
                DB      2

                DB      MUSEQ_TRK_CMD_NXC_N
                DW      CLIP_PRCSN_0
                DB      3
                        
                DB      MUSEQ_TRK_CMD_NXC_1
                DW      CLIP_PRCSN_1
                
                DB      MUSEQ_TRK_CMD_JMP
                DW      TRK_0_PRCSN


CLIP_LEAD_0:    DB      MUSEQ_CLIP_CTRL_P_FREQ|MUSEQ_CLIP_CTRL_P_ENVELOPE|MUSEQ_CLIP_CTRL_P_STATE
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX, MUSEQ_CLIP_DECAY_PORTATO, 10000000b
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D4_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E5_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_8 - 1)
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_EOS

CLIP_LEAD_1:    DB      MUSEQ_CLIP_CTRL_P_FREQ|MUSEQ_CLIP_CTRL_P_ENVELOPE|MUSEQ_CLIP_CTRL_P_STATE
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F4_IDX, MUSEQ_CLIP_DECAY_PORTATO, 10000000b
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_8 - 1)
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C5_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_8 - 1)
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C5_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_8 - 1)
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D4_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_8 - 1)
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D4_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_8 - 1)
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C5_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_8 - 1)
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C5_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_GSHARP4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E5_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D5_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_GSHARP4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 + MUSEQ_DUR_1_32 - 1)
                
                DB      MUSEQ_CLIP_CTRL_EOS

CLIP_CHORD_0:   DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_32 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ|MUSEQ_CLIP_CTRL_P_ENVELOPE|MUSEQ_CLIP_CTRL_P_STATE
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX, 080h, 10000000b
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX

                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_C4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D4_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E5_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_8 - 1)
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_B4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G4_IDX
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E4_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_EOS

CLIP_CHORD_1:   DB      MUSEQ_CLIP_CTRL_P_FREQ|MUSEQ_CLIP_CTRL_P_ENVELOPE|MUSEQ_CLIP_CTRL_P_STATE
                DB          (MUSEQ_DUR_1 - 1), APU_NOTE_G4_IDX, 080h, 10000000b
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1 - 1), APU_NOTE_G4_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1 - 1), APU_NOTE_C5_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_P_FREQ
                DB          (MUSEQ_DUR_1 - 1), APU_NOTE_E4_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_EOS

CLIP_BASS_0:    DB      MUSEQ_CLIP_CTRL_T_FREQ|MUSEQ_CLIP_CTRL_T_ENVELOPE|MUSEQ_CLIP_CTRL_T_STATE
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX, 040h, 11000000b
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_F2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_G2_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_A2_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_E2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D2_IDX
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_8 - 1), APU_NOTE_D2_IDX
                
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                
                DB      MUSEQ_CLIP_CTRL_EOS

CLIP_BASS_1:    DB      MUSEQ_CLIP_CTRL_T_FREQ|MUSEQ_CLIP_CTRL_T_ENVELOPE|MUSEQ_CLIP_CTRL_T_STATE
                DB          (MUSEQ_DUR_1_4 - 1), APU_NOTE_F2_IDX, 040h, 11000000b
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_4 - 1)
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_4 - 1), APU_NOTE_F2_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 + MUSEQ_DUR_1_4 - 1)
                
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_4 - 1), APU_NOTE_G2_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_4 - 1)
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_4 - 1), APU_NOTE_G2_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 + MUSEQ_DUR_1_4 - 1)
                
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_4 - 1), APU_NOTE_C2_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_4 - 1)
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_4 - 1), APU_NOTE_C2_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 + MUSEQ_DUR_1_4 - 1)
                
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_4 - 1), APU_NOTE_E2_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_4 - 1)
                DB      MUSEQ_CLIP_CTRL_T_FREQ
                DB          (MUSEQ_DUR_1_4 - 1), APU_NOTE_E2_IDX
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 + MUSEQ_DUR_1_4 + MUSEQ_DUR_1_32 - 1)
                
                DB      MUSEQ_CLIP_CTRL_EOS
                
CLIP_PRCSN_0:   MUSEQ_CLIP_CHH_1_2
                MUSEQ_CLIP_CHH_1_2
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_2 - 1)
                DB      MUSEQ_CLIP_CTRL_EOS

CLIP_PRCSN_1:   MUSEQ_CLIP_CHH_1_2
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_4 - 1)
                MUSEQ_CLIP_OHH_1_2
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1_4 - 1)
                DB      MUSEQ_CLIP_CTRL_EOS

CLIP_BLNK_BAR:  DB      MUSEQ_CLIP_CTRL_REST
                DB          (MUSEQ_DUR_1 + MUSEQ_DUR_1_2 - 1)
                DB      MUSEQ_CLIP_CTRL_EOS

CLIP_BLNK_MEASR:DB      MUSEQ_CLIP_CTRL_REST
                DB          (3 * MUSEQ_DUR_1 - 1)
                DB      MUSEQ_CLIP_CTRL_REST
                DB          (3 * MUSEQ_DUR_1 - 1)
                DB      MUSEQ_CLIP_CTRL_EOS
