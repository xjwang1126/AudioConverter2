import SwiftUI
import AVKit

struct AudioConvertView: View {
    
    // MARK: - 状态机
    enum ConversionState {
        case initial       // 初始状态 - 未选择文件
        case fileSelected  // 已选择文件
        case fileConverted    // 已转换文件
    }
    
    @State private var conversionState: ConversionState = .initial
    
    @StateObject private var converter = AudioConverterService()
    
    @State private var selectedFileURL: URL?
    @State private var selectedFormat: AudioConverterService.ConversionFormat = .m4a
    @State private var showFilePicker = false
    @State private var showResult = false
    @State private var showShareSheet = false
    
    @State private var audioFileURL: URL?
    
    @State private var mediaPlayer: AVPlayer?
    
    @State private var audioPlayer: AVPlayer?
    
    // 进度变量 0.46 = 46%
    @State private var extractProgress: Double = 0.46
    
    @State private var showProgressOverlay = false
    
    private let availableFormats = AudioConverterService.ConversionFormat.allCases
    
    var body: some View {
        ZStack {
            ScrollView {
                VStack(spacing: 10) {
                    mediaPlayerSection(player: mediaPlayer)
                    
                    controlSection(url: selectedFileURL)
                    
                    formatSection(url: selectedFileURL)
                    
                    audioPlayerSection(player: audioPlayer, url: converter.convertedURL)
                    
                    audioOperationSection(url: converter.convertedURL)
                }
                .padding()
            }
            .background(Color(.systemGroupedBackground))
            .sheet(isPresented: $showShareSheet) {
                if let url = converter.convertedURL {
                    ShareSheet(items: [url])
                } else if let url = selectedFileURL {
                    ShareSheet(items: [url])
                }
            }
            .navigationTitle("极简音频转换器")
            .navigationBarTitleDisplayMode(.inline)
            .fileSelector(isPresented: $showFilePicker) { url in
                // 停止并清空旧播放器
                mediaPlayer?.pause()
                mediaPlayer = nil
                
                conversionState = .fileSelected
                selectedFileURL = url
                showResult = false
                converter.convertedURL = nil
                converter.errorMessage = nil
                
                mediaPlayer = AVPlayer(url: url)
            }
            .onAppear {
            }
            .onDisappear {
            }
            
            // ========== 核心环形进度区域 ==========
            if showProgressOverlay {
                ZStack {
                    // 半透明灰色全屏蒙版
                    Color.black.opacity(0.5)
                        .ignoresSafeArea()
                    
                    VStack(spacing: 12) {
                        ZStack {
                            // 环形进度圈
                            RingProgressView(
                                progress: extractProgress,
                                ringWidth: 8,
                                ringColor: Color.green,
                                bgRingColor: Color.white
                            )
                            .frame(width: 150, height: 150)
                            
                            Text("\(Int(extractProgress * 100))%")
                                .font(.system(size: 30, weight: .bold))
                                .foregroundColor(.white)
                        }
                        
                        // 中间百分比文字
                        VStack(spacing: 8) {
                            Text("正在转换音频")
                                .font(.title2)
                                .foregroundColor(.white)
                                .padding(.top, 10)
                            
                            Text("请不要关闭极简音频转换器")
                                .font(.subheadline)
                                .foregroundColor(.white.opacity(0.7))
                        }
                        
                        // 【新增】取消按钮，放在VStack最底部
                        Button(action: {
                            // 点击关闭弹窗
                            showProgressOverlay = false
                        }) {
                            Text("取消")
                                .font(.title2)
                                .foregroundColor(.white)
                                .padding(.top, 100)
                        }
                    }
                }
                .zIndex(999)
            }
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
                    }
                    .frame(maxWidth: .infinity)
                    .frame(height: 220)
                }
                .buttonStyle(.plain)
            }
        }
    }
    
    // MARK: - 控制区域（根据状态切换按钮样式）
    private func controlSection(url: URL?) -> some View {
        HStack(spacing: 12) {
            // 选择文件按钮 - 不同状态不同样式
            Button(action: {
                showFilePicker = true
            }) {
                HStack(spacing: 6) {
                    Image(systemName: "doc.badge.plus")
                    Text("选择文件")
                }
                .font(.subheadline)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 12)
                .background(selectFileBg)
                .foregroundColor(selectFileFg)
                .cornerRadius(10)
            }
            .padding(.horizontal, 10)
            .padding(.vertical, 10)
            
            // 转换音频按钮 - 不同状态不同样式
            Button(action: startConversion) {
                HStack(spacing: 6) {
                    Image(systemName: "arrow.triangle.swap")
                    Text("转换音频")
                }
                .font(.subheadline)
                .fontWeight(.semibold)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 12)
                .background(convertFileBg)
                .foregroundColor(convertFileFg)
                .cornerRadius(10)
            }
            .disabled(convertFileDisabled)
            .padding(.horizontal, 10)
            .padding(.vertical, 10)
        }
        .background(Color.white)
        .cornerRadius(12)
    }
    
    // MARK: - 状态驱动的按钮属性
    
    private var selectFileBg: AnyShapeStyle {
        switch conversionState {
        case .initial:
            return AnyShapeStyle(
                LinearGradient(colors: [.accentColor, .accentColor.opacity(0.8)], startPoint: .leading, endPoint: .trailing)
            )
        case .fileSelected:
            return AnyShapeStyle(Color.accentColor.opacity(0.1))
        case .fileConverted:
            return AnyShapeStyle(Color.accentColor.opacity(0.1))
        }
    }
    
    private var selectFileFg: Color {
        switch conversionState {
        case .initial: return .white
        case .fileSelected: return .accentColor
        case .fileConverted: return .accentColor
        }
    }
    
    private var convertFileBg: AnyShapeStyle {
        switch conversionState {
        case .initial:
            return AnyShapeStyle(Color.accentColor.opacity(0.1))
        case .fileSelected:
            return AnyShapeStyle(
                LinearGradient(colors: [.accentColor, .accentColor.opacity(0.8)], startPoint: .leading, endPoint: .trailing)
            )
        case .fileConverted:
            if let url = converter.convertedURL {
                if (url.pathExtension.uppercased() == selectedFormat.displayName) {
                    return AnyShapeStyle(Color.accentColor.opacity(0.1))
                } else {
                    return AnyShapeStyle(
                        LinearGradient(colors: [.accentColor, .accentColor.opacity(0.8)], startPoint: .leading, endPoint: .trailing)
                    )
                }
            } else {
                return AnyShapeStyle(Color.accentColor.opacity(0.1))
            }
        }
    }
    
    private var convertFileFg: Color {
        switch conversionState {
        case .initial: return .accentColor
        case .fileSelected: return .white
        case .fileConverted:
            if let url = converter.convertedURL {
                if (url.pathExtension.uppercased() == selectedFormat.displayName) {
                    return .accentColor
                } else {
                    return .white
                }
            } else {
                return .accentColor
            }
        }
    }
    
    private var convertFileDisabled: Bool {
        switch conversionState {
        case .initial: return true
        case .fileSelected: return false
        case .fileConverted: return false
        }
    }
    
    // MARK: - 格式选择区域
    private func formatSection(url: URL?) -> some View {
        VStack(alignment: .leading, spacing: 12) {
            if let url = url {
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
        }
        .background(Color.white)
        .cornerRadius(12)
    }
    
    private func audioPlayerSection(player: AVPlayer?, url: URL?) -> some View {
        VStack(alignment: .leading, spacing: 12) {
            if let url = url {
                HStack(spacing: 6) {
                    Label("文件信息", systemImage: "music.note")
                        .font(.callout)
                        .foregroundColor(.secondary)
                        .padding(.horizontal,10)
                        .padding(.top, 10)
                    
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
        }
        .background(Color.white)
        .cornerRadius(12)
    }
    
    private func audioOperationSection(url: URL?) -> some View {
        HStack(spacing: 12) {
            if let url = url {
                // 保存按钮 - 根据状态切换样式
                Button(action: { saveToFiles(url: url) }) {
                    HStack(spacing: 6) {
                        Image(systemName: "folder.badge.plus")
                        Text("保存文件")
                    }
                    .font(.subheadline)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 12)
                    .background(saveFileBg)
                    .foregroundColor(saveFileFg)
                    .cornerRadius(10)
                }
                .padding(.horizontal, 10)
                .padding(.vertical, 10)
                
                // 分享按钮 - 根据状态切换样式
                Button(action: { showShareSheet = true }) {
                    HStack(spacing: 6) {
                        Image(systemName: "square.and.arrow.up")
                        Text("分享文件")
                    }
                    .font(.subheadline)
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 12)
                    .background(shareFileBg)
                    .foregroundColor(shareFileFg)
                    .cornerRadius(10)
                }
                .padding(.horizontal, 10)
                .padding(.vertical, 10)
            }
        }
        .background(Color.white)
        .cornerRadius(12)
    }
    
    private var saveFileBg: AnyShapeStyle {
        switch conversionState {
        case .initial:
            return AnyShapeStyle(Color.accentColor.opacity(0.1))
        case .fileSelected:
            return AnyShapeStyle(Color.accentColor.opacity(0.1))
        case .fileConverted:
            if let url = converter.convertedURL {
                if (url.pathExtension.uppercased() == selectedFormat.displayName) {
                    return AnyShapeStyle(LinearGradient(colors: [.accentColor, .accentColor.opacity(0.8)], startPoint: .leading, endPoint: .trailing))
                } else {
                    return AnyShapeStyle(Color.accentColor.opacity(0.1))
                }
            } else {
                return AnyShapeStyle(LinearGradient(colors: [.accentColor, .accentColor.opacity(0.8)], startPoint: .leading, endPoint: .trailing))
            }
        }
    }
    
    private var saveFileFg: Color {
        switch conversionState {
        case .initial: return .accentColor
        case .fileSelected: return .accentColor
        case .fileConverted:
            if let url = converter.convertedURL {
                if (url.pathExtension.uppercased() == selectedFormat.displayName) {
                    return .white
                } else {
                    return .accentColor
                }
            } else {
                return .white
            }
        }
    }
    
    private var shareFileBg: AnyShapeStyle {
        switch conversionState {
        case .initial:
            return AnyShapeStyle(Color.accentColor.opacity(0.1))
        case .fileSelected:
            return AnyShapeStyle(Color.accentColor.opacity(0.1))
        case .fileConverted:
            return AnyShapeStyle(Color.accentColor.opacity(0.1))
        }
    }
    
    private var shareFileFg: Color {
        switch conversionState {
        case .initial: return .accentColor
        case .fileSelected: return .accentColor
        case .fileConverted: return .accentColor
        }
    }
    
    private func fileSize(for url: URL) -> String? {
        guard let attrs = try? FileManager.default.attributesOfItem(atPath: url.path),
              let size = attrs[.size] as? Int64 else { return nil }
        return ByteCountFormatter().string(fromByteCount: size)
    }
    
    private func saveToFiles(url: URL) {
        let picker = UIDocumentPickerViewController(forExporting: [url], asCopy: true)
        guard let windowScene = UIApplication.shared.connectedScenes.first as? UIWindowScene,
              let rootVC = windowScene.windows.first?.rootViewController else { return }
        
        var topVC = rootVC
        while let presented = topVC.presentedViewController {
            topVC = presented
        }
        topVC.present(picker, animated: true)
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
        showProgressOverlay = true;
        
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
            conversionState = .fileConverted
            
            showResult = true
            
            audioPlayer?.pause()
            audioPlayer = nil
            
            guard let url = converter.convertedURL else { return }
            audioPlayer = AVPlayer(url: url)
            
            showProgressOverlay = false;
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
