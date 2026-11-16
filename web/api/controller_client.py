"""
Thin IPC client used by FastAPI to communicate with the RobotController.

Responsibilities:
- Send JSON-RPC requests over a Unix Domain Socket.
- Validate the controller response.
- Convert transport/protocol errors into RobotControllerClientError.

This module intentionally contains no robot control logic.
"""

from __future__ import annotations

import asyncio
import json
import os
import random
import time
from typing import Any

class RobotControllerClientError(Exception):
    """Raised when the RobotController RPC call fails."""


class RobotControllerClient:
    """
    Thin IPC client used by the FastAPI backend to communicate with the
    RobotController service.

    Responsibilities:
    - Send JSON-RPC requests over a Unix Domain Socket (UDS).
    - Validate the controller's response format.
    - Convert transport and protocol errors into RobotControllerClientError.

    This class intentionally contains no robot control logic. All robot
    state, decision making, and hardware interaction are owned by the
    RobotController service.
    """

    def __init__(self) -> None:
        self.socket_path = os.getenv(
            "ROBOT_CONTROLLER_SOCKET",
            "/tmp/pacerbot-controller.sock",
        )
        self.timeout_s = float(
            os.getenv("ROBOT_CONTROLLER_TIMEOUT_S", "1.5")
        )

    async def _call(
        self,
        method: str,
        params: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        request = {
            "method": method,
            "params": params or {},
        }

        try:
            reader, writer = await asyncio.wait_for(
                asyncio.open_unix_connection(self.socket_path),
                timeout=self.timeout_s,
            )
        except Exception as exc:
            raise RobotControllerClientError(
                f"Failed to connect to RobotController at "
                f"{self.socket_path}: {exc}"
            ) from exc

        try:
            writer.write(
                (json.dumps(request) + "\n").encode("utf-8")
            )
            await asyncio.wait_for(
                writer.drain(),
                timeout=self.timeout_s,
            )

            raw = await asyncio.wait_for(
                reader.readline(),
                timeout=self.timeout_s,
            )

            if not raw:
                raise RobotControllerClientError(
                    "RobotController returned an empty response"
                )

            response = json.loads(raw.decode("utf-8"))

        except RobotControllerClientError:
            raise
        except asyncio.TimeoutError as exc:
            raise RobotControllerClientError(
                f"RobotController RPC timeout for method '{method}'"
            ) from exc
        except json.JSONDecodeError as exc:
            raise RobotControllerClientError(
                "RobotController returned invalid JSON"
            ) from exc
        except Exception as exc:
            raise RobotControllerClientError(
                f"RobotController RPC failed for method '{method}': {exc}"
            ) from exc
        finally:
            writer.close()
            await writer.wait_closed()

        if not isinstance(response, dict):
            raise RobotControllerClientError(
                "RobotController response must be a JSON object"
            )

        if response.get("ok") is False:
            raise RobotControllerClientError(
                str(response.get("error", "Unknown controller error"))
            )

        if response.get("ok") is not True:
            raise RobotControllerClientError(
                "RobotController response missing valid 'ok' field"
            )

        if "result" not in response:
            raise RobotControllerClientError(
                "RobotController response missing 'result'"
            )

        result = response["result"]

        if not isinstance(result, dict):
            raise RobotControllerClientError(
                "RobotController result must be a JSON object"
            )

        return result

    async def set_pace(
        self,
        pace_kmh: float,
    ) -> dict[str, Any]:
        return await self._call(
            "set_pace",
            {"pace_kmh": pace_kmh},
        )

    async def start(self) -> dict[str, Any]:
        return await self._call("start")

    async def stop(self) -> dict[str, Any]:
        return await self._call("stop")

    async def estop(self) -> dict[str, Any]:
        return await self._call("estop")

    async def get_telemetry(self) -> dict[str, Any]:
        return await self._call("get_telemetry")

    async def get_health(self) -> dict[str, Any]:
        return await self._call("health")


class MockRobotControllerClient:
    """
    Mock IPC client for local development without robot hardware.

    Activated when USE_MOCK_TELEMETRY=true in the environment.
    Returns plausible fake data so the frontend can be developed and
    tested without a live RobotController.
    """

    def __init__(self) -> None:
        self._running = False
        self._pace_kmh = 10.0
        self._start_time = time.time()

    async def set_pace(self, pace_kmh: float) -> dict[str, Any]:
        self._pace_kmh = pace_kmh
        return {}

    async def start(self) -> dict[str, Any]:
        self._running = True
        return {}

    async def stop(self) -> dict[str, Any]:
        self._running = False
        return {}

    async def estop(self) -> dict[str, Any]:
        self._running = False
        return {}

    async def get_telemetry(self) -> dict[str, Any]:
        # Simulate gentle sensor noise around the target pace
        noise = random.uniform(-0.3, 0.3)
        current_speed = round((self._pace_kmh + noise) if self._running else 0.0, 2)
        speed_error = round(current_speed - self._pace_kmh, 2) if self._running else 0.0
        return {
            "timestamp_ms": int(time.time() * 1000),
            "current_speed": current_speed,
            "target_speed": self._pace_kmh,
            "speed_error": speed_error,
            "state": "running" if self._running else "idle",
        }

    async def get_health(self) -> dict[str, Any]:
        return {"status": "mock", "controller": "MockRobotControllerClient"}


def make_controller() -> RobotControllerClient | MockRobotControllerClient:
    """Return a mock client when USE_MOCK_TELEMETRY is enabled, otherwise real."""
    if os.getenv("USE_MOCK_TELEMETRY", "false").lower() == "true":
        return MockRobotControllerClient()
    return RobotControllerClient()