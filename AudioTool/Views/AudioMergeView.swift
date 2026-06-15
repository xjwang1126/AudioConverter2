import SwiftUI
import AVFoundation

class AudioMergeViewModel: ObservableObject {
    @Published var audioFiles: [URL] = []
    @Published var isMerging = false
    @Published var mergeProgress: Double = 0
    
    func addFile() {
        let paths = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)
        if let documentsPath = paths.first,
           let files = try? FileManager.default.contentsOfDirectory(at: documentsPath, includingPropertiesForKeys: nil)
            .filter({ $0.pathExtension == "m4a" || $0.pathExtension == "wav" }) {
            for file in files {
                if !audioFiles.contains(file) {
                    audioFiles.append(file)
                }
            }
        }
    }
    
    func moveItem(from source: Int, to destination: Int) {
        let item = audioFiles.remove(at: source)
        audioFiles.insert(item, at: destination)
    }
    
    func deleteItems(at offsets: IndexSet) {
        audioFiles.remove(atOffsets: offsets)
    }
    
    func mergeAudioFiles() {
        guard audioFiles.count >= 2 else { return }
        isMerging = true
        mergeProgress = 0
        
        let composition = AVMutableComposition()
        guard let audioTrack = composition.addMutableTrack(withMediaType: .audio, preferredTrackID: kCMPersistentTrackID_Invalid) else {
            isMerging = false
            return
        }
        
        var currentTime = CMTime.zero
        
        for (index, fileURL) in audioFiles.enumerated() {
            let asset = AVAsset(url: fileURL)
            guard let track = asset.tracks(withMediaType: .audio).first else { continue }
            
            let timeRange = CMTimeRange(start: .zero, duration: asset.duration)
            try? audioTrack.insertTimeRange(timeRange, of: track, at: currentTime)
            currentTime = CMTimeAdd(currentTime, asset.duration)
            
            DispatchQueue.main.async {
                self.mergeProgress = Double(index + 1) / Double(self.audioFiles.count)
            }
        }
        
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let outputURL = documentsPath.appendingPathComponent("merged_\(UUID().uuidString).m4a")
        
        guard let exportSession = AVAssetExportSession(asset: composition, presetName: AVAssetExportPresetAppleM4A) else {
            isMerging = false
            return
        }
        
        exportSession.outputURL = outputURL
        exportSession.outputFileType = .m4a
        
        exportSession.exportAsynchronously { [weak self] in
            DispatchQueue.main.async {
                self?.isMerging = false
                self?.mergeProgress = 1.0
            }
        }
    }
}

struct AudioMergeView: View {
    @StateObject private var viewModel = AudioMergeViewModel()
    
    var body: some View {
        VStack(spacing: 20) {
            List {
                if viewModel.audioFiles.isEmpty {
                    ContentUnavailableView(
                        "添加音频文件",
                        systemImage: "plus.square.on.square",
                        description: Text("点击添加按钮选择要合并的音频文件")
                    )
                }
                
                ForEach(viewModel.audioFiles.indices, id: \.self) { index in
                    HStack {
                        Image(systemName: "\(index + 1).circle.fill")
                            .foregroundColor(.accentColor)
                        Text(viewModel.audioFiles[index].lastPathComponent)
                            .lineLimit(1)
                        Spacer()
                        
                        if index > 0 {
                            Button(action: { viewModel.moveItem(from: index, to: index - 1) }) {
                                Image(systemName: "arrow.up")
                            }
                            .buttonStyle(.plain)
                        }
                        if index < viewModel.audioFiles.count - 1 {
                            Button(action: { viewModel.moveItem(from: index, to: index + 1) }) {
                                Image(systemName: "arrow.down")
                            }
                            .buttonStyle(.plain)
                        }
                    }
                }
                .onDelete(perform: viewModel.deleteItems)
            }
            .listStyle(.insetGrouped)
            
            HStack(spacing: 16) {
                Button(action: viewModel.addFile) {
                    Label("添加", systemImage: "plus")
                        .frame(maxWidth: .infinity)
                        .padding()
                        .background(Color.accentColor.opacity(0.1))
                        .cornerRadius(12)
                }
                
                Button(action: viewModel.mergeAudioFiles) {
                    Label(viewModel.isMerging ? "合并中..." : "合并", systemImage: "arrow.triangle.merge")
                        .frame(maxWidth: .infinity)
                        .padding()
                        .background(Color.accentColor)
                        .foregroundColor(.white)
                        .cornerRadius(12)
                }
                .disabled(viewModel.audioFiles.count < 2 || viewModel.isMerging)
            }
            .padding(.horizontal)
            
            if viewModel.isMerging {
                ProgressView(value: viewModel.mergeProgress)
                    .padding(.horizontal)
            }
        }
        .navigationTitle("音频合并")
    }
}

#Preview {
    NavigationStack {
        AudioMergeView()
    }
}
