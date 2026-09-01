# PacerBot Control API

Minimal FastAPI backend that exposes an HTTP API for the PacerBot frontend. The API forwards control requests to the Linux RobotController service over a Unix Domain Socket using a lightweight JSON-RPC protocol.

The FastAPI service is intentionally thin. It validates HTTP requests, forwards RPC commands to the RobotController, and returns responses to the frontend. Robot state, control logic, and hardware communication are owned by the RobotController service.

## Files

- main.py - FastAPI application and HTTP routes
- controller_client.py - Unix Domain Socket JSON-RPC client
- models.py - Pydantic request and response models
- run_api.sh - Convenience script to start the server

## Architecture

```text
React Frontend
        │ HTTP
        ▼
FastAPI API
        │
        ▼
controller_client.py
        │ Unix Domain Socket (JSON-RPC)
        ▼
RobotController (C++)
        │ UART
        ▼
STM32
```

## Endpoints

- GET /health - Check communication with the RobotController
- POST /api/pace/set - Update the robot target pace
- POST /api/control/start - Start the robot
- POST /api/control/stop - Stop the robot
- POST /api/control/estop - Trigger an emergency stop
- GET /api/telemetry - Retrieve the latest telemetry

## Run

```bash
cd web/api
./run_api.sh
```

The default port is `8000`. You can override it with `API_PORT`:

```bash
API_PORT=8000 ./run_api.sh
```

## Test

```bash
curl http://localhost:8000/health

curl -X POST http://localhost:8000/api/pace/set \
  -H "Content-Type: application/json" \
  -d '{"pace_kmh": 12.5}'

curl -X POST http://localhost:8000/api/control/start

curl http://localhost:8000/api/telemetry
```

If you want to inspect the API by hand, open `http://localhost:8000/docs`.
