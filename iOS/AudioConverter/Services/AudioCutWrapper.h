//
//  AudioCutWrapper.h
//  AudioConverter
//
//  桥接 AudioCut.framework (C++) 到 Objective-C
//

#ifndef AudioCutWrapper_h
#define AudioCutWrapper_h

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// 转换结果回调
typedef void (^AudioCutProgressBlock)(float progress);
typedef void (^AudioCutCompletionBlock)(BOOL success, NSString * _Nullable outputPath, NSString * _Nullable error);

@interface AudioCutWrapper : NSObject

/// 音频转换（同步，会阻塞调用线程）
/// - Parameters:
///   - inputPath: 输入音频文件路径
///   - outputPath: 输出音频文件路径
///   - format: 输出格式，如 "mp3", "m4a", "aac", "wav"
/// - Returns: 成功返回 YES，失败返回 NO
+ (BOOL)convertAudio:(NSString *)inputPath
          outputPath:(NSString *)outputPath;

/// 异步音频转换
+ (void)convertAudioAsync:(NSString *)inputPath
               outputPath:(NSString *)outputPath
                 progress:(AudioCutProgressBlock _Nullable)progress
               completion:(AudioCutCompletionBlock)completion;

/// 取消当前转换
+ (void)cancel;

@end

NS_ASSUME_NONNULL_END

#endif /* AudioCutWrapper_h */
