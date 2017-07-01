/*
 * TTS.h
 *
 *  Created on: 02/08/2012
 *      Author: arturo
 */

#ifndef TTS_H_
#define TTS_H_

//#define USE_FESTIVAL_SERVER
#define USE_FLITE

#include "ofConstants.h"
#include "ofTypes.h"
#include "ofSoundBuffer.h"
#include "ofThread.h"
#include "ofEvents.h"
#include "Poco/Condition.h"
#include <queue>

#include "flite.h"


class TTSData{
public:
	ofSoundBuffer * buffer;
	string text;
	int processingTime;
	float timeLength;
};

class TTS: public ofThread {
public:
	TTS();
	virtual ~TTS();

	void initialize();
	void start();

	void addText(string text);
	TTSData convertToAudio(string text, int samplingRate, ofSoundBuffer & buffer);

	ofEvent<const TTSData> newSoundE;

private:
	void threadedFunction();
	bool bCountTime;


	cst_voice *voice;

	queue<string> texts;
	ofMutex mutex;
	Poco::Condition condition;
	ofSoundBuffer soundBuffer;
};

#endif /* TTS_H_ */
