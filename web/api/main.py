"""FastAPI entry point for the PacerBot web API.

Responsibilities:
- Expose HTTP endpoints for the React frontend.
- Validate incoming requests using Pydantic models.
- Forward commands to the RobotController over IPC.
- Return controller responses to the frontend.

This module intentionally contains no robot control logic.
"""

from contextlib import asynccontextmanager
import os

from dotenv import load_dotenv
from fastapi import FastAPI
from fastapi import HTTPException
from fastapi.middleware.cors import CORSMiddleware

from controller_client import RobotControllerClient
from controller_client import RobotControllerClientError
from models import APIResponse, PaceRequest, TelemetryResponse

load_dotenv()

# Single IPC client shared by all API routes.
# Communication with the RobotController is performed over a
# Unix Domain Socket using a lightweight JSON-RPC protocol.
controller = RobotControllerClient()


# Perform a startup health check to verify that the RobotController
# service is reachable before serving requests.
@asynccontextmanager
async def lifespan(app: FastAPI):
    # Check whether the RobotController is available, but don't prevent
    # the API from starting if the controller is not running yet.
    try:
        await controller.get_health()
        print("RobotController connected")
    except RobotControllerClientError as exc:
        print(f"Warning: RobotController unavailable: {exc}")

    yield


app = FastAPI(title="PacerBot Control API", version="0.1.0", lifespan=lifespan)

# Enable CORS so the React frontend can access the API during development.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# ---------------------------------------------------------------------
# Control Endpoints
# These endpoints validate HTTP requests and forward them to the
# RobotController. No robot state or control decisions are made here.
# ---------------------------------------------------------------------
@app.get("/")
async def root() -> APIResponse:
    return APIResponse(success=True, message="PacerBot Control API is running")


@app.post("/api/pace/set")
async def set_pace(request: PaceRequest) -> APIResponse:
    try:
        await controller.set_pace(request.pace_kmh)
    except RobotControllerClientError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc
    return APIResponse(success=True, message="Pace updated", data={"pace_kmh": request.pace_kmh})


@app.post("/api/control/start")
async def start() -> APIResponse:
    try:
        await controller.start()
    except RobotControllerClientError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc
    return APIResponse(success=True, message="Started")


@app.post("/api/control/stop")
async def stop() -> APIResponse:
    try:
        await controller.stop()
    except RobotControllerClientError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc
    return APIResponse(success=True, message="Stopped")


@app.post("/api/control/estop")
async def estop() -> APIResponse:
    try:
        await controller.estop()
    except RobotControllerClientError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc
    return APIResponse(success=True, message="Emergency stop triggered")


@app.get("/api/telemetry")
async def telemetry() -> TelemetryResponse:
    try:
        data = await controller.get_telemetry()
    except RobotControllerClientError as exc:
        raise HTTPException(status_code=503, detail=str(exc)) from exc

    try:
        return TelemetryResponse(**data)
    except Exception as exc:
        raise HTTPException(status_code=502, detail=f"Invalid telemetry payload: {exc}") from exc


@app.get("/health")
async def health_check() -> APIResponse:
    try:
        data = await controller.get_health()
        return APIResponse(success=True, message="healthy", data=data)
    except RobotControllerClientError as exc:
        return APIResponse(success=False, message="controller_unavailable", data={"error": str(exc)})


# Local development entry point.
# In production, the API can also be started directly with Uvicorn.
if __name__ == "__main__":
    import uvicorn

    host = os.getenv("API_HOST", "0.0.0.0")
    port = int(os.getenv("API_PORT", "8000"))
    debug = os.getenv("DEBUG", "true").lower() == "true"

    uvicorn.run("main:app", host=host, port=port, reload=debug, log_level="info")
