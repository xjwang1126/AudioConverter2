import Foundation
import AVFoundation
import UniformTypeIdentifiers

class AudioConverterService: ObservableObject {
    // 支持的转换格式
    enum ConversionFormat: String, CaseIterable, Identifiable {
        case m4a = "m4a"
        case mp3 = "mp3"
        case aac = "aac"
        case flac = "flac"
        
        var id: String { rawValue }
        
        var displayName: String {
            switch self {
            case .m4a: return "M4A"
            case .mp3: return "MP3"
            case .aac: return "AAC"
            case .flac: return "FLAC"
            }
        }
        
        var icon: String {
            switch self {
            case .m4a: return "waveform"
            case .mp3: return "music.note"
            case .aac: return "speaker.wave.2"
            case .flac: return "opticaldisc"
            }
        }
        
        var utType: UTType {
            switch self {
            case .m4a: return .mpeg4Audio
            case .mp3: return .mp3
            case .aac: return .appleProtectedMPEG4Audio
            case .flac: return UTType(filenameExtension: "flac") ?? .audio
            }
        }
        
        var avFileType: AVFileType {
            switch self {
            case .m4a: return .m4a
            case .mp3: return .mp3  // macOS可能不支持直接导出mp3
            case .aac: return .m4a
            case .flac: return .caf
            }
        }
        
        var presetName: String {
            switch self {
            case .m4a, .aac: return AVAssetExportPresetAppleM4A
            case .mp3: return AVAssetExportPresetAppleM4A  // MP3用M4A替代
            case .flac: return AVAssetExportPresetAppleM4A
            }
        }
        
        // 是否可以直接用 AVAssetExportSession 导出
        var supportsDirectExport: Bool {
            switch self {
            case .m4a, .aac: return true
            case .mp3, .flac: return false  // 需要通过 AVAssetReader/Writer 转换
            }
        }
    }
    
    @Published var isConverting = false
    @Published var conversionProgress: Double = 0
    @Published var errorMessage: String?
    @Published var convertedURL: URL?
    
    private var exportSession: AVAssetExportSession?
    private var progressTimer: Timer?
    
    func convertAudio(from sourceURL: URL, to format: ConversionFormat) {
        isConverting = true
        errorMessage = nil
        conversionProgress = 0
        convertedURL = nil
        
        let asset = AVAsset(url: sourceURL)
        
        if format.supportsDirectExport {
            convertWithExportSession(asset: asset, format: format)
        } else {
            convertWithAssetReader(asset: asset, sourceURL: sourceURL, format: format)
        }
    }
    
    // 方式1: 直接用 AVAssetExportSession
    private func convertWithExportSession(asset: AVAsset, format: ConversionFormat) {
        guard let session = AVAssetExportSession(asset: asset, presetName: format.presetName) else {
            errorMessage = "无法创建导出会话，格式可能不受支持"
            isConverting = false
            return
        }
        
        exportSession = session
        
        let outputURL = generateOutputURL(format: format)
        session.outputURL = outputURL
        session.outputFileType = format.avFileType
        session.shouldOptimizeForNetworkUse = true
        
        startProgressMonitoring()
        
        session.exportAsynchronously { [weak self] in
            DispatchQueue.main.async {
                guard let self = self else { return }
                self.stopProgressMonitoring()
                self.isConverting = false
                
                switch session.status {
                case .completed:
                    self.conversionProgress = 1.0
                    self.convertedURL = outputURL
                case .failed:
                    self.errorMessage = session.error?.localizedDescription ?? "转换失败"
                case .cancelled:
                    self.errorMessage = "转换已取消"
                default:
                    self.errorMessage = "未知错误"
                }
            }
        }
    }
    
    // 方式2: 使用 AVAssetReader + AVAssetWriter（支持 MP3/FLAC）
    private func convertWithAssetReader(asset: AVAsset, sourceURL: URL, format: ConversionFormat) {
        do {
            guard let audioTrack = asset.tracks(withMediaType: .audio).first else {
                errorMessage = "文件中没有音频轨道"
                isConverting = false
                return
            }
            
            let outputURL = generateOutputURL(format: format)
            
            let reader = try AVAssetReader(asset: asset)
            let readerOutput = AVAssetReaderTrackOutput(track: audioTrack, outputSettings: [
                AVFormatIDKey: kAudioFormatLinearPCM,
                AVLinearPCMBitDepthKey: 16,
                AVLinearPCMIsFloatKey: false,
                AVLinearPCMIsBigEndianKey: false,
                AVNumberOfChannelsKey: 2,
                AVSampleRateKey: 44100
            ])
            
            guard reader.canAdd(readerOutput) else {
                errorMessage = "无法读取音频数据"
                isConverting = false
                return
            }
            reader.add(readerOutput)
            
            // 配置 Writer 的输出格式
            let audioSettings: [String: Any]
            switch format {
            case .mp3:
                // iOS 原生不支持编码 MP3，使用 AAC 作为替代
                audioSettings = [
                    AVFormatIDKey: kAudioFormatMPEG4AAC,
                    AVSampleRateKey: 44100,
                    AVNumberOfChannelsKey: 2,
                    AVEncoderAudioQualityKey: AVAudioQuality.high.rawValue
                ]
            case .flac:
                // iOS 原生不支持 FLAC，使用 Apple Lossless 作为替代
                audioSettings = [
                    AVFormatIDKey: kAudioFormatAppleLossless,
                    AVNumberOfChannelsKey: 2,
                    AVSampleRateKey: 44100
                ]
            default:
                audioSettings = [
                    AVFormatIDKey: kAudioFormatMPEG4AAC,
                    AVSampleRateKey: 44100,
                    AVNumberOfChannelsKey: 2,
                    AVEncoderAudioQualityKey: AVAudioQuality.high.rawValue
                ]
            }
            
            let writer = try AVAssetWriter(outputURL: outputURL, fileType: format.avFileType)
            let writerInput = AVAssetWriterInput(mediaType: .audio, outputSettings: audioSettings)
            writerInput.expectsMediaDataInRealTime = false
            
            guard writer.canAdd(writerInput) else {
                errorMessage = "无法写入音频数据"
                isConverting = false
                return
            }
            writer.add(writerInput)
            
            writer.startWriting()
            reader.startReading()
            writer.startSession(atSourceTime: .zero)
            
            // 异步处理
            let totalDuration = CMTimeGetSeconds(asset.duration)
            var processedDuration: Double = 0
            
            let processingQueue = DispatchQueue(label: "audio.conversion")
            processingQueue.async { [weak self] in
                guard let strongSelf = self else { return }
                
                writerInput.requestMediaDataWhenReady(on: processingQueue) {
                    guard reader.status == .reading else {
                        writerInput.markAsFinished()
                        return
                    }
                    
                    while writerInput.isReadyForMoreMediaData {
                        guard reader.status == .reading else {
                            writerInput.markAsFinished()
                            return
                        }
                        
                        if let sampleBuffer = readerOutput.copyNextSampleBuffer() {
                            let pts = CMSampleBufferGetPresentationTimeStamp(sampleBuffer)
                            processedDuration = CMTimeGetSeconds(pts)
                            
                            // 更新进度
                            if totalDuration > 0 {
                                let progress = processedDuration / totalDuration
                                DispatchQueue.main.async {
                                    strongSelf.conversionProgress = progress
                                }
                            }
                            
                            if !writerInput.append(sampleBuffer) {
                                reader.cancelReading()
                                writerInput.markAsFinished()
                                return
                            }
                        } else {
                            writerInput.markAsFinished()
                            
                            switch reader.status {
                            case .completed:
                                writer.finishWriting {
                                    DispatchQueue.main.async {
                                        self?.isConverting = false
                                        self?.conversionProgress = 1.0
                                        if writer.status == .completed {
                                            self?.convertedURL = outputURL
                                        } else {
                                            self?.errorMessage = writer.error?.localizedDescription ?? "写入失败"
                                        }
                                    }
                                }
                            case .failed:
                                writer.finishWriting {
                                    DispatchQueue.main.async {
                                        self?.isConverting = false
                                        self?.errorMessage = reader.error?.localizedDescription ?? "读取失败"
                                    }
                                }
                            default:
                                DispatchQueue.main.async {
                                    self?.isConverting = false
                                    self?.errorMessage = "转换中断"
                                }
                            }
                            return
                        }
                    }
                }
            }
            
        } catch {
            errorMessage = error.localizedDescription
            isConverting = false
        }
    }
    
    func cancelConversion() {
        exportSession?.cancelExport()
        exportSession = nil
        stopProgressMonitoring()
        isConverting = false
    }
    
    private func startProgressMonitoring() {
        stopProgressMonitoring()
        progressTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            guard let self = self, let session = self.exportSession else { return }
            DispatchQueue.main.async {
                self.conversionProgress = Double(session.progress)
            }
        }
    }
    
    private func stopProgressMonitoring() {
        progressTimer?.invalidate()
        progressTimer = nil
    }
    
    private func generateOutputURL(format: ConversionFormat) -> URL {
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "yyyyMMdd_HHmmss"
        let dateStr = dateFormatter.string(from: Date())
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let fileName = "converted_\(dateStr).\(format.rawValue)"
        return documentsPath.appendingPathComponent(fileName)
    }
    
    deinit {
        stopProgressMonitoring()
    }
}
