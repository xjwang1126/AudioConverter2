import SwiftUI
import AVFoundation

class AudioTrimViewModel: ObservableObject {
    @Published var selectedFileURL: URL?
    @Published var startTime: Double = 0
    @Published var endTime: Double = 1
    @Published var duration: Double = 1
    @Published var isExporting = false
    
    func selectFile() {
        let paths = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)
        if let documentsPath = paths.first,
           let files = try? FileManager.default.contentsOfDirectory(at: documentsPath, includingPropertiesForKeys: nil),
           let firstFile = files.first(where: { $0.pathExtension == "m4a" || $0.pathExtension == "wav" || $0.pathExtension == "mp3" }) {
            selectedFileURL = firstFile
            startTime = 0
            endTime = 1
        }
    }
    
    func exportTrimmedAudio() {
        guard let url = selectedFileURL else { return }
        isExporting = true
        
        let asset = AVAsset(url: url)
        guard let exportSession = AVAssetExportSession(asset: asset, presetName: AVAssetExportPresetAppleM4A) else {
            isExporting = false
            return
        }
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let outputURL = documentsPath.appendingPathComponent("trimmed_\(UUID().uuidString).m4a")
        
        let startCMTime = CMTime(seconds: startTime * duration, preferredTimescale: 1000)
        let endCMTime = CMTime(seconds: endTime * duration, preferredTimescale: 1000)
        let timeRange = CMTimeRange(start: startCMTime, end: endCMTime)
        
        exportSession.outputURL = outputURL
        exportSession.outputFileType = .m4a
        exportSession.timeRange = timeRange
        
        exportSession.exportAsynchronously {
            DispatchQueue.main.async {
                self.isExporting = false
            }
        }
    }
}

struct AudioTrimView: View {
    @EnvironmentObject private var audioPlayer: AudioPlayer
    @StateObject private var viewModel = AudioTrimViewModel()
    
    var body: some View {
        VStack(spacing: 20) {
            if let url = viewModel.selectedFileURL {
                VStack(spacing: 16) {
                    // 文件名
                    HStack {
                        Image(systemName: "waveform")
                            .foregroundColor(.accentColor)
                        Text(url.lastPathComponent)
                            .lineLimit(1)
                        Spacer()
                        Button("更换", systemImage: "arrow.triangle.2.circlepath") {
                            viewModel.selectedFileURL = nil
                        }
                        .labelStyle(.iconOnly)
                    }
                    .padding(.horizontal)
                    
                    // 播放控制
                    HStack(spacing: 20) {
                        Button(action: { audioPlayer.playPause() }) {
                            Image(systemName: audioPlayer.isPlaying ? "pause.circle.fill" : "play.circle.fill")
                                .font(.system(size: 44))
                                .foregroundColor(.accentColor)
                        }
                        
                        VStack(alignment: .leading) {
                            Text(formatTime(audioPlayer.currentTime))
                                .font(.system(.body, design: .monospaced))
                            Text(formatTime(viewModel.duration))
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                    }
                    
                    // 裁剪滑块
                    VStack {
                        Slider(value: $audioPlayer.playbackProgress, in: 0...1) { editing in
                            if !editing {
                                audioPlayer.seek(to: audioPlayer.playbackProgress)
                            }
                        }
                        
                        RangeSliderView(
                            startTime: $viewModel.startTime,
                            endTime: $viewModel.endTime,
                            duration: viewModel.duration
                        )
                        .frame(height: 40)
                        
                        HStack {
                            Text("起始: \(formatTime(viewModel.startTime * viewModel.duration))")
                                .font(.caption)
                            Spacer()
                            Text("结束: \(formatTime(viewModel.endTime * viewModel.duration))")
                                .font(.caption)
                        }
                    }
                    .padding(.horizontal)
                    
                    Button(action: viewModel.exportTrimmedAudio) {
                        HStack {
                            Image(systemName: "square.and.arrow.down")
                            Text(viewModel.isExporting ? "导出中..." : "导出裁剪音频")
                        }
                        .frame(maxWidth: .infinity)
                        .padding()
                        .background(Color.accentColor)
                        .foregroundColor(.white)
                        .cornerRadius(12)
                    }
                    .disabled(viewModel.isExporting)
                    .padding(.horizontal)
                }
            } else {
                ContentUnavailableView(
                    "选择音频文件",
                    systemImage: "music.note.list",
                    description: Text("从文件中选择要裁剪的音频")
                )
                
                Button("浏览文件") {
                    viewModel.selectFile()
                }
                .buttonStyle(.borderedProminent)
            }
            
            Spacer()
        }
        .padding(.top)
        .navigationTitle("音频裁剪")
        .onReceive(audioPlayer.$duration) { newDuration in
            if newDuration > 0 {
                viewModel.duration = newDuration
                viewModel.endTime = 1.0
            }
        }
        .onChange(of: viewModel.selectedFileURL) { _, newURL in
            if let url = newURL {
                audioPlayer.play(url: url)
                viewModel.startTime = 0
                viewModel.endTime = 1
            }
        }
    }
    
    private func formatTime(_ time: TimeInterval) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        return String(format: "%02d:%02d", minutes, seconds)
    }
}

// MARK: - 范围滑块视图
struct RangeSliderView: View {
    @Binding var startTime: Double
    @Binding var endTime: Double
    let duration: Double
    
    var body: some View {
        GeometryReader { geometry in
            let width = geometry.size.width
            let startX = CGFloat(startTime) * width
            let endX = CGFloat(endTime) * width
            
            ZStack(alignment: .leading) {
                Rectangle()
                    .fill(Color.secondary.opacity(0.2))
                    .frame(height: 6)
                    .cornerRadius(3)
                
                Rectangle()
                    .fill(Color.accentColor.opacity(0.3))
                    .frame(width: max(endX - startX, 0), height: 6)
                    .offset(x: startX)
                    .cornerRadius(3)
                
                Circle()
                    .fill(Color.white)
                    .frame(width: 20, height: 20)
                    .shadow(radius: 2)
                    .offset(x: startX - 10)
                    .gesture(DragGesture()
                        .onChanged { value in
                            let newStart = Double(value.location.x / width)
                            startTime = max(0, min(newStart, endTime - 0.05))
                        }
                    )
                
                Circle()
                    .fill(Color.white)
                    .frame(width: 20, height: 20)
                    .shadow(radius: 2)
                    .offset(x: endX - 10)
                    .gesture(DragGesture()
                        .onChanged { value in
                            let newEnd = Double(value.location.x / width)
                            endTime = min(1, max(newEnd, startTime + 0.05))
                        }
                    )
            }
            .frame(maxHeight: .infinity)
        }
    }
}

#Preview {
    NavigationStack {
        AudioTrimView()
            .environmentObject(AudioPlayer())
    }
}
