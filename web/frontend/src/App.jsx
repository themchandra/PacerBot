import { useState, useEffect } from 'react'

const API_BASE = 'http://localhost:8000'

export default function App() {
  // ---------------------- State ----------------------
  // Local UI state for pace, telemetry, status, and timing.
  const [pace, setPace] = useState("6:00")
  const [telemetry, setTelemetry] = useState(null)
  const [status, setStatus] = useState('idle')
  const [startTime, setStartTime] = useState(null)

  // ---------------------- Telemetry Polling ----------------------
  // Poll `/api/telemetry` at a regular interval (ms defined below).
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

  // ---------------------- Pace Conversion ----------------------
  // Convert pace string "mm:ss" into km/h for the API.
  const paceToKmh = (paceString) => {
    const parts = paceString.split(':')

    if (parts.length !== 2) {
      throw new Error('Pace must be in mm:ss format')
    }

    const minutes = Number(parts[0])
    const seconds = Number(parts[1])

    if (
      !Number.isFinite(minutes) ||
      !Number.isFinite(seconds) ||
      minutes <= 0 ||
      seconds < 0 ||
      seconds >= 60
    ) {
      throw new Error('Invalid pace')
    }

    const minutesPerKm = minutes + seconds / 60
    return 60 / minutesPerKm
  }

  // ---------------------- Pace Formatting ----------------------
  // Convert a speed in km/h into a pace string formatted as "m:ss".
  const kmhToPaceString = (kmh) => {
    if (!Number.isFinite(kmh) || kmh <= 0) return '--:--'
    // minutes per km
    const minutesPerKm = 60 / kmh
    const totalSeconds = Math.round(minutesPerKm * 60)
    const minutes = Math.floor(totalSeconds / 60)
    const seconds = totalSeconds % 60
    return `${minutes}:${String(seconds).padStart(2, '0')}`
  }

  // ---------------------- Pacing Status ----------------------
  // Compare current and target speeds (km/h) and return a human-readable status.
  const pacingStatus = (currentKmh, targetKmh) => {
    if (!Number.isFinite(currentKmh) || currentKmh <= 0) return 'No pace data'
    if (!Number.isFinite(targetKmh) || targetKmh <= 0) return 'No target pace'

    const currSecPerKm = 3600 / currentKmh
    const targetSecPerKm = 3600 / targetKmh
    const diff = Math.round(currSecPerKm - targetSecPerKm)

    if (Math.abs(diff) <= 1) return 'On pace'
    if (diff > 0) return `${diff} sec/km behind target`
    return `${Math.abs(diff)} sec/km ahead of target`
  }

  // ---------------------- Set Pace Handler ----------------------
  // Send the converted target pace to the backend API.
  const setPaceHandler = async () => {
    try {
      const paceKmh = paceToKmh(pace)

      const res = await fetch(`${API_BASE}/api/pace/set`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ pace_kmh: paceKmh })
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

  // ----------------------- Start Handler -----------------------
  // Send a start command to the backend API.
  const startHandler = async () => {
    try {
      const res = await fetch(`${API_BASE}/api/control/start`, { method: 'POST' })
      if (res.ok) setStatus('Started')
    } catch (err) {
      setStatus('Error starting')
      console.error(err)
    }
  }

  // ----------------------- Stop Handler ------------------------
  // Send a stop command to the backend API.
  const stopHandler = async () => {
    try {
      const res = await fetch(`${API_BASE}/api/control/stop`, { method: 'POST' })
      if (res.ok) setStatus('Stopped')
    } catch (err) {
      setStatus('Error stopping')
      console.error(err)
    }
  }

  // ----------------------- E-Stop Handler ----------------------
  // Send an emergency-stop command to the backend API.
  const estopHandler = async () => {
    try {
      const res = await fetch(`${API_BASE}/api/control/estop`, { method: 'POST' })
      if (res.ok) setStatus('Emergency stop')
    } catch (err) {
      setStatus('Error')
      console.error(err)
    }
  }

  // ---------------------- Render ----------------------
  return (
    <div className="app">
      <header>
        <h1>PacerBot Control</h1>
        {status && <p className="status">{status}</p>}
      </header>

      <section className="controls">
        <div className="pace-control">
          <label>Target Pace (min/km)</label>
          <input
            type="text"
            value={pace}
            onChange={(e) => setPace(e.target.value)}
            placeholder="6:00"
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
              <span>Current Pace:</span>
              <strong>{kmhToPaceString(telemetry.current_speed)} /km</strong>
            </div>
            <div className="data-row">
              <span>Target Pace:</span>
              <strong>{kmhToPaceString(telemetry.target_speed)} /km</strong>
            </div>
            <div className="data-row">
              <span>Status:</span>
              <strong>{pacingStatus(telemetry.current_speed, telemetry.target_speed)}</strong>
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
      
      <section className="debug">
        <details>
          <summary>Debug</summary>
          {telemetry?.imu ? (
            <div className="telemetry-data">
              <div className="data-row">
                <span>Accel:</span>
                <strong>
                  {Array.isArray(telemetry.imu.accel)
                    ? telemetry.imu.accel.map(v => (typeof v === 'number' ? v.toFixed(2) : 'N/A')).join(', ')
                    : 'N/A'}
                </strong>
              </div>
              <div className="data-row">
                <span>Gyro:</span>
                <strong>
                  {Array.isArray(telemetry.imu.gyro)
                    ? telemetry.imu.gyro.map(v => (typeof v === 'number' ? v.toFixed(2) : 'N/A')).join(', ')
                    : 'N/A'}
                </strong>
              </div>
            </div>
          ) : (
            <p>No IMU data</p>
          )}
        </details>
      </section>
    </div>
  )
}
