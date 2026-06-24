//
//  AudioCutWrapper.m
//  AudioConverter
//
//  Created by wxj on 2026/6/24.
//

#import <Foundation/Foundation.h>
#import <AudioCut/AudioUtil/audioutil.h>

#include "AudioCutWrapper.h"

@implementation AudioCutWrapper

+ (BOOL)convertAudio:(NSString *)inputPath to:(NSString *)outputPath
{
    // 1. 参数校验
    if (!inputPath || inputPath.length == 0 || !outputPath || outputPath.length == 0) {
        NSLog(@"输入/输出路径不能为空");
        return NO;
    }
    
    NSFileManager *fileMgr = [NSFileManager defaultManager];
    if (![fileMgr fileExistsAtPath:inputPath]) {
        NSLog(@"源文件不存在: %@", inputPath);
        return NO;
    }
    
    // ======================
    // 这里写音频转码/裁剪逻辑
    // 示例1：纯OC简单示例（复制文件仅演示占位）
    // ======================
    NSError *err = nil;
    BOOL success = [fileMgr copyItemAtPath:inputPath toPath:outputPath error:&err];
    if (!success) {
        NSLog(@"音频转换失败: %@", err.localizedDescription);
        return NO;
    }
    
    // ======================
    // 示例2：如果要调用C++函数，写法如下
    // ======================
    /*
    std::string inStd = [inputPath UTF8String];
    std::string outStd = [outputPath UTF8String];
    bool ret = audioCutProcess(inStd, outStd); // 自定义C++函数
    if (!ret) {
        return NO;
    }
    */
    
    NSLog(@"音频转换完成，输出路径: %@", outputPath);
    return YES;
}

@end
