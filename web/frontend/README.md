# PacerBot Control UI

Minimal React frontend for the PacerBot control API.

## Files

- `src/App.jsx` - main component with all UI
- `src/App.css` - minimal styling
- `vite.config.js` - Vite config with API proxy

## Run

```bash
cd web/frontend
npm install
npm run dev
```

The app runs on `http://localhost:5173` and proxies API calls to `http://localhost:8000`.

Make sure the backend API is running on port 8000:

```bash
cd web/api
API_PORT=8000 ./run_api.sh
```

## Features

- Pace input + set button
- Start / Stop / E-Stop buttons
- Live telemetry display (updated every 500ms)
- Simple, readable code
