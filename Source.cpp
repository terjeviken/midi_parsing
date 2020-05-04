#include "MidiFile.hpp"

#include <fstream>
#include <ios>

int main() {

    std::ifstream midistream("organ.mid", std::ios::binary);    

    if (midistream.is_open()) {
        MidiFile midi(midistream);
        // will pull all the notes-tracks and sort into a single
        // vector sorted by start_time and print out the xxx first sorted elements
        midi_test_read(midi, 300); 
    }
	
	return 0;
}