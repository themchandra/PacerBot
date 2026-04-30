"""Minimal FastAPI app for testing control flow."""

from contextlib import asynccontextmanager
import os

from dotenv import load_dotenv
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from bridge import UARTBridge
from models import APIResponse, PaceRequest, TelemetryResponse

load_dotenv()

bridge = UARTBridge()


@asynccontextmanager
async def lifespan(app: FastAPI):
    await bridge.init()
    yield
    await bridge.deinit()


app = FastAPI(title="PacerBot Control API", version="0.1.0", lifespan=lifespan)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/")
async def root() -> APIResponse:
    return APIResponse(success=True, message="PacerBot Control API is running")


@app.post("/api/pace/set")
async def set_pace(request: PaceRequest) -> APIResponse:
    await bridge.set_pace(request.pace_kmh)
    return APIResponse(success=True, message="Pace updated", data={"pace_kmh": request.pace_kmh})


@app.post("/api/control/start")
async def start() -> APIResponse:
    await bridge.start()
    return APIResponse(success=True, message="Started")


@app.post("/api/control/stop")
async def stop() -> APIResponse:
    await bridge.stop()
    return APIResponse(success=True, message="Stopped")


@app.post("/api/control/estop")
async def estop() -> APIResponse:
    await bridge.estop()
    return APIResponse(success=True, message="Emergency stop triggered")


@app.get("/api/telemetry")
async def telemetry() -> TelemetryResponse:
    return await bridge.get_telemetry()


@app.get("/health")
async def health_check() -> APIResponse:
    return APIResponse(
        success=True,
        message="healthy",
        data={"connected": bridge.is_connected, "running": bridge.is_running},
    )


if __name__ == "__main__":
    import uvicorn

    host = os.getenv("API_HOST", "0.0.0.0")
    port = int(os.getenv("API_PORT", "8000"))
    debug = os.getenv("DEBUG", "true").lower() == "true"

    uvicorn.run("main:app", host=host, port=port, reload=debug, log_level="info")
