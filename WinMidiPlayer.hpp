/*
    Trying some old simple code from https://www.rosettacode.org/wiki/Musical_scale

    in the MidiFile we have only saved MidiOn and midiOff messages.
    No program or control messages recorded in this first and probably final version.

    So only goal is to have some control of timing. Stopping, and replay.
    It will probably sound awful - but hopefully recognizable melody.

    If that happens... I am happy will probably stop.

    I get most of my midi-file details from this link: http://www.somascape.org/midi/tech/mfile.html

 */

#ifndef WIN_MIDI_PLAYER_HPP_
#define WIN_MIDI_PLAYER_HPP_

#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <vector>
#include "MidiFile.hpp"

#pragma comment ( lib, "winmm.lib" )

typedef unsigned char byte;

typedef union
{
    unsigned long word;
    unsigned char data[4];
} midi_msg;


class WinMidiPlayer {
public:
    WinMidiPlayer(const MidiFile& midi)
        : tracks{ midi.midi_tracks() }
    {
        if (midiOutOpen(&device, 0, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        {
            throw std::runtime_error("Error opening MIDI Output...");            
        }
        else
            std::cout << "Success";
    }
    ~WinMidiPlayer() {
        Reset(); // free timers etc?
        midiOutReset(device);
        midiOutClose(device);
        // free resources from mm-library
    }

    void Play() {
        if (is_running) Rewind();
        // activate timers
    }

    void Pause() {
        is_running = false;
        is_paused = true;
        // deactivate timers
        // freeze timer information     
    }

    void Rewind() { // 
        Reset();
    }
    void PlayResume() {

    }

private:
    void Reset() {
        if (is_running) {
            Pause();
            for (auto& i : track_positions)
                i = 0; 
        }
    }
    std::vector<MidiTrack> tracks;
    std::vector<size_t> track_positions; // will
    bool    is_running = false;
    bool    is_paused = false;
    HMIDIOUT device = 0;
    midi_msg scratch_msg;
};

#endif WIN_MIDI_PLAYER_HPP_