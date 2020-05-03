// MidiConsole_1.cpp : This file contains the 'main' function. Program execution begins and ends there.
// http://www.somascape.org/midi/tech/mfile.html

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <iomanip>
#include <ios>
#include <memory>

// we are currently only interested in note on-/off messages

struct MidiEvent {
    uint32_t    sinceLastEvent;
    uint16_t    channel;
    uint16_t    note;
    bool        noteOn;
};

// Track, output MidiEvents


struct MidiTrack {
    std::string name;
    uint32_t channel; // a track with format 1 has may spread to all channels
    std::vector<MidiEvent> events;
};

auto print_hex = [](uint32_t val) {
    auto flgs = std::cout.flags();
    std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << val << '\n';
    std::cout.flags(flgs);
};

class MidiFile {

    enum EventType : uint8_t {
        VoiceNoteOff = 0x80,
        VoiceNoteOn = 0x90, 
        VoiceAfterTouch = 0xA0,
        VoiceControlChange = 0xB0,
        VoiceProgramChange = 0xC0,
        VoiceChannelPressure = 0xD0,
        VoicePitchBend = 0xE0,
        SystemExclusive = 0xF0
    };

public:
    MidiFile(){}
    MidiFile(std::ifstream& file) { ParseFile(file); }
   
    void ParseFile(std::ifstream& file) {
      
        auto swap_32 = [](uint32_t chunk) {
            return ((chunk >> 24) | ((chunk >> 8) & 0x0000FF00) | ((chunk << 8) & 0x00FF0000) | (chunk << 24));
        };
        auto swap_16 = [](uint16_t chunk) {
            return chunk >> 8 | chunk << 8;
        };
        auto midi_string = [&file](uint32_t length) {
            std::string s;
            for (uint32_t i = 0; i < length; ++i)
                s += file.get();
            return s;
        };
        auto read_bytes = [&file]() { // should probably throw if prematue eof()
            uint32_t result;
            uint8_t bt;

            result = file.get();

            // check if bit 8 is set
            if (result & 0x80) { 

                // clear bit 8, and kepp reading bytes until bit 8 is zero
                result &= 0x80; 
                do {                    
                    bt = file.get();
                    result = result << 7; // make place for new 7 bits
                    result |= (bt & 0x7F); // put last 7 bits. Results become 14, 21 or 28 bits (hopefully not more)
                } while (bt & 0x80);
            }
            return result;
        };

        // read in the midi file
        uint32_t tmp32;
        uint16_t tmp16; 

        // 4 byte file header file id 0x6468544d TMhd
        file.read((char*)&file_id, sizeof(uint32_t));
        if (!(file_id & 0x6468544d)) {
            std::cerr << "File does not appear to be a valid midi-file:\n";
            return;
        }        

        // next are 3 16 bit ints - only interested in number of tracks and maybe timing
        // 4 byte header length should be six... for now read and ignore some data
        file.read((char*)&tmp32, sizeof(uint32_t));        

        // read and ignore format
        file.read((char*)&tmp16, sizeof(uint16_t));

        // read number of tracks
        file.read((char*)&tmp16, sizeof(uint16_t));
        num_tracks = swap_16(tmp16);
        print_hex(num_tracks);

        // read time standard and resolution
        file.read((char*)&tmp16, sizeof(uint16_t));
        tmp16 = swap_16(tmp16);
        SMPTE = tmp16 & 0x8000; // highest bit set => SMPTE
        if (SMPTE) {
            // frames per second
            fps = ((~((tmp16 & 0xEF00) >> 8)) + 1) & 0x00FF; // Two's complement of bit 8-15... gotta be abetter way
            std::cout << "fps: " << fps;

            // subframes per second
            sfps = tmp16 & 0x00FF;
        }        
        else
        {
            ppqn = tmp16 & 0xEFFF;
        }


        // read midi tracks
        for (uint16_t trk = 0; trk < num_tracks; ++trk) {

            // read track header then consume the events            
            file.read((char*)&tmp32, sizeof(uint32_t));
            
            // length in bytes of track chunk
            file.read((char*)&tmp32, sizeof(uint32_t));
            uint32_t numBytes = swap_32(tmp32);
            std::cout << "New track -------\n";

            bool end_of_track = false;
            uint8_t prev_status = 0;

            while (!file.eof() && !end_of_track) 
            {
                // read deltatimes 2 bytes, and status 8 bytes 
                uint32_t    delta_time = 0;
                uint8_t     status = 0;
                uint8_t note = 0;
                uint8_t velocity = 0;
                uint8_t channel = 0;

                delta_time = read_bytes();
                status = file.get();

                uint8_t opcode = status & 0xF0;

                switch (opcode) {

                case EventType::VoiceNoteOff:     
                    channel = ((status & 0x0F00) >> 8);
                    note = file.get();
                    velocity = file.get();
                    break;

                case EventType::VoiceNoteOn:
                    break;

                case EventType::VoiceProgramChange:
                    break;

                case EventType::VoicePitchBend:
                    break;

                case EventType::VoiceAfterTouch:
                    break;

                case EventType::VoiceChannelPressure:
                    break;

                case EventType::SystemExclusive:
                    break;

                case 0xFF: // End of track
                    end_of_track = true;

                }
            } // end loop through track events
            

        } // loop through tracks
        


    }

private:
    uint32_t                file_id = 0;    
    uint16_t                num_tracks=0;
    std::vector<MidiTrack>  tracks;

    // timing stuff
    bool                    SMPTE  = false;
    uint16_t                fps = 0; // not in use if metrics
    uint16_t                sfps = 0; 
    uint16_t                ppqn = 0; // not in use if
};


int main()
{

   /* uint16_t tmp16 = 0xE200;
    uint16_t fps = ((~((tmp16 & 0xEF00)>>8)) + 1) & 0x00FF;*/

    std::ifstream midistream("organ.mid", std::ios::binary);                

    if (midistream.is_open()) {
        MidiFile midi(midistream);
        midistream.close();
    }

    return 0;
}
