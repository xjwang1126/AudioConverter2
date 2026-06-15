import Foundation
import AVFoundation
import Combine

class AudioRecorder: ObservableObject {
    private var audioRecorder: AVAudioRecorder?
    private var timer: Timer?
    
    @Published var isRecording = false
    @Published var currentTime: TimeInterval = 0
    @Published var audioLevels: [Float] = []
    @Published var lastRecordingURL: URL?
    
    private let settings: [String: Any] = [
        AVFormatIDKey: Int(kAudioFormatMPEG4AAC),
        AVSampleRateKey: 44100,
        AVNumberOfChannelsKey: 2,
        AVEncoderAudioQualityKey: AVAudioQuality.high.rawValue
    ]
    
    init() {
        setupAudioSession()
    }
    
    private func setupAudioSession() {
        let session = AVAudioSession.sharedInstance()
        do {
            try session.setCategory(.playAndRecord, options: [.defaultToSpeaker, .allowBluetooth])
            try session.setActive(true)
        } catch {
            print("音频会话设置失败: \(error.localizedDescription)")
        }
    }
    
    func startRecording() {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let dateFormatter = DateFormatter()
        dateFormatter.dateFormat = "yyyyMMdd_HHmmss"
        let fileName = "录音_\(dateFormatter.string(from: Date())).m4a"
        let audioURL = documentsPath.appendingPathComponent(fileName)
        
        guard let recorder = try? AVAudioRecorder(url: audioURL, settings: settings) else {
            print("录音器初始化失败")
            return
        }
        
        audioRecorder = recorder
        recorder.isMeteringEnabled = true
        recorder.record()
        isRecording = true
        lastRecordingURL = audioURL
        
        startMetering()
    }
    
    func stopRecording() {
        audioRecorder?.stop()
        audioRecorder = nil
        isRecording = false
        currentTime = 0
        timer?.invalidate()
        timer = nil
    }
    
    private func startMetering() {
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] _ in
            guard let self = self, let recorder = self.audioRecorder else { return }
            recorder.updateMeters()
            self.currentTime = recorder.currentTime
            let level = recorder.averagePower(forChannel: 0)
            let normalizedLevel = self.normalizeAudioLevel(level)
            DispatchQueue.main.async {
                self.audioLevels.append(normalizedLevel)
                if self.audioLevels.count > 100 {
                    self.audioLevels.removeFirst()
                }
            }
        }
    }
    
    private func normalizeAudioLevel(_ level: Float) -> Float {
        let minLevel: Float = -60.0
        let maxLevel: Float = 0.0
        let clamped = max(level, minLevel)
        return (clamped - minLevel) / (maxLevel - minLevel)
    }
}
