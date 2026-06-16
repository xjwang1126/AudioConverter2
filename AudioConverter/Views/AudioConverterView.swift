import SwiftUI
import AVFoundation

struct AudioConvertView: View {
    @EnvironmentObject private var audioPlayer: AudioPlayer
    @StateObject private var converter = AudioConverterService()
    
    @State private var selectedFileURL: URL?
    @State private var selectedFormat: AudioConverterService.ConversionFormat = .m4a
    @State private var showFilePicker = false
    @State private var showResult = false
    @State private var showShareSheet = false
    
    private let availableFormats = AudioConverterService.ConversionFormat.allCases
    
    var body: some View {
        ScrollView {
            VStack(spacing: 24) {
                // 文件选择区域
                fileSelectionSection
                
                if selectedFileURL != nil {
                    // 格式选择
                    formatSelectionSection
                    
                    // 转换按钮
                    convertButtonSection
                    
                    // 转换进度
                    if converter.isConverting {
                        progressSection
                    }
                    
                    // 转换结果
                    if showResult, let convertedURL = converter.convertedURL {
                        resultSection(url: convertedURL)
                    }
                    
                    // 错误信息
                    if let error = converter.errorMessage {
                        errorSection(message: error)
                    }
                    
                    // 已转换文件列表
                    convertedFilesSection
                }
            }
            .padding()
        }
        .navigationTitle("极简音频转换器")
        .navigationBarTitleDisplayMode(.inline)
        .fileSelector(isPresented: $showFilePicker) { url in
            selectedFileURL = url
            showResult = false
            converter.convertedURL = nil
            converter.errorMessage = nil
            // 播放预览
            audioPlayer.play(url: url)
        }
        .onDisappear {
            audioPlayer.stop()
        }
    }
    
    // MARK: - 文件选择区域
    private var fileSelectionSection: some View {
        VStack(spacing: 12) {
            if let url = selectedFileURL {
                // 已选文件
                VStack(spacing: 16) {
                    AudioFileInfoView(url: url, showPlayButton: true, onPlay: {
                        audioPlayer.play(url: url)
                    }, onRemove: {
                        selectedFileURL = nil
                        showResult = false
                        converter.convertedURL = nil
                        audioPlayer.stop()
                    })
                    .padding()
                    .background(Color(.systemGray6))
                    .cornerRadius(12)
                    
                    // 源文件格式标签
                    HStack {
                        Text("源格式")
                            .font(.subheadline)
                            .foregroundColor(.secondary)
                        Text(url.pathExtension.uppercased())
                            .font(.subheadline)
                            .fontWeight(.semibold)
                            .foregroundColor(.accentColor)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 3)
                            .background(Color.accentColor.opacity(0.1))
                            .cornerRadius(4)
                        Spacer()
                    }
                }
            } else {
                // 未选择文件
                Button(action: { showFilePicker = true }) {
                    VStack(spacing: 12) {
                        Image(systemName: "plus.circle.fill")
                            .font(.system(size: 40))
                            .foregroundColor(.accentColor)
                        Text("选择文件")
                            .font(.headline)
                    }
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 40)
                    .background(Color(.systemGray6))
                    .cornerRadius(16)
                    .overlay(
                        RoundedRectangle(cornerRadius: 16)
                            .stroke(Color.accentColor.opacity(0.3), style: StrokeStyle(lineWidth: 2, dash: [8, 4]))
                    )
                }
                .buttonStyle(.plain)
            }
        }
    }
    
    // MARK: - 格式选择区域
    private var formatSelectionSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            Label("选择目标格式", systemImage: "arrow.triangle.swap")
                .font(.headline)
            
            LazyVGrid(columns: [
                GridItem(.flexible()),
                GridItem(.flexible()),
                GridItem(.flexible())
            ], spacing: 10) {
                ForEach(availableFormats) { format in
                    FormatCard(
                        format: format,
                        isSelected: selectedFormat == format,
                        isOriginal: format.rawValue == selectedFileURL?.pathExtension.lowercased()
                    ) {
                        selectedFormat = format
                    }
                }
            }
        }
    }
    
    // MARK: - 转换按钮
    private var convertButtonSection: some View {
        Button(action: startConversion) {
            HStack(spacing: 8) {
                Image(systemName: "arrow.triangle.swap")
                    .font(.title3)
                Text("转换为 \(selectedFormat.displayName)")
                    .fontWeight(.semibold)
            }
            .frame(maxWidth: .infinity)
            .padding(.vertical, 14)
            .background(
                LinearGradient(
                    colors: [.accentColor, .accentColor.opacity(0.8)],
                    startPoint: .leading,
                    endPoint: .trailing
                )
            )
            .foregroundColor(.white)
            .cornerRadius(12)
        }
        .disabled(converter.isConverting)
    }
    
    // MARK: - 进度区域
    private var progressSection: some View {
        VStack(spacing: 8) {
            ProgressView(value: converter.conversionProgress) {
                HStack {
                    Text("转换中...")
                        .font(.subheadline)
                    Spacer()
                    Text("\(Int(converter.conversionProgress * 100))%")
                        .font(.subheadline.monospacedDigit())
                        .foregroundColor(.accentColor)
                }
            }
            .tint(.accentColor)
            
            Button("取消", role: .destructive) {
                converter.cancelConversion()
            }
            .font(.caption)
        }
        .padding()
        .background(Color(.systemGray6))
        .cornerRadius(12)
    }
    
    // MARK: - 结果区域
    private func resultSection(url: URL) -> some View {
        VStack(spacing: 12) {
            Label("转换完成", systemImage: "checkmark.circle.fill")
                .font(.headline)
                .foregroundColor(.green)
            
            AudioFileInfoView(url: url, showPlayButton: true, onPlay: {
                audioPlayer.play(url: url)
            }, onRemove: nil)
            
            HStack(spacing: 12) {
                Button(action: { showShareSheet = true }) {
                    Label("分享", systemImage: "square.and.arrow.up")
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 10)
                        .background(Color.accentColor.opacity(0.1))
                        .cornerRadius(8)
                }
                
                Button(action: {
                    selectedFileURL = url
                    showResult = false
                }) {
                    Label("继续编辑", systemImage: "arrow.triangle.2.circlepath")
                        .frame(maxWidth: .infinity)
                        .padding(.vertical, 10)
                        .background(Color.accentColor.opacity(0.1))
                        .cornerRadius(8)
                }
            }
        }
        .padding()
        .background(Color.green.opacity(0.05))
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(Color.green.opacity(0.3), lineWidth: 1)
        )
        .sheet(isPresented: $showShareSheet) {
            ShareSheet(items: [url])
        }
    }
    
    // MARK: - 错误区域
    private func errorSection(message: String) -> some View {
        HStack {
            Image(systemName: "exclamationmark.triangle.fill")
                .foregroundColor(.orange)
            Text(message)
                .font(.subheadline)
                .foregroundColor(.secondary)
            Spacer()
            Button("重试") {
                converter.errorMessage = nil
                startConversion()
            }
            .font(.subheadline)
        }
        .padding()
        .background(Color.orange.opacity(0.05))
        .cornerRadius(12)
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(Color.orange.opacity(0.3), lineWidth: 1)
        )
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
                            audioPlayer.play(url: url)
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
    let isOriginal: Bool
    let onTap: () -> Void
    
    var body: some View {
        Button(action: onTap) {
            VStack(spacing: 8) {
                Image(systemName: format.icon)
                    .font(.title3)
                
                Text(format.displayName)
                    .font(.caption)
                    .fontWeight(.medium)
                    .multilineTextAlignment(.center)
                
                if isOriginal {
                    Text("原格式")
                        .font(.system(size: 8))
                        .fontWeight(.bold)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(Color.secondary.opacity(0.2))
                        .cornerRadius(3)
                }
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
            .environmentObject(AudioPlayer())
    }
}
