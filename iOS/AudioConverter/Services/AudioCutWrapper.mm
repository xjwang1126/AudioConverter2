//
//  AudioCutWrapper.mm
//  AudioConverter
//
//  Objective-C++ 桥接 AudioCut.framework
//

#import "AudioCutWrapper.h"

// 导入 AudioCut 的 C++ 头文件
#import <AudioCut/AudioUtil/audioutil.h>
#import <AudioCut/AudioUtil/audioutilinfo.h>

using namespace MediaLibrary;

// 全局实例用于取消操作
static AudioUtil *g_audioUtil = nullptr;

@implementation AudioCutWrapper

+ (BOOL)convertAudio:(NSString *)inputPath
          outputPath:(NSString *)outputPath
         startTimeMs:(int64_t)startTimeMs
           endTimeMs:(int64_t)endTimeMs
              format:(NSString *)format
{
    if (!inputPath || !outputPath) {
        return NO;
    }
    
    // 映射格式字符串到 AudioFormat
    AudioFormat audioFormat;
    /*
    if ([format isEqualToString:@"mp3"]) {
        audioFormat = AudioFormat::MP3;
    } else if ([format isEqualToString:@"aac"]) {
        audioFormat = AudioFormat::AAC;
    } else if ([format isEqualToString:@"m4a"]) {
        audioFormat = AudioFormat::M4A;
    } else if ([format isEqualToString:@"wav"]) {
        audioFormat = AudioFormat::WAV;
    } else if ([format isEqualToString:@"flac"]) {
        audioFormat = AudioFormat::FLAC;
    } else {
        audioFormat = AudioFormat::M4A; // 默认
    }
     */
    
    @autoreleasepool {
        AudioUtil util;
        g_audioUtil = &util;
        
        BOOL success = util.convert(
            [inputPath UTF8String],
            [outputPath UTF8String],
            startTimeMs * 1000,  // 转为微秒
            endTimeMs * 1000,
            audioFormat,
            nullptr
        );
        
        g_audioUtil = nullptr;
        return success ? YES : NO;
    }
}

+ (void)convertAudioAsync:(NSString *)inputPath
               outputPath:(NSString *)outputPath
              startTimeMs:(int64_t)startTimeMs
                endTimeMs:(int64_t)endTimeMs
                   format:(NSString *)format
                 progress:(AudioCutProgressBlock _Nullable)progress
               completion:(AudioCutCompletionBlock)completion
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        BOOL success = [self convertAudio:inputPath
                               outputPath:outputPath
                              startTimeMs:startTimeMs
                                endTimeMs:endTimeMs
                                   format:format];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if (success) {
                if (completion) {
                    completion(YES, outputPath, nil);
                }
            } else {
                if (completion) {
                    completion(NO, nil, @"音频转换失败");
                }
            }
        });
    });
}

+ (void)cancel
{
    if (g_audioUtil) {
        g_audioUtil->cancel();
    }
}

@end
