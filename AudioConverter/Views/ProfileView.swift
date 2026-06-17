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
                        Text("极简音频转换器")
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
                    FormatGuideView()
                } label: {
                    Label("支持的格式说明", systemImage: "doc.text.magnifyingglass")
                }
                
                NavigationLink {
                    AboutView()
                } label: {
                    Label("关于应用", systemImage: "info.circle")
                }
                
                NavigationLink {
                    PrivacyPolicyView()
                } label: {
                    Label("隐私政策", systemImage: "hand.raised")
                }
                
                NavigationLink {
                    TermsOfUseView()
                } label: {
                    Label("使用条款", systemImage: "doc.text")
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
        ("MP3", "最主流的音频格式，兼容性极广", "music.note"),
        ("AAC", "Advanced Audio Coding，比 MP3 压缩效率更高", "speaker.wave.2"),
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

// MARK: - 隐私政策
struct PrivacyPolicyView: View {
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                Group {
                    Text("隐私政策")
                        .font(.title2)
                        .fontWeight(.bold)
                    
                    Text("最后更新日期：2025年1月")
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                    
                    Text("我们尊重并保护您的隐私。本隐私政策说明了我们如何收集、使用和保护您的个人信息。")
                        .font(.body)
                    
                    Text("1. 信息收集")
                        .font(.headline)
                    Text("本应用不会收集您的任何个人信息。所有音频文件处理均在您的设备本地完成，不会上传至任何服务器。")
                        .font(.body)
                        .foregroundColor(.secondary)
                    
                    Text("2. 文件处理")
                        .font(.headline)
                    Text("您选择的音频/视频文件仅用于格式转换，所有转换过程在设备本地进行。转换后的文件保存在应用的临时目录中，您可以自行保存到文件App或分享给其他应用。")
                        .font(.body)
                        .foregroundColor(.secondary)
                    
                    Text("3. 数据安全")
                        .font(.headline)
                    Text("由于所有处理均在本地完成，您的文件数据不会离开您的设备。我们采取合理的安全措施保护您的数据安全。")
                        .font(.body)
                        .foregroundColor(.secondary)
                    
                    Text("4. 第三方服务")
                        .font(.headline)
                    Text("本应用不使用任何第三方分析服务或广告SDK，不会向任何第三方共享您的数据。")
                        .font(.body)
                        .foregroundColor(.secondary)
                }
                
                Group {
                    Text("5. 联系我们")
                        .font(.headline)
                    Text("如果您对本隐私政策有任何疑问，请通过应用内反馈功能联系我们。")
                        .font(.body)
                        .foregroundColor(.secondary)
                }
            }
            .padding(20)
        }
        .navigationTitle("隐私政策")
        .navigationBarTitleDisplayMode(.inline)
    }
}

// MARK: - 使用条款
struct TermsOfUseView: View {
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                Group {
                    Text("使用条款")
                        .font(.title2)
                        .fontWeight(.bold)
                    
                    Text("最后更新日期：2025年1月")
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                    
                    Text("欢迎使用极简音频格式转换器。下载或使用本应用即表示您同意以下条款。")
                        .font(.body)
                    
                    Text("1. 服务说明")
                        .font(.headline)
                    Text("本应用提供音频格式转换功能，支持将音频/视频文件转换为多种常见音频格式。所有功能均在设备本地运行。")
                        .font(.body)
                        .foregroundColor(.secondary)
                    
                    Text("2. 使用许可")
                        .font(.headline)
                    Text("您被授予在iOS设备上使用本应用的非独占、不可转让的许可。您不得对本应用进行反向工程、修改或分发。")
                        .font(.body)
                        .foregroundColor(.secondary)
                    
                    Text("3. 用户责任")
                        .font(.headline)
                    Text("您应确保拥有转换文件的合法权利。本应用仅提供技术工具，不对用户转换的文件内容负责。")
                        .font(.body)
                        .foregroundColor(.secondary)
                    
                    Text("4. 免责声明")
                        .font(.headline)
                    Text("本应用按'现状'提供，不提供任何明示或暗示的保证。在适用法律允许的最大范围内，开发者不承担任何因使用本应用而产生的损害赔偿。")
                        .font(.body)
                        .foregroundColor(.secondary)
                    
                    Text("5. 条款变更")
                        .font(.headline)
                    Text("我们保留随时修改这些条款的权利。修改后的条款将在应用更新后生效。继续使用本应用即表示您接受修改后的条款。")
                        .font(.body)
                        .foregroundColor(.secondary)
                }
                
                Group {
                    Text("6. 联系我们")
                        .font(.headline)
                    Text("如有任何问题，请通过应用内反馈功能与我们联系。")
                        .font(.body)
                        .foregroundColor(.secondary)
                }
            }
            .padding(20)
        }
        .navigationTitle("使用条款")
        .navigationBarTitleDisplayMode(.inline)
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
