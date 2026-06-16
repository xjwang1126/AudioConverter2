import SwiftUI
import AVKit
import AVFoundation

// MARK: - 媒体播放器视图
struct MediaPlayerView: View {
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
                VStack(spacing: 8) {
                    if let player = playerManager.player {
                        VideoPlayer(player: player)
                            .frame(height: 250)
                            .cornerRadius(12)
                            .padding(.horizontal)
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
    
    var player: AVPlayer? {
        didSet {
        }
    }
    
    private var timeObserver: Any?
    private var itemObserver: NSKeyValueObservation?
    
    func loadMedia(url: URL) {
        currentURL = url
        
        let playerItem = AVPlayerItem(url: url)
        let newPlayer = AVPlayer(playerItem: playerItem)
        player = newPlayer
        
        player?.play()
    }
    
    deinit {
        player?.pause()
    }
}

#Preview("MediaPlayerView") {
    NavigationStack {
        MediaPlayerView()
    }
}
