import SwiftUI

struct HomeView: View {
    @EnvironmentObject private var audioRecorder: AudioRecorder
    @EnvironmentObject private var audioPlayer: AudioPlayer
    
    var body: some View {
        NavigationStack {
            AudioConvertView()
        }
    }
}

#Preview {
    HomeView()
        .environmentObject(AudioRecorder())
        .environmentObject(AudioPlayer())
}
