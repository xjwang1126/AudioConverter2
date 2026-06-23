import SwiftUI

struct ContentView: View {
    @State private var selectedTab = 0
    
    var body: some View {
        TabView(selection: $selectedTab) {
            // 主页
            NavigationStack {
                AudioConvertView()
            }
            .tabItem {
                Label("主页", systemImage: "house")
            }
            .tag(0)
            
            // 我的
            NavigationStack {
                ProfileView()
            }
            .tabItem {
                Label("我的", systemImage: "person")
            }
            .tag(1)
        }
        .tint(.accentColor)
    }
}

#Preview {
    ContentView()
}
