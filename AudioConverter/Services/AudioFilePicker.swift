import SwiftUI
import UniformTypeIdentifiers
import PhotosUI
import AVFoundation

// MARK: - 从文件App选择音频
struct AudioDocumentPicker: UIViewControllerRepresentable {
    var onPick: (URL) -> Void
    var onDismiss: (() -> Void)?
    
    func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
        let supportedTypes: [UTType] = [
            .audio,
            .mp3,
            .wav,
            .mpeg4Audio,
            .appleProtectedMPEG4Audio,
            UTType(filenameExtension: "m4a") ?? .audio,
            UTType(filenameExtension: "caf") ?? .audio,
            UTType(filenameExtension: "aac") ?? .audio,
            UTType(filenameExtension: "flac") ?? .audio,
        ]
        let picker = UIDocumentPickerViewController(forOpeningContentTypes: supportedTypes, asCopy: true)
        picker.delegate = context.coordinator
        picker.allowsMultipleSelection = false
        return picker
    }
    
    func updateUIViewController(_ uiViewController: UIDocumentPickerViewController, context: Context) {}
    
    func makeCoordinator() -> Coordinator {
        Coordinator(onPick: onPick, onDismiss: onDismiss)
    }
    
    class Coordinator: NSObject, UIDocumentPickerDelegate {
        let onPick: (URL) -> Void
        let onDismiss: (() -> Void)?
        
        init(onPick: @escaping (URL) -> Void, onDismiss: (() -> Void)?) {
            self.onPick = onPick
            self.onDismiss = onDismiss
        }
        
        func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
            guard let url = urls.first else { return }
            onPick(url)
        }
        
        func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
            onDismiss?()
        }
    }
}

// MARK: - 从相册选择视频（提取音频）
struct AudioPhotoPicker: UIViewControllerRepresentable {
    var onPick: (URL) -> Void
    var onDismiss: (() -> Void)?
    
    func makeUIViewController(context: Context) -> PHPickerViewController {
        var config = PHPickerConfiguration()
        config.filter = .videos
        config.selectionLimit = 1
        config.preferredAssetRepresentationMode = .current
        
        let picker = PHPickerViewController(configuration: config)
        picker.delegate = context.coordinator
        return picker
    }
    
    func updateUIViewController(_ uiViewController: PHPickerViewController, context: Context) {}
    
    func makeCoordinator() -> Coordinator {
        Coordinator(onPick: onPick, onDismiss: onDismiss)
    }
    
    class Coordinator: NSObject, PHPickerViewControllerDelegate {
        let onPick: (URL) -> Void
        let onDismiss: (() -> Void)?
        
        init(onPick: @escaping (URL) -> Void, onDismiss: (() -> Void)?) {
            self.onPick = onPick
            self.onDismiss = onDismiss
        }
        
        func picker(_ picker: PHPickerViewController, didFinishPicking results: [PHPickerResult]) {
            guard let result = results.first else {
                onDismiss?()
                picker.dismiss(animated: true)
                return
            }
            
            // 加载视频文件
            let videoType = UTType.movie.identifier
            result.itemProvider.loadFileRepresentation(forTypeIdentifier: videoType) { url, error in
                if let url = url {
                    let tempDir = FileManager.default.temporaryDirectory
                    let destURL = tempDir.appendingPathComponent(url.lastPathComponent)
                    try? FileManager.default.copyItem(at: url, to: destURL)
                    
                    // 从视频中提取音频
                    self.extractAudio(from: destURL) { audioURL in
                        DispatchQueue.main.async {
                            if let audioURL = audioURL {
                                self.onPick(audioURL)
                            } else {
                                self.onPick(destURL)
                            }
                            picker.dismiss(animated: true)
                        }
                    }
                } else {
                    // 备选方案
                    result.itemProvider.loadItem(forTypeIdentifier: videoType, options: nil) { item, error in
                        DispatchQueue.main.async {
                            guard let url = item as? URL else {
                                self.onDismiss?()
                                picker.dismiss(animated: true)
                                return
                            }
                            let tempDir = FileManager.default.temporaryDirectory
                            let destURL = tempDir.appendingPathComponent(url.lastPathComponent)
                            try? FileManager.default.copyItem(at: url, to: destURL)
                            
                            self.extractAudio(from: destURL) { audioURL in
                                if let audioURL = audioURL {
                                    self.onPick(audioURL)
                                } else {
                                    self.onPick(destURL)
                                }
                                picker.dismiss(animated: true)
                            }
                        }
                    }
                }
            }
        }
        
        private func extractAudio(from videoURL: URL, completion: @escaping (URL?) -> Void) {
            let asset = AVAsset(url: videoURL)
            
            guard asset.tracks(withMediaType: .audio).first != nil else {
                completion(nil)
                return
            }
            
            guard let exportSession = AVAssetExportSession(asset: asset, presetName: AVAssetExportPresetAppleM4A) else {
                completion(nil)
                return
            }
            
            let outputURL = FileManager.default.temporaryDirectory
                .appendingPathComponent("extracted_audio_\(UUID().uuidString).m4a")
            
            exportSession.outputURL = outputURL
            exportSession.outputFileType = .m4a
            exportSession.shouldOptimizeForNetworkUse = true
            exportSession.timeRange = CMTimeRange(start: .zero, duration: asset.duration)
            
            exportSession.exportAsynchronously {
                switch exportSession.status {
                case .completed:
                    completion(outputURL)
                case .failed, .cancelled:
                    print("音频提取失败: \(exportSession.error?.localizedDescription ?? "未知错误")")
                    completion(nil)
                default:
                    completion(nil)
                }
            }
        }
    }
}

// MARK: - 统一文件选择器
struct AudioFileSelectorModifier: ViewModifier {
    @Binding var isPresented: Bool
    var onPickFile: (URL) -> Void
    
    @State private var showDocumentPicker = false
    @State private var showPhotoPicker = false
    
    func body(content: Content) -> some View {
        content
            .confirmationDialog("选择音频来源", isPresented: $isPresented, titleVisibility: .visible) {
                Button("从文件选择") {
                    showDocumentPicker = true
                }
                Button("从相册选择") {
                    showPhotoPicker = true
                }
                Button("取消", role: .cancel) { }
            }
            .sheet(isPresented: $showDocumentPicker) {
                AudioDocumentPicker(onPick: { url in
                    onPickFile(url)
                    showDocumentPicker = false
                }, onDismiss: {
                    showDocumentPicker = false
                })
            }
            .sheet(isPresented: $showPhotoPicker) {
                AudioPhotoPicker(onPick: { url in
                    onPickFile(url)
                    showPhotoPicker = false
                }, onDismiss: {
                    showPhotoPicker = false
                })
            }
    }
}

extension View {
    func audioFileSelector(isPresented: Binding<Bool>, onPickFile: @escaping (URL) -> Void) -> some View {
        modifier(AudioFileSelectorModifier(isPresented: isPresented, onPickFile: onPickFile))
    }
}

// MARK: - 音频文件信息展示
struct AudioFileInfoView: View {
    let url: URL
    var showPlayButton: Bool = true
    var onPlay: (() -> Void)?
    var onRemove: (() -> Void)?
    
    @State private var fileSize: String = ""
    @State private var duration: String = ""
    
    var body: some View {
        HStack(spacing: 12) {
            ZStack {
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.accentColor.opacity(0.15))
                    .frame(width: 44, height: 44)
                Image(systemName: "music.note")
                    .font(.title3)
                    .foregroundColor(.accentColor)
            }
            
            VStack(alignment: .leading, spacing: 2) {
                Text(url.lastPathComponent)
                    .font(.subheadline)
                    .fontWeight(.medium)
                    .lineLimit(1)
                
                HStack(spacing: 8) {
                    if !fileSize.isEmpty {
                        Text(fileSize)
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                    if !duration.isEmpty {
                        Text(duration)
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                    Text(url.pathExtension.uppercased())
                        .font(.caption2)
                        .fontWeight(.semibold)
                        .foregroundColor(.accentColor)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(Color.accentColor.opacity(0.1))
                        .cornerRadius(3)
                }
            }
            
            Spacer()
            
            HStack(spacing: 4) {
                if showPlayButton {
                    Button(action: { onPlay?() }) {
                        Image(systemName: "play.circle.fill")
                            .font(.title3)
                            .foregroundColor(.accentColor)
                    }
                    .buttonStyle(.plain)
                }
                if let onRemove = onRemove {
                    Button(action: onRemove) {
                        Image(systemName: "xmark.circle.fill")
                            .font(.title3)
                            .foregroundColor(.secondary)
                    }
                    .buttonStyle(.plain)
                }
            }
        }
        .padding(.vertical, 4)
        .onAppear { loadFileInfo() }
    }
    
    private func loadFileInfo() {
        if let attrs = try? FileManager.default.attributesOfItem(atPath: url.path),
           let size = attrs[.size] as? Int64 {
            fileSize = ByteCountFormatter().string(fromByteCount: size)
        }
        
        let asset = AVAsset(url: url)
        let durationSeconds = CMTimeGetSeconds(asset.duration)
        if durationSeconds.isFinite && durationSeconds > 0 {
            let m = Int(durationSeconds) / 60
            let s = Int(durationSeconds) % 60
            duration = String(format: "%02d:%02d", m, s)
        }
    }
}
