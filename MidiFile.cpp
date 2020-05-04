
// Good info on the midi file standard http://www.somascape.org/midi/tech/mfile.html
//
// This project is just an academic exercise to freshen up C++ and the brain after watching
// javidx9's video on youtube https://www.youtube.com/watch?v=040BKtnDdg0
// I saw that I had to refresh my bit manipulation skills a bit and found that trying to recreate
// the project (and using javdx9's video/solution as a crutch on the way) would be a good exercise.
// 
// After the parsing - I wish to look into how to control real time marshalling of the note-data.
// as a very simple playback sequencer. Probably not to a midi device but a time logged file or something.

#include "MidiFile.hpp"
#include <iostream>
#include <map>
#include <iomanip>
#include <ios>


void debug_print_hex(uint32_t val) {
    auto flgs = std::cout.flags();
    std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << val << '\n';
    std::cout.flags(flgs);
};

// we are currently only interested in note on-/off messages

MidiFile::MidiFile(std::ifstream& file) { 
    parse_midi_file(file); 
}

void MidiFile::parse_midi_file(std::ifstream& file) {

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
    // Some numbers like length of text and sysex will need anything from 1-4 bytes to
    // be exåressed. Only 7 bits of each byte is used to form a 7, 14, 21 or 28 bit number.
    // read_multi_bytes does the bit shifting.
    // DO NOT swap numbers after reading multi byte numbers...
    auto read_multi_bytes = [&file]() { // should probably throw if prematue eof()
        uint32_t result;
        uint8_t bt;

        result = file.get();

        // check if bit 8 is set
        // then the up to 4 bytes may be needed to resolve the value.
        if (result & 0x80) {

            // clear bit 8, and keep reading bytes until bit 8 is zero
            result &= 0x7F;
            do {
                bt = file.get();
                result = result << 7; // make place for new 7 bits
                result |= (bt & 0x7F); // put last 7 bits. Results become 14, 21 or 28 bits
            } while (bt & 0x80);
        }
        return result;
    };
    //auto skip_read = [&file](uint32_t bytes_left_to_skip) {
    //    // Realizing that I can better use file.seekg to move number of bytes into the stream
    //    // but anyways.... 
    //    static char buffer[256];            
    //    while (bytes_left_to_skip) {
    //        uint32_t chunk_size = bytes_left_to_skip > 256 ? 256 : bytes_left_to_skip;
    //        file.read(buffer, chunk_size);
    //        bytes_left_to_skip -= chunk_size;
    //    }
    //}

    // read in the midi file - a few scratch variables
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
    debug_print_hex(num_tracks);

    // read time standard and resolution
    file.read((char*)&tmp16, sizeof(uint16_t));
    tmp16 = swap_16(tmp16);
    SMPTE = tmp16 & 0x8000; // highest bit set => SMPTE
    if (SMPTE) {
        // frames per second
        // TODO: need another midi file to test this.
        fps = ((~((tmp16 & 0xEF00) >> 8)) + 1) & 0x00FF; // Two's complement of bit 8-15... gotta be abetter way
        std::cout << "fps: " << fps;

        // subframes per second
        sfps = tmp16 & 0x00FF;
    }
    else
    {
        ppqn = tmp16 & 0xEFFF;
    }


    // Go through each midi track. Each track has all its content/chunks
    // in contigous areas in the file.
    for (uint16_t trk = 0; trk < num_tracks; ++trk) {

        // read track header then consume the events            
        file.read((char*)&tmp32, sizeof(uint32_t));

        // length in bytes of track chunk
        file.read((char*)&tmp32, sizeof(uint32_t));
        uint32_t numBytes = swap_32(tmp32);
        std::cout << "TRACK --- " << trk << " --- (" << numBytes << " bytes long)\n";

        tracks.push_back(MidiTrack());

        bool end_of_track = false;
        uint8_t prev_status = 0; // needed for "running state" where several midi events share a previous status

        while (!file.eof() && !end_of_track)
        {
            // read deltatimes 2 bytes, and then status (type of operation)
            uint32_t    delta_time = 0;
            uint8_t     status = 0;
            uint8_t     note = 0;
            uint8_t     velocity = 0;
            uint8_t     channel = 0;

            uint32_t    length = 0;
            std::string tmp_string;

            delta_time = read_multi_bytes(); // DO NOT SWAP the decoded multi bytes
            status = file.get();

            // check if running state and restore previous status if so
            if (status < 0x80) { // this is not a status byte - but midi event data
                // move back a position in the stream or else the next reads will be off
                file.seekg(-1, std::ios::cur);
                status = prev_status;
            }

            uint8_t opcode = status & 0xF0;

            switch (opcode) {

            case EventType::NoteOff:  // Most implementation uses NoteOn with velocity==0 a NoteOff   
                prev_status = status;
                channel = status & 0x0F; // channel 0-16, lowest 4 bits
                note = file.get();
                velocity = file.get(); // not used for anything but part of standard event
                tracks[trk].events.push_back({ MidiEvent::EventType::NoteOff, delta_time, note, velocity, channel });
                break;

            case EventType::NoteOn:
                prev_status = status;
                channel = status & 0x0F; // channel 0-16, lowest 4 bits
                note = file.get();
                velocity = file.get();
                if (velocity > 0)
                    tracks[trk].events.push_back({ MidiEvent::EventType::NoteOn, delta_time, note, velocity, channel });
                else // running state it's a NoteOff event.
                    tracks[trk].events.push_back({ MidiEvent::EventType::NoteOff, delta_time, note, velocity, channel });
                break;

            case EventType::ControlChange:
                prev_status = status;
                file.seekg(2, std::ios::cur); // we don't care     
                tracks[trk].events.push_back({ MidiEvent::EventType::Other, delta_time, 0, 0, 0 });
                break;

            case EventType::ProgramChange:
                prev_status = status;
                // channel = status & 0x0F;
                // program = file.get();
                file.get(); // we don't save
                tracks[trk].events.push_back({ MidiEvent::EventType::Other, delta_time, 0, 0, 0 });
                break;

            case EventType::PitchBend:
                prev_status = status;
                // channel = status & 0x0F;
                // amount = next two bytes
                file.seekg(2, std::ios::cur); // skip two
                tracks[trk].events.push_back({ MidiEvent::EventType::Other, delta_time, 0, 0, 0 });
                break;

            case EventType::AfterTouch:
                prev_status = status;
                // channel = status & 0x0F;
                // note/key = file.get();
                // amount = file.get();
                tracks[trk].events.push_back({ MidiEvent::EventType::Other, delta_time, 0, 0, 0 });
                file.seekg(2, std::ios::cur); // skip two
                break;

            case EventType::ChannelPressure:
                prev_status = status;
                // channel = status & 0x0F;
                // amount = file.get();
                file.get(); // we don't care
                tracks[trk].events.push_back({ MidiEvent::EventType::Other, delta_time, 0, 0, 0 });
                break;

            case EventType::SysExAndMeta: // Really system exclusive and metadata...
                prev_status = 0;
                // You need to add these events as well - or accumulate the none Note-Off/On deltatimes
                // to adjust note on/off times for correct note-spacing. I choose to add.
                tracks[trk].events.push_back({ MidiEvent::EventType::Other, delta_time, 0, 0, 0 });

                if (status == 0xF7 || status == 0xF0) { // System exclusive type 1 (not escaped)
                    prev_status = 0;
                    uint32_t length = read_multi_bytes();
                    file.seekg(length, std::ios::cur); // skip                        ´
                }
                else   // Meta event 
                {
                    uint8_t  type = file.get();
                    switch (type) {
                    case 0x00: // Sequence number FF 00 ss ss
                        file.seekg(2, std::ios::cur); // skip  two                       ´
                        break;
                    case 0x01: // text: FF 01 multibytelength <string>
                        length = read_multi_bytes();
                        tmp_string = midi_string(length);
                        break;
                    case 0x02: // copyright: FF 02 multibytelength <string>
                        length = read_multi_bytes();
                        std::cout << "COPYRIGHT: " << midi_string(length) << '\n';
                        break;
                    case 0x03: // FF 03 length text Track or sequence name. 
                        length = read_multi_bytes();
                        tmp_string = midi_string(length);
                        //std::cout << "TRACK NAME: " << tmp_string << '\n';
                        tracks[trk].name = tmp_string;
                        break;
                    case 0x04: // Instrument name
                        length = read_multi_bytes();
                        tmp_string = midi_string(length);
                        std::cout << "INSTRUMENT NAME: " << tmp_string << '\n';
                        tracks[trk].instrument = tmp_string;
                        break;
                    case 0x05: // Lyric
                        length = read_multi_bytes();
                        tmp_string = midi_string(length);
                        //std::cout << "Lyric event: " << tmp_string << '\n'; // Don't keep                            
                        break;
                    case 0x06: // Marker
                        length = read_multi_bytes();
                        tmp_string = midi_string(length);
                        //std::cout << "Marker found: " << tmp_string << '\n';
                        break;
                    case 0x07: // Cue point
                        length = read_multi_bytes();
                        tmp_string = midi_string(length);
                        //std::cout << "CUE POINT: " << tmp_string << '\n';                            
                        break;
                    case 0x08: // Program name
                        length = read_multi_bytes();
                        tmp_string = midi_string(length);
                        //std::cout << "PROGRAM NAME: " << tmp_string << '\n';
                        break;
                    case 0x09: // Device name
                        length = read_multi_bytes();
                        tmp_string = midi_string(length);
                        //std::cout << "DEVICE NAME: " << tmp_string << '\n';
                        break;
                    case 0x20: // FF 20 01 cc Midi Channel Prefix                            
                        file.seekg(2, std::ios::cur); // skip  two    
                        break;
                    case 0x21: // FF 21 01 pp Midi Port
                        file.get(); // Read the 01
                        tracks[trk].port = file.get();
                        break;
                    case 0x2F: // FF 2F 00 End of track                            
                        file.get(); // should be 00                            
                        end_of_track = true;
                        break;
                    case 0x51: // FF 51 03 tt tt tt tempo
                        file.seekg(4, std::ios::cur); // skip  five                            
                        break;
                    case 0x54: // FF 54 05 hr mn se fr ff - SMPTE offset
                        file.seekg(6, std::ios::cur); // skip  payload                            
                        break;
                    case 0x58: // FF 58 04 nn dd cc bb - Time signature
                        file.seekg(5, std::ios::cur); // skip  payload                            
                        break;
                    case 0x59: // FF 59 02 sf mi - Key signature
                        file.seekg(3, std::ios::cur); // skip  payload                            
                        break;
                    case 0x7F: // FF 7F length data - Sequencer specific
                        length = read_multi_bytes();
                        file.seekg(length, std::ios::cur); // skip  payload                         
                        break;
                    default:
                        std::cerr << "ERR: We really should not be here :|\n";
                    } // end switch case META

                } // if some kind of sysex or meta
                break;
            default:
                std::cerr << "ERR: Nope - Should not be here\n";
            } // case status messages
        } // end loop track events            
        std::cout << '\n';
    } // loop through tracks     


    // And convert the events to notes with duration
    // Creating Notes list
    for (auto& t : tracks) {

        uint32_t running_time = 0;

        auto& events = t.events;
        auto& notes = t.notes;
        std::map<uint8_t, MidiEvent> being_processed;

        for (auto e : events) {

            running_time += e.delta_time;

            if (e.event == MidiEvent::EventType::NoteOn)
            {
                e.delta_time = running_time; // it's a copy of the event so we use it as a scratch variable
                being_processed.insert({ e.note, e });
            }
            else if (e.event == MidiEvent::EventType::NoteOff) {
                auto result = being_processed.find(e.note);
                if (result != being_processed.end()) {
                    auto on_event = result->second;
                    notes.push_back({ on_event.note, on_event.velocity, on_event.delta_time, running_time - on_event.delta_time });
                    being_processed.erase(e.note);
                }
            }
        }
    }
}

void midi_test_read(const MidiFile& midi, size_t num_sorted_to_print)
{

    std::vector<MidiTrack> tracks = midi.midi_tracks(); // copy
    

    // for the fun of it... sort all event vectors into one single vector
    // Thinking that would have to be the view for a sequencer
    // I would loose data without the track info. But for now lets try

    uint32_t total_num_events = 0;
    // Get size needed for all events merged. 
    for (uint32_t i = 0; i < tracks.size(); ++i)
        total_num_events += tracks[i].notes.size();

    std::vector<MidiNote> all_tracks_merged;
    all_tracks_merged.reserve(total_num_events);

    // We prepare the sort... keeping one vector with curent index of each track
    std::vector<size_t> current_pos(tracks.size(), { 0 });  // index of where we are in each vector  

    for (size_t tr_idx = 0; tr_idx < tracks.size(); ++tr_idx) {

        while (current_pos[tr_idx] < tracks[tr_idx].notes.size()) {

            size_t winner_track = tr_idx; // we cross fingers for main track
            auto min_val = tracks[tr_idx].notes[current_pos[tr_idx]].start_time;

            for (size_t sub_tx = tr_idx + 1; sub_tx < tracks.size(); ++sub_tx)
            {
                if (current_pos[sub_tx] < tracks[sub_tx].notes.size()) {
                    auto contester = tracks[sub_tx].notes[current_pos[sub_tx]].start_time;
                    if (min_val > contester) {
                        min_val = contester;
                        winner_track = sub_tx;
                    }
                }
            }
            all_tracks_merged.push_back(tracks[winner_track].notes[current_pos[winner_track]]);
            current_pos[winner_track]++;
        }
    }

    size_t counter = 0;
    for (size_t i = 0; i < num_sorted_to_print; ++i )
    {
        MidiNote e = all_tracks_merged[i];
        std::cout << e.start_time << "\t note: " << (int)e.note << "\tDuration: " << e.duration << '\n';        
    }    
}
