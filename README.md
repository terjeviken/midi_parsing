# midi_parsing
Midi Parsing

I watched Javidx9's youtube video on parsing a midi file. Being a former hobby musician having had my share of midi instruments I found this inspiring. So took it upon me to find a good description of the midi file standard and try recreate his work. This to freshen up my c++ skills. - I must admit I used javidx9's code as a crutch several times. I even should have read his code more detailed than I did. I spent hours locating my errors. Good practice though. 

javidx9's video on youtube https://www.youtube.com/watch?v=040BKtnDdg0
Good info on the midi file standard http://www.somascape.org/midi/tech/mfile.html

The midi file parsing is covered by MidiFile.hpp and MidiFile.cpp.

The next thing I would like to test (after reading in all tracks) is to simulate a simple sequencer. Maybe run it in a loop and dispatching the events in realtime. - For now it will just be writing each event timely to a file or std::cout, not midi-device - and based on the system clock.

