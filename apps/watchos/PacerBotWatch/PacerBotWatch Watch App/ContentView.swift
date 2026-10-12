import SwiftUI

struct ContentView: View {
    @Bindable var session: PacerBotSession

    var body: some View {
        TabView {
            NavigationStack {
                HomeView(session: session)
            }
            .containerBackground(Color.black.gradient, for: .tabView)

            NavigationStack {
                SpeedControlView(session: session)
            }
            .containerBackground(Color.black.gradient, for: .tabView)

            NavigationStack {
                StatisticsView(session: session)
            }
            .containerBackground(Color.black.gradient, for: .tabView)

            NavigationStack {
                SettingsView(session: session)
            }
            .containerBackground(Color.black.gradient, for: .tabView)
        }
        .tabViewStyle(.verticalPage)
    }
}

#Preview {
    ContentView(session: PacerBotSession())
}
