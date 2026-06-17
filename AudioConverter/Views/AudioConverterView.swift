import SwiftUI
import AVKit

struct AudioConvertView: View {    
    @StateObject private var converter = AudioConverterService()
    
    @State private var selectedFileURL: URL?
    @State private var selectedFormat: AudioConverterService.ConversionFormat = .m4a
    @State private var showFilePicker = false
    @State private var showResult = false
    @State private var showShareSheet = false
    
    @State private var audioFileURL: URL?
    
    @State private var mediaPlayer: AVPlayer?
    
    @State private var audioPlayer: AVPlayer?
    
    private let availableFormats = AudioConverterService.ConversionFormat.allCases
    
    var body: some View {
        ScrollView {
            VStack(spacing: 10) {
                mediaPlayerSection(player: mediaPlayer)
                
                controlSection
                
                formatSection
                
                audioPlayerSection(player: audioPlayer, url: converter.convertedURL)
            }
            .padding()
        }
        .background(Color(.systemGroupedBackground))
        .navigationTitle("极简音频转换器")
        .navigationBarTitleDisplayMode(.inline)
        .fileSelector(isPresented: $showFilePicker) { url in
            // 停止并清空旧播放器
            mediaPlayer?.pause()
            mediaPlayer = nil
            
            selectedFileURL = url
            showResult = false
            converter.convertedURL = nil
            converter.errorMessage = nil
            
            mediaPlayer = AVPlayer(url: url)
        }
        .onAppear {
            if let url = selectedFileURL {
                showResult = false
                converter.convertedURL = nil
                converter.errorMessage = nil
                
                // 新增预览播放器
                mediaPlayer = AVPlayer(url: url)
            }
        }
        .onDisappear {
            mediaPlayer?.pause()
            mediaPlayer = nil
        }
    }
    
    // MARK: - 媒体播放器区域
    private func mediaPlayerSection(player: AVPlayer?) -> some View {
        // 未选择文件 - 显示 AVPlayer 背景（带选择提示）
        ZStack(alignment: .center) {
            if let player = player {
                // AVKit 视频播放器作为背景
                VideoPlayer(player: player)
                    .frame(height: 220)
                    .cornerRadius(16)
                    .overlay(
                        RoundedRectangle(cornerRadius: 16)
                            .stroke(Color.accentColor.opacity(0.3), lineWidth: 1)
                    )
            } else {
                VideoPlayer(player: nil)
                    .frame(height: 220)
                    .cornerRadius(16)
                    .overlay(
                        RoundedRectangle(cornerRadius: 16)
                            .stroke(Color.accentColor.opacity(0.3), lineWidth: 1))
                        
                // 半透明遮罩 + 选择提示
                RoundedRectangle(cornerRadius: 16)
                    .fill(Color.black.opacity(0.3))
                    .frame(height: 220)
                
                // 点击选择区域
                Button(action: { showFilePicker = true }) {
                    VStack(spacing: 10) {
                        Image(systemName: "play.circle.fill")
                            .font(.system(size: 44))
                            .foregroundColor(.white)
                        Text("点击选择文件")
                            .font(.headline)
                            .foregroundColor(.white)
                        Text("支持音频和视频格式")
                            .font(.caption)
                            .foregroundColor(.white.opacity(0.8))
                    }
                    .frame(maxWidth: .infinity)
                    .frame(height: 220)
                }
                .buttonStyle(.plain)
            }
        }
    }
    
    // MARK: - 控制区域
    private var controlSection: some View {
        // 两个操作按钮横排
        HStack(spacing: 12) {
            // 选择其他文件
            Button(action: { showFilePicker = true }) {
                HStack(spacing: 6) {
                    Image(systemName: "doc.badge.plus")
                    Text("选择文件")
                }
                .font(.subheadline)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 12)
                .background(
                    LinearGradient(
                        colors: [.accentColor, .accentColor.opacity(0.8)],
                        startPoint: .leading,
                        endPoint: .trailing
                    )
                )
                .foregroundColor(.white)
                .cornerRadius(10)
            }
            .padding(.horizontal, 10)
            .padding(.vertical, 10)
            
            // 开始转换
            Button(action: startConversion) {
                HStack(spacing: 6) {
                    Image(systemName: "arrow.triangle.swap")
                    Text("转换音频")
                }
                .font(.subheadline)
                .fontWeight(.semibold)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 12)
                .background(Color.accentColor.opacity(0.1))
                .foregroundColor(.accentColor)
                .cornerRadius(10)
            }
            .disabled(converter.isConverting)
            .opacity(converter.isConverting ? 0.6 : 1)
            .padding(.horizontal, 10)
            .padding(.vertical, 10)
        }
        .background(Color.white)
        .cornerRadius(12)
    }
    
    // MARK: - 格式选择区域
    private var formatSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Label("转换格式", systemImage: "arrow.triangle.swap")
                .font(.callout)
                .foregroundColor(.secondary)
                .padding(.horizontal,10)
                .padding(.top, 10)
            
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 10) {
                ForEach(availableFormats) { format in
                    FormatCard(
                        format: format,
                        isSelected: selectedFormat == format
                    ) {
                        selectedFormat = format
                    }
                }
            }
            .padding(.horizontal, 10)
            .padding(.bottom, 10)
        }
        .background(Color.white)
        .cornerRadius(12)
    }
    
    private func audioPlayerSection(player: AVPlayer?, url: URL?) -> some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack(spacing: 6) {
                Label("文件信息", systemImage: "music.note")
                    .font(.callout)
                    .foregroundColor(.secondary)
                    .padding(.horizontal,10)
                    .padding(.top, 10)
                
                if let url = url {
                    // 格式标签
                    Text(url.pathExtension.uppercased())
                        .font(.callout)
                        .padding(.horizontal, 5)
                        .padding(.top, 10)
                    
                    // 文件大小
                    if let size = fileSize(for: url) {
                        Text(size)
                            .font(.callout)
                            .padding(.top, 10)
                    }
                }
            }
            
            if let player = player {
                VideoPlayer(player: player)
                    .frame(height: 60)
                    .cornerRadius(10)
                    .overlay(
                        RoundedRectangle(cornerRadius: 16)
                            .stroke(Color.accentColor.opacity(0.3), lineWidth: 1)
                    )
                    .padding(.horizontal, 10)
                    .padding(.bottom, 10)
            } else {
                VideoPlayer(player: nil)
                    .frame(height: 60)
                    .cornerRadius(10)
                    .overlay(
                        RoundedRectangle(cornerRadius: 16)
                            .stroke(Color.accentColor.opacity(0.3), lineWidth: 1)
                    )
                    .padding(.horizontal, 10)
                    .padding(.bottom, 10)
            }
        }
        .background(Color.white)
        .cornerRadius(12)
    }
    
    private func fileSize(for url: URL) -> String? {
        guard let attrs = try? FileManager.default.attributesOfItem(atPath: url.path),
              let size = attrs[.size] as? Int64 else { return nil }
        return ByteCountFormatter().string(fromByteCount: size)
    }

    // MARK: - 已转换文件列表
    private var convertedFilesSection: some View {
        Group {
            if let convertedFiles = loadConvertedFiles(), !convertedFiles.isEmpty {
                VStack(alignment: .leading, spacing: 8) {
                    Label("已转换的文件", systemImage: "clock.arrow.circlepath")
                        .font(.headline)
                    
                    ForEach(convertedFiles, id: \.self) { url in
                        AudioFileInfoView(url: url, showPlayButton: true, onPlay: {
                        }, onRemove: {
                            try? FileManager.default.removeItem(at: url)
                        })
                        .padding(.vertical, 4)
                    }
                }
                .padding()
                .background(Color(.systemGray6))
                .cornerRadius(12)
            }
        }
    }
    
    // MARK: - 操作
    private func startConversion() {
        guard let url = selectedFileURL else { return }
        showResult = false
        converter.convertAudio(from: url, to: selectedFormat)
        
        // 监听转换完成
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            // 通过 publisher 方式监听
            checkConversionComplete()
        }
    }
    
    private func checkConversionComplete() {
        if converter.convertedURL != nil {
            showResult = true
            
            audioPlayer?.pause()
            audioPlayer = nil
            
            guard let url = converter.convertedURL else { return }
            audioPlayer = AVPlayer(url: url)
        } else if converter.isConverting {
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                checkConversionComplete()
            }
        }
    }
    
    private func loadConvertedFiles() -> [URL]? {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        guard let files = try? FileManager.default.contentsOfDirectory(
            at: documentsPath,
            includingPropertiesForKeys: nil
        ) else { return nil }
        
        return files
            .filter { $0.lastPathComponent.hasPrefix("converted_") }
            .sorted(by: { $0.lastPathComponent > $1.lastPathComponent })
            .prefix(5).map { $0 } // 只显示最近5个
    }
}

// MARK: - 格式卡片组件
struct FormatCard: View {
    let format: AudioConverterService.ConversionFormat
    let isSelected: Bool
    let onTap: () -> Void
    
    var body: some View {
        Button(action: onTap) {
            VStack(spacing: 8) {
                Text(format.displayName)
                    .font(.body)
                    .fontWeight(.medium)
                    .multilineTextAlignment(.center)
            }
            .frame(maxWidth: .infinity)
            .padding(.vertical, 12)
            .background(
                RoundedRectangle(cornerRadius: 10)
                    .fill(isSelected ? Color.accentColor : Color(.systemGray6))
            )
            .foregroundColor(isSelected ? .white : .primary)
            .overlay(
                RoundedRectangle(cornerRadius: 10)
                    .stroke(isSelected ? Color.accentColor : Color.clear, lineWidth: 2)
            )
        }
        .buttonStyle(.plain)
    }
}

// MARK: - ShareSheet (UIActivityViewController)
struct ShareSheet: UIViewControllerRepresentable {
    let items: [Any]
    
    func makeUIViewController(context: Context) -> UIActivityViewController {
        UIActivityViewController(activityItems: items, applicationActivities: nil)
    }
    
    func updateUIViewController(_ uiViewController: UIActivityViewController, context: Context) {}
}

#Preview("AudioConvertView") {
    NavigationStack {
        AudioConvertView()
    }
}
