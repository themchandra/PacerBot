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