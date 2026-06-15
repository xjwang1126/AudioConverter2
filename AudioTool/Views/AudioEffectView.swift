import SwiftUI
import AVFoundation

struct AudioEffectView: View {
    @EnvironmentObject private var audioPlayer: AudioPlayer
    @State private var selectedFileURL: URL?
    @State private var selectedEffect: AudioEffect = .normal
    @State private var speed: Double = 1.0
    @State private var pitch: Double = 1.0
    @State private var volume: Double = 1.0
    @State private var isProcessing = false
    
    enum AudioEffect: String, CaseIterable {
        case normal = "原声"
        case speedUp = "加速"
        case slowDown = "减速"
        case chipmunk = "小黄人"
        case deepVoice = "低沉"
        case echo = "回声"
        case reverb = "混响"
        
        var icon: String {
            switch self {
            case .normal: return "waveform"
            case .speedUp: return "forward.fill"
            case .slowDown: return "backward.fill"
            case .chipmunk: return "hare.fill"
            case .deepVoice: return "tortoise.fill"
            case .echo: return "repeat"
            case .reverb: return "dot.radiowaves.left.and.right"
            }
        }
    }
    
    var body: some View {
        VStack(spacing: 20) {
            if selectedFileURL != nil {
                // 当前文件信息
                HStack {
                    Image(systemName: "music.note")
                        .foregroundColor(.accentColor)
                    Text(selectedFileURL?.lastPathComponent ?? "")
                        .lineLimit(1)
                    Spacer()
                    Button("更换") { selectedFileURL = nil }
                        .font(.caption)
                }
                .padding(.horizontal)
                
                // 播放控制
                HStack(spacing: 20) {
                    Button(action: { audioPlayer.playPause() }) {
                        Image(systemName: audioPlayer.isPlaying ? "pause.circle.fill" : "play.circle.fill")
                            .font(.system(size: 40))
                    }
                    
                    VStack(alignment: .leading) {
                        Text(formatTime(audioPlayer.currentTime))
                            .font(.system(.body, design: .monospaced))
                        Text(formatTime(audioPlayer.duration))
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    
                    Spacer()
                    
                    Button(action: { audioPlayer.stop() }) {
                        Image(systemName: "stop.circle")
                            .font(.title2)
                    }
                }
                .padding(.horizontal)
                
                // 效果选择
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: 12) {
                        ForEach(AudioEffect.allCases, id: \.self) { effect in
                            VStack(spacing: 8) {
                                Image(systemName: effect.icon)
                                    .font(.title2)
                                Text(effect.rawValue)
                                    .font(.caption)
                            }
                            .frame(width: 70, height: 70)
                            .background(selectedEffect == effect ? Color.accentColor : Color.secondary.opacity(0.1))
                            .foregroundColor(selectedEffect == effect ? .white : .primary)
                            .cornerRadius(12)
                            .onTapGesture {
                                selectedEffect = effect
                                applyEffect(effect)
                            }
                        }
                    }
                    .padding(.horizontal)
                }
                
                // 参数调节
                VStack(spacing: 16) {
                    if selectedEffect == .speedUp || selectedEffect == .slowDown {
                        ParameterSlider(value: $speed, range: 0.5...2.0, title: "速度", unit: "x")
                    }
                    if selectedEffect == .chipmunk || selectedEffect == .deepVoice {
                        ParameterSlider(value: $pitch, range: 0.5...2.0, title: "音调", unit: "x")
                    }
                    ParameterSlider(value: $volume, range: 0...1.0, title: "音量", unit: "%")
                }
                .padding(.horizontal)
                
                // 导出按钮
                Button(action: exportProcessedAudio) {
                    HStack {
                        Image(systemName: "square.and.arrow.down")
                        Text(isProcessing ? "处理中..." : "导出处理后的音频")
                    }
                    .frame(maxWidth: .infinity)
                    .padding()
                    .background(Color.accentColor)
                    .foregroundColor(.white)
                    .cornerRadius(12)
                }
                .disabled(isProcessing)
                .padding(.horizontal)
                
            } else {
                ContentUnavailableView(
                    "选择音频文件",
                    systemImage: "music.note.list",
                    description: Text("选择一个音频文件来应用音效")
                )
                
                Button("浏览文件") {
                    selectFile()
                }
                .buttonStyle(.borderedProminent)
            }
            
            Spacer()
        }
        .padding(.top)
        .navigationTitle("音效处理")
    }
    
    private func selectFile() {
        let paths = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)
        if let documentsPath = paths.first,
           let files = try? FileManager.default.contentsOfDirectory(at: documentsPath, includingPropertiesForKeys: nil),
           let firstFile = files.first(where: { $0.pathExtension == "m4a" || $0.pathExtension == "wav" }) {
            selectedFileURL = firstFile
            audioPlayer.play(url: firstFile)
        }
    }
    
    private func applyEffect(_ effect: AudioEffect) {
        // 实际实现需要使用 AudioEngine 来处理
        // 这里为 UI 预览设置参数
        switch effect {
        case .normal:
            speed = 1.0
            pitch = 1.0
            volume = 1.0
        case .speedUp:
            speed = 1.5
            pitch = 1.0
        case .slowDown:
            speed = 0.75
            pitch = 1.0
        case .chipmunk:
            speed = 1.0
            pitch = 1.5
        case .deepVoice:
            speed = 1.0
            pitch = 0.7
        case .echo, .reverb:
            speed = 1.0
            pitch = 1.0
        }
    }
    
    private func exportProcessedAudio() {
        // 使用 AVAudioEngine + AVAudioUnit 处理音频
        isProcessing = true
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
            isProcessing = false
        }
    }
    
    private func formatTime(_ time: TimeInterval) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        return String(format: "%02d:%02d", minutes, seconds)
    }
}

// MARK: - 参数滑块组件
struct ParameterSlider: View {
    @Binding var value: Double
    let range: ClosedRange<Double>
    let title: String
    let unit: String
    
    var body: some View {
        VStack(spacing: 4) {
            HStack {
                Text(title)
                    .font(.subheadline)
                Spacer()
                Text(String(format: "%.1f %@", value, unit))
                    .font(.subheadline.monospacedDigit())
                    .foregroundColor(.accentColor)
            }
            
            Slider(value: $value, in: range)
        }
    }
}

#Preview {
    NavigationStack {
        AudioEffectView()
            .environmentObject(AudioPlayer())
    }
}
