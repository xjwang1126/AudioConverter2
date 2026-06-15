import SwiftUI
import AVKit

struct VideoPlayerView: UIViewControllerRepresentable {
    let url: URL
    
    func makeUIViewController(context: Context) -> AVPlayerViewController {
        let player = AVPlayer(url: url)
        let controller = AVPlayerViewController()
        controller.player = player
        controller.showsPlaybackControls = true
        return controller
    }
    
    func updateUIViewController(_ uiViewController: AVPlayerViewController, context: Context) {
        // 更新逻辑
    }
}

// MARK: - 视频选择器
struct VideoPickerView: View {
    @Environment(\.dismiss) private var dismiss
    let onSelect: (URL) -> Void
    
    var body: some View {
        Group {
            if let url = Bundle.main.url(forResource: "sample", withExtension: "mp4") {
                VStack {
                    VideoPlayerView(url: url)
                        .edgesIgnoringSafeArea(.all)
                    
                    Button("选择此视频") {
                        onSelect(url)
                        dismiss()
                    }
                    .buttonStyle(.borderedProminent)
                    .padding()
                }
            } else {
                ContentUnavailableView(
                    "暂无视频",
                    systemImage: "video.slash",
                    description: Text("请将视频文件添加到项目中")
                )
            }
        }
    }
}
