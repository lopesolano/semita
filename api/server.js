// API REST - Control de giro con sensor (EAI441) - Opcion 2: Web y API separadas
// La Arduino UNO R4 WiFi consulta esta API (polling) porque no es alcanzable desde Internet.
const express = require('express');
const cors = require('cors');

const app = express();
const PORT = process.env.PORT || 3000;
const DEVICE_KEY = process.env.DEVICE_KEY || 'WEB_ORIGIN';        // la usa el Arduino
const WEB_ORIGIN = process.env.WEB_ORIGIN || 'https://lopesolano.github.io/';                          // ej: https://tuusuario.github.io
const DEVICE_TIMEOUT_MS = 5000;                                           // sin sync = dispositivo offline

app.use(cors({ origin: WEB_ORIGIN }));
app.use(express.json());

// ---- Estado en memoria ----
const state = {
  mode: 'manual',          // 'manual' | 'auto'
  manualCommand: 'stop',   // 'left' | 'right' | 'stop'
  distance: null,          // cm
  motor: 'stop',           // estado reportado por el dispositivo
  updatedAt: null
};

// ---- Logica del modo automatico ----
function autoCommand(d) {
  if (d == null || d < 0) return 'stop';
  if (d >= 10 && d <= 25) return 'right';
  if (d > 25 && d <= 40) return 'left';
  return 'stop';
}
const currentCommand = () => (state.mode === 'auto' ? autoCommand(state.distance) : state.manualCommand);
const online = () => state.updatedAt && Date.now() - state.updatedAt < DEVICE_TIMEOUT_MS;

// ---- Autenticacion del dispositivo ----
function deviceAuth(req, res, next) {
  if (req.get('x-api-key') !== DEVICE_KEY) return res.status(401).json({ error: 'API key invalida' });
  next();
}

// ====== Endpoints para la pagina web ======
app.get('/api/status', (req, res) => {
  res.json({
    mode: state.mode,
    command: currentCommand(),
    motor: state.motor,
    distance: state.distance,
    deviceOnline: !!online(),
    updatedAt: state.updatedAt
  });
});

app.get('/api/distance', (req, res) => {
  res.json({ distance: state.distance, unit: 'cm', deviceOnline: !!online(), updatedAt: state.updatedAt });
});

app.get('/api/mode', (req, res) => res.json({ mode: state.mode }));

app.post('/api/mode', (req, res) => {
  const { mode } = req.body || {};
  if (!['auto', 'manual'].includes(mode)) return res.status(400).json({ error: "mode debe ser 'auto' o 'manual'" });
  state.mode = mode;
  state.manualCommand = 'stop';      // al cambiar de modo el motor se detiene
  res.json({ mode: state.mode, command: currentCommand() });
});

app.post('/api/motor', (req, res) => {
  const { action } = req.body || {};
  if (!['left', 'right', 'stop'].includes(action)) {
    return res.status(400).json({ error: "action debe ser 'left', 'right' o 'stop'" });
  }
  if (state.mode !== 'manual') {
    return res.status(409).json({ error: 'El sistema esta en modo automatico. Cambia a manual primero.' });
  }
  state.manualCommand = action;
  res.json({ mode: state.mode, command: state.manualCommand });
});

// ====== Endpoint para el dispositivo (Arduino) ======
// El Arduino envia distancia y estado del motor; recibe el comando a ejecutar.
app.post('/api/device/sync', deviceAuth, (req, res) => {
  const { distance, motor } = req.body || {};
  if (typeof distance === 'number') state.distance = distance;
  if (['left', 'right', 'stop'].includes(motor)) state.motor = motor;
  state.updatedAt = Date.now();
  res.json({ mode: state.mode, command: currentCommand() });
});

app.get('/', (req, res) => res.json({ name: 'Semita API', status: 'ok' }));
app.use((req, res) => res.status(404).json({ error: 'Ruta no encontrada' }));

app.listen(PORT, () => console.log(`API escuchando en puerto ${PORT}`));
