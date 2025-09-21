#ifndef Yin_h
#define Yin_h
#include "Arduino.h"
class Yin{
	
public: 
	Yin();	
public:
	Yin(int sampleRate,int bufferSize,float threshold);
public:
	void initialize(int sampleRate,int bufferSize,float threshold);
	
public: 
	float getPitch(int32_t* buffer);
public: 
	float getProbability();
	
private: 
	float parabolicInterpolation(int tauEstimate);
private: 
	int absoluteThreshold();
private: 
	void cumulativeMeanNormalizedDifference();
private: 
	void difference(int32_t* buffer);
private:
	float threshold;
	int bufferSize;
	int halfBufferSize;
	int sampleRate;
	float* yinBuffer;
	float probability;
};

#endif
