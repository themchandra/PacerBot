"""Simple in-memory mock bridge for testing control flow."""

from time import time

from models import TelemetryResponse


class UARTBridge:
    def __init__(self) -> None:
        self.is_connected = False
        self.is_running = False
        self.target_pace = 0.0

    async def init(self) -> None:
        self.is_connected = True

    async def deinit(self) -> None:
        self.is_connected = False
        self.is_running = False

    async def set_pace(self, pace_kmh: float) -> None:
        self.target_pace = pace_kmh

    async def start(self) -> None:
        self.is_running = True

    async def stop(self) -> None:
        self.is_running = False

    async def estop(self) -> None:
        self.is_running = False
        self.target_pace = 0.0

    async def get_telemetry(self) -> TelemetryResponse:
        current_speed = self.target_pace if self.is_running else 0.0
        return TelemetryResponse(
            timestamp_ms=int(time() * 1000),
            current_speed=current_speed,
            target_speed=self.target_pace,
            speed_error=self.target_pace - current_speed,
            state="RUNNING" if self.is_running else "IDLE",
        )