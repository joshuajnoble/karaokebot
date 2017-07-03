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


bool isUsingGUI = false;

int whichString = 0;
int tone = 0;


////////////////

//--------------------------------------------------------------
void ofApp::setup(){
    
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
    
    
    /// initialize the sound
    int bufferSize		= 512;
    sampleRate 			= 48000;
    volume				= 0.5f;
    
    soundStream.printDeviceList();
    
    soundStream.setDeviceID(1); 	//note some devices are input only and some are output only
    soundStream.setup(this, 2, 0, sampleRate, bufferSize, 4);
    
    int midiMin = 21;
    int midiMax = 108;
    
    // now make filterbank so we can figure out the sounds
    filterBank.setup(bufferSize, midiMin, midiMax, 2, BANDWITH, sampleRate, 1.0);
    filterBank.setColor(ofColor::orange);
    
    // tesseract - void setup(string dataPath = "", bool absolute = false, string language = "eng");
    tess.setup("", false, "eng");
    tess.setWhitelist("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.,");
    
    
    // start camera
    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(30);
    vidGrabber.initGrabber(640, 480);
    
    // init grayscale
    grayImage.allocate(320,240);
    
    // vert sync
    ofSetVerticalSync(true);

}

//--------------------------------------------------------------
void ofApp::update(){
    ofBackground(100, 100, 100);
    vidGrabber.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    
    ofSetHexColor(0xffffff);
    vidGrabber.draw(20, 20);
    
    gui.draw();
    
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    
    if( key == OF_KEY_RETURN) {
        cout << "setting variables " << endl;

        flipImage = vidGrabber.getPixels();
        //flipImage.mirror(true, false);

        string toBeSaid = runOcr( flipImage, 0, 0);
        cout << toBeSaid << endl;
        
        synthNewSpeech(toBeSaid);
        
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
    
    //Analyze Input Buffer with ofxFilterbank
    filterBank.analyze(input);
    
}

//--------------------------------------------------------------
void ofApp::audioOut(float * output, int bufferSize, int nChannels){
    
    double x;
    short temp;
    int i = loadedSamples;
    
#ifdef USING_MAXIMILIAN
    
    
    
#else
    
    for (int j = 0; j < bufferSize && i < availableSamples; i++, j++) {
        float(x) = HTS_Engine_get_generated_speech((HTS_Engine*)&engine, i);
        
        // fill of sound buffer
        output[j*nChannels    ] = x * 0.001 * volume;
        output[j*nChannels + 1] = x * 0.001 * volume;
    }
    loadedSamples = i;
    
    if(availableSamples > 0 && loadedSamples >= availableSamples) {
        
        HTS_Engine_refresh(&(engine.engine));
        
        for (int k = 0; k < label_size; k++) {
            free(label_data[k]);
        }
        free(label_data);
        
        delete_utterance(u);
        unregister_cmu_us_kal(v);
        
        availableSamples = 0;
        loadedSamples = 0;
    }
#endif
}

int ofApp::calcHighestEnergy()
{
    float *energies = filterBank.getSmthEnergies();
    
    int highestEnergy = 0;
    int highestEnergyIndex = -1.0;
    int i = 0;
    
    while( &energies[i] != NULL )
    {
        if(energies[i] > highestEnergyIndex)
        {
            highestEnergy = i;
            highestEnergyIndex = energies[i];
        }
        i++;
    }
    
    return highestEnergyIndex;
}

void ofApp::synthNewSpeech(string utterance)
{
    
    if( isUsingGUI ) {
    
        Flite_HTS_Engine_set_speed(&engine, singingSpeedSlider);
        Flite_HTS_Engine_add_half_tone(&engine, toneSlider);
        Flite_HTS_Engine_set_alpha(&engine, alphaSlider);
        Flite_HTS_Engine_set_beta(&engine, betaSlider);
        
    } else {
        
        int highest = calcHighestEnergy();
        int adjustedPitchRange = (highest - 100)/3; // ???
        
        Flite_HTS_Engine_add_half_tone(&engine, adjustedPitchRange);
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
    
    // put these into the maxim player here?
    
    // clear memory
    delete[] maxiSampleInst.temp;
    
    // allocate new buffer
    maxiSampleInst.temp = new short[availableSamples];
    
    // make a new sample
    for (int i = 0; i < availableSamples; i++ ) {
        float(x) = HTS_Engine_get_generated_speech((HTS_Engine*)&engine, i);
        maxiSampleInst.temp[i] = x * 0.001 * volume;
    }

    
    timeStretch = new maxiTimePitchStretch<hannWinFunctor, maxiSample>(&maxiSampleInst);
    
    ofLog( OF_LOG_NOTICE, ofToString(availableSamples));

}


// see what we see
string ofApp::runOcr(ofPixels& pix, float scale, int medianSize) {
    
    return tess.findText(pix);
}

void ofApp::exit()
{
    soundStream.stop();
    soundStream.close();
    filterBank.exit();
    
    //Flite_HTS_Engine_clear(&engine);
}
