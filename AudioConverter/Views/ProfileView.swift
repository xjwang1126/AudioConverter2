import SwiftUI

struct ProfileView: View {
    @State private var showAbout = false
    
    var body: some View {
        List {
            // 应用信息
            Section {
                HStack(spacing: 16) {
                    ZStack {
                        RoundedRectangle(cornerRadius: 14)
                            .fill(
                                LinearGradient(
                                    colors: [.accentColor, .accentColor.opacity(0.7)],
                                    startPoint: .topLeading,
                                    endPoint: .bottomTrailing
                                )
                            )
                            .frame(width: 60, height: 60)
                        
                        Image(systemName: "arrow.triangle.swap")
                            .font(.title)
                            .foregroundColor(.white)
                    }
                    
                    VStack(alignment: .leading, spacing: 4) {
                        Text("极简音频格式转换器")
                            .font(.headline)
                        Text("版本 1.0.0")
                            .font(.subheadline)
                            .foregroundColor(.secondary)
                    }
                }
                .padding(.vertical, 4)
            }
            
            // 功能列表
            Section("功能") {
                NavigationLink {
                    MediaPlayerView()
                } label: {
                    Label("媒体播放器", systemImage: "play.circle")
                }
                
                NavigationLink {
                    FormatGuideView()
                } label: {
                    Label("支持的格式说明", systemImage: "doc.text.magnifyingglass")
                }
                
                NavigationLink {
                    AboutView()
                } label: {
                    Label("关于应用", systemImage: "info.circle")
                }
            }
            
            // 统计信息
            Section("转换统计") {
                StatRow(title: "已转换文件", value: "0 个", icon: "clock.arrow.circlepath")
                StatRow(title: "已用存储", value: "0 MB", icon: "externaldrive")
            }
        }
        .listStyle(.insetGrouped)
    }
}

// MARK: - 格式说明视图
struct FormatGuideView: View {
    let formats: [(name: String, desc: String, icon: String)] = [
        ("M4A (AAC)", "高级音频编码格式，兼容性好，iOS 原生支持", "waveform"),
        ("WAV", "无损 PCM 格式，音质最佳，文件体积较大", "waveform.path"),
        ("MP3", "最主流的音频格式，兼容性极广", "music.note"),
        ("AAC", "Advanced Audio Coding，比 MP3 压缩效率更高", "speaker.wave.2"),
        ("CAF", "Core Audio Format，Apple 的容器格式", "square.stack"),
        ("FLAC", "Free Lossless Audio Codec，开源无损格式", "opticaldisc"),
    ]
    
    var body: some View {
        List {
            ForEach(formats, id: \.name) { format in
                HStack(spacing: 12) {
                    Image(systemName: format.icon)
                        .font(.title2)
                        .foregroundColor(.accentColor)
                        .frame(width: 36)
                    
                    VStack(alignment: .leading, spacing: 2) {
                        Text(format.name)
                            .font(.subheadline)
                            .fontWeight(.medium)
                        Text(format.desc)
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                }
                .padding(.vertical, 2)
            }
        }
        .listStyle(.insetGrouped)
        .navigationTitle("格式说明")
    }
}

// MARK: - 关于视图
struct AboutView: View {
    var body: some View {
        VStack(spacing: 24) {
            Spacer()
            
            ZStack {
                RoundedRectangle(cornerRadius: 20)
                    .fill(
                        LinearGradient(
                            colors: [.accentColor, .accentColor.opacity(0.6)],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .frame(width: 100, height: 100)
                
                Image(systemName: "arrow.triangle.swap")
                    .font(.system(size: 40))
                    .foregroundColor(.white)
            }
            
            Text("极简音频格式转换器")
                .font(.title2)
                .fontWeight(.bold)
            
            Text("版本 1.0.0")
                .font(.subheadline)
                .foregroundColor(.secondary)
            
            Text("一款简洁高效的音频格式转换工具。\n支持从文件App或相册选择音频/视频，\n快速转换为多种常见音频格式。")
                .font(.body)
                .multilineTextAlignment(.center)
                .foregroundColor(.secondary)
                .padding(.horizontal, 40)
            
            Spacer()
            
            Text("© 2025 AudioConverter")
                .font(.caption)
                .foregroundColor(.secondary)
                .padding(.bottom, 20)
        }
        .navigationTitle("关于")
    }
}

// MARK: - 统计行组件
struct StatRow: View {
    let title: String
    let value: String
    let icon: String
    
    var body: some View {
        HStack {
            Image(systemName: icon)
                .foregroundColor(.accentColor)
                .frame(width: 24)
            Text(title)
            Spacer()
            Text(value)
                .foregroundColor(.secondary)
        }
    }
}

#Preview("ProfileView") {
    NavigationStack {
        ProfileView()
    }
}
