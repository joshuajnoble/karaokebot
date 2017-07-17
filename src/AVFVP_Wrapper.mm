//
//  AVFVP_Wrapper.m
//  emptyExample
//
//  Created by Joshua Noble on 12/7/17.
//
//

#import <Foundation/Foundation.h>
#import "ofAVFoundationPlayer.h"
#import "ofAVFoundationVideoPlayer.h"
#import "AVFVP_Wrapper.h"

@implementation VideoWrapper

VideoInterface::VideoInterface( void )
: self( NULL )
{   }

VideoInterface::~VideoInterface( void )
{
    [(id)self dealloc];
}

void VideoInterface::init( void )
{
    self = [[VideoWrapper alloc] init];
}

void VideoInterface::enableSampling( ofAVFoundationPlayer &player )
{
    //[player.getAVFoundationVideoPlayer() setEnableAudioSampling:true];
}

void VideoInterface::getAudioBuffer( ofAVFoundationPlayer &player, std::vector<float> &buffer )
{
    
    //ofAVFoundationVideoPlayer *avfp = (ofAVFoundationVideoPlayer *) player->getAVFoundationVideoPlayer();
    
//    if( player.getAVFoundationVideoPlayer() == nullptr ) {
//        return;
//    }
//    
//    @try {
//        if([player.getAVFoundationVideoPlayer() isReady] == NO || [player.getAVFoundationVideoPlayer() isLoaded] == NO ) {
//            return;
//        }
//    }
//    @catch (NSException *exception) {
//        NSLog(@"%@", exception.reason);
//    }
//    @finally {
//        NSLog(@"no good");
//    }
//    
    
    
    CMSampleBufferRef buf = [player.getAVFoundationVideoPlayer() getAudioSampleBuffer];
    
    if(!buf) {
        return;
    }
    
    AudioBufferList audioBufferList;
    NSMutableData *data=[[NSMutableData alloc] init];
    CMBlockBufferRef blockBuffer;
    //OSStatus err = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(nextBuffer, NULL, &bufferList, sizeof(bufferList), NULL, NULL, kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment, &blockBuffer);

    
    //
    
    
    OSStatus err = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(buf, NULL, &audioBufferList, sizeof(audioBufferList), NULL, NULL, kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment, &blockBuffer);
    std::cout << err << endl;
    
    if(err < 0) {
        return;
    }
    
    for( int y=0; y<audioBufferList.mNumberBuffers; y++ )
    {
        AudioBuffer audioBuffer = audioBufferList.mBuffers[y];
        Float32 *frame = (Float32*)audioBuffer.mData;
        //[data appendBytes:frame length:audioBuffer.mDataByteSize];
        for (int i = 0; i < audioBuffer.mDataByteSize; i++) {
            buffer.push_back( frame[i] );
        }
    }
    
    
    if (blockBuffer) // Double check that what you are releasing actually exists!
    {
        CFRelease(blockBuffer);
    }
    
    if (buf) // Double check that what you are releasing actually exists!
    {
        CFRelease(buf);
    }
    
    buf=NULL;
    blockBuffer=NULL;
    [data release];
}

@end
