import SwiftUI

struct SettingsView: View {
    @Bindable var session: PacerBotSession

    var body: some View {
        List {
            Section("Connection") {
                HStack {
                    Text("Status")
                    Spacer()
                    Text(session.connectionStatus.rawValue.capitalized)
                        .foregroundStyle(.secondary)
                        .minimumScaleFactor(0.8)
                        .lineLimit(1)
                }
                Button(session.isConnected ? "Disconnect" : "Connect") {
                    session.toggleConnection()
                }
                LabeledContent("Host", value: session.hostAddress)
                    .font(.caption)
                LabeledContent("Port", value: String(session.port))
                    .font(.caption)
            }

            Section("Units") {
                Picker("Pace units", selection: $session.paceUnit) {
                    ForEach(PaceUnit.allCases) { unit in
                        Text(unit.rawValue).tag(unit)
                    }
                }
                .pickerStyle(.navigationLink)
            }
        }
        .navigationTitle("Settings")
        .navigationBarTitleDisplayMode(.inline)
    }
}

#Preview {
    NavigationStack {
        SettingsView(session: PacerBotSession())
    }
}
