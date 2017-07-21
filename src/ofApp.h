#pragma once

#define FLITE_PLUS_HTS_ENGINE

/*
 
 
 What addons are we using?
 
 https://github.com/micknoise/Maximilian - to tweak the sound
 https://github.com/kylemcdonald/ofxCv - to read characters
 https://github.com/kylemcdonald/ofxTesseract   - to read characters
 https://github.com/leozimmerman/ofxFilterbank   - to get the sound we're hearing, sorta
 
 
 */

#include <rubberband/RubberBandStretcher.h>

#include "ofMain.h"
#include "TTS.h"
#include "flite.h"
#include "flite_hts_engine.h"
#include "HTS_engine.h"
#include "ofxGui.h"

#include "soundfile.h"

#include "ofxAubio.h"

// opencv
#include "ofxOpenCv.h"

// tess
#include "ofxTesseract.h"

// ofxcv
#include "ofxCv.h"

class ofApp : public ofBaseApp{
    
	public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    void audioIn(float * input, int bufferSize, int nChannels);
    void audioOut(float * output, int bufferSize, int nChannels);
    
    void onsetEvent(float & time);
    void beatEvent(float & time);
    
    void synthNewSpeech(string utterance);
    void exit();
    
    // tesseract
    string runOcr(ofPixels& pix, float scale, int medianSize);
    
    ofxTesseract tess;
    vector<string> ocrResults;
    vector<string>::iterator currentSubString;

    ofImage rgbImage;
    ofImage flipImage;
    ofxCvGrayscaleImage grayImage;
    
    float 	pan;
    int		sampleRate;
    float 	volume;

    char **label_data = NULL;
    cst_voice *v = NULL;
    cst_utterance *u = NULL;
    int label_size;
    
    Flite_HTS_Engine engine;
    ofSoundStream soundStream;
    int loadedSamples;
    int availableSamples;
    bool isCurrentlySpeaking;
    
    ofxPanel gui;
    ofxFloatSlider toneSlider;
    ofxFloatSlider volumeSlider;
    ofxFloatSlider alphaSlider;
    ofxFloatSlider betaSlider;
    ofxFloatSlider singingSpeedSlider;
    vector<string> testPhrases;
    
    SoundFile soundFile;
    
    ofVideoGrabber vidGrabber;
    ofAVFoundationPlayer vidPlayer;
    
    ofImage thresh;
    ofRectangle rect;
    
    // aubio stuff
    ofxAubioOnset onset;
    ofxAubioPitch pitch;
    ofxAubioBeat beat;
    
    RubberBand::RubberBandStretcher *rubberband = NULL;
    
    // Input buffers for the RubberBandStretcher
    std::vector<float*> stretchInBuf;
    std::vector<float> stretchInBufL;
    std::vector<float> stretchInBufR;
    
    // Output buffers for the RubberBandStretcher
    std::vector<float*> stretchOutBuf;
    std::vector<float> stretchOutBufL;
    std::vector<float> stretchOutBufR;

    std::vector<float> inputSamples;
    
    
    float sfBuffer[512];
    int samplePlayPosition, soundFilePlayhead;
    
    deque<float> pitchBuffer;
    
    float lastOCRRun;
    string currentOCRString;
    
    
};
