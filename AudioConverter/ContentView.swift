import SwiftUI

struct ContentView: View {
    var body: some View {
        NavigationStack {
            AudioConvertView()
        }
    }
}

#Preview {
    ContentView()
        .environmentObject(AudioPlayer())
}
