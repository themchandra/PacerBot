import SwiftUI

@main
struct PacerBotWatch_Watch_AppApp: App {
    @State private var session = PacerBotSession()

    var body: some Scene {
        WindowGroup {
            ContentView(session: session)
        }
    }
}
