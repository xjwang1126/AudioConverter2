import Foundation
import AVFoundation

class AudioConverter: ObservableObject {
    enum ConversionFormat: String, CaseIterable {
        case mp3 = "mp3"
        case wav = "wav"
        case m4a = "m4a"
        case caf = "caf"
        
        var displayName: String {
            switch self {
            case .mp3: return "MP3"
            case .wav: return "WAV"
            case .m4a: return "M4A"
            case .caf: return "CAF"
            }
        }
        
        var audioFormat: AudioFormatID {
            switch self {
            case .mp3: return kAudioFormatMPEGLayer3
            case .wav: return kAudioFormatLinearPCM
            case .m4a: return kAudioFormatMPEG4AAC
            case .caf: return kAudioFormatAppleLossless
            }
        }
        
        var fileExtension: String {
            return rawValue
        }
    }
    
    @Published var isConverting = false
    @Published var conversionProgress: Double = 0
    @Published var errorMessage: String?
    
    func convertAudio(from sourceURL: URL, to format: ConversionFormat, completion: @escaping (URL?) -> Void) {
        isConverting = true
        errorMessage = nil
        conversionProgress = 0
        
        let asset = AVAsset(url: sourceURL)
        guard let exportSession = AVAssetExportSession(asset: asset, presetName: AVAssetExportPresetAppleM4A) else {
            errorMessage = "无法创建导出会话"
            isConverting = false
            completion(nil)
            return
        }
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let outputURL = documentsPath.appendingPathComponent("converted_\(UUID().uuidString).\(format.fileExtension)")
        
        exportSession.outputURL = outputURL
        exportSession.outputFileType = .m4a
        
        // 监控进度
        let progressTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
            self.conversionProgress = Double(exportSession.progress)
            if exportSession.status == .completed || exportSession.status == .failed || exportSession.status == .cancelled {
                // timer will be invalidated when export completes
            }
        }
        
        exportSession.exportAsynchronously { [weak self] in
            DispatchQueue.main.async {
                progressTimer.invalidate()
                guard let self = self else { return }
                self.isConverting = false
                switch exportSession.status {
                case .completed:
                    self.conversionProgress = 1.0
                    completion(outputURL)
                case .failed:
                    self.errorMessage = exportSession.error?.localizedDescription ?? "转换失败"
                    completion(nil)
                default:
                    self.errorMessage = "转换被取消"
                    completion(nil)
                }
            }
        }
    }
}
