import SwiftUI

struct StatisticsView: View {
    var session: PacerBotSession

    var body: some View {
        ScrollView {
            VStack(spacing: 8) {
                StatRow(
                    title: "Current",
                    value: PaceFormatting.pace(session.currentPaceSeconds),
                    unit: session.paceUnit.rawValue
                )
                StatRow(
                    title: "Average",
                    value: PaceFormatting.pace(session.averagePaceSeconds),
                    unit: session.paceUnit.rawValue
                )
                StatRow(
                    title: "Distance",
                    value: PaceFormatting.distance(session.displayedDistance),
                    unit: session.distanceUnitLabel
                )
                StatRow(
                    title: "Time",
                    value: PaceFormatting.duration(session.elapsedSeconds),
                    unit: ""
                )
            }
        }
        .navigationTitle("Stats")
        .navigationBarTitleDisplayMode(.inline)
    }
}

private struct StatRow: View {
    let title: String
    let value: String
    let unit: String

    var body: some View {
        HStack(alignment: .firstTextBaseline) {
            Text(title)
                .font(.caption)
                .foregroundStyle(.secondary)
            Spacer(minLength: 8)
            HStack(alignment: .firstTextBaseline, spacing: 4) {
                Text(value)
                    .font(.title3.monospacedDigit().weight(.semibold))
                    .minimumScaleFactor(0.7)
                    .lineLimit(1)
                if !unit.isEmpty {
                    Text(unit)
                        .font(.caption2)
                        .foregroundStyle(.secondary)
                        .lineLimit(1)
                }
            }
        }
        .padding(.vertical, 4)
        .accessibilityElement(children: .combine)
        .accessibilityLabel("\(title) \(value) \(unit)")
    }
}

#Preview {
    NavigationStack {
        StatisticsView(session: PacerBotSession())
    }
}
