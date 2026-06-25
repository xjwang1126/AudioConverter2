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
#import <AudioCut/MediaUtil/mediautildelegate.h>

using namespace MediaLibrary;

// =============================================
// 内部：连接 C++ MediaUtilDelegate 到 ObjC block
// =============================================
static AudioCutProgressBlock s_progressBlock = nil;
static AudioCutCompletionBlock s_completionBlock = nil;

class MediaUtilDelegateImpl : public MediaUtilDelegate
{
public:
    virtual ~MediaUtilDelegateImpl() override = default;
    
    virtual void notifyExportProgress(int64_t currentTime, int64_t totalTime) const override
    {
        if (s_progressBlock && totalTime > 0) {
            float progress = (float)((double)currentTime / (double)totalTime);
            if (progress > 1.0f) progress = 1.0f;
            if (progress < 0.0f) progress = 0.0f;
            
            // 回到主线程回调
            dispatch_async(dispatch_get_main_queue(), ^{
                s_progressBlock(progress);
            });
        }
    }
    
    virtual void notifyExportFinish() const override
    {
        // 完成后回调 100%
        if (s_progressBlock) {
            dispatch_async(dispatch_get_main_queue(), ^{
                s_progressBlock(1.0f);
            });
        }
    }
};

@implementation AudioCutWrapper

// =============================================
// 同步转换
// =============================================
+ (BOOL)convertAudio:(NSString *)inputPath
          outputPath:(NSString *)outputPath
{
    if (!inputPath || !outputPath) {
        return NO;
    }
    
    AudioFormat audioFormat;
    
    @autoreleasepool {
        AudioUtil util;
        
        MediaUtilDelegateImpl delegate;
        
        BOOL success = util.convert(
            [inputPath UTF8String],
            [outputPath UTF8String],
            -1,
            -1,
            audioFormat,
            &delegate
        );
        
        return success ? YES : NO;
    }
}

+ (void)convertAudioAsync:(NSString *)inputPath
               outputPath:(NSString *)outputPath
                 progress:(AudioCutProgressBlock _Nullable)progress
               completion:(AudioCutCompletionBlock)completion
{
    // 保存 block
    s_progressBlock = progress;
    s_completionBlock = completion;
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        BOOL success = [self convertAudio:inputPath
                               outputPath:outputPath];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if (s_completionBlock) {
                if (success) {
                    s_completionBlock(YES, outputPath, nil);
                } else {
                    s_completionBlock(NO, nil, @"音频转换失败");
                }
            }
            // 清理 block，避免循环引用
            s_progressBlock = nil;
            s_completionBlock = nil;
        });
    });
}

// =============================================
// 取消
// =============================================
+ (void)cancel
{
    // AudioUtil 的 cancel 是成员方法，需要通过全局实例
    // 当前实现中 convertAudio 是类方法 + 局部变量，无法外部取消
    // 如果需要支持取消，需要改为单例或保留实例引用
    // 暂时不实现
}

@end
