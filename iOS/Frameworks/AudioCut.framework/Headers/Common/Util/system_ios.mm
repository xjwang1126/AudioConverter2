#include "system.h"

#include <Photos/Photos.h>
#include <UIKit/UIKit.h>
#include <MediaPlayer/MediaPlayer.h>

namespace MediaLibrary {

std::string getDumpDataPath()
{
    return [NSTemporaryDirectory() cStringUsingEncoding:NSUTF8StringEncoding];
}

bool fileExists(const std::string& filePath)
{
    @autoreleasepool {
    if (filePath.empty()) {
        return false;
    }

    bool result = false;

    NSString* _filePath = [NSString stringWithUTF8String:filePath.c_str()];
    NSString* localIdentifierRegex = @"[a-zA-Z0-9-/]*";
    NSPredicate* localIdentifierPredicate = [NSPredicate predicateWithFormat:@"SELF MATCHES %@", localIdentifierRegex];
    NSString* musicRegex = @"ipod-library";
    NSPredicate* musicPredicate = [NSPredicate predicateWithFormat:@"SELF BEGINSWITH %@", musicRegex];
    if ([localIdentifierPredicate evaluateWithObject:_filePath]) {
        PHFetchResult* fetchResult = [PHAsset fetchAssetsWithLocalIdentifiers:@[_filePath] options:nil];
        if (fetchResult && fetchResult.firstObject) {
            result = true;
        }
    } else if ([musicPredicate evaluateWithObject:_filePath]) {
        MPMediaQuery *mediaQuery = [[MPMediaQuery alloc] init];
        MPMediaPropertyPredicate *mediaPropertyPredicate = [MPMediaPropertyPredicate predicateWithValue:[NSNumber numberWithInt:MPMediaTypeMusic] forProperty:MPMediaItemPropertyMediaType];
        [mediaQuery addFilterPredicate:mediaPropertyPredicate];
        NSArray* musics = [mediaQuery items];
        for (MPMediaItem* music in musics) {
            NSURL *musicFileURL = [music valueForProperty:MPMediaItemPropertyAssetURL];
            NSString* musicFilePath = musicFileURL.absoluteString;
            if ([musicFilePath isEqualToString:_filePath]) {
                result = true;
                break;
            }
        }
        [mediaQuery release];
        mediaQuery = nil;
    } else {
        NSString* _prefix = [NSString stringWithUTF8String:"file://"];
        NSString* _filePathRemovePrefix = nil;
        if ([_filePath hasPrefix:_prefix]) {
            _filePathRemovePrefix = [_filePath stringByReplacingOccurrencesOfString:_prefix withString:@""];
        } else {
            _filePathRemovePrefix = _filePath;
        }
        if ([[NSFileManager defaultManager] fileExistsAtPath:_filePathRemovePrefix] == YES) {
            result = true;
        }
    }

    return result;
    }
}

bool deleteFile(const std::string& filePath)
{
    if (filePath.empty()) {
        return false;
    }

    NSString* _filePath = [NSString stringWithUTF8String:filePath.c_str()];
    if ([[NSFileManager defaultManager] isDeletableFileAtPath:_filePath]) {
        return [[NSFileManager defaultManager] removeItemAtPath:_filePath error:nil];
    }
    return false;
}

}
