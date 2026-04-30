"""Request and response models for the minimal control API."""

from typing import Optional

from pydantic import BaseModel, Field


class PaceRequest(BaseModel):
    pace_kmh: float = Field(..., gt=0)


class APIResponse(BaseModel):
    success: bool
    message: str
    data: Optional[dict] = None


class TelemetryResponse(BaseModel):
    timestamp_ms: int
    current_speed: float
    target_speed: float
    speed_error: float
    state: str
