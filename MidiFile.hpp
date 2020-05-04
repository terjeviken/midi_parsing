#ifndef  MIDIFILE_HPP_
#define MIDIFILE_HPP_


// Good info on the midi file standard http://www.somascape.org/midi/tech/mfile.html
//
// This project is just an academic exercise to freshen up C++ and the brain after watching
// javidx9's video on youtube https://www.youtube.com/watch?v=040BKtnDdg0
// I saw that I had to refresh my bit manipulation skills a bit and found that trying to recreate
// the project (and using javdx9's video/solution as a crutch on the way) would be a good exercise.
// 
// After the parsing - I wish to look into how to control real time marshalling of the note-data.
// as a very simple playback sequencer. Probably not to a midi device but a time logged file or something.


#include <fstream>
#include <vector>

// we are currently only interested in note on-/off messages

struct MidiEvent {
    enum class EventType {
        NoteOn,
        NoteOff,
        Other // As with javidx9 - I will only care about note on and off.
    } event;
    uint32_t    delta_time = 0;
    uint8_t     note = 0;
    uint8_t     velocity = 0;
    uint8_t     channel = 0; // will probably never use before moving on to another project
};
struct MidiNote {
    uint8_t     note = 0;
    uint8_t     velocity = 0;
    uint32_t    start_time = 0; // absolute start time from beginning or delta time?
    uint32_t    duration = 0;
};
struct MidiTrack {
    std::string             name;
    std::string             instrument;
    std::vector<MidiEvent>  events;
    std::vector<MidiNote>   notes;
    uint8_t                 port; // may change during track? Hmm. think so    
};


class MidiFile {

    enum EventType : uint8_t {
        NoteOff = 0x80,
        NoteOn = 0x90,
        AfterTouch = 0xA0,
        ControlChange = 0xB0,
        ProgramChange = 0xC0,
        ChannelPressure = 0xD0,
        PitchBend = 0xE0,
        SysExAndMeta = 0xF0 // hmmm sysex1 and 2 + META        
    };

public:
    MidiFile(std::ifstream& file);
    std::vector<MidiTrack> midi_tracks() const {
        return tracks;
    }

protected:
    void parse_midi_file(std::ifstream& file);

private:
    uint32_t                file_id = 0;
    uint16_t                num_tracks = 0;
    std::vector<MidiTrack>  tracks;

    // timing stuff
    bool                    SMPTE = false;
    uint16_t                fps = 0; // not in use if metrics
    uint16_t                sfps = 0;
    uint16_t                ppqn = 0; 
};


void midi_test_read(const MidiFile&, size_t num_sorted_to_print);

#endif // ! MIDIFILE_HPP_