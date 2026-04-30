import { useState, useEffect } from 'react'

const API_BASE = 'http://localhost:8000'

export default function App() {
  const [pace, setPace] = useState(12.0)
  const [telemetry, setTelemetry] = useState(null)
  const [status, setStatus] = useState('idle')
  const [startTime, setStartTime] = useState(null)

  // Poll telemetry every 500ms
  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const res = await fetch(`${API_BASE}/api/telemetry`)
        if (res.ok) {
          const data = await res.json()
          setTelemetry(data)
          if (!startTime) {
            setStartTime(data.timestamp_ms)
          }
        }
      } catch (err) {
        console.error('Failed to fetch telemetry:', err)
      }
    }, 500)

    return () => clearInterval(interval)
  }, [startTime])

  const setPaceHandler = async () => {
    try {
      const res = await fetch(`${API_BASE}/api/pace/set`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ pace_kmh: parseFloat(pace) })
      })
      if (res.ok) {
        setStatus('Pace updated')
        setTimeout(() => setStatus(''), 2000)
      }
    } catch (err) {
      setStatus('Error setting pace')
      console.error(err)
    }
  }

  const startHandler = async () => {
    try {
      const res = await fetch(`${API_BASE}/api/control/start`, { method: 'POST' })
      if (res.ok) setStatus('Started')
    } catch (err) {
      setStatus('Error starting')
      console.error(err)
    }
  }

  const stopHandler = async () => {
    try {
      const res = await fetch(`${API_BASE}/api/control/stop`, { method: 'POST' })
      if (res.ok) setStatus('Stopped')
    } catch (err) {
      setStatus('Error stopping')
      console.error(err)
    }
  }

  const estopHandler = async () => {
    try {
      const res = await fetch(`${API_BASE}/api/control/estop`, { method: 'POST' })
      if (res.ok) setStatus('Emergency stop')
    } catch (err) {
      setStatus('Error')
      console.error(err)
    }
  }

  return (
    <div className="app">
      <header>
        <h1>PacerBot Control</h1>
        {status && <p className="status">{status}</p>}
      </header>

      <section className="controls">
        <div className="pace-control">
          <label>Target Pace (km/h)</label>
          <input
            type="number"
            value={pace}
            onChange={(e) => setPace(e.target.value)}
            step="0.5"
            min="5"
            max="30"
          />
          <button onClick={setPaceHandler}>Set Pace</button>
        </div>

        <div className="buttons">
          <button onClick={startHandler} className="start">Start</button>
          <button onClick={stopHandler} className="stop">Stop</button>
          <button onClick={estopHandler} className="estop">E-Stop</button>
        </div>
      </section>

      <section className="telemetry">
        <h2>Telemetry</h2>
        {telemetry ? (
          <div className="telemetry-data">
            <div className="data-row">
              <span>State:</span>
              <strong>{telemetry.state}</strong>
            </div>
            <div className="data-row">
              <span>Current Speed:</span>
              <strong>{telemetry.current_speed.toFixed(2)} km/h</strong>
            </div>
            <div className="data-row">
              <span>Target Speed:</span>
              <strong>{telemetry.target_speed.toFixed(2)} km/h</strong>
            </div>
            <div className="data-row">
              <span>Speed Error:</span>
              <strong>{telemetry.speed_error.toFixed(2)} km/h</strong>
            </div>
            <div className="data-row">
              <span>Time:</span>
              <strong>{((telemetry.timestamp_ms - (startTime || telemetry.timestamp_ms)) / 1000).toFixed(1)}s</strong>
            </div>
          </div>
        ) : (
          <p>Waiting for telemetry...</p>
        )}
      </section>
    </div>
  )
}
