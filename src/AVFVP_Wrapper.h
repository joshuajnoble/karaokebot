//
//  AVFVP_Wrapper.h
//  emptyExample
//
//  Created by Joshua Noble on 12/7/17.
//
//

#import "AVFVP_Interface.h"

@interface VideoWrapper : NSObject
{
    // empty
}


- (void)  enableSampling: ( ofAVFoundationPlayer &) player;
- (void)  getAudioBuffer: (ofAVFoundationPlayer&) player: (std::vector<float>&) buffer;

@end
