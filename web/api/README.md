# PacerBot Control API

Minimal FastAPI backend for testing control flow from a future frontend.

## Files

- `main.py` - defines the app and routes directly
- `models.py` - request and response models only
- `bridge.py` - simple in-memory mock bridge
- `run_api.sh` - convenience script to start the server

## Endpoints

- `GET /health` - check whether the mock bridge is connected and running
- `POST /api/pace/set` - set the target pace
  ```json
  {"pace_kmh": 12.5}
  ```
- `POST /api/control/start` - start the mock cart
- `POST /api/control/stop` - stop the mock cart
- `POST /api/control/estop` - emergency stop
- `GET /api/telemetry` - return a single telemetry snapshot

## Run

```bash
cd web/api
./run_api.sh
```

The default port is `8000`. You can override it with `API_PORT`:

```bash
API_PORT=8001 ./run_api.sh
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
