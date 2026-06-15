import SwiftUI

@main
struct AudioToolApp: App {
    @StateObject private var audioPlayer = AudioPlayer()
    
    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(audioPlayer)
        }
    }
}
