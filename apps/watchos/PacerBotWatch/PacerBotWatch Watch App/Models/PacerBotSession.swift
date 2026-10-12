import Foundation
import Observation

enum ConnectionStatus: String, CaseIterable {
    case connected
    case connecting
    case disconnected
}

enum RobotState: String {
    case idle = "Idle"
    case running = "Running"
    case stopped = "Stopped"
}

enum PaceUnit: String, CaseIterable, Identifiable {
    case minPerKm = "min/km"
    case minPerMile = "min/mile"

    var id: String { rawValue }
}

@Observable
final class PacerBotSession {
    static let paceStepSeconds = 5.0
    static let minPaceSecondsPerKm = 3 * 60.0
    static let maxPaceSecondsPerKm = 10 * 60.0
    static let metersPerMile = 1609.344

    var connectionStatus: ConnectionStatus = .connected
    var robotState: RobotState = .idle
    var paceUnit: PaceUnit = .minPerKm
    var hostAddress = "192.168.1.10"
    var port = 8000

    /// Target pace stored as seconds per kilometer.
    var targetPaceSecondsPerKm: Double = 5 * 60

    var currentSpeedMetersPerSecond: Double = 0
    var distanceMeters: Double = 0
    var elapsedSeconds: TimeInterval = 0

    private var simulationTask: Task<Void, Never>?
    private var tickCount = 0

    var isConnected: Bool {
        connectionStatus == .connected
    }

    var canStart: Bool {
        isConnected && robotState != .running
    }

    var canStop: Bool {
        isConnected && robotState == .running
    }

    var displayedTargetPaceSeconds: Double {
        convertedPace(fromSecondsPerKm: targetPaceSecondsPerKm)
    }

    var currentPaceSeconds: Double? {
        guard currentSpeedMetersPerSecond > 0.05 else { return nil }
        let secondsPerKm = 1000.0 / currentSpeedMetersPerSecond
        return convertedPace(fromSecondsPerKm: secondsPerKm)
    }

    var averagePaceSeconds: Double? {
        guard distanceMeters > 5, elapsedSeconds > 0 else { return nil }
        let secondsPerKm = elapsedSeconds / (distanceMeters / 1000.0)
        return convertedPace(fromSecondsPerKm: secondsPerKm)
    }

    var displayedDistance: Double {
        switch paceUnit {
        case .minPerKm:
            return distanceMeters / 1000.0
        case .minPerMile:
            return distanceMeters / Self.metersPerMile
        }
    }

    var distanceUnitLabel: String {
        paceUnit == .minPerKm ? "km" : "mi"
    }

    var speedUnitLabel: String {
        paceUnit == .minPerKm ? "km/h" : "mph"
    }

    var displayedSpeed: Double {
        let kmh = currentSpeedMetersPerSecond * 3.6
        switch paceUnit {
        case .minPerKm:
            return kmh
        case .minPerMile:
            return kmh / 1.609344
        }
    }

    func start() {
        guard canStart else { return }
        robotState = .running
        if elapsedSeconds == 0 {
            distanceMeters = 0
        }
        startSimulation()
    }

    func stop() {
        guard robotState == .running else { return }
        robotState = .idle
        currentSpeedMetersPerSecond = 0
        stopSimulation()
    }

    func emergencyStop() {
        robotState = .stopped
        currentSpeedMetersPerSecond = 0
        stopSimulation()
    }

    func increaseTargetPace() {
        targetPaceSecondsPerKm = min(
            Self.maxPaceSecondsPerKm,
            targetPaceSecondsPerKm + Self.paceStepSeconds
        )
    }

    func decreaseTargetPace() {
        targetPaceSecondsPerKm = max(
            Self.minPaceSecondsPerKm,
            targetPaceSecondsPerKm - Self.paceStepSeconds
        )
    }

    func toggleConnection() {
        if connectionStatus == .connected {
            connectionStatus = .disconnected
            if robotState == .running {
                emergencyStop()
            }
        } else {
            connectionStatus = .connecting
            Task { @MainActor in
                try? await Task.sleep(for: .milliseconds(600))
                connectionStatus = .connected
            }
        }
    }

    func convertedPace(fromSecondsPerKm secondsPerKm: Double) -> Double {
        switch paceUnit {
        case .minPerKm:
            return secondsPerKm
        case .minPerMile:
            return secondsPerKm * Self.metersPerMile / 1000.0
        }
    }

    private func startSimulation() {
        stopSimulation()
        simulationTask = Task { @MainActor [weak self] in
            while let self, !Task.isCancelled {
                try? await Task.sleep(for: .seconds(1))
                guard !Task.isCancelled, self.robotState == .running else { return }
                self.tickSimulation()
            }
        }
    }

    private func stopSimulation() {
        simulationTask?.cancel()
        simulationTask = nil
    }

    private func tickSimulation() {
        tickCount += 1
        elapsedSeconds += 1

        let targetSpeed = 1000.0 / targetPaceSecondsPerKm
        let wobble = sin(Double(tickCount) / 4.0) * 0.12
        currentSpeedMetersPerSecond = max(0.4, targetSpeed + wobble)
        distanceMeters += currentSpeedMetersPerSecond
    }
}

enum PaceFormatting {
    static func pace(_ seconds: Double?) -> String {
        guard let seconds, seconds.isFinite, seconds > 0 else { return "--:--" }
        let total = Int(seconds.rounded())
        return String(format: "%d:%02d", total / 60, total % 60)
    }

    static func duration(_ seconds: TimeInterval) -> String {
        let total = Int(seconds)
        let hours = total / 3600
        let minutes = (total % 3600) / 60
        let secs = total % 60
        if hours > 0 {
            return String(format: "%d:%02d:%02d", hours, minutes, secs)
        }
        return String(format: "%d:%02d", minutes, secs)
    }

    static func distance(_ value: Double) -> String {
        String(format: value >= 10 ? "%.1f" : "%.2f", value)
    }

    static func speed(_ value: Double) -> String {
        String(format: "%.1f", value)
    }
}
