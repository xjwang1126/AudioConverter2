import SwiftUI
import AVKit
import AVFoundation

// MARK: - 媒体播放器视图
struct MediaPlayerView: View {
    @EnvironmentObject private var audioPlayer: AudioPlayer
    @StateObject private var playerManager = MediaPlayerManager()
    
    @State private var showFilePicker = false
    
    var body: some View {
        VStack(spacing: 0) {
            if playerManager.currentURL != nil {
                mediaPlayerContent
            } else {
                emptyStateContent
            }
        }
        .fileSelector(isPresented: $showFilePicker) { url in
            playerManager.loadMedia(url: url)
        }
    }
    
    // MARK: - 空状态
    private var emptyStateContent: some View {
        VStack(spacing: 20) {
            Spacer()
            
            Image(systemName: "play.circle")
                .font(.system(size: 64))
                .foregroundColor(.accentColor.opacity(0.5))
            
            Text("选择要播放的媒体文件")
                .font(.headline)
                .foregroundColor(.secondary)
            
            Text("支持音频和视频文件")
                .font(.subheadline)
                .foregroundColor(.secondary)
            
            Button(action: { showFilePicker = true }) {
                Label("选择文件", systemImage: "doc.badge.plus")
                    .font(.headline)
                    .frame(maxWidth: 200)
                    .padding()
                    .background(Color.accentColor)
                    .foregroundColor(.white)
                    .cornerRadius(12)
            }
            
            Spacer()
        }
        .frame(maxWidth: .infinity)
    }
    
    // MARK: - 播放内容
    private var mediaPlayerContent: some View {
        ScrollView {
            VStack(spacing: 16) {
                // 视频播放器（SwiftUI 内置）- 仅视频文件显示
                if playerManager.isVideo {
                    VStack(spacing: 8) {
                        if let player = playerManager.player {
                            VideoPlayer(player: player)
                                .frame(height: 250)
                                .cornerRadius(12)
                                .padding(.horizontal)
                        }
                    }
                } else {
                    // 音频文件 - 显示音频专辑图（纯色渐变背景）
                    VStack(spacing: 8) {
                        ZStack {
                            RoundedRectangle(cornerRadius: 12)
                                .fill(
                                    LinearGradient(
                                        colors: [.accentColor.opacity(0.3), .accentColor.opacity(0.1)],
                                        startPoint: .topLeading,
                                        endPoint: .bottomTrailing
                                    )
                                )
                                .frame(height: 200)
                                .padding(.horizontal)
                            
                            // 音频动效
                            AudioVisualizerView(isPlaying: playerManager.isPlaying)
                                .frame(height: 80)
                                .padding(.horizontal, 60)
                        }
                    }
                }
            }
            .padding(.vertical)
        }
    }
    
    private func formatTime(_ time: TimeInterval) -> String {
        guard time.isFinite, time >= 0 else { return "00:00" }
        let m = Int(time) / 60
        let s = Int(time) % 60
        return String(format: "%02d:%02d", m, s)
    }
}

// MARK: - 媒体播放管理器
class MediaPlayerManager: ObservableObject {
    @Published var currentURL: URL?
    @Published var isPlaying = false
    @Published var isVideo = false
    @Published var currentTime: TimeInterval = 0
    @Published var duration: TimeInterval = 0
    @Published var playbackProgress: Double = 0
    @Published var volume: Double = 0.8 {
        didSet { player?.volume = Float(volume) }
    }
    @Published var isRepeatOn = false
    
    var player: AVPlayer? {
        didSet {
            setupObservers()
            player?.volume = Float(volume)
        }
    }
    
    private var timeObserver: Any?
    private var itemObserver: NSKeyValueObservation?
    
    func loadMedia(url: URL) {
        currentURL = url
        isVideo = isVideoFile(url: url)
        
        let playerItem = AVPlayerItem(url: url)
        let newPlayer = AVPlayer(playerItem: playerItem)
        newPlayer.volume = Float(volume)
        player = newPlayer
        
        // 获取时长
        let asset = AVAsset(url: url)
        duration = CMTimeGetSeconds(asset.duration)
        
        newPlayer.play()
        isPlaying = true
    }
    
    private func setupObservers() {
        removeObservers()
        
        guard let player = player else { return }
        
        // 时间观察者
        timeObserver = player.addPeriodicTimeObserver(forInterval: CMTime(seconds: 0.5, preferredTimescale: 600), queue: .main) { [weak self] time in
            guard let self = self else { return }
            self.currentTime = CMTimeGetSeconds(time)
            if self.duration > 0 {
                self.playbackProgress = self.currentTime / self.duration
            }
        }
        
        // 播放结束通知
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(playerDidFinishPlaying),
            name: .AVPlayerItemDidPlayToEndTime,
            object: player.currentItem
        )
        
        // 状态监听
        itemObserver = player.currentItem?.observe(\.status, options: [.new]) { [weak self] item, _ in
            if item.status == .readyToPlay {
                self?.duration = CMTimeGetSeconds(item.duration)
            }
        }
    }
    
    private func removeObservers() {
        if let timeObserver = timeObserver {
            player?.removeTimeObserver(timeObserver)
            self.timeObserver = nil
        }
        itemObserver?.invalidate()
        itemObserver = nil
        NotificationCenter.default.removeObserver(self)
    }
    
    @objc private func playerDidFinishPlaying() {
        isPlaying = false
        
        if isRepeatOn {
            player?.seek(to: .zero)
            player?.play()
            isPlaying = true
        }
    }
    
    func togglePlayPause() {
        guard let player = player else { return }
        
        if isPlaying {
            player.pause()
        } else {
            player.play()
        }
        isPlaying.toggle()
    }
    
    func seek(to progress: Double) {
        guard let player = player, duration > 0 else { return }
        let time = CMTime(seconds: progress * duration, preferredTimescale: 600)
        player.seek(to: time)
    }
    
    func skip(by seconds: Double) {
        guard let player = player else { return }
        let newTime = min(max(currentTime + seconds, 0), duration)
        let time = CMTime(seconds: newTime, preferredTimescale: 600)
        player.seek(to: time)
    }
    
    func toggleRepeat() {
        isRepeatOn.toggle()
    }
    
    private func isVideoFile(url: URL) -> Bool {
        let videoExtensions = ["mp4", "mov", "m4v", "avi", "mkv", "wmv", "flv", "webm"]
        return videoExtensions.contains(url.pathExtension.lowercased())
    }
    
    deinit {
        removeObservers()
        player?.pause()
    }
}

// MARK: - 音频波形视觉组件
struct AudioVisualizerView: View {
    let isPlaying: Bool
    @State private var animate = false
    
    var body: some View {
        HStack(spacing: 4) {
            ForEach(0..<20) { index in
                RoundedRectangle(cornerRadius: 2)
                    .fill(
                        LinearGradient(
                            colors: [.accentColor, .accentColor.opacity(0.5)],
                            startPoint: .top,
                            endPoint: .bottom
                        )
                    )
                    .frame(width: 4, height: CGFloat.random(in: 10...60))
                    .scaleEffect(y: animate && isPlaying ? 1.0 : 0.3, anchor: .center)
                    .animation(
                        .easeInOut(duration: 0.4)
                            .repeatForever(autoreverses: true)
                            .delay(Double(index) * 0.05),
                        value: animate && isPlaying
                    )
            }
        }
        .onAppear {
            animate = true
        }
    }
}

#Preview("MediaPlayerView") {
    NavigationStack {
        MediaPlayerView()
            .environmentObject(AudioPlayer())
    }
}
