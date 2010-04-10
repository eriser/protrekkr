// ------------------------------------------------------
// Protrekkr
// Based on Juan Antonio Arguelles Rius's NoiseTrekker.
//
// Copyright (C) 2008-2010 Franck Charlet.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL FRANCK CHARLET OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
// ------------------------------------------------------

// ------------------------------------------------------
// Includes
#include "../extralibs/zlib-1.2.3/zlib.h"

#include "../include/ptk.h"
#include "include/files.h"
#include "include/samples_pack.h"
#include "include/mods.h"
#include "include/patterns.h"
#include "include/reverbs.h"
#include "include/303s.h"
#include "include/synths.h"
#include "include/ptps.h"
#include "../ui/include/misc_draw.h"
#include "../editors/include/editor_synth.h"
#include "../editors/include/editor_diskio.h"
#include "../editors/include/editor_reverb.h"
#include "../editors/include/editor_instrument.h"
#include "../editors/include/editor_sequencer.h"
#include "../editors/include/editor_setup.h"
#include "../editors/include/editor_303.h"
#include "../editors/include/editor_sample.h"
#include "../editors/include/editor_fx_setup.h"
#include "../editors/include/editor_track_fx.h"
#include "../editors/include/editor_track.h"
#include "../editors/include/editor_pattern.h"

#include "../../release/distrib/replay/lib/include/endianness.h"

// ------------------------------------------------------
// Variables
#if !defined(__WINAMP__)
extern REQUESTER Overwrite_Requester;
extern char OverWrite_Name[1024];
#endif

extern SynthParameters PARASynth[128];

extern int Beveled;

int Mod_Length;
int Mod_Simulate;
Uint8 *Mod_Memory;
int Mod_Mem_Pos;
int Final_Mod_Length;

// ------------------------------------------------------
// Functions
int Read_Mod_Data(void *Datas, int Unit, int Length, FILE *Handle);
int Read_Mod_Data_Swap(void *Datas, int Unit, int Length, FILE *Handle);
short *Unpack_Sample(FILE *FileHandle, int Dest_Length, char Pack_Type, int BitRate);
void Swap_Short_Buffer(short *buffer, int size);
short *Swap_New_Sample(short *buffer, int sample, int bank);

// ------------------------------------------------------
// Prepare the tracker interface once a module has been loaded
void Init_Tracker_Context_After_ModLoad(void)
{
#if !defined(__WINAMP__)
    Track_Under_Caret = 0;
    Current_Instrument = 0;
    Column_Under_Caret = 0;
#endif

    Pattern_Line = 0;
    Pattern_Line_Visual = 0;
    Song_Position = 0;
    Song_Position_Visual = 0;

    Final_Mod_Length = 0;

    Post_Song_Init();

#if !defined(__WINAMP__)
    Draw_Scope();
    gui_track = 0;
#endif

    lchorus_counter = MIX_RATE;
    rchorus_counter = MIX_RATE;
    lchorus_counter2 = MIX_RATE - lchorus_delay;
    rchorus_counter2 = MIX_RATE - rchorus_delay;
    Initreverb();

#if defined(PTK_LIMITER_MASTER)
    Mas_Compressor_Set_Variables_Master(mas_comp_threshold_Master,
                                        mas_comp_ratio_Master);
#endif

#if defined(PTK_LIMITER_TRACKS)
    int i;
    for(i = 0; i < MAX_TRACKS; i++)
    {
        Mas_Compressor_Set_Variables_Track(i,
                                           mas_comp_threshold_Track[i],
                                           mas_comp_ratio_Track[i]);
    }
#endif

    Reset_Song_Length();

#if !defined(__WINAMP__)
    Display_Song_Length();

    Scopish = SCOPE_ZONE_MOD_DIR;
    //Draw_Scope_Files_Button();

    Reset_Tracks_To_Render();

    Refresh_UI_Context();
#endif

}

// ------------------------------------------------------
// Load a module file
int LoadPtk(char *FileName)
{
    int Ye_Old_Phony_Value;
    int New_adsr = FALSE;
    int New_Comp = FALSE;
    int Portable = FALSE;
    int Poly = FALSE;
    int Multi = FALSE;
    int Sel_Interpolation = FALSE;
    int New_Reverb = FALSE;
    int Env_Modulation = FALSE;
    int New_Env = FALSE;
    int Fx2 = FALSE;
    char Comp_Flag;
    int i;
    int j;
    int k;
    int Old_Ntk;
    int Ntk_Beta;
    unsigned char *TmpPatterns;
    unsigned char *TmpPatterns_Tracks;
    unsigned char *TmpPatterns_Notes;
    unsigned char *TmpPatterns_Rows;
    int Old_Bug = TRUE;
    int new_disto = FALSE;
    int Pack_Scheme = FALSE;
    int Mp3_Scheme = FALSE;
    int Compress_Tracks = FALSE;
    int tps_pos;
    int tps_trk;
    int twrite;
    int fake_value;
    int Packed_Size;
    int UnPacked_Size;
    int Flanger_Bug = FALSE;
    unsigned char *Packed_Module = NULL;

    Mod_Simulate = LOAD_READ;
    Mod_Mem_Pos = 0;
    Mod_Memory = NULL;

    FILE *in;
    in = fopen(FileName, "rb");
    Old_Ntk = FALSE;
    Ntk_Beta = FALSE;

    if(in != NULL)
    {

#if !defined(__WINAMP__)
        Status_Box("Attempting to load the song file...");
#endif

        Songplaying = FALSE;

        // Reading and checking extension...
        char extension[10];
        Read_Data(extension, sizeof(char), 9, in);

        switch(extension[7])
        {

            case 'K':
                Compress_Tracks = TRUE;
            case 'J':
                Flanger_Bug = TRUE;
            case 'I':
                Fx2 = TRUE;
            case 'H':
                New_Env = TRUE;
            case 'G':
                Env_Modulation = TRUE;
            case 'F':
                New_Reverb = TRUE;
            case 'E':
                Sel_Interpolation = TRUE;
            case 'D':
                Mp3_Scheme = TRUE;
            case 'C':
                Multi = TRUE;
            case 'B':
                Poly = TRUE;
            case 'A':
                Portable = TRUE;
            case '9':
                Mod_Simulate = LOAD_READMEM;
            case '8':
                New_Comp = TRUE;
            case '7':
                New_adsr = TRUE;
            case '6':
                Pack_Scheme = TRUE;
            case '5':
                new_disto = TRUE;
            case '4':
                Old_Bug = FALSE;
            case '3':
                goto Read_Mod_File;

            // Old noisetrekker
            case '2':
                Old_Ntk = TRUE;

            // Noisetrekker Beta (1.6)
            case '1':
                Ntk_Beta = TRUE;
        }

Read_Mod_File:

#if !defined(__WINAMP__)
        Status_Box("Loading song -> Header...");
#endif
        Free_Samples();

#if defined(PTK_LIMITER_MASTER)
        mas_comp_threshold_Master = 100.0f;
        mas_comp_ratio_Master = 0.0f;
#endif

#if defined(PTK_LIMITER_TRACKS)
        for(i = 0; i < MAX_TRACKS; i++)
        {
            mas_comp_threshold_Track[i] = 100.0f;
            mas_comp_ratio_Track[i] = 0.0f;
        }
#endif

#if !defined(__WINAMP__)
        allow_save = TRUE;
#endif

        Clear_Patterns_Pool();

#if !defined(__NO_MIDI__) && !defined(__WINAMP__)
        Midi_Reset();
#endif

        init_sample_bank();
        Pre_Song_Init();

        // Load the module into memory and depack it
        if(Mod_Simulate == LOAD_READMEM)
        {
            Packed_Size = Get_File_Size(in) - 9;
            Packed_Module = (unsigned char *) malloc(Packed_Size);
            if(Packed_Module)
            {
                if(Poly)
                {
                    Read_Data_Swap(&UnPacked_Size, sizeof(int), 1, in);
                }
                else
                {
                    UnPacked_Size = Packed_Size * 10;
                }
                Read_Data(Packed_Module, sizeof(char), Packed_Size, in);
                Mod_Memory = Depack_Data(Packed_Module, Packed_Size, UnPacked_Size);
                Mod_Mem_Pos = 0;
                free(Packed_Module);
            }
        }

        Read_Mod_Data(FileName, sizeof(char), 20, in);
        Read_Mod_Data(&nPatterns, sizeof(char), 1, in);

        Songtracks = MAX_TRACKS;
        Read_Mod_Data(&sLength, sizeof(char), 1, in);

        Use_Cubic = CUBIC_INT;

        if(Sel_Interpolation)
        {
            Read_Mod_Data(&Use_Cubic, sizeof(char), 1, in);
        }

        Read_Mod_Data(pSequence, sizeof(char), 256, in);

        Clear_Patterns_Pool();

        // Load the patterns rows infos
        if(!Ntk_Beta)
        {
            for(i = 0; i < MAX_ROWS; i++)
            {
                Read_Mod_Data_Swap(&patternLines[i], sizeof(short), 1, in);
            }
        }

        // Multi notes
        if(Multi)
        {
            Read_Mod_Data(Channels_MultiNotes, sizeof(char), MAX_TRACKS, in);
        }

        // Load the patterns data
        int Bytes_Per_Track = PATTERN_BYTES;
        if(!Multi)
        {
            Bytes_Per_Track = 6;
        }

        TmpPatterns = RawPatterns;
        for(int pwrite = 0; pwrite < nPatterns; pwrite++)
        {
            TmpPatterns_Rows = TmpPatterns + (pwrite * PATTERN_LEN);
            int Rows_To_Read = MAX_ROWS;
            if(Ntk_Beta) Rows_To_Read = 64;

            for(j = 0; j < Rows_To_Read; j++)
            {
                // Bytes / track
                for(k = 0; k < Songtracks; k++)
                {
                    // Tracks
                    TmpPatterns_Tracks = TmpPatterns_Rows + (k * PATTERN_BYTES);
                    // Rows
                    TmpPatterns_Notes = TmpPatterns_Tracks + (j * PATTERN_ROW_LEN);
                    if(Multi)
                    {
                        for(i = 0; i < MAX_POLYPHONY; i++)
                        {
                            Read_Mod_Data(TmpPatterns_Notes + PATTERN_NOTE1 + (i * 2), sizeof(char), 1, in);
                            Read_Mod_Data(TmpPatterns_Notes + PATTERN_INSTR1 + (i * 2), sizeof(char), 1, in);
                        }
                    }
                    else
                    {
                        Read_Mod_Data(TmpPatterns_Notes + PATTERN_NOTE1, sizeof(char), 1, in);
                        Read_Mod_Data(TmpPatterns_Notes + PATTERN_INSTR1, sizeof(char), 1, in);
                    }
                    Read_Mod_Data(TmpPatterns_Notes + PATTERN_VOLUME, sizeof(char), 1, in);
                    Read_Mod_Data(TmpPatterns_Notes + PATTERN_PANNING, sizeof(char), 1, in);
                    Read_Mod_Data(TmpPatterns_Notes + PATTERN_FX, sizeof(char), 1, in);
                    Read_Mod_Data(TmpPatterns_Notes + PATTERN_FXDATA, sizeof(char), 1, in);
                    if(Fx2)
                    {
                        Read_Mod_Data(TmpPatterns_Notes + PATTERN_FX2, sizeof(char), 1, in);
                        Read_Mod_Data(TmpPatterns_Notes + PATTERN_FXDATA2, sizeof(char), 1, in);
                    }

                }
            }
        }

#if !defined(__WINAMP__)
        Status_Box("Loading song -> Sample data...");
#endif
        for(int swrite = 0; swrite < MAX_INSTRS; swrite++)
        {
            Read_Mod_Data(&nameins[swrite], sizeof(char), 20, in);
            Read_Mod_Data(&Midiprg[swrite], sizeof(char), 1, in);
            Read_Mod_Data(&Synthprg[swrite], sizeof(char), 1, in);

            PARASynth[swrite].disto = 0;

            PARASynth[swrite].lfo1_attack = 0;
            PARASynth[swrite].lfo1_decay = 0;
            PARASynth[swrite].lfo1_sustain = 128;
            PARASynth[swrite].lfo1_release = 0x10000;

            PARASynth[swrite].lfo2_attack = 0;
            PARASynth[swrite].lfo2_decay = 0;
            PARASynth[swrite].lfo2_sustain = 128;
            PARASynth[swrite].lfo2_release = 0x10000;

            Read_Synth_Params(Read_Mod_Data, Read_Mod_Data_Swap, in, swrite,
                              new_disto, New_adsr, Portable, Env_Modulation, New_Env, Ntk_Beta);

            // Fix some very old Ntk bugs
            if(PARASynth[swrite].lfo1_period > 128) PARASynth[swrite].lfo1_period = 128;
            if(PARASynth[swrite].lfo2_period > 128) PARASynth[swrite].lfo2_period = 128;
            if(Old_Ntk)
            {
                if(PARASynth[swrite].ptc_glide < 1) PARASynth[swrite].ptc_glide = 64;
                if(PARASynth[swrite].glb_volume < 1) PARASynth[swrite].glb_volume = 64;
            }

            // Compression type
            if(Pack_Scheme)
            {
                Read_Mod_Data(&SampleCompression[swrite], sizeof(char), 1, in);
                if(Mp3_Scheme)
                {
                    switch(SampleCompression[swrite])
                    {
                        case SMP_PACK_MP3:
                            Read_Mod_Data(&Mp3_BitRate[swrite], sizeof(char), 1, in);
                            break;

                        case SMP_PACK_AT3:
                            Read_Mod_Data(&At3_BitRate[swrite], sizeof(char), 1, in);
                            break;

                        case SMP_PACK_INTERNAL:
                            Read_Mod_Data(&Internal_Quality[swrite], sizeof(char), 1, in);
                            break;
                    }
                }
            }
            for(int slwrite = 0; slwrite < MAX_INSTRS_SPLITS; slwrite++)
            {
                Read_Mod_Data(&SampleType[swrite][slwrite], sizeof(char), 1, in);
                if(SampleType[swrite][slwrite])
                {
                    if(Old_Bug) Read_Mod_Data(&SampleName[swrite][slwrite], sizeof(char), 256, in);
                    else Read_Mod_Data(&SampleName[swrite][slwrite], sizeof(char), 64, in);
                    Read_Mod_Data(&Basenote[swrite][slwrite], sizeof(char), 1, in);

                    Read_Mod_Data_Swap(&LoopStart[swrite][slwrite], sizeof(int), 1, in);
                    Read_Mod_Data_Swap(&LoopEnd[swrite][slwrite], sizeof(int), 1, in);
                    Read_Mod_Data(&LoopType[swrite][slwrite], sizeof(char), 1, in);

                    Read_Mod_Data_Swap(&SampleLength[swrite][slwrite], sizeof(int), 1, in);
                    Read_Mod_Data(&Finetune[swrite][slwrite], sizeof(char), 1, in);
                    Read_Mod_Data_Swap(&Sample_Amplify[swrite][slwrite], sizeof(float), 1, in);
                    Read_Mod_Data_Swap(&FDecay[swrite][slwrite], sizeof(float), 1, in);

                    AllocateWave(swrite, slwrite, SampleLength[swrite][slwrite], 1, FALSE, NULL, NULL);
                    Read_Mod_Data(RawSamples[swrite][0][slwrite], sizeof(short), SampleLength[swrite][slwrite], in);
                    Swap_Sample(RawSamples[swrite][0][slwrite], swrite, slwrite);
                    *RawSamples[swrite][0][slwrite] = 0;

                    // Stereo flag
                    Read_Mod_Data(&SampleChannels[swrite][slwrite], sizeof(char), 1, in);
                    if(SampleChannels[swrite][slwrite] == 2)
                    {
                        RawSamples[swrite][1][slwrite] = (short *) malloc(SampleLength[swrite][slwrite] * sizeof(short) + 8);
                        memset(RawSamples[swrite][1][slwrite], 0, SampleLength[swrite][slwrite] * sizeof(short) + 8);
                        Read_Mod_Data(RawSamples[swrite][1][slwrite], sizeof(short), SampleLength[swrite][slwrite], in);
                        Swap_Sample(RawSamples[swrite][1][slwrite], swrite, slwrite);
                        *RawSamples[swrite][1][slwrite] = 0;
                    }
                } // Exist Sample
            }
        }

#if !defined(__WINAMP__)
        Status_Box("Loading song -> Track info, patterns and sequence...");   
#endif

        Set_Default_Channels_Polyphony();

        // Reading Track Properties
        for(twrite = 0; twrite < Songtracks; twrite++)
        {
            Read_Mod_Data_Swap(&TCut[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&ICut[twrite], sizeof(float), 1, in);
            if(ICut[twrite] > 0.0078125f) ICut[twrite] = 0.0078125f;
            if(ICut[twrite] < 0.00006103515625f) ICut[twrite] = 0.00006103515625f;
            Read_Mod_Data_Swap(&TPan[twrite], sizeof(float), 1, in);
            ComputeStereo(twrite);
            Read_Mod_Data_Swap(&FType[twrite], sizeof(int), 1, in);
            Read_Mod_Data_Swap(&FRez[twrite], sizeof(int), 1, in);
            Read_Mod_Data_Swap(&DThreshold[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&DClamp[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&DSend[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&CSend[twrite], sizeof(int), 1, in);
            if(Poly) Read_Mod_Data(&Channels_Polyphony[twrite], sizeof(char), 1, in);
        }

        // Reading mod properties
        Read_Mod_Data_Swap(&compressor, sizeof(int), 1, in);
        Read_Mod_Data_Swap(&c_threshold, sizeof(int), 1, in);
        Read_Mod_Data_Swap(&BeatsPerMin, sizeof(int), 1, in);
        Read_Mod_Data_Swap(&TicksPerBeat, sizeof(int), 1, in);
        Read_Mod_Data_Swap(&mas_vol, sizeof(float), 1, in);
        if(mas_vol < 0.01f) mas_vol = 0.01f;
        if(mas_vol > 1.0f) mas_vol = 1.0f;

        if(New_Comp)
        {
            Comp_Flag = 0;
            Read_Mod_Data(&Comp_Flag, sizeof(char), 1, in);
            if(Comp_Flag)
            {
                Read_Mod_Data_Swap(&mas_comp_threshold_Master, sizeof(float), 1, in);
                if(mas_comp_threshold_Master < 0.01f) mas_comp_threshold_Master = 0.01f;
                if(mas_comp_threshold_Master > 100.0f) mas_comp_threshold_Master = 100.0f;

                Read_Mod_Data_Swap(&mas_comp_ratio_Master, sizeof(float), 1, in);
                if(mas_comp_ratio_Master < 0.01f) mas_comp_ratio_Master = 0.01f;
                if(mas_comp_ratio_Master > 100.0f) mas_comp_ratio_Master = 100.0f;
            }
            if(Compress_Tracks)
            {
                for(i = 0; i < MAX_TRACKS; i++)
                {
                    Read_Mod_Data_Swap(&mas_comp_threshold_Track[i], sizeof(float), 1, in);
                }

                for(i = 0; i < MAX_TRACKS; i++)
                {
                    Read_Mod_Data_Swap(&mas_comp_ratio_Track[i], sizeof(float), 1, in);
                }
                Read_Mod_Data(&Compress_Track, sizeof(char), MAX_TRACKS, in);
            }
        }

        if(!New_Reverb) Read_Mod_Data_Swap(&delay_time, sizeof(int), 1, in);
        Read_Mod_Data_Swap(&Feedback, sizeof(float), 1, in);
        if(!New_Reverb) Read_Mod_Data_Swap(&DelayType, sizeof(int), 1, in);
        Read_Mod_Data_Swap(&lchorus_delay, sizeof(int), 1, in);
        Read_Mod_Data_Swap(&rchorus_delay, sizeof(int), 1, in);
        Read_Mod_Data_Swap(&lchorus_feedback, sizeof(float), 1, in);
        Read_Mod_Data_Swap(&rchorus_feedback, sizeof(float), 1, in);
        Read_Mod_Data_Swap(&shuffle, sizeof(int), 1, in);

        // Load the new reverb data
        if(New_Reverb)
        {
            Load_Reverb_Data(Read_Mod_Data, Read_Mod_Data_Swap, in);
        }

        // Reading track part sequence
        for(tps_pos = 0; tps_pos < 256; tps_pos++)
        {
            for(tps_trk = 0; tps_trk < MAX_TRACKS; tps_trk++)
            {
                Read_Mod_Data(&CHAN_ACTIVE_STATE[tps_pos][tps_trk], sizeof(char), 1, in);
                CHAN_HISTORY_STATE[tps_pos][tps_trk] = FALSE;
            }
        }

        for(int spl = 0; spl < Songtracks; spl++)
        {
            CCoef[spl] = float((float) CSend[spl] / 127.0f);
        }

        for(twrite = 0; twrite < Songtracks; twrite++)
        {
            Read_Mod_Data_Swap(&CHAN_MIDI_PRG[twrite], sizeof(int), 1, in);
        }

        for(twrite = 0; twrite < Songtracks; twrite++)
        {
            Read_Mod_Data(&LFO_ON[twrite], sizeof(char), 1, in);
            Read_Mod_Data_Swap(&LFO_RATE[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&LFO_AMPL[twrite], sizeof(float), 1, in);
        }

        for(twrite = 0; twrite < Songtracks; twrite++)
        {
            Read_Mod_Data(&FLANGER_ON[twrite], sizeof(char), 1, in);
            Read_Mod_Data_Swap(&FLANGER_AMOUNT[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&FLANGER_DEPHASE[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&FLANGER_RATE[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&FLANGER_AMPL[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&FLANGER_FEEDBACK[twrite], sizeof(float), 1, in);
            Read_Mod_Data_Swap(&FLANGER_DELAY[twrite], sizeof(int), 1, in);
            FLANGER_OFFSET[twrite] = 8192;
            FLANGER_OFFSET2[twrite] = float(FLANGER_OFFSET[twrite] - FLANGER_DELAY[twrite]);
            FLANGER_OFFSET1[twrite] = float(FLANGER_OFFSET[twrite] - FLANGER_DELAY[twrite]);
        }

        if(!Flanger_Bug)
        {
            Read_Mod_Data_Swap(&FLANGER_DEPHASE, sizeof(float), 1, in);
        }

        for(tps_trk = 0; tps_trk < Songtracks; tps_trk++)
        {
            Read_Mod_Data_Swap(&CHAN_MUTE_STATE[tps_trk], sizeof(int), 1, in);
        }

        Read_Mod_Data(&Songtracks, sizeof(char), 1, in);

        for(tps_trk = 0; tps_trk < MAX_TRACKS; tps_trk++)
        {
            Read_Mod_Data(&Disclap[tps_trk], sizeof(char), 1, in);
            if(!Portable) Read_Mod_Data(&fake_value, sizeof(char), 1, in);
        }

        if(!Ntk_Beta)       // Nothing like that in ntk beta
        {
            Read_Mod_Data(artist, sizeof(char), 20, in);
            Read_Mod_Data(style, sizeof(char), 20, in);

            if(!Portable) Read_Mod_Data(&Ye_Old_Phony_Value, sizeof(char), 1, in);

            Read_Mod_Data(beatsync, sizeof(char), MAX_INSTRS, in);

            for(i = 0; i < MAX_INSTRS; i++)
            {
                Read_Mod_Data_Swap(&beatlines[i], sizeof(short), 1, in);
            }

            Read_Mod_Data_Swap(&REVERBFILTER, sizeof(float), 1, in);

            for(i = 0; i < MAX_INSTRS; i++)
            {
                Read_Mod_Data_Swap(&Sample_Vol[i], sizeof(float), 1, in);
            }

            if(!Portable) Read_Mod_Data(&Ye_Old_Phony_Value, sizeof(char), 1, in);

            // Read the 303 datas
            for(j = 0; j < 2; j++)
            {
                Read_Mod_Data(&tb303[j].enabled, sizeof(char), 1, in);
                Read_Mod_Data(&tb303[j].selectedpattern, sizeof(char), 1, in);
                Read_Mod_Data(&tb303[j].tune, sizeof(char), 1, in);
                Read_Mod_Data(&tb303[j].cutoff, sizeof(char), 1, in);
                Read_Mod_Data(&tb303[j].resonance, sizeof(char), 1, in);
                Read_Mod_Data(&tb303[j].envmod, sizeof(char), 1, in);
                Read_Mod_Data(&tb303[j].decay, sizeof(char), 1, in);
                Read_Mod_Data(&tb303[j].accent, sizeof(char), 1, in);
                Read_Mod_Data(&tb303[j].waveform, sizeof(char), 1, in);
                if(Portable)
                {
                    for(i = 0; i < 32; i++)
                    {
                        Load_303_Data(Read_Mod_Data, Read_Mod_Data_Swap, in, j, i);
                    }
                }
                else
                {
                    Read_Mod_Data(&tb303[j].patternlength, sizeof(char), 32, in);
                    Read_Mod_Data(&tb303[j].tone, sizeof(char), 32 * 16, in);
                    Read_Mod_Data(&Ye_Old_Phony_Value, sizeof(char), 1, in);
                    Read_Mod_Data(&Ye_Old_Phony_Value, sizeof(char), 1, in);
                    Read_Mod_Data(&Ye_Old_Phony_Value, sizeof(char), 1, in);
                    for(k = 0; k < 32; k++)
                    {
                        for(i = 0; i < 16; i++)
                        {
                            Read_Mod_Data_Swap(&tb303[j].flag[k][i], sizeof(struct flag303), 1, in);
                        }                            
                    }
                    Read_Mod_Data(&tb303[j].pattern_name, sizeof(char), 32 * 20, in);
                }
            }
            Read_Mod_Data_Swap(&tb303engine[0].tbVolume, sizeof(float), 1, in);
            Read_Mod_Data_Swap(&tb303engine[1].tbVolume, sizeof(float), 1, in);
        }

        fclose(in);

        if(!New_Reverb)
        {
            // Set the reverb to one of the old presets
            Load_Old_Reverb_Presets(DelayType);
        }

        // Init the tracker context
        Init_Tracker_Context_After_ModLoad();

#if !defined(__WINAMP__)
        Status_Box("Module loaded sucessfully...");
#endif

    }
    else
    {

#if !defined(__WINAMP__)
        Status_Box("Module loading failed. (Probably: file not found)");
#endif

        return(FALSE);
    }

#if !defined(__WINAMP__)
    Clear_Input();
    if(Mod_Memory) free(Mod_Memory);
#endif

    return(TRUE);
}

// ------------------------------------------------------
// Load and decode a packed sample
short *Unpack_Sample(FILE *FileHandle, int Dest_Length, char Pack_Type, int BitRate)
{
    int Packed_Length;

    short *Dest_Buffer;

    Uint8 *Packed_Read_Buffer;

    Read_Mod_Data(&Packed_Length, sizeof(int), 1, FileHandle);
    if(Packed_Length == -1)
    {
        // Sample wasn't packed
        Packed_Read_Buffer = (Uint8 *) malloc(Dest_Length * 2 + 8);
        memset(Packed_Read_Buffer, 0, Dest_Length * 2 + 8);
        Read_Mod_Data(Packed_Read_Buffer, sizeof(char), Dest_Length * 2, FileHandle);
        return((short *) Packed_Read_Buffer);
    }
    else
    {

        Packed_Read_Buffer = (Uint8 *) malloc(Packed_Length);
        // Read the packer buffer
        Read_Mod_Data(Packed_Read_Buffer, sizeof(char), Packed_Length, FileHandle);
        Dest_Buffer = (short *) malloc(Dest_Length * 2 + 8);
        memset(Dest_Buffer, 0, Dest_Length * 2 + 8);

        switch(Pack_Type)
        {
#if defined(__AT3_CODEC__)
            case SMP_PACK_AT3:
                UnpackAT3(Packed_Read_Buffer, Dest_Buffer, Packed_Length, Dest_Length, BitRate);
                break;
#endif
#if defined(__GSM_CODEC__)
            case SMP_PACK_GSM:
                UnpackGSM(Packed_Read_Buffer, Dest_Buffer, Packed_Length, Dest_Length);
                break;
#endif
#if defined(__MP3_CODEC__)
            case SMP_PACK_MP3:
                UnpackMP3(Packed_Read_Buffer, Dest_Buffer, Packed_Length, Dest_Length, BitRate);
                break;
#endif
#if defined(__TRUESPEECH_CODEC__)
            case SMP_PACK_TRUESPEECH:
                UnpackTrueSpeech(Packed_Read_Buffer, Dest_Buffer, Packed_Length, Dest_Length);
                break;
#endif
#if defined(__ADPCM_CODEC__)
            case SMP_PACK_ADPCM:
                UnpackADPCM(Packed_Read_Buffer, Dest_Buffer, Packed_Length, Dest_Length);
                break;
#endif
            case SMP_PACK_8BIT:
                Unpack8Bit(Packed_Read_Buffer, Dest_Buffer, Packed_Length, Dest_Length);
                break;

            case SMP_PACK_INTERNAL:
                UnpackInternal(Packed_Read_Buffer, Dest_Buffer, Packed_Length, Dest_Length);
                break;
        }
        free(Packed_Read_Buffer);
        return(Dest_Buffer);

    }
}

// ------------------------------------------------------
// Save a packed sample
#if !defined(__WINAMP__)
void Pack_Sample(FILE *FileHandle, short *Sample, int Size, char Pack_Type, int BitRate)
{
    int PackedLen = 0;
    short *PackedSample = NULL;

#if defined(__ADPCM_CODEC__) || defined(__TRUESPEECH_CODEC__)
    short *AlignedSample;
    int Aligned_Size;
#endif

    switch(Pack_Type)
    {
#if defined(__AT3_CODEC__)
        case SMP_PACK_AT3:
            PackedSample = (short *) malloc(Size * 2 + 8);
            memset(PackedSample, 0, Size * 2 + 8);
            PackedLen = ToAT3(Sample, PackedSample, Size * 2, BitRate);
            break;
#endif
#if defined(__GSM_CODEC__)
        case SMP_PACK_GSM:
            PackedSample = (short *) malloc(Size * 2 + 8);
            memset(PackedSample, 0, Size * 2 + 8);
            PackedLen = ToGSM(Sample, PackedSample, Size * 2);
            break;
#endif
#if defined(__MP3_CODEC__)
        case SMP_PACK_MP3:
            PackedSample = (short *) malloc(Size * 2 + 8);
            memset(PackedSample, 0, Size * 2 + 8);
            PackedLen = ToMP3(Sample, PackedSample, Size * 2, BitRate);
            break;
#endif
#if defined(__TRUESPEECH_CODEC__)
        case SMP_PACK_TRUESPEECH:
            Aligned_Size = (Size * 2) + 0x400;
            AlignedSample = (short *) malloc(Aligned_Size + 8);
            if(AlignedSample)
            {
                memset(AlignedSample, 0, Aligned_Size + 8);
                memcpy(AlignedSample, Sample, Size * 2);
                // Size must be aligned
                PackedSample = (short *) malloc(Aligned_Size + 8);
                if(PackedSample)
                {
                    memset(PackedSample, 0, Aligned_Size + 8);
                    PackedLen = ToTrueSpeech(AlignedSample, PackedSample, Aligned_Size);
                }
                free(AlignedSample);
            }
            break;
#endif
#if defined(__ADPCM_CODEC__)
        case SMP_PACK_ADPCM:
            Aligned_Size = (Size * 2) + 0x1000;
            AlignedSample = (short *) malloc(Aligned_Size + 8);
            if(AlignedSample)
            {
                memset(AlignedSample, 0, Aligned_Size + 8);
                memcpy(AlignedSample, Sample, Size * 2);
                // Size must be aligned
                PackedSample = (short *) malloc(Aligned_Size + 8);
                if(PackedSample)
                {
                    memset(PackedSample, 0, Aligned_Size + 8);
                    PackedLen = ToADPCM(AlignedSample, PackedSample, Aligned_Size);
                }
                free(AlignedSample);
            }
            break;
#endif
        case SMP_PACK_8BIT:
            PackedSample = (short *) malloc(Size * 2 + 8);
            memset(PackedSample, 0, Size * 2 + 8);
            PackedLen = To8Bit(Sample, PackedSample, Size);
            break;

        case SMP_PACK_INTERNAL:
            PackedSample = (short *) malloc(Size * 2 + 8);
            memset(PackedSample, 0, Size * 2 + 8);
            PackedLen = ToInternal(Sample, PackedSample, Size);
            break;

        case SMP_PACK_NONE:
            PackedLen = 0;
            break;
    }
    if(PackedLen)
    {
        // Write the encoded length
        Write_Mod_Data(&PackedLen, sizeof(char), 4, FileHandle);
        // Write the encoded datas
        Write_Mod_Data(PackedSample, sizeof(char), PackedLen, FileHandle);
    }
    else
    {
        // Couldn't pack (too small or user do not want that to happen)
        PackedLen = -1;
        Write_Mod_Data(&PackedLen, sizeof(char), 4, FileHandle);
        Write_Mod_Data(Sample, sizeof(char), Size * 2, FileHandle);
    }
    if(PackedSample) free(PackedSample);
}

// ------------------------------------------------------
// Write data into a module file
int Write_Mod_Data(void *Datas, int Unit, int Length, FILE *Handle)
{
    switch(Mod_Simulate)
    {
        case SAVE_WRITE:
            Write_Data(Datas, Unit, Length, Handle);
            break;

        case SAVE_CALCLEN:
            Mod_Length += Unit * Length;
            break;

        case SAVE_WRITEMEM:
            memcpy(Mod_Memory + Mod_Mem_Pos, Datas, Unit * Length);
            Mod_Mem_Pos += Unit * Length;
            break;
    }
    return(0);
}

// ------------------------------------------------------
// Write data into a module file (handling bytes swapping)
int Write_Mod_Data_Swap(void *Datas, int Unit, int Length, FILE *Handle)
{
    short sswap_value;
    int iswap_value;
    short *svalue;
    int *ivalue;
    
    switch(Mod_Simulate)
    {
        case SAVE_WRITE:
            Write_Data_Swap(Datas, Unit, Length, Handle);
            break;

        case SAVE_CALCLEN:
            Mod_Length += Unit * Length;
            break;

        case SAVE_WRITEMEM:
            switch(Unit)
            {
                case 2:
                    svalue = (short *) Datas;
                    sswap_value = Swap_16(*svalue);
                    memcpy(Mod_Memory + Mod_Mem_Pos, &sswap_value, Unit * Length);
                    Mod_Mem_Pos += Unit * Length;
                    break;

                case 4:
                    ivalue = (int *) Datas;
                    iswap_value = Swap_32(*ivalue);
                    memcpy(Mod_Memory + Mod_Mem_Pos, &iswap_value, Unit * Length);
                    Mod_Mem_Pos += Unit * Length;
                    break;

                default:
                    printf("Invalid writing size.\n");
                    break;
            }
            break;
    }
    return(0);
}
#endif // __WINAMP__

// ------------------------------------------------------
// Read data from a module file
int Read_Mod_Data(void *Datas, int Unit, int Length, FILE *Handle)
{
    switch(Mod_Simulate)
    {
        case LOAD_READ:
            Read_Data(Datas, Unit, Length, Handle);
            break;

        case LOAD_READMEM:
            memcpy(Datas, Mod_Memory + Mod_Mem_Pos, Unit * Length);
            Mod_Mem_Pos += Unit * Length;
            break;
    }
    return(0);
}

// ------------------------------------------------------
// Read data from a module file
int Read_Mod_Data_Swap(void *Datas, int Unit, int Length, FILE *Handle)
{
    short svalue;
    int ivalue;

    switch(Mod_Simulate)
    {
        case LOAD_READ:
            Read_Data_Swap(Datas, Unit, Length, Handle);
            break;

        case LOAD_READMEM:
            switch(Unit)
            {
                case 2:
                    memcpy(&svalue, Mod_Memory + Mod_Mem_Pos, Unit * Length);
                    svalue = Swap_16(svalue);
                    *((short *) Datas) = (int) svalue;
                    Mod_Mem_Pos += Unit * Length;
                    break;

                case 4:
                    memcpy(&ivalue, Mod_Memory + Mod_Mem_Pos, Unit * Length);
                    ivalue = Swap_32(ivalue);
                    *((int *) Datas) = (int) ivalue;
                    Mod_Mem_Pos += Unit * Length;
                    break;

                default:
                    printf("Invalid reading size.\n");
                    break;
            }
            break;
    }
    return(0);
}

// ------------------------------------------------------
// Check if a file exists
#if !defined(__WINAMP__)
int File_Exist(char *Format, char *Directory, char *FileName)
{
    FILE *in;
    char Temph[MAX_PATH];

    sprintf(Temph, Format, Directory, FileName);

    in = fopen(Temph, "rb");
    if(in)
    {
        sprintf(OverWrite_Name, "File '%s' already exists, overwrite ?", Temph);
        Overwrite_Requester.Text = OverWrite_Name;
        fclose(in);
        return(TRUE);
    }
    return(FALSE);
}
#endif

// ------------------------------------------------------
// Return the size of an opened file
int Get_File_Size(FILE *Handle)
{
    int File_Size;
    int Current_Pos;

    Current_Pos = ftell(Handle);
    fseek(Handle, 0, SEEK_END);
    File_Size = ftell(Handle);
    fseek(Handle, Current_Pos, SEEK_SET);
    return(File_Size);
}

#if !defined(__WINAMP__)

// ------------------------------------------------------
// Remvove the spaces chars located at the end of a string
void rtrim_string(char *string, int maxlen)
{
    int len = strlen(string);

    while(len < maxlen)
    {
        string[len] = 0;
        len++;
    }
}

// ------------------------------------------------------
// Entry point function for modules saving
int SavePtk(char *FileName, int NewFormat, int Simulate, Uint8 *Memory)
{
    FILE *in;
    int i;
    int j;
    int k;
    int l;
    char Temph[MAX_PATH];
    int Ok_Memory = TRUE;
    char Comp_Flag = TRUE;
    unsigned char *cur_pattern_col;
    unsigned char *cur_pattern;

    int twrite;
    int tps_trk;
    
    Mod_Length = 0;
    Mod_Mem_Pos = 0;
    Mod_Simulate = SAVE_WRITE;
    Mod_Memory = Memory;

    // Store the different possible modes
    if(Simulate)
    {
        Mod_Simulate = Simulate;
    }

    if(!Simulate)
    {
        if(NewFormat)
        {
            sprintf(Temph, "Saving '%s.ptp' song in modules directory...", FileName);
            Status_Box(Temph);
            sprintf(Temph, "%s"SLASH"%s.ptp", Dir_Mods, FileName);
        }
        else
        {
            sprintf(Temph, "Saving '%s.ptk' song in modules directory...", FileName);
            Status_Box(Temph);
            sprintf(Temph, "%s"SLASH"%s.ptk", Dir_Mods, FileName);
        }
        in = fopen(Temph, "wb");
    }
    else
    {
        in = (FILE *) -1;
    }

    if(in != NULL)
    {
        if(NewFormat)
        {
            // .ptp
            Ok_Memory = SavePtp(in, Simulate, FileName);
        }
        else
        {
            // .ptk
            if(strlen(FileName)) rtrim_string(FileName, 20);
            Write_Mod_Data(FileName, sizeof(char), 20, in);

            // Calc the real number of patterns
            nPatterns = 0;
            for(i = 0 ; i < sLength; i++)
            {
                if((pSequence[i] + 1) > nPatterns)
                {
                    nPatterns = pSequence[i] + 1;
                }
            }

            Write_Mod_Data(&nPatterns, sizeof(char), 1, in);
            Write_Mod_Data(&sLength, sizeof(char), 1, in);
            Write_Mod_Data(&Use_Cubic, sizeof(char), 1, in);

            Write_Mod_Data(pSequence, sizeof(char), MAX_SEQUENCES, in);

            Swap_Short_Buffer(patternLines, MAX_ROWS);
            Write_Mod_Data(patternLines, sizeof(short), MAX_ROWS, in);
            Swap_Short_Buffer(patternLines, MAX_ROWS);

            Write_Mod_Data(Channels_MultiNotes, sizeof(char), MAX_TRACKS, in);

            // Clean the unused patterns garbage (doesn't seem to do much)
            for(i = Songtracks; i < MAX_TRACKS; i++)
            {
                // Next column
                cur_pattern_col = RawPatterns + (i * PATTERN_BYTES);
                for(j = 0; j < nPatterns; j++)
                {
                    // Next pattern
                    cur_pattern = cur_pattern_col + (j * PATTERN_LEN);
                    for(k = 0; k < patternLines[j]; k++)
                    {
                        for(l = 0; l < MAX_POLYPHONY; l += 2)
                        {
                            cur_pattern[PATTERN_NOTE1 + i] = 121;
                            cur_pattern[PATTERN_INSTR1 + i] = 255;
                        }

                        cur_pattern[PATTERN_VOLUME] = 255;
                        cur_pattern[PATTERN_PANNING] = 255;
                        cur_pattern[PATTERN_FX] = 0;
                        cur_pattern[PATTERN_FXDATA] = 0;
                        cur_pattern[PATTERN_FX2] = 0;
                        cur_pattern[PATTERN_FXDATA2] = 0;
                        // Next line
                        cur_pattern += PATTERN_ROW_LEN;
                    }
                }
            }

            for(int pwrite = 0; pwrite < nPatterns; pwrite++)
            {
                Write_Mod_Data(RawPatterns + (pwrite * PATTERN_LEN),
                               sizeof(char), PATTERN_LEN, in);
            }

            // Writing sample data
            for(int swrite = 0; swrite < MAX_INSTRS; swrite++)
            {
                rtrim_string(nameins[swrite], 20);
                Write_Mod_Data(nameins[swrite], sizeof(char), 20, in);
                Write_Mod_Data(&Midiprg[swrite], sizeof(char), 1, in);
                Write_Mod_Data(&Synthprg[swrite], sizeof(char), 1, in);

                Write_Synth_Params(Write_Mod_Data, Write_Mod_Data_Swap, in, swrite);

                // Compression type
                Write_Mod_Data(&SampleCompression[swrite], sizeof(char), 1, in);
                switch(SampleCompression[swrite])
                {
                    case SMP_PACK_MP3:
                        Write_Mod_Data(&Mp3_BitRate[swrite], sizeof(char), 1, in);
                        break;

                    case SMP_PACK_AT3:
                        Write_Mod_Data(&At3_BitRate[swrite], sizeof(char), 1, in);
                        break;

                    case SMP_PACK_INTERNAL:
                        Write_Mod_Data(&Internal_Quality[swrite], sizeof(char), 1, in);
                        break;
                }

                // 16 splits / instrument
                for(int slwrite = 0; slwrite < MAX_INSTRS_SPLITS; slwrite++)
                {
                    Write_Mod_Data(&SampleType[swrite][slwrite], sizeof(char), 1, in);
                    if(SampleType[swrite][slwrite])
                    {
                        rtrim_string(SampleName[swrite][slwrite], 64);
                        Write_Mod_Data(SampleName[swrite][slwrite], sizeof(char), 64, in);
                        Write_Mod_Data(&Basenote[swrite][slwrite], sizeof(char), 1, in);
                        
                        Write_Mod_Data_Swap(&LoopStart[swrite][slwrite], sizeof(int), 1, in);
                        Write_Mod_Data_Swap(&LoopEnd[swrite][slwrite], sizeof(int), 1, in);
                        Write_Mod_Data(&LoopType[swrite][slwrite], sizeof(char), 1, in);
                        
                        Write_Mod_Data_Swap(&SampleLength[swrite][slwrite], sizeof(int), 1, in);
                        Write_Mod_Data(&Finetune[swrite][slwrite], sizeof(char), 1, in);
                        Write_Mod_Data_Swap(&Sample_Amplify[swrite][slwrite], sizeof(float), 1, in);
                        Write_Mod_Data_Swap(&FDecay[swrite][slwrite], sizeof(float), 1, in);

                        // All samples are converted into 16 bits
                        Write_Unpacked_Sample(Write_Mod_Data, in, swrite, slwrite);
                    }// Exist Sample
                }
            }

            // Writing Track Properties
            for(twrite = 0; twrite < MAX_TRACKS; twrite++)
            {
                Write_Mod_Data_Swap(&TCut[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&ICut[twrite], sizeof(float), 1, in);

                Write_Mod_Data_Swap(&TPan[twrite], sizeof(float), 1, in);

                Write_Mod_Data_Swap(&FType[twrite], sizeof(int), 1, in);
                Write_Mod_Data_Swap(&FRez[twrite], sizeof(int), 1, in);
                Write_Mod_Data_Swap(&DThreshold[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&DClamp[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&DSend[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&CSend[twrite], sizeof(int), 1, in);
                Write_Mod_Data(&Channels_Polyphony[twrite], sizeof(char), 1, in);
            }

            // Writing mod properties
            Write_Mod_Data_Swap(&compressor, sizeof(int), 1, in);
            Write_Mod_Data_Swap(&c_threshold, sizeof(int), 1, in);
            Write_Mod_Data_Swap(&BeatsPerMin, sizeof(int), 1, in);
            Write_Mod_Data_Swap(&TicksPerBeat, sizeof(int), 1, in);
            Write_Mod_Data_Swap(&mas_vol, sizeof(float), 1, in);
            
            Write_Mod_Data(&Comp_Flag, sizeof(char), 1, in);
            Write_Mod_Data_Swap(&mas_comp_threshold_Master, sizeof(float), 1, in);
            Write_Mod_Data_Swap(&mas_comp_ratio_Master, sizeof(float), 1, in);

            for(i = 0; i < MAX_TRACKS; i++)
            {
                if(mas_comp_threshold_Track[i] < 0.01f) mas_comp_threshold_Track[i] = 0.01f;
                if(mas_comp_threshold_Track[i] > 100.0f) mas_comp_threshold_Track[i] = 100.0f;
                Write_Mod_Data_Swap(&mas_comp_threshold_Track[i], sizeof(float), 1, in);
            }
            for(i = 0; i < MAX_TRACKS; i++)
            {
                if(mas_comp_ratio_Track[i] < 0.01f) mas_comp_ratio_Track[i] = 0.01f;
                if(mas_comp_ratio_Track[i] > 100.0f) mas_comp_ratio_Track[i] = 100.0f;
                Write_Mod_Data_Swap(&mas_comp_ratio_Track[i], sizeof(float), 1, in);
            }
            Write_Mod_Data(&Compress_Track, sizeof(char), MAX_TRACKS, in);

            Write_Mod_Data_Swap(&Feedback, sizeof(float), 1, in);
            Write_Mod_Data_Swap(&lchorus_delay, sizeof(int), 1, in);
            Write_Mod_Data_Swap(&rchorus_delay, sizeof(int), 1, in);
            Write_Mod_Data_Swap(&lchorus_feedback, sizeof(float), 1, in);
            Write_Mod_Data_Swap(&rchorus_feedback, sizeof(float), 1, in);
            Write_Mod_Data_Swap(&shuffle, sizeof(int), 1, in);

            // Save the reverb data
            Save_Reverb_Data(Write_Mod_Data, Write_Mod_Data_Swap, in);

            // Writing part sequence data
            for(int tps_pos = 0; tps_pos < MAX_SEQUENCES; tps_pos++)
            {
                for(tps_trk = 0; tps_trk < MAX_TRACKS; tps_trk++)
                {
                    Write_Mod_Data(&CHAN_ACTIVE_STATE[tps_pos][tps_trk], sizeof(char), 1, in);
                }
            }

            for(twrite = 0; twrite < MAX_TRACKS; twrite++)
            {
                Write_Mod_Data_Swap(&CHAN_MIDI_PRG[twrite], sizeof(int), 1, in);
            }

            for(twrite = 0; twrite < MAX_TRACKS; twrite++)
            {
                Write_Mod_Data(&LFO_ON[twrite], sizeof(char), 1, in);
                Write_Mod_Data_Swap(&LFO_RATE[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&LFO_AMPL[twrite], sizeof(float), 1, in);
            }

            for(twrite = 0; twrite < MAX_TRACKS; twrite++)
            {
                Write_Mod_Data(&FLANGER_ON[twrite], sizeof(char), 1, in);
                Write_Mod_Data_Swap(&FLANGER_AMOUNT[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&FLANGER_DEPHASE[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&FLANGER_RATE[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&FLANGER_AMPL[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&FLANGER_FEEDBACK[twrite], sizeof(float), 1, in);
                Write_Mod_Data_Swap(&FLANGER_DELAY[twrite], sizeof(int), 1, in);
            }

            // Was a bug
            //Write_Mod_Data_Swap(&FLANGER_DEPHASE, sizeof(float), 1, in);

            for(tps_trk = 0; tps_trk < MAX_TRACKS; tps_trk++)
            {
                Write_Mod_Data_Swap(&CHAN_MUTE_STATE[tps_trk], sizeof(int), 1, in);
            }
            Write_Mod_Data(&Songtracks, sizeof(char), 1, in);

            for(tps_trk = 0; tps_trk < MAX_TRACKS; tps_trk++)
            {
                Write_Mod_Data(&Disclap[tps_trk], sizeof(char), 1, in);
            }

            rtrim_string(artist, 20);
            Write_Mod_Data(artist, sizeof(char), 20, in);
            rtrim_string(style, 20);
            Write_Mod_Data(style, sizeof(char), 20, in);

            Write_Mod_Data(beatsync, sizeof(char), 128, in);

            for(i = 0; i < 128; i++)
            {
                Write_Mod_Data_Swap(&beatlines[i], sizeof(short), 1, in);
            }
            Write_Mod_Data_Swap(&REVERBFILTER, sizeof(float), 1, in);

            for(i = 0; i < 128; i++)
            {
                Write_Mod_Data_Swap(&Sample_Vol[i], sizeof(float), 1, in);
            }

            // Include the patterns names
            for(i = 0; i < 32; i++)
            {
                rtrim_string(tb303[0].pattern_name[i], 20);
                rtrim_string(tb303[1].pattern_name[i], 20);
            }
            
            for(j = 0; j < 2; j++)
            {
                Write_Mod_Data(&tb303[j].enabled, sizeof(char), 1, in);
                Write_Mod_Data(&tb303[j].selectedpattern, sizeof(char), 1, in);
                Write_Mod_Data(&tb303[j].tune, sizeof(char), 1, in);
                Write_Mod_Data(&tb303[j].cutoff, sizeof(char), 1, in);
                Write_Mod_Data(&tb303[j].resonance, sizeof(char), 1, in);
                Write_Mod_Data(&tb303[j].envmod, sizeof(char), 1, in);
                Write_Mod_Data(&tb303[j].decay, sizeof(char), 1, in);
                Write_Mod_Data(&tb303[j].accent, sizeof(char), 1, in);
                Write_Mod_Data(&tb303[j].waveform, sizeof(char), 1, in);
                for(i = 0; i < 32; i++)
                {
                    Save_303_Data(Write_Mod_Data, Write_Mod_Data_Swap, in, j, i); 
                }
            }

            Write_Mod_Data_Swap(&tb303engine[0].tbVolume, sizeof(float), 1, in);
            Write_Mod_Data_Swap(&tb303engine[1].tbVolume, sizeof(float), 1, in);
        }

        if(!Simulate)
        {
            fclose(in);
            last_index = -1;
            Read_SMPT();
            Actualize_Files_List(0);

            if(Ok_Memory)
            {
                char name[128];
                if(NewFormat)
                {
                    sprintf(name, "Module '%s.ptp' saved succesfully.", FileName);
                }
                else
                {
                    sprintf(name, "Module '%s.ptk' saved succesfully.", FileName);
                }
                Status_Box(name);
            }
        }
        if(!Ok_Memory) Status_Box("Not enough memory.");
    }
    else
    {
        if(!Simulate) Status_Box("Module save failed.");   
    }

    return(Mod_Length);
}

// ------------------------------------------------------
// Pack a module
Uint8 *Pack_Data(Uint8 *Memory, int *Size)
{
    z_stream c_stream; // compression stream
    int Comp_Len = -1;
    Uint8 *Final_Mem_Out = (Uint8 *) malloc(*Size);

    memset(&c_stream, 0, sizeof(c_stream));
    deflateInit(&c_stream, Z_BEST_COMPRESSION);
    c_stream.next_in = (Bytef *) Memory;
    c_stream.next_out = Final_Mem_Out;
    while (c_stream.total_in != *Size)
    {
        c_stream.avail_in = 1;
        c_stream.avail_out = 1;
        deflate(&c_stream, Z_NO_FLUSH);
    }
    for (;;)
    {
        c_stream.avail_out = 1;
        if (deflate(&c_stream, Z_FINISH) == Z_STREAM_END) break;
    }
    deflateEnd(&c_stream);
    *Size = c_stream.total_out;
    return(Final_Mem_Out);
}
#endif // __WINAMP__

// ------------------------------------------------------
// Depack a compressed module
Uint8 *Depack_Data(Uint8 *Memory, int Sizen, int Size_Out)
{
    z_stream d_stream;

    Uint8 *Test_Mem = (Uint8 *) malloc(Size_Out);
    if(Test_Mem)
    {
        memset(&d_stream, 0, sizeof(d_stream));
        d_stream.next_in = (Uint8 *) Memory;
        d_stream.next_out = Test_Mem;
        inflateInit(&d_stream);
        while (d_stream.total_out < Size_Out &&
               d_stream.total_in < Sizen)
        {
            d_stream.avail_in = 1;
            d_stream.avail_out = 1;
            if (inflate(&d_stream, Z_NO_FLUSH) == Z_STREAM_END) break;
        }
        inflateEnd(&d_stream);
    }
    return(Test_Mem);
}

// ------------------------------------------------------
// Save a packed .ptk module
#if !defined(__WINAMP__)
int Pack_Module(char *FileName)
{
    FILE *output;
    char name[128];
    char extension[10];
    char Temph[MAX_PATH];
    Uint8 *Final_Mem_Out;
    int Depack_Size;

    if(!strlen(FileName))
    {
        sprintf(name, "Can't save module without a name...");
        Status_Box(name);
        return(FALSE);
    }

    sprintf(Temph, "%s"SLASH"%s.ptk", Dir_Mods, FileName);

    int Len = SavePtk("", FALSE, SAVE_CALCLEN, NULL);

    Depack_Size = Len;

    Uint8 *Final_Mem = (Uint8 *) malloc(Len);
    SavePtk(FileName, FALSE, SAVE_WRITEMEM, Final_Mem);

    Final_Mem_Out = Pack_Data(Final_Mem, &Len);

    output = fopen(Temph, "wb");
    if(output)
    {
        sprintf(extension, "PROTREKK");
        Write_Data(extension, sizeof(char), 9, output);
        Write_Data_Swap(&Depack_Size, sizeof(int), 1, output);
        Write_Data(Final_Mem_Out, sizeof(char), Len, output);
        fclose(output);
        sprintf(name, "Module '%s.ptk' saved succesfully.", FileName);
    }
    else
    {
        sprintf(name, "Module save failed.");
    }
    if(Final_Mem_Out) free(Final_Mem_Out);
    if(Final_Mem) free(Final_Mem);

    Clear_Input();
    Status_Box(name);
    Read_SMPT();
    last_index = -1;
    Actualize_Files_List(0);
    return(FALSE);
}

// ------------------------------------------------------
// Return the length of a compressed module
int TestMod(void)
{
    Uint8 *Final_Mem_Out;
    int Len = SavePtk("", TRUE, SAVE_CALCLEN, NULL);

    Uint8 *Final_Mem = (Uint8 *) malloc(Len);
    SavePtk("", TRUE, SAVE_WRITEMEM, Final_Mem);

    Final_Mem_Out = Pack_Data(Final_Mem, &Len);

    if(Final_Mem_Out) free(Final_Mem_Out);
    if(Final_Mem) free(Final_Mem);
    return(Len);
}
#endif

// ------------------------------------------------------
// Switch the endianness of a 16 bit buffer
// (size is the number of short elements, not bytes)
void Swap_Short_Buffer(short *buffer, int size)
{
#if defined(__BIG_ENDIAN__)
    int i;

    for(i = 0; i < size; i++)
    {
        buffer[i] = Swap_16(buffer[i]);
    }
#endif
}

// ------------------------------------------------------
// Switch the endianness of a sample
void Swap_Sample(short *buffer, int sample, int bank)
{
#if defined(__BIG_ENDIAN__)
    Swap_Short_Buffer(buffer, SampleLength[sample][bank]);
#endif
}

// ------------------------------------------------------
// Create a new buffer and switch the endianness of a sample
short *Swap_New_Sample(short *buffer, int sample, int bank)
{
#if defined(__BIG_ENDIAN__)
    short *new_buffer = (short *) malloc(SampleLength[sample][bank] * sizeof(short) + 8);
    memset(new_buffer, 0, SampleLength[sample][bank] * sizeof(short) + 8);
    memcpy(new_buffer, buffer, SampleLength[sample][bank] * sizeof(short));
    Swap_Sample(new_buffer, sample, bank);
    return(new_buffer);
#else
    return(NULL);
#endif
}

// ------------------------------------------------------
// Save a given unpacked sample
void Write_Unpacked_Sample(int (*Write_Function)(void *, int ,int, FILE *),
                           FILE *in, int sample, int bank)
{
    short *swap_buffer;

    swap_buffer = Swap_New_Sample(Get_WaveForm(sample, 0, bank), sample, bank);
    if(swap_buffer)
    {
        Write_Function(swap_buffer, sizeof(short), SampleLength[sample][bank], in);
        free(swap_buffer);
    }
    else
    {
        Write_Function(Get_WaveForm(sample, 0, bank), sizeof(short), SampleLength[sample][bank], in);
    }

    Write_Function(&SampleChannels[sample][bank], sizeof(char), 1, in);
    if(SampleChannels[sample][bank] == 2)
    {
        swap_buffer = Swap_New_Sample(Get_WaveForm(sample, 1, bank), sample, bank);
        if(swap_buffer)
        {
            Write_Function(swap_buffer, sizeof(short), SampleLength[sample][bank], in);
            free(swap_buffer);
        }
        else
        {
            Write_Function(Get_WaveForm(sample, 1, bank), sizeof(short), SampleLength[sample][bank], in);
        }
    }
}

// ------------------------------------------------------
// Write data into a file
int Write_Data(void *value, int size, int amount, FILE *handle)
{
    return(fwrite(value, size, amount, handle));
}

// ------------------------------------------------------
// Write data into a file taking care of the endianness
int Write_Data_Swap(void *value, int size, int amount, FILE *handle)
{
    short sswap_value;
    int iswap_value;
    short *svalue;
    int *ivalue;

    switch(size)
    {
        case 2:
            svalue = (short *) value;
            sswap_value = Swap_16(*svalue);
            return(Write_Data(&sswap_value, size, amount, handle));

        case 4:
            ivalue = (int *) value;
            iswap_value = Swap_32(*ivalue);
            return(Write_Data(&iswap_value, size, amount, handle));

        default:
            printf("Invalid writing size.\n");
            break;
    }
    return(TRUE);
}

// ------------------------------------------------------
// Read data from a file
int Read_Data(void *value, int size, int amount, FILE *handle)
{
    return(fread(value, size, amount, handle));
}

// ------------------------------------------------------
// Read data from a file taking care of the endianness
int Read_Data_Swap(void *value, int size, int amount, FILE *handle)
{
    short svalue;
    int ivalue;
    int ret_value;

    switch(size)
    {
        case 2:
            ret_value = Read_Data(&svalue, size, amount, handle);
            svalue = Swap_16(svalue);
            *((short *) value) = (int) svalue;
            return(ret_value);

        case 4:
            ret_value = Read_Data(&ivalue, size, amount, handle);
            ivalue = Swap_32(ivalue);
            *((int *) value) = (int) ivalue;
            return(ret_value);

        default:
            printf("Invalid reading size.\n");
            break;
    }
    return(0);
}

// -------------------------------------
// Calculate the length of the song in hours:minutes:seconds
int song_Seconds;
int song_Minutes;
int song_Hours;

int Calc_Length(void)
{
    int i;
    int k;
    int l;
    int pos_patt;
    int patt_cmd[MAX_FX];
    int patt_datas[MAX_FX];
    Uint8 *Cur_Patt;
    float Ticks = (float) TicksPerBeat;
    float BPM = (float) BeatsPerMin;
    int rep_pos = 0;
    int rep_counter = -1;
    int have_break = 255;
    int PosTicks;
    int shuffle_switch;
    int shuffle_stp = shuffle;
    double len;
    int nbr_ticks;
    int Samples;
    int ilen;
    int early_exit = FALSE;

    shuffle_switch = -1;
    Samples = (int) ((60 * MIX_RATE) / (BeatsPerMin * TicksPerBeat));
    if(shuffle_switch == 1) shuffle_stp = -((Samples * shuffle) / 200);
    else shuffle_stp = (Samples * shuffle) / 200;

    PosTicks = 0;
    nbr_ticks = 0;
    len = 0;
    i = 0;
    while(i < sLength)
    {
        if(have_break < MAX_ROWS) pos_patt = have_break;
        else pos_patt = 0;
        have_break = 255;
        while(pos_patt < patternLines[pSequence[i]])
        {
            Cur_Patt = RawPatterns + (pSequence[i] * PATTERN_LEN) + (pos_patt * PATTERN_ROW_LEN);
            if(!PosTicks)
            {
                for(k = 0; k < Songtracks; k++)
                {
                    // Check if there's a pattern loop command
                    // or a change in the tempo/ticks
                    patt_cmd[0] = Cur_Patt[PATTERN_FX];
                    patt_datas[0] = Cur_Patt[PATTERN_FXDATA];
                    patt_cmd[1] = Cur_Patt[PATTERN_FX2];
                    patt_datas[1] = Cur_Patt[PATTERN_FXDATA2];

                    for(l = 0; l < MAX_FX; l++)
                    {
                        switch(patt_cmd[l])
                        {
                            case 0x6:
                                if(!patt_datas[l])
                                {
                                    rep_counter = -1;
                                    rep_pos = pos_patt;
                                }
                                else
                                {
                                    if(rep_counter == -1)
                                    {
                                        rep_counter = (int) patt_datas[l];
                                        pos_patt = rep_pos;
                                    }
                                    else
                                    {
                                        // count
                                        rep_counter--;
                                        if(rep_counter)
                                        {
                                            pos_patt = rep_pos;
                                        }
                                        else
                                        {
                                            rep_counter = -1;
                                            rep_pos = 0;
                                        }
                                    }
                                }
                                break;

                            case 0xd:
                                if(patt_datas[l] < MAX_ROWS) have_break = patt_datas[l];
                                break;
                        
                            case 0x1f:
                                // Avoid looping the song when jumping
                                if(i == (sLength - 1) || patt_datas[l] <= i)
                                {
                                    early_exit = TRUE;
                                }
                                i = patt_datas[l];
                                // Was there a break already ?
                                if(have_break >= MAX_ROWS) have_break = 0;
                                break;
                        
                            case 0xf:
                                Ticks = (float) patt_datas[l];
                                break;
    
                            case 0xf0:
                                BPM = (float) patt_datas[l];
                                break;
                        }
                    }
                    Cur_Patt += PATTERN_BYTES;
                }
            }
            Samples = (int) ((60 * MIX_RATE) / (BPM * Ticks));

            PosTicks++;
            if(PosTicks > Samples + shuffle_stp)
            {
                shuffle_switch = -shuffle_switch;

                if(shuffle_switch == 1)
                {
                    shuffle_stp = -((Samples * shuffle) / 200);
                }
                else
                {
                    shuffle_stp = (Samples * shuffle) / 200;
                }
                len += PosTicks - 1;

                nbr_ticks++;
                PosTicks = 0;
                if(have_break > 127)
                {
                    pos_patt++;
                }
                else
                {
                    // End the pattern here
                    pos_patt = patternLines[pSequence[i]];
                    rep_counter = -1;
                    rep_pos = 0;
                }
            }
        }
        if(early_exit) break;
        i++;
    }
    len /= MIX_RATE;

    ilen = (int) len;

    song_Seconds = (int) ilen;
    song_Seconds %= 60;
    song_Minutes = (ilen / 60);
    song_Hours = ilen / 60 / 24;

#if !defined(__WINAMP__)
    Display_Song_Length();
#endif

    return((int) (len * 1000));
}

void Reset_Song_Length(void)
{
    song_Hours = 0;
    song_Minutes = 0;
    song_Seconds = 0;
}

// ------------------------------------------------------
// Return the data of an unpacked sample
short *Get_WaveForm(int Instr_Nbr, int Channel, int Split)
{
#if !defined(__WINAMP__)
    if(SamplesSwap[Instr_Nbr])
    {
        return(RawSamples_Swap[Instr_Nbr][Channel][Split]); 
    }
    else
#endif
    {
        return(RawSamples[Instr_Nbr][Channel][Split]); 
    }
}

// ------------------------------------------------------
// Return the number of splits used in an instrument
int Get_Number_Of_Splits(int n_index)
{
    int i;

    // If this is the first waveform of the instrument
    // we set the global volume to max
    int nbr_splits = 0;
    for(i = 0; i < MAX_INSTRS_SPLITS; i++)
    {
        if(RawSamples[n_index][0][i])
        {
            nbr_splits++;
        }
    }
    return nbr_splits;
}

// ------------------------------------------------------
// Clear the data of a given instrument
void Clear_Instrument_Dat(int n_index, int split, int lenfir)
{
    SampleType[n_index][split] = 0;
    LoopStart[n_index][split] = 0;
    LoopEnd[n_index][split] = lenfir;
    LoopType[n_index][split] = SMP_LOOP_NONE;
    SampleLength[n_index][split] = lenfir;
    Finetune[n_index][split] = 0;
    Sample_Amplify[n_index][split] = 1.0f;
    FDecay[n_index][split] = 0.0f;
    Basenote[n_index][split] = DEFAULT_BASE_NOTE;

    if(Get_Number_Of_Splits(n_index) == 0)
    {
        Sample_Vol[n_index] = 0.0f;
        Midiprg[n_index] = -1;
        Synthprg[n_index] = SYNTH_WAVE_OFF;
        beatsync[n_index] = FALSE;

    // Gsm is default compression
#if !defined(__WINAMP__)
        SampleCompression[n_index] = SMP_PACK_GSM;
        SamplesSwap[n_index] = FALSE;
#else
        SampleCompression[n_index] = SMP_PACK_NONE;
#endif
        Mp3_BitRate[n_index] = 0;
        At3_BitRate[n_index] = 0;
        Internal_Quality[n_index] = 0;
    }
}

// ------------------------------------------------------
// Allocate space for a waveform
void AllocateWave(int n_index, int split, long lenfir,
                  int samplechans, int clear,
                  short *Waveform1, short *Waveform2)
{
    // Freeing if memory was allocated before -----------------------
    if(SampleType[n_index][split] != 0)
    {
        if(RawSamples[n_index][0][split]) free(RawSamples[n_index][0][split]);
        RawSamples[n_index][0][split] = NULL;
        if(SampleChannels[n_index][split] == 2)
        {
            if(RawSamples[n_index][1][split]) free(RawSamples[n_index][1][split]);
            RawSamples[n_index][1][split] = NULL;
        }

#if !defined(__WINAMP__)
        if(RawSamples_Swap[n_index][0][split]) free(RawSamples_Swap[n_index][0][split]);
        RawSamples_Swap[n_index][0][split] = NULL;
        if(SampleChannels[n_index][split] == 2)
        {
            if(RawSamples_Swap[n_index][1][split]) free(RawSamples_Swap[n_index][1][split]);
            RawSamples_Swap[n_index][1][split] = NULL;
        }
#endif

    }

    if(clear) Clear_Instrument_Dat(n_index, split, lenfir);

    SampleType[n_index][split] = 1;

    SampleChannels[n_index][split] = samplechans;
    // Was it already supplied ?
    if(Waveform1)
    {
        RawSamples[n_index][0][split] = Waveform1;
    }
    else
    {
        RawSamples[n_index][0][split] = (short *) malloc((lenfir * 2) + 8);
        memset(RawSamples[n_index][0][split], 0, (lenfir * 2) + 8);
    }
    if(samplechans == 2)
    {
        if(Waveform2)
        {
            RawSamples[n_index][1][split] = Waveform2;
        }
        else
        {
            RawSamples[n_index][1][split] = (short *) malloc((lenfir * 2) + 8);
            memset(RawSamples[n_index][1][split], 0, (lenfir * 2) + 8);
        }
    }
}

// ------------------------------------------------------
// Clear any pending input
#if !defined(__WINAMP__)
void Clear_Input(void)
{
    if(snamesel == INPUT_303_PATTERN)
    {
        snamesel = INPUT_NONE;
        Actualize_303_Ed(0);
    }

    if(snamesel == INPUT_SYNTH_NAME)
    {
        snamesel = INPUT_NONE;
        Actualize_Synth_Ed(0);
    }

    if(snamesel == INPUT_MODULE_NAME ||
       snamesel == INPUT_MODULE_ARTIST ||
       snamesel == INPUT_MODULE_STYLE)
    {
        snamesel = INPUT_NONE;
        Actualize_DiskIO_Ed(0);
    }

    if(snamesel == INPUT_INSTRUMENT_NAME)
    {
        snamesel = INPUT_NONE;
        Actualize_Patterned();
    }

    if(snamesel == INPUT_REVERB_NAME)
    {
        snamesel = INPUT_NONE;
        Actualize_Reverb_Ed(0);
    }

    if(snamesel == INPUT_SELECTION_NAME)
    {
        snamesel = INPUT_NONE;
        Actualize_Seq_Ed(0);
    }
}
#endif
