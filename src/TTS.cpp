/*
 * TTS.cpp
 *
 *  Created on: 02/08/2012
 *      Author: arturo
 */

#include "TTS.h"
#include "ofUtils.h"
#include "flite.h"

static bool initialized = false;

extern "C"{
    cst_val *flite_set_voice_list(void);
    //cst_voice* register_cmu_us_awb();
    cst_voice* register_cmu_us_rms();

}

TTS::TTS() {
	voice = NULL;
	bCountTime=false;
}

TTS::~TTS() {
}

void TTS::initialize(){
	if(!initialized){
		flite_init();
		initialized = true;
	}
	/*flite_voice_list = flite_set_voice_list();
	voice = flite_voice_select("alice.wav");*/
    
	flite_voice_list = flite_set_voice_list();
	//voice = flite_voice_select("charles");
    voice=register_cmu_us_rms();
	if(!voice) ofLogError() << "couldn't initialize voices";
}

void TTS::start(){
	startThread();
}



TTSData TTS::convertToAudio(string text, int samplingRate, ofSoundBuffer & soundBuffer){
	TTSData data;
	data.processingTime = ofGetElapsedTimeMicros();
	cst_wave * wave = flite_text_to_wave(text.c_str(),voice);
	if(wave){
		/*for(int i=0;i<wave->num_samples;i++){
			wave->samples[i]) cout << wave->samples[i] << " at " << i<< "/" << wave->num_samples << endl;
		}*/
		soundBuffer.copyFrom(wave->samples,wave->num_samples,wave->num_channels,wave->sample_rate);
		if(samplingRate!=-1) soundBuffer.resample(float(wave->sample_rate)/float(samplingRate),ofSoundBuffer::Hermite);
		soundBuffer.setSampleRate(samplingRate);
		data.buffer = &soundBuffer;
		data.text = text;
		delete[] wave->samples;
		delete wave;
		if(bCountTime){
            data.timeLength= flite_text_to_speech(text.c_str(), voice, "none");
        }
	}else{
		ofLogError() << "couldn't generate wave";
	}

	data.processingTime = ofGetElapsedTimeMicros() - data.processingTime;
	return data;
}

void TTS::addText(string text){
	mutex.lock();
	texts.push(text);
	condition.signal();
	mutex.unlock();
}

void TTS::threadedFunction(){
	initialize();
	while(isThreadRunning()){
		while(!texts.empty()){
			string text = texts.front();
			texts.pop();
			mutex.unlock();
			TTSData data = convertToAudio(text,44100,soundBuffer);
			ofNotifyEvent(newSoundE,data);
			mutex.lock();
		}
		condition.wait(mutex);
	}
}

