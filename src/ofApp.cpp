#include "ofApp.h"

extern "C"{
    cst_val *flite_set_voice_list(void);
    //cst_voice* register_cmu_us_awb();
    cst_voice* register_cmu_us_kal(const char *voxdir);
    void unregister_cmu_us_kal(cst_voice * vox);
    
}

#define MAXBUFLEN 1024

#define BANDWITH  1.0
#define LIN2dB(x) (double)(20. * log10(x))
#define SR 44100

#define USING_RUBBERBAND

#define maxProcessSize 512

bool isUsingGUI = true;

int whichString = 0;
int tone = 0;


////////////////

//--------------------------------------------------------------
void ofApp::setup(){
    
    lastOCRRun = 0.f;
    currentOCRString = "";
    
    //////// flite
    
    availableSamples = 0;
    loadedSamples = 0;

    Flite_HTS_Engine_initialize(&engine);
    
    string voice = ofToDataPath("cmu_us_arctic_slt.htsvoice");
    
    if(Flite_HTS_Engine_load( &engine, voice.c_str() ) != TRUE) {
        ofLog( OF_LOG_ERROR, " can't load voice ");
    }
    
    Flite_HTS_Engine_add_half_tone(&engine, 0.0);
    Flite_HTS_Engine_set_alpha(&engine, alphaSlider);
    Flite_HTS_Engine_set_beta(&engine, betaSlider);

    gui.setup(); // most of the time you don't need a name
    gui.add(toneSlider.setup("tone", 0, -40, 40));
    gui.add(volumeSlider.setup("volume", 0.2, 0.0, 1.0));
    gui.add(singingSpeedSlider.setup("speed", 1.0, 0.0, 4.0));
    gui.add(alphaSlider.setup("alpha", 0.57, -1.0, 1.0));
    gui.add(betaSlider.setup("beta", -0.6, -1.0, 1.0));
    
    isCurrentlySpeaking = false;
    
    /// initialize the sound
    int bufferSize = 512;
    sampleRate = 48000;
    volume = 0.5f;
    
    
    int midiMin = 21;
    int midiMax = 108;
    
    // aubio stuff
    // setup onset object
    //onset.setup();
//    onset.setup("mkl", 4 * bufferSize, bufferSize, sampleRate);
    // listen to onset event
//    ofAddListener(onset.gotOnset, this, &ofApp::onsetEvent);
    
    // setup pitch object
//    pitch.setup();
    pitch.setup("yinfft", 8 * bufferSize, bufferSize, 44100);
    
    // setup beat object
//    beat.setup();
    beat.setup("default", 4 * bufferSize, bufferSize, 44100);
    // listen to beat event
    ofAddListener(beat.gotBeat, this, &ofApp::beatEvent);
    
    // set up the rubberband
    rubberband = new RubberBand::RubberBandStretcher(sampleRate, 2,
                                                     RubberBand::RubberBandStretcher::DefaultOptions |
                                                     RubberBand::RubberBandStretcher::OptionProcessRealTime);
    rubberband->setMaxProcessSize(maxProcessSize);
    rubberband->setPitchScale(std::pow(2.0, 6.0 / 12.0));
    
    stretchInBufL.resize(maxProcessSize);
    stretchInBufR.resize(maxProcessSize);
    stretchInBuf.resize(2);
    stretchInBuf[0] = &(stretchInBufL[0]);
    stretchInBuf[1] = &(stretchInBufR[0]);
    
    stretchOutBufL.resize(maxProcessSize);
    stretchOutBufR.resize(maxProcessSize);
    stretchOutBuf.resize(2);
    stretchOutBuf[0] = &(stretchOutBufL[0]);
    stretchOutBuf[1] = &(stretchOutBufR[0]);
    
    
    // finally the OF setup
    soundStream.printDeviceList();
    soundStream.setDeviceID(1); 	//note some devices are input only and some are output only
    soundStream.setup(this, 2, 0, sampleRate, bufferSize, 4);
    
    // tesseract - void setup(string dataPath = "", bool absolute = false, string language = "eng");
    tess.setup("", false, "eng");
    tess.setWhitelist("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.,'1234567890");
    
#ifdef USE_CAMERA
    
    // start camera
    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(30);
    vidGrabber.initGrabber(640, 480);
    
#else

    vidPlayer.load("cant_help_falling.mov");
    vidPlayer.setVolume(0);
    vidPlayer.play();
    
    soundFile.load(ofToDataPath("cant_help_falling.mp3"));
    soundFilePlayhead = 0;
    
#endif
    
    // init grayscale
    thresh.allocate(640, 480, OF_IMAGE_GRAYSCALE);
    
    // vert sync
    ofSetVerticalSync(true);


}

//--------------------------------------------------------------
void ofApp::update(){
    
    ofBackground(100, 100, 100);
    
#ifdef USE_CAMERA
    
    vidGrabber.update();
    if(vidGrabber.isFrameNew()) {
        ofxCv::convertColor(vidGrabber, thresh, CV_RGB2GRAY);
        //ofxCv::autothreshold(thresh);
        thresh.update();
    }
    
#else
    
    vidPlayer.update();
    if(vidPlayer.isFrameNew()) {
        ofxCv::convertColor(vidPlayer, thresh, CV_RGB2GRAY);
        thresh.update();
    }
    
    if(soundFile.isLoaded()) {
        
        //onset.audioIn(&soundFile.getSamples()[soundFilePlayhead], bufferSize, nChannels);
        // compute pitch detection
        pitch.audioIn(&soundFile.getSamples()[soundFilePlayhead], 1024, 2);
        // compute beat location
        beat.audioIn(&soundFile.getSamples()[soundFilePlayhead], 1024, 2);
    }
    
    if( availableSamples > 0 ) {
        
        if(pitchBuffer.size() > 20) {
            pitchBuffer.pop_front();
        }
        
        pitchBuffer.push_back(std::pow(2.0, ((pitch.latestPitch - 40) / 6) / 12.0));
        
        float summedPitch = 0;
        for( int i = 0; i < pitchBuffer.size(); i++ ) {
            summedPitch += pitchBuffer.at(i);
        }
        
        rubberband->setPitchScale(summedPitch/pitchBuffer.size()); //rubberband->setPitchScale( pitch.latestPitch/40.0);
        //rubberband->setTimeRatio( max(0.5, 2.0 - (beat.bpm/100.0)));
    }
    
    
#endif
    
    if(ofGetElapsedTimef() - lastOCRRun > 2.0 && !isCurrentlySpeaking)
    {
        
        // what OCR do we have?
        string toBeSaid = runOcr( thresh, 0, 0);
        
        cout << toBeSaid << endl;
        cout << currentOCRString << endl;
        
        // got a new string?
        if(currentOCRString != toBeSaid && toBeSaid != "")
        {
            currentOCRString = toBeSaid;
            ocrResults = ofSplitString(toBeSaid, "\n");
            currentSubString = ocrResults.begin();

            synthNewSpeech(*currentSubString);
        }
        lastOCRRun = ofGetElapsedTimef();
    }

}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofSetHexColor(0xffffff);
    
    vidGrabber.draw(20, 20, 320, 240);
    thresh.draw(20, 260, 320, 240);
    
    gui.draw();
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    
    if( key == OF_KEY_RETURN) {

        string toBeSaid = runOcr( thresh, 0, 0);
        ocrResults = ofSplitString(toBeSaid, "\n");
        currentSubString = ocrResults.begin();
        synthNewSpeech(*currentSubString);
        
    }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
void ofApp::audioIn(float * input, int bufferSize, int nChannels){

#ifdef USING_CAMERA
 
    // compute onset detection
    onset.audioIn(input, bufferSize, nChannels);
    // compute pitch detection
    pitch.audioIn(input, bufferSize, nChannels);
    // compute beat location
    beat.audioIn(input, bufferSize, nChannels);
    
#endif

}

//--------------------------------------------------------------
void ofApp::audioOut(float * output, int bufferSize, int nChannels){
    
    double x;
    short temp;
    
    //TODO: fix playhead pos problem
    
    int i = loadedSamples; // increment number loaded so far
    while (rubberband->available() < bufferSize)
    {
        
        int j;
        for (j = 0;
             j < bufferSize && i < availableSamples;
             i++, j++)
        {
            stretchInBufL[j] = inputSamples[i] * 0.001 * volume;
            stretchInBufR[j] = inputSamples[i] * 0.001 * volume;
        }
        while (j < maxProcessSize)
        {
            stretchInBufL[j] = 0;
            stretchInBufR[j] = 0;
            j++;
        }
        
        rubberband->process(&(stretchInBuf[0]), maxProcessSize, false);
        samplePlayPosition += maxProcessSize;
    }
    
    // now swap back
    loadedSamples = i;
    
    size_t samplesRetrieved = rubberband->retrieve(&(stretchOutBuf[0]), bufferSize);
    
    if( soundFile.isLoaded() ) {
    
        // Interleave output from rubberband into audio output
        for (i = 0; i < samplesRetrieved && i < bufferSize; i++) {
            output[i * nChannels] = stretchOutBufL[i];
            output[i * nChannels + 1] = stretchOutBufR[i];
        }
    }
    
    if(availableSamples > 0 && loadedSamples >= availableSamples) {
        
        cout << " clearing " << endl;
        
        HTS_Engine_refresh(&(engine.engine));
        
        for (int k = 0; k < label_size; k++) {
            free(label_data[k]);
        }
        free(label_data);
        
        delete_utterance(u);
        unregister_cmu_us_kal(v);
        
        availableSamples = 0;
        loadedSamples = 0;
        
        
        ++currentSubString;
        while(*currentSubString == "" && currentSubString != ocrResults.end()) {
            ++currentSubString;
        }
        
        // totally finished with everything on the screen
        if(currentSubString == ocrResults.end()) {
            ocrResults.clear();
            isCurrentlySpeaking = false;
        } else {
            if(*currentSubString != "") {
                synthNewSpeech(*currentSubString);
            }
        }
        
    }
    
    
    // if we're reading from an MP3 file
#ifndef USING_CAMERA
    
    soundFilePlayhead += bufferSize;
    
#endif

    
}

void ofApp::synthNewSpeech(string utterance)
{
    
    //beat.bpm)/100.0
    if(beat.bpm > 40.0) {
        Flite_HTS_Engine_set_speed(&engine, (beat.bpm/400.0));
    }
    
    if( isUsingGUI ) {
    
//        Flite_HTS_Engine_set_speed(&engine, singingSpeedSlider);
        Flite_HTS_Engine_add_half_tone(&engine, toneSlider);
        Flite_HTS_Engine_set_alpha(&engine, alphaSlider);
        Flite_HTS_Engine_set_beta(&engine, betaSlider);
        
    } else {
        
        int adjustedPitchRange = 1.0; // ???
        
        Flite_HTS_Engine_add_half_tone(&engine, pitch.latestPitch);
        Flite_HTS_Engine_set_alpha(&engine, alphaSlider);
        Flite_HTS_Engine_set_beta(&engine, betaSlider);
        
    }
    
    v = NULL;
    u = NULL;
    cst_item *s = NULL;
    label_data = NULL;
    label_size = 0;
    
    int i;
    
    /* text analysis part */
    v = register_cmu_us_kal(NULL);
    if (v == NULL)
        return FALSE;
    u = flite_synth_text(utterance.c_str(), v);
    if (u == NULL) {
        unregister_cmu_us_kal(v);
        return FALSE;
    }
    for (s = relation_head(utt_relation(u, "Segment")); s; s = item_next(s))
        label_size++;
    if (label_size <= 0) {
        delete_utterance(u);
        unregister_cmu_us_kal(v);
        return FALSE;
    }
    label_data = (char **) calloc(label_size, sizeof(char *));
    for (i = 0, s = relation_head(utt_relation(u, "Segment")); s; s = item_next(s), i++) {
        label_data[i] = (char *) calloc(MAXBUFLEN, sizeof(char));
        create_label(s, label_data[i]);
    }
    
    /* speech synthesis part */
    HTS_Engine_synthesize_from_strings(&(engine.engine), label_data, label_size);
    
    HTS_GStreamSet *gss = &( (HTS_Engine*)&engine)->gss;
    availableSamples = HTS_Engine_get_nsamples((HTS_Engine*)&engine);
    
    isCurrentlySpeaking = true;
    
#ifdef USING_RUBBERBAND
    
    samplePlayPosition = 0;
    
    // reset everything
    inputSamples.clear();
    stretchInBufL.clear();
    stretchInBufL.resize(maxProcessSize);
    stretchInBufR.clear();
    stretchInBufR.resize(maxProcessSize);
    stretchInBuf.resize(2);
    stretchInBuf[0] = &(stretchInBufL[0]);
    stretchInBuf[1] = &(stretchInBufR[0]);
    
    stretchOutBufL.clear();
    stretchOutBufL.resize(maxProcessSize);
    stretchOutBufR.clear();
    stretchOutBufR.resize(maxProcessSize);
    stretchOutBuf.resize(2);
    stretchOutBuf[0] = &(stretchOutBufL[0]);
    stretchOutBuf[1] = &(stretchOutBufR[0]);
    

    // make a new sample
    for (int i = 0; i < availableSamples; i++ ) {
        float x = HTS_Engine_get_generated_speech((HTS_Engine*)&engine, i);
        inputSamples.push_back(x);
    }
    
    ofLog( OF_LOG_NOTICE, ofToString(availableSamples));
    
#endif

}

///////////////////////////////////////////////////////////////////////////////////////
// aubio methods
///////////////////////////////////////////////////////////////////////////////////////

void ofApp::onsetEvent(float & time) {
    ofLog() << "got onset at " << time << " s";
}

///////////////////////////////////////////////////////////////////////////////////////
void ofApp::beatEvent(float & time) {
    ofLog() << "got beat at " << time << " s";
}


// see what we see
string ofApp::runOcr(ofPixels& pix, float scale, int medianSize) {
    
    tess.findText(pix);
    
    string acceptedWords;
    
    int lcount = 1;
    tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
    
    tesseract::ResultIterator* ri = tess.getTess().GetIterator();
    if (ri != 0) {
        do {
            const char* word = ri->GetUTF8Text(tesseract::RIL_WORD);
            float conf = ri->Confidence(level);
            
            if (word != 0 && conf > 70) {
                const char *font_name;
                bool bold, italic, underlined, monospace, serif, smallcaps;
                int pointsize, font_id;
                
                font_name = ri->WordFontAttributes(&bold, &italic, &underlined,
                                                   &monospace, &serif,
                                                   &smallcaps, &pointsize,
                                                   &font_id);
                
                
                if(pointsize > 10) {
                    acceptedWords += word;
                    acceptedWords += " ";
                }
            }
            delete[] word;
            lcount++;
        } while (ri->Next(tesseract::RIL_WORD));
    }
    return acceptedWords;
}



void ofApp::exit()
{
    soundStream.stop();
    soundStream.close();
    
    //Flite_HTS_Engine_clear(&engine);
}
