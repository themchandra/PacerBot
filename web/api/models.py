"""Pydantic models defining the HTTP request and response schema.

These models validate incoming requests from the React frontend and
ensure responses returned by the FastAPI API follow a consistent format.
"""

from typing import Optional

from pydantic import BaseModel, Field


class PaceRequest(BaseModel):
    """Request payload for updating the robot's target pace."""
    pace_kmh: float = Field(..., gt=0)


class APIResponse(BaseModel):
    """Standard response wrapper returned by control API endpoints."""
    success: bool
    message: str
    data: Optional[dict] = None


class TelemetryResponse(BaseModel):
    """Current robot telemetry returned by the RobotController."""
    timestamp_ms: int
    current_speed: float
    target_speed: float
    speed_error: float
    state: str
