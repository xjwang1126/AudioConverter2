import SwiftUI

struct HomeView: View {
    @EnvironmentObject private var audioRecorder: AudioRecorder
    @EnvironmentObject private var audioPlayer: AudioPlayer
    @State private var selectedTab = 0
    
    var body: some View {
        TabView(selection: $selectedTab) {
            // 录音
            NavigationStack {
                RecordingView()
            }
            .tabItem {
                Label("录音", systemImage: "mic.fill")
            }
            .tag(0)
            
            // 格式转换
            NavigationStack {
                AudioConvertView()
            }
            .tabItem {
                Label("转换", systemImage: "arrow.triangle.swap")
            }
            .tag(1)
            
            // 音频剪辑
            NavigationStack {
                AudioTrimView()
            }
            .tabItem {
                Label("剪辑", systemImage: "scissors")
            }
            .tag(2)
            
            // 音频合并
            NavigationStack {
                AudioMergeView()
            }
            .tabItem {
                Label("合并", systemImage: "plus.square.fill.on.square.fill")
            }
            .tag(3)
            
            // 音效
            NavigationStack {
                AudioEffectView()
            }
            .tabItem {
                Label("音效", systemImage: "waveform")
            }
            .tag(4)
        }
        .tint(.accentColor)
    }
}

// MARK: - 录音视图
struct RecordingView: View {
    @EnvironmentObject private var audioRecorder: AudioRecorder
    @EnvironmentObject private var audioPlayer: AudioPlayer
    @State private var isRecording = false
    @State private var recordings: [URL] = []
    
    var body: some View {
        VStack(spacing: 30) {
            // 波形显示
            AudioWaveformView(audioLevels: audioRecorder.audioLevels)
                .frame(height: 200)
                .padding()
            
            // 录音时长
            if isRecording {
                Text(formatTime(audioRecorder.currentTime))
                    .font(.system(size: 48, weight: .light, design: .monospaced))
                    .foregroundColor(.red)
            }
            
            // 录音按钮
            Button(action: toggleRecording) {
                ZStack {
                    Circle()
                        .fill(isRecording ? Color.red : Color.accentColor)
                        .frame(width: 80, height: 80)
                    
                    if isRecording {
                        RoundedRectangle(cornerRadius: 4)
                            .fill(Color.white)
                            .frame(width: 24, height: 24)
                    } else {
                        Circle()
                            .fill(Color.white)
                            .frame(width: 24, height: 24)
                    }
                }
            }
            .shadow(radius: 10)
            
            // 录音列表
            List {
                ForEach(recordings, id: \.self) { url in
                    HStack {
                        Image(systemName: "waveform")
                            .foregroundColor(.accentColor)
                        Text(url.lastPathComponent)
                            .lineLimit(1)
                        Spacer()
                        Button(action: { playAudio(url: url) }) {
                            Image(systemName: "play.circle.fill")
                                .font(.title2)
                        }
                    }
                }
                .onDelete(perform: deleteRecording)
            }
            .listStyle(.insetGrouped)
        }
        .navigationTitle("录音")
        .onAppear {
            loadRecordings()
        }
    }
    
    private func toggleRecording() {
        if isRecording {
            audioRecorder.stopRecording()
            if let url = audioRecorder.lastRecordingURL {
                recordings.append(url)
            }
        } else {
            audioRecorder.startRecording()
        }
        isRecording.toggle()
    }
    
    private func playAudio(url: URL) {
        audioPlayer.play(url: url)
    }
    
    private func loadRecordings() {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        if let files = try? FileManager.default.contentsOfDirectory(at: documentsPath, 
                                                                   includingPropertiesForKeys: nil).filter({ $0.pathExtension == "m4a" }) {
            recordings = files.sorted(by: { $0.lastPathComponent > $1.lastPathComponent })
        }
    }
    
    private func deleteRecording(at offsets: IndexSet) {
        for index in offsets {
            try? FileManager.default.removeItem(at: recordings[index])
        }
        recordings.remove(atOffsets: offsets)
    }
    
    private func formatTime(_ time: TimeInterval) -> String {
        let minutes = Int(time) / 60
        let seconds = Int(time) % 60
        let milliseconds = Int((time.truncatingRemainder(dividingBy: 1)) * 100)
        return String(format: "%02d:%02d.%02d", minutes, seconds, milliseconds)
    }
}

#Preview {
    HomeView()
        .environmentObject(AudioRecorder())
        .environmentObject(AudioPlayer())
}
