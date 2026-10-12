import Testing
@testable import PacerBotWatch_Watch_App

struct PacerBotWatch_Watch_AppTests {

    @Test func startTransitionsToRunning() {
        let session = PacerBotSession()
        session.start()
        #expect(session.robotState == .running)
    }

    @Test func stopReturnsToIdle() {
        let session = PacerBotSession()
        session.start()
        session.stop()
        #expect(session.robotState == .idle)
        #expect(session.currentSpeedMetersPerSecond == 0)
    }

    @Test func emergencyStopSetsStopped() {
        let session = PacerBotSession()
        session.start()
        session.emergencyStop()
        #expect(session.robotState == .stopped)
        #expect(session.currentSpeedMetersPerSecond == 0)
    }

    @Test func paceButtonsAdjustTargetByFiveSeconds() {
        let session = PacerBotSession()
        let original = session.targetPaceSecondsPerKm
        session.increaseTargetPace()
        #expect(session.targetPaceSecondsPerKm == original + 5)
        session.decreaseTargetPace()
        #expect(session.targetPaceSecondsPerKm == original)
    }

    @Test func paceFormattingUsesMinutesAndSeconds() {
        #expect(PaceFormatting.pace(305) == "5:05")
        #expect(PaceFormatting.pace(nil) == "--:--")
        #expect(PaceFormatting.duration(125) == "2:05")
    }

    @Test func mileUnitsConvertDisplayedPace() {
        let session = PacerBotSession()
        session.targetPaceSecondsPerKm = 300
        session.paceUnit = .minPerMile
        let expected = 300 * PacerBotSession.metersPerMile / 1000.0
        #expect(abs(session.displayedTargetPaceSeconds - expected) < 0.01)
    }
}
