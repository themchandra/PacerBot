import SwiftUI

struct HomeView: View {
    @Bindable var session: PacerBotSession

    var body: some View {
        VStack(spacing: 8) {
            connectionRow
            stateBadge
            Spacer(minLength: 4)
            controlButton
        }
        .padding(.horizontal, 4)
        .navigationTitle("PacerBot")
        .navigationBarTitleDisplayMode(.inline)
    }

    private var connectionRow: some View {
        HStack(spacing: 6) {
            Circle()
                .fill(connectionColor)
                .frame(width: 8, height: 8)
            Text(session.connectionStatus.rawValue.capitalized)
                .font(.caption)
                .foregroundStyle(.secondary)
                .minimumScaleFactor(0.8)
            Spacer()
        }
        .accessibilityElement(children: .combine)
        .accessibilityLabel("Robot \(session.connectionStatus.rawValue)")
    }

    private var stateBadge: some View {
        VStack(spacing: 2) {
            Text("STATE")
                .font(.caption2)
                .foregroundStyle(.secondary)
            Text(session.robotState.rawValue.uppercased())
                .font(.title2.weight(.bold))
                .foregroundStyle(stateColor)
                .minimumScaleFactor(0.7)
                .lineLimit(1)
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 8)
        .background(stateColor.opacity(0.15), in: RoundedRectangle(cornerRadius: 12, style: .continuous))
        .accessibilityLabel("Robot state \(session.robotState.rawValue)")
    }

    private var controlButton: some View {
        Button {
            if session.robotState == .running {
                session.stop()
            } else {
                session.start()
            }
        } label: {
            Label(
                session.robotState == .running ? "Stop" : "Start",
                systemImage: session.robotState == .running ? "stop.fill" : "play.fill"
            )
            .font(.headline)
            .frame(maxWidth: .infinity, minHeight: 44)
        }
        .buttonStyle(.borderedProminent)
        .tint(session.robotState == .running ? .red : .green)
        .disabled(!session.isConnected)
        .accessibilityIdentifier("homeStartStopButton")
    }

    private var connectionColor: Color {
        switch session.connectionStatus {
        case .connected: .green
        case .connecting: .yellow
        case .disconnected: .red
        }
    }

    private var stateColor: Color {
        switch session.robotState {
        case .idle: .cyan
        case .running: .green
        case .stopped: .orange
        }
    }
}

#Preview {
    NavigationStack {
        HomeView(session: PacerBotSession())
    }
}
