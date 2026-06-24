//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//
#import <Foundation/Foundation.h>

@interface AudioCutWrapper : NSObject
+ (BOOL)convertAudio:(NSString *)inputPath to:(NSString *)outputPath;
@end
