#include "ofApp.h"

extern "C"{
    cst_val *flite_set_voice_list(void);
    //cst_voice* register_cmu_us_awb();
    cst_voice* register_cmu_us_kal(const char *voxdir);
    void unregister_cmu_us_kal(cst_voice * vox);
    
}

#define MAXBUFLEN 1024

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

    
    //HTS_Boolean HTS_Engine_synthesize_from_strings(HTS_Engine * engine, char **lines, size_t num_lines);
    testPhrases.push_back("Karmaaaaaaaaaa police Arrest this man He talks in maths He buzzes like a fridge");
    testPhrases.push_back("He's like a detuned radio");
    
    gui.setup(); // most of the time you don't need a name
    gui.add(toneSlider.setup("tone", 0, -40, 40));
    gui.add(volumeSlider.setup("volume", 0.2, 0.0, 1.0));
    gui.add(singingSpeedSlider.setup("speed", 1.0, 0.0, 4.0));
    gui.add(alphaSlider.setup("alpha", 0.57, -1.0, 1.0));
    gui.add(betaSlider.setup("beta", -0.6, -1.0, 1.0));
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
   
    synthNewSpeech();
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////

    
    /// initialize the sound
    int bufferSize		= 512;
    sampleRate 			= 48000;
    volume				= 0.5f;
    
    lAudio.assign(bufferSize, 0.0);
    rAudio.assign(bufferSize, 0.0);
    
    soundStream.printDeviceList();
    
    //if you want to set the device id to be different than the default
    soundStream.setDeviceID(1); 	//note some devices are input only and some are output only
    soundStream.setup(this, 2, 0, sampleRate, bufferSize, 4);
    
    // tesseract - void setup(string dataPath = "", bool absolute = false, string language = "eng");
    tess.setup("", false, "eng");
    tess.setWhitelist("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.,");
//    tess.setAccuracy(ofxTesseract::ACCURATE);
//    tess.setMode(ofxTesseract::AUTO);
}

//--------------------------------------------------------------
void ofApp::update(){
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    cout << "setting variables " << endl;
    
    Flite_HTS_Engine_add_half_tone(&engine, toneSlider);
    Flite_HTS_Engine_set_speed(&engine, singingSpeedSlider);
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
void ofApp::audioOut(float * output, int bufferSize, int nChannels){
    
    double x;
    short temp;
    int i = loadedSamples;
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
        
        whichString++;
        if(whichString > testPhrases.size()-1) {
            whichString = 0;
        }
        
        synthNewSpeech();
    }
}

void ofApp::synthNewSpeech()
{
    
    Flite_HTS_Engine_set_speed(&engine, singingSpeedSlider);
    Flite_HTS_Engine_add_half_tone(&engine, toneSlider);
    Flite_HTS_Engine_set_alpha(&engine, alphaSlider);
    Flite_HTS_Engine_set_beta(&engine, betaSlider);
    
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
    u = flite_synth_text(testPhrases.at(whichString).c_str(), v);
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
    
    ofLog( OF_LOG_NOTICE, ofToString(availableSamples));

}


// see what we see
string ofApp::runOcr(ofPixels& pix, float scale, int medianSize) {
    
    return tess.findText(pix);
}

void ofApp::exit()
{
    //Flite_HTS_Engine_clear(&engine);
}
