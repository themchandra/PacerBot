import SwiftUI

struct SpeedControlView: View {
    @Bindable var session: PacerBotSession

    var body: some View {
        ScrollView {
            VStack(spacing: 10) {
                Text("Target Pace")
                    .font(.caption2)
                    .foregroundStyle(.secondary)

                HStack(spacing: 8) {
                    Button {
                        session.decreaseTargetPace()
                    } label: {
                        Image(systemName: "minus")
                            .font(.title3.weight(.bold))
                            .frame(width: 40, height: 40)
                    }
                    .buttonStyle(.bordered)
                    .accessibilityLabel("Faster target pace")
                    .accessibilityIdentifier("decreasePaceButton")

                    VStack(spacing: 0) {
                        Text(PaceFormatting.pace(session.displayedTargetPaceSeconds))
                            .font(.title.monospacedDigit().weight(.semibold))
                            .minimumScaleFactor(0.6)
                            .lineLimit(1)
                            .accessibilityIdentifier("targetPaceValue")
                        Text(session.paceUnit.rawValue)
                            .font(.caption2)
                            .foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity)
                    .focusable()
                    .digitalCrownRotation(
                        Binding(
                            get: { session.targetPaceSecondsPerKm },
                            set: { session.targetPaceSecondsPerKm = $0 }
                        ),
                        from: PacerBotSession.minPaceSecondsPerKm,
                        through: PacerBotSession.maxPaceSecondsPerKm,
                        by: PacerBotSession.paceStepSeconds,
                        sensitivity: .medium,
                        isContinuous: false,
                        isHapticFeedbackEnabled: true
                    )

                    Button {
                        session.increaseTargetPace()
                    } label: {
                        Image(systemName: "plus")
                            .font(.title3.weight(.bold))
                            .frame(width: 40, height: 40)
                    }
                    .buttonStyle(.bordered)
                    .accessibilityLabel("Slower target pace")
                    .accessibilityIdentifier("increasePaceButton")
                }

                HStack {
                    Text("Speed")
                        .foregroundStyle(.secondary)
                    Spacer()
                    Text("\(PaceFormatting.speed(session.displayedSpeed)) \(session.speedUnitLabel)")
                        .font(.headline.monospacedDigit())
                        .minimumScaleFactor(0.7)
                        .lineLimit(1)
                }
                .font(.caption)

                Button(role: .destructive) {
                    session.emergencyStop()
                } label: {
                    Label("E-Stop", systemImage: "exclamationmark.octagon.fill")
                        .font(.headline)
                        .frame(maxWidth: .infinity, minHeight: 40)
                }
                .accessibilityIdentifier("emergencyStopButton")
            }
            .padding(.horizontal, 2)
        }
        .navigationTitle("Speed")
        .navigationBarTitleDisplayMode(.inline)
    }
}

#Preview {
    NavigationStack {
        SpeedControlView(session: PacerBotSession())
    }
}
