
#include "ofxBeatClock.h"


//---------------------------
void ofxBeatClock::setup_Gui_CLOCKER(){
    
    //-
    
    int panelW, x, y, padW;
    panelW = 200;
    x = 300;
    y = 10;
    padW = 5;
    
    //-
    
    // 1. control
    
    params_control.setName("CLOCK CONTROL");
    params_control.add(ENABLE_CLOCKS.set("ENABLE", true));
    params_control.add(PLAYER_state.set("PLAY", false));
    params_control.add(ENABLE_INTERNAL_CLOCK.set("INTERNAL", false));
    params_control.add(ENABLE_MIDI_CLOCK.set("MIDI-IN CLOCK", true));
    ofAddListener(params_control.parameterChangedE(), this, &ofxBeatClock::Changed_gui_CLOCKER);
    //-
    
    // 2. monitor
    
    params_clocker.setName("BPM CONTROL");
    params_clocker.add(BPM_Global.set("BPM", BPM_INIT, 60, 300));
    params_clocker.add(BPM_TimeBar.set("ms", 60000 / BPM_Global, 1, 5000));
    params_clocker.add(BPM_Tap_Tempo_TRIG.set("TAP", false));
    ofAddListener(params_clocker.parameterChangedE(), this, &ofxBeatClock::Changed_gui_CLOCKER);

    //-
    
    // 3.1
    
    // TAP TEMPO
    
    tapMachine = make_shared<ofxTapMachine>();
    ofAddListener(tapMachine->bar.event, this, &ofxBeatClock::barFunc);
    ofAddListener(tapMachine->minim.event, this, &ofxBeatClock::minimFunc);
    ofAddListener(tapMachine->crochet.event, this, &ofxBeatClock::crochetFunc);
    
    //-
    
    // 3.2
    
    // daw metro
    
    // add bar/beat/sixteenth listener on demand
    metro.addBeatListener(this);
    metro.addBarListener(this);
    metro.addSixteenthListener(this);
    
    DAW_bpm.addListener(this, &ofxBeatClock::Changed_DAW_bpm);
    DAW_active.addListener(this, &ofxBeatClock::Changed_DAW_active);
    DAW_bpm.set("BPM", 120, 30, 240);
    DAW_active.set("Active", false);
    
    params_daw.setName("DAW CONTROL");
    params_daw.add(DAW_bpm);
    params_daw.add(DAW_active);
    
    metro.setBpm(DAW_bpm);
    
    //--
    
    container_controls = gui_CLOCKER.addGroup(params_control);
    container_controls->setPosition(ofPoint(x, y));
    
    container_clocker = gui_CLOCKER.addGroup(params_clocker);
    container_clocker->setPosition(ofPoint(x + (panelW + padW), y));
    
    container_daw = gui_CLOCKER.addGroup(params_daw);
    container_daw->setPosition(ofPoint(x + 2 * (panelW + padW), y));
    
    //-

    // LOAD LAST SETTINGS
    
    pathSettings = "CLOCKER_settings.xml";//default
    loadSettings(pathSettings);
    
    //--
    
    
}

//--------------------------------------------------------------
void ofxBeatClock::setup()
{
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    //-
    
    // DRAW STUFF
    
    TTF_small.load("fonts/mono.ttf", 8);
    TTF_medium.load("fonts/mono.ttf", 12);
    TTF_big.load("fonts/mono.ttf", 18);
    
    metronome_ball_pos.set(400, 700);
    metronome_ball_radius = 25;
    
    //---
    
    // SOUNDS
    
    tic.load("sounds/click1.wav");
    tic.setVolume(1.0f);
    tic.setMultiPlay(false);
    
    tac.load("sounds/click2.wav");
    tac.setVolume(0.25f);
    tac.setMultiPlay(false);
    
    //-
    
    // TEXT DISPLAY
    
    BPM_bar = "0";
    BPM_beat = "0";
    BPM_sixteen = "0";
    
    //--
    
    // MIDI IN CLOCK
    
    setup_MIDI_CLOCK();
   
    
    //-----
    
    setup_Gui_CLOCKER();
}

//--------------------------------------------------------------
void ofxBeatClock::setup_MIDI_CLOCK()
{
    
    // EXTERNAL MIDI CLOCK:
    
    ofLogNotice() << "==================================================";
    ofLogNotice() << "\t MIDI CLOCK IN:";
    ofSetLogLevel("ofxMidiClock", OF_LOG_NOTICE);
    
    midiIn_CLOCK.listInPorts();
    //midiIn_CLOCK.openPort(7);
    int midi_IN_Clock_PortSelected = 0;
    midiIn_CLOCK.openPort(midi_IN_Clock_PortSelected);
    //midiIn_CLOCK.openVirtualPort(midi_IN_Clock_PortSelected);
    
    // --------------------------------------------------------------------
    
    //midiIn_CLOCK.openPort("loopMIDI Port 1 7");
    //ofLogNotice() << "PORT NAME " << midiIn_CLOCK.getInPortName(7);
    
    ofLogNotice() << "\t connected to MIDI CLOCK IN port: " << midiIn_CLOCK.getPort();
    ofLogNotice() << "==================================================";
    
    // TODO: IGNORE SYSX
    midiIn_CLOCK.ignoreTypes(true, // sysex  <-- don't ignore timecode messages!
                             false, // timing <-- don't ignore clock messages!
                             true); // sensing
    
    // add ofxBeatClock as a listener
    midiIn_CLOCK.addListener(this);
    
    clockRunning = false; //< is the clock sync running?
    MIDI_beats = 0; //< song pos in beats
    MIDI_seconds = 0; //< song pos in seconds, computed from beats
    MIDI_CLOCK_bpm = 120; //< song tempo in bpm, computed from clock length

    //bpm_CLOCK.addListener(this, &ofxBeatClock::bpm_CLOCK_Changed);
    
    //-
    
    MIDI_beatsInBar.addListener(this, &ofxBeatClock::Changed_MIDI_beatsInBar);
    
    //-
    
}


//--------------------------------------------------------------
void ofxBeatClock::Changed_gui_CLOCKER(ofAbstractParameter& e) // patch change
{
    string wid = e.getName();
    ofLogNotice() << "Changed_gui_CLOCKER '" << wid << "': " << e;
    
    if (wid == " ")
    {

    }
    
    else if (wid == "TAP" && BPM_Tap_Tempo_TRIG)
    {
        if (BPM_Tap_Tempo_TRIG == true)
        {
            BPM_Tap_Tempo_TRIG = false; //should be button
            
            ofLogNotice() << "BPM_Tap_Tempo_TRIG: " << BPM_Tap_Tempo_TRIG;
            
            tapMachine->tap();
        }
    }
    
    else if (wid == "PLAY")
    {
        ofLogNotice() << "PLAYER_state: " << PLAYER_state;

        // clocks are disabled or using external midi clock. dont need playing, just to be enabled
        
        if ( (ENABLE_MIDI_CLOCK || !ENABLE_CLOCKS || !ENABLE_INTERNAL_CLOCK)  && PLAYER_state )
        {
            PLAYER_state = false; //skip and restore to false the play button
        }
       
        else if (ENABLE_CLOCKS && ENABLE_INTERNAL_CLOCK)
        {
            if (PLAYER_state == true) //play
            {
                PLAYER_START();
            }
            
            else //stop
            {
                PLAYER_STOP();
            }
        }
    }
    
    else if (wid == "BPM")
    {
        ofLogNotice() << "NEW BPM   : " << BPM_Global;
        
        BPM_TimeBar = 60000 / BPM_Global;// 60,000 / BPM = one beat in milliseconds
        
        ofLogNotice() << "TIME BEAT : " << BPM_TimeBar << "ms";
        ofLogNotice() << "TIME BAR  : " << 4 * BPM_TimeBar << "ms";
    }
    
    else if (wid == "INTERNAL")
    {
        ofLogNotice() << "CLOCK INTERNAL: " << ENABLE_INTERNAL_CLOCK;
        
        if (ENABLE_INTERNAL_CLOCK)
        {
            ENABLE_MIDI_CLOCK = false;
            
            //-
            
            // text display
            BPM_input = "INTERNAL";
            BPM_name = "'DAW METRO'";
        }
        else
        {
            if (PLAYER_state) PLAYER_state = false;
            if (DAW_active) DAW_active = false;
            
            // TEXT DISPLAY
            BPM_bar = "0";
            BPM_beat = "0";
            BPM_sixteen = "0";
        }
    }
    
    else if (wid == "MIDI-IN CLOCK")
    {
        ofLogNotice() << "MIDI-IN CLOCK: " << ENABLE_MIDI_CLOCK;
        
        if (ENABLE_MIDI_CLOCK)
        {
            ENABLE_INTERNAL_CLOCK = false;

            if (PLAYER_state) PLAYER_state = false;
            if (DAW_active) DAW_active = false;
            
            //-
            
            // text display
            BPM_input = "EXTERNAL MIDI " + ofToString(midiIn_CLOCK.getPort());
            BPM_name = "'" + ofToString( midiIn_CLOCK.getName() +"'");
        }
        else
        {
            // TEXT DISPLAY
            BPM_bar = "0";
            BPM_beat = "0";
            BPM_sixteen = "0";
        }
    }
    
    else if (wid == "ENABLE")
    {
        ofLogNotice() << "ENABLE: " << ENABLE_CLOCKS;
        
        if (!ENABLE_CLOCKS)
        {
            if (ENABLE_MIDI_CLOCK)
            {
                ENABLE_MIDI_CLOCK = false;
            }
            
            if (ENABLE_INTERNAL_CLOCK)
            {
                ENABLE_INTERNAL_CLOCK = false;
            }
            
            //PLAYER_STOP();
            if (PLAYER_state) PLAYER_state = false;
            if (DAW_active) DAW_active = false;
        }
        
//        else if (ENABLE_CLOCKS)
//        {
//            if (!ENABLE_INTERNAL_CLOCK)
//            {
//                ENABLE_INTERNAL_CLOCK = true;
//            }
//        }
    }
}


//--------------------------------------------------------------
void ofxBeatClock::update()
{
    
    //-
    
    // INTERNAL
    
//    if (ENABLE_INTERNAL_CLOCK){
//        BPM_value = tapMachine->getBPM();
//    }
    
    //------------------------------------------------------------------------
    
    // MIDI CLOCK
    
    // read bpm with a clock refresh or every frame if not defined time-clock-refresh:
#ifdef BPM_MIDI_CLOCK_REFRESH_RATE
    if (ofGetElapsedTimeMillis() - bpm_CheckUpdated_lastTime >= BPM_MIDI_CLOCK_REFRESH_RATE)
    {
#endif
        //ofLogNotice() << "BPM UPDATED" << ofGetElapsedTimeMillis() - bpm_CheckUpdated_lastTime;
        
        //-
        
#ifdef BPM_MIDI_CLOCK_REFRESH_RATE
        bpm_CheckUpdated_lastTime = ofGetElapsedTimeMillis();
    }
#endif
    
    //--
    
    // BPM ENGINE:
    
//    // TODO: SE ESTA USANDO EN DOS LUGARES LA VARIABLE FLAG!!
//    if (BPM_gotBeat == true)
//    {
//        //----
//
//        BPM_LAST_Tick_Time_ELLAPSED_PRE = BPM_LAST_Tick_Time_ELLAPSED;
//        BPM_LAST_Tick_Time_ELLAPSED = ofGetElapsedTimeMillis() - BPM_LAST_Tick_Time_LAST;//test
//        BPM_LAST_Tick_Time_LAST = ofGetElapsedTimeMillis();//test
//        ELLAPSED_diff = BPM_LAST_Tick_Time_ELLAPSED_PRE - BPM_LAST_Tick_Time_ELLAPSED;
//
//        //----
//
//        if (bpm.barIndex == 0)
//        {
//            if (ENABLE_sound)
//                mySound1.play();
//
//            //ofLogNotice() << "| ! BEAT ! |";
//            ofLogNotice() << "| ! BEAT ! | " << BPM_LAST_Tick_Time_ELLAPSED << " | " << ELLAPSED_diff;//test
//        }
//
//        else
//        {
//            if (ENABLE_sound)
//                mySound2.play();
//
//            //ofLogNotice() << "|   BEAT   |";
//            ofLogNotice() << "|   BEAT   | " << BPM_LAST_Tick_Time_ELLAPSED << " | " << ELLAPSED_diff;//test
//        }
//
//        BPM_gotBeat = false;
//    }
    
    //-
    
    ofSoundUpdate();
}

//--------------------------------------------------------------
void ofxBeatClock::draw()
{
    draw_BPM_CLOCK();
    
    draw_Tapper();
    
    draw_DAW();
}

//--------------------------------------------------------------
void ofxBeatClock::draw_BPM_CLOCK(){
    
    int interline = 10; // line heigh
    int i = 0;
    int px = 10;
    int py = 20;
    int paddingAlign = 0;
    
    //--
    
    // 2. TEXT INFO:
    
    if (ENABLE_INTERNAL_CLOCK)
    {

    }
    
    else if(ENABLE_MIDI_CLOCK)
    {
        MIDI_quarters = MIDI_beats / 4; // convert total # beats to # quarters
        MIDI_bars = ( MIDI_quarters / 4) + 1; // compute # of bars
        MIDI_beatsInBar = ( MIDI_quarters % 4) + 1; // compute remainder as # notes within the current bar
        
        BPM_bar = ofToString( MIDI_bars );
        BPM_beat = ofToString( MIDI_beatsInBar );
        BPM_sixteen = "-";
    }
    
    ofPushMatrix();
    ofTranslate(px, py);
    
    TTF_message = "CLOCK: " + BPM_input ;
    TTF_small.drawString(TTF_message, 0, interline * i++);
    
    TTF_message = ofToString(BPM_name);
    TTF_small.drawString(TTF_message, 0, interline * i++); i++;
    
    //ofTranslate(paddingAlign, 7);
    
    TTF_message = ("BPM : " + ofToString( BPM_Global ));
    TTF_small.drawString(TTF_message, 0, interline * i++);
    
    TTF_message = ("BAR : " + BPM_bar );
    TTF_small.drawString(TTF_message, 0, interline * i++);
    
    TTF_message = ("BEAT: " + BPM_beat );
    TTF_small.drawString(TTF_message, 0, interline * i++);
    
    TTF_message = ("16th: " + BPM_sixteen );
    TTF_small.drawString(TTF_message, 0, interline * i++);
    
    ofPopMatrix();
    
    //--
    
    // 2. BEATS SQUARES
    
    py += 95;
    int w = 50;//squares beats size
    
    for (int i = 0; i < 4; i++) {
        ofPushStyle();
        
        //--
        
        if (ENABLE_MIDI_CLOCK)
        {
            if (i <= (MIDI_beatsInBar - 1))
                clockRunning ? ofSetColor(192) : ofSetColor(32);
            else
                ofSetColor(32);
        }
        
        //--
        
        else if (ENABLE_INTERNAL_CLOCK)
        {
            if ( metro.getBeat() > i )
            {
                PLAYER_state ? ofSetColor(192) : ofSetColor(32);
            }
            
            else
                ofSetColor(32);
            
//            if ( (tapMachine->bar.count % 4) >= i )
//            {
//                //PLAYER_state ? ofSetColor(192) : ofSetColor(32);
//                ofSetColor(192);//for tap mode..
//            }
//
//            else
//                ofSetColor(32);
        }
        
        //-
        
        else //disabled both modes
        {
            ofSetColor(32);//black
        }
        
        //--
        
        
        ofFill();
        ofDrawRectangle(px + i * w, py, w, w);
        ofNoFill();
        ofSetColor(8);
        ofSetLineWidth(2);//border
        ofDrawRectangle(px + i * w, py, w, w);
        
        ofPopStyle();
    }
    
    //--
    
    // 3. TICK BALL:
    
    ofPushStyle();
    
    ofSetColor(16); // ball background
    ofDrawCircle(metronome_ball_pos.x, metronome_ball_pos.y, metronome_ball_radius);
    
    if (ENABLE_MIDI_CLOCK)
    {
        if (bpm_beat_TICKER == true)
        {
            if (MIDI_beatsInBar == 1)
                ofSetColor(ofColor::red);
            else
                ofSetColor(ofColor::white);
            
            ofDrawCircle(metronome_ball_pos.x, metronome_ball_pos.y, metronome_ball_radius);
            
            bpm_beat_TICKER = false;
            
            beatsInBar_PRE = MIDI_beatsInBar;//test
        }
    }
    
    else if (ENABLE_INTERNAL_CLOCK && beat_changed)
    {
        beat_changed = false;
        
        if (tapMachine->bar.count % 4 == 0)
            ofSetColor(ofColor::red);
        else
            ofSetColor(ofColor::white);
        
        ofDrawCircle(metronome_ball_pos.x, metronome_ball_pos.y, metronome_ball_radius);
    }
    
    ofPopStyle();
    
    //-
}

//--------------------------------------------------------------
void ofxBeatClock::exit()
{

    PLAYER_state = false;
    if (!ENABLE_INTERNAL_CLOCK && !ENABLE_MIDI_CLOCK)
        ENABLE_INTERNAL_CLOCK = true;
    
    saveSettings(pathSettings);

    //--
    
    midiIn_CLOCK.closePort();
    midiIn_CLOCK.removeListener(this);
    
    //-
    
    // TODO: remove listeners
}

//--------------------------------------------------------

void ofxBeatClock::PLAYER_START()//only used in internal mode
{
    ofLogNotice() << "PLAYER_START";
    
    if (ENABLE_INTERNAL_CLOCK && ENABLE_CLOCKS)
    {
        ofLogNotice() << "START";
        
        if (!PLAYER_state) PLAYER_state = true;
        if (!DAW_active) DAW_active = true;
        
        //        tapMachine->toggleStart();
        
        //        metro.start();
    }
    else
    {
        ofLogNotice() << "skip";
    }
}

//--------------------------------------------------------------
void ofxBeatClock::PLAYER_STOP()//only used in internal mode
{
    ofLogNotice() << "PLAYER_STOP";
    
    if (ENABLE_INTERNAL_CLOCK && ENABLE_CLOCKS)
    {
        ofLogNotice() << "STOP";
        
        if (PLAYER_state) PLAYER_state = false;
        if (DAW_active) DAW_active = false;
        
        ////        if ( tapMachine->isStart() )
        //        tapMachine->toggleStart();
        
        //        metro.stop();
    }
    else
    {
        ofLogNotice() << "skip";
    }
}


//--------------------------------------------------------------
void ofxBeatClock::Changed_MIDI_beatsInBar(int & beatsInBar) {
    if (beatsInBar != beatsInBar_PRE) {
        ofLogVerbose() << "MIDI-IN CLOCK TICK! " << beatsInBar;
        bpm_beat_TICKER = true;

        if (ENABLE_MIDI_CLOCK && ENABLE_CLOCKS && ENABLE_sound)
        {
            // play tic on the first beat of a bar
            if (beatsInBar == 1) {
                tic.play();
            }
            else {
                tac.play();
            }
        }
    }
}


//--------------------------------------------------------------
void ofxBeatClock::newMidiMessage(ofxMidiMessage& message) {
    
    if ( ENABLE_MIDI_CLOCK && ENABLE_CLOCKS )
    {
        // 1. MIDI CLOCK:
        
        if ((message.status == MIDI_TIME_CLOCK) ||
            (message.status == MIDI_SONG_POS_POINTER) ||
            (message.status == MIDI_START) ||
            (message.status == MIDI_CONTINUE) ||
            (message.status == MIDI_STOP))
        {
            
            //    midiCLOCK_Message = message;
            
            // 1. MIDI CLOCK
            
            // update the clock length and song pos in beats
            if (MIDI_clock.update(message.bytes))
            {
                // we got a new song pos
                MIDI_beats = MIDI_clock.getBeats();
                MIDI_seconds = MIDI_clock.getSeconds();
            }
            
            // compute the seconds and bpm
            
            switch (message.status)
            {
                    // compute seconds and bpm live, you may or may not always need this
                    // which is why it is not integrated into the ofxMidiClock parser class
                    
                case MIDI_TIME_CLOCK:
                    MIDI_seconds = MIDI_clock.getSeconds();
                    MIDI_CLOCK_bpm += (MIDI_clock.getBpm() - MIDI_CLOCK_bpm) / 5; // average the last 5 bpm values
                   // no break here so the next case statement is checked,
                   // this way we can set clockRunning if we've missed a MIDI_START
                   // ie. master was running before we started this example
                    
                    BPM_Global = MIDI_CLOCK_bpm;
                    
                    //-
                    
                    if (DAW_SlaveToInternal)
                        DAW_bpm = MIDI_CLOCK_bpm;
                    
                    //-
                    
                    // transport control
                    
                case MIDI_START: case MIDI_CONTINUE:
                    if (!clockRunning) {
                        clockRunning = true;
                        ofLog() << "clock started";
                    }
                    break;
                    
                case MIDI_STOP:
                    if (clockRunning) {
                        clockRunning = false;
                        ofLog() << "clock stopped";
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
}

//--------------------------------------------------------------

// TAP TEMPO

void ofxBeatClock::barFunc(int &count){
    cout<<"barCount : "<<count<<endl;
    beat_changed = true;
}
void ofxBeatClock::minimFunc(int &count){
    cout<<"minimCount : "<<count<<endl;
}
void ofxBeatClock::crochetFunc(int &count){
    cout<<"crochetCount : "<<count<<endl;
}

//--------------------------------------------------------------
void ofxBeatClock::draw_Tapper(){
    
    ofPushMatrix();
    ofTranslate(0, 300);
    ofPushStyle();
    
//    ofSetColor(255, 0, 0);
//    ofDrawCircle(50+(ofGetWidth()-100)*tapMachine->bar.normalized, 200, 10);
//
//    ofSetColor(0, 255, 0);
//    ofDrawCircle(50+(ofGetWidth()-100)*tapMachine->minim.normalized, 250, 10);
//
//    ofSetColor( 0, 0, 255);
//    ofDrawCircle(50+(ofGetWidth()-100)*tapMachine->crochet.normalized, 300, 10);
//
//
//    ofSetColor(255, 255, 0);
//    ofDrawCircle(50+(ofGetWidth()-100)*tapMachine->twoBar.normalized, 350, 10);
//    ofSetColor(0, 255, 255);
//    ofDrawCircle(50+(ofGetWidth()-100)*tapMachine->fourBar.normalized, 400, 10);
    
    
    string msg="===============[ ofxTapMachine example ]===============\n";
    msg += "press space bar more than 3 times to get average BPM.\n";
//    msg += "FPS        : " + ofToString(ofGetFrameRate(), 2) + "\n";
    msg += "BPM        : " + ofToString(tapMachine->getBPM(),2) + "\n";

//    msg += "bar        : " + ofToString(tapMachine->bar.count) + "\n";
//    msg += "minim      : " + ofToString(tapMachine->minim.count) + "\n";
//    msg += "crochet    : " + ofToString(tapMachine->crochet.count) + "\n";
//    msg += "quaver     : " + ofToString(tapMachine->quaver.count) + "\n";
    
    msg += "bar        : " + ofToString(tapMachine->bar.count % 4) + "\n";
    msg += "minim      : " + ofToString(tapMachine->minim.count % 8) + "\n";
    msg += "crochet    : " + ofToString(tapMachine->crochet.count % 16) + "\n";
    msg += "quaver     : " + ofToString(tapMachine->quaver.count % 32) + "\n";
    
    ofDrawBitmapStringHighlight(msg, 10, 10);
    
    ofPopStyle();
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofxBeatClock::saveSettings(string path)
{
    pathSettings = path;//store default
    
    // save settings
    ofXml settings;
    ofSerialize(settings, params_control);
    settings.save(path);
}

//--------------------------------------------------------------
void ofxBeatClock::loadSettings(string path)
{
    pathSettings = path;//store default
    
    // load settings
    ofXml settings;
    settings.load(path);
    ofDeserialize(settings, params_control);

}

//--------------------------------------------------------------

// DAW METRO

//--------------------------------------------------------------
void ofxBeatClock::onBarEvent(int & bar) {
    
    ofLogVerbose() << "DAW METRO - BAR: " << bar;
    
    if (ENABLE_INTERNAL_CLOCK)
    {
        BPM_bar = ofToString(bar);
    }
}

//--------------------------------------------------------------
void ofxBeatClock::onBeatEvent(int & beat) {

    ofLogVerbose() << "DAW METRO - BEAT: " << beat;
    
    if (ENABLE_INTERNAL_CLOCK && ENABLE_CLOCKS && ENABLE_sound)
    {
        BPM_beat = ofToString(beat);
        
        // play tic on the first beat of a bar
        if (beat == 1)
        {
            tic.play();
        }
        else
        {
            tac.play();
        }
    }
}

//--------------------------------------------------------------
void ofxBeatClock::onSixteenthEvent(int & sixteenth) {
    
    ofLogVerbose() << "DAW METRO - 16th: " << sixteenth;
    
    if (ENABLE_INTERNAL_CLOCK)
    {
        BPM_sixteen = ofToString(sixteenth);
    }
}

//--------------------------------------------------------------
void ofxBeatClock::Changed_DAW_bpm(float & value) {
    metro.setBpm(value);
    metro.resetTimer();
    
    //-
    
    BPM_Global = value;
}

//--------------------------------------------------------------
void ofxBeatClock::Changed_DAW_active(bool & value) {
    if (value)
    {
        metro.start();
    }
    else
    {
        metro.stop();
    }
}

//--------------------------------------------------------------
void ofxBeatClock::draw_DAW(){

    int x, y;
    x = 10;
    y = 200;
    
    // BIG LETTERS:
    
    // TODO: PERFORMANCE: reduce number of drawings..
    

        {
            std::string timePos =
            ofToString(BPM_bar, 3, ' ') + " : " +
            ofToString(BPM_beat) + " : " +
            ofToString(BPM_sixteen);
    
            TTF_big.drawString(timePos, x, y);
        }
}
