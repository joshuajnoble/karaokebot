//
//  AVFVP_Interface.h
//  emptyExample
//
//  Created by Joshua Noble on 12/7/17.
//
//

#ifndef AVFVP_Interface_h
#define AVFVP_Interface_h

class VideoInterface
{
public:
    VideoInterface ( void );
    ~VideoInterface( void );
    
    void init( void );
    void enableSampling( ofAVFoundationPlayer & player);
    void getAudioBuffer( ofAVFoundationPlayer &player, std::vector<float> &buffer );
    
private:
    void * self;
};


#endif /* AVFVP_Interface_h */
