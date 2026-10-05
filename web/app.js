/**
 * KTM 1290 CANSMART // ACCESSORY & LIGHTING MANAGER
 * Application Controller, Simulation Engine & REST API Client
 */

// Definición de las 13 funciones oficiales
const FUNCTIONS = [
  { id: 1, key: 'LEFT_LIGHT_1', name: 'LEFT LIGHT 1', colorClass: '', icon: getIconSvg('left_light') },
  { id: 2, key: 'RIGHT_LIGHT_1', name: 'RIGHT LIGHT 1', colorClass: '', icon: getIconSvg('right_light') },
  { id: 3, key: 'LIGHT_PAIR_1', name: 'LIGHT PAIR 1', colorClass: '', icon: getIconSvg('light_pair') },
  { id: 9, key: 'ACCESSORY', name: 'ACCESSORY', colorClass: 'color-accessory', icon: getIconSvg('accessory') },
  { id: 7, key: 'HORN', name: 'HORN', colorClass: 'color-horn', icon: getIconSvg('horn') },
  
  { id: 4, key: 'LEFT_LIGHT_2', name: 'LEFT LIGHT 2', colorClass: '', icon: getIconSvg('left_light') },
  { id: 5, key: 'RIGHT_LIGHT_2', name: 'RIGHT LIGHT 2', colorClass: '', icon: getIconSvg('right_light') },
  { id: 6, key: 'LIGHT_PAIR_2', name: 'LIGHT PAIR 2', colorClass: '', icon: getIconSvg('light_pair') },
  { id: 11, key: 'LEFT_TURN', name: 'LEFT TURN', colorClass: 'color-turn', icon: getIconSvg('left_turn') },
  { id: 12, key: 'RIGHT_TURN', name: 'RIGHT TURN', colorClass: 'color-turn', icon: getIconSvg('right_turn') },
  
  { id: 8, key: 'BRAKE_LIGHT', name: 'BRAKE LIGHT', colorClass: 'color-brake', icon: getIconSvg('brake') },
  { id: 13, key: 'BRAKE_TURN_L', name: 'BRAKE + TURN L', colorClass: 'color-brake', icon: getIconSvg('brake_turn_l') },
  { id: 14, key: 'BRAKE_TURN_R', name: 'BRAKE + TURN R', colorClass: 'color-brake', icon: getIconSvg('brake_turn_r') },
  { id: 10, key: 'HEATED_GEAR', name: 'HEATED GEAR', colorClass: 'color-heated', icon: getIconSvg('heated') },
  { id: 0, key: 'DISABLED', name: 'DESACTIVADO', colorClass: '', icon: getIconSvg('disabled') }
];

// Estado global de la aplicación
const state = {
  selectedChannel: 3, // Canal actualmente seleccionado en la pestaña de matriz (0: Blanco, 1: Amarillo, 2: Azul, 3: Rojo)
  channels: [
    { name: 'Blanco', functionKey: 'LEFT_LIGHT_1', fuseAmps: 10, duty: 40, amps: 1.2 },
    { name: 'Amarillo', functionKey: 'RIGHT_LIGHT_1', fuseAmps: 10, duty: 40, amps: 1.2 },
    { name: 'Azul', functionKey: 'LIGHT_PAIR_2', fuseAmps: 10, duty: 0, amps: 0.0 },
    { name: 'Rojo', functionKey: 'HORN', fuseAmps: 10, duty: 0, amps: 0.0 }
  ],
  settings: {
    set1Enabled: true,
    set1Day: 40,
    set1Night: 20,
    set1High: 100,
    set2Enabled: false, // Nieblas apagadas al inicio
    set2Day: 50,
    set2Night: 30,
    turnCutoff: true,
    strobeHorn: true,
    strobePass: true,
    inverseHazard: true,
    offDelay: 15
  },
  bike: {
    ignition: true,
    engineRunning: true,
    rpm: 1450,
    speed: 0.0,
    batteryVoltage: 13.8,
    isNightMode: false,
    highBeam: false,
    passTrigger: false,
    turnLeft: false,
    turnRight: false,
    horn: false,
    brake: false
  },
  dimmer: {
    active: false,
    target: null, // 'SET1' o 'SET2'
    timer: null,
    countdownSec: 5
  },
  cancelClickCount: 0,
  lastCancelClickTime: 0,
  cancelHoldTimer: null
};

// Generador de Iconos SVG vectoriales limpios
function getIconSvg(type) {
  switch (type) {
    case 'left_light':
      return `<svg viewBox="0 0 24 24"><path d="M12 4v16M8 8H5a2 2 0 0 0-2 2v4a2 2 0 0 0 2 2h3l4 4V4L8 8z"/><line x1="16" y1="8" x2="21" y2="8"/><line x1="16" y1="12" x2="22" y2="12"/><line x1="16" y1="16" x2="21" y2="16"/></svg>`;
    case 'right_light':
      return `<svg viewBox="0 0 24 24"><path d="M12 4v16M16 8h3a2 2 0 0 1 2 2v4a2 2 0 0 1-2 2h-3l-4 4V4l4 4z"/><line x1="8" y1="8" x2="3" y2="8"/><line x1="8" y1="12" x2="2" y2="12"/><line x1="8" y1="16" x2="3" y2="16"/></svg>`;
    case 'light_pair':
      return `<svg viewBox="0 0 24 24"><path d="M2 12h4m12 0h4M7 8l-3 4 3 4V8zm10 0l3 4-3 4V8z"/><ellipse cx="12" cy="12" rx="3" ry="5"/></svg>`;
    case 'accessory':
      return `<svg viewBox="0 0 24 24"><rect x="5" y="2" width="14" height="20" rx="2" ry="2"/><line x1="12" y1="18" x2="12.01" y2="18"/><path d="M12 6v6"/></svg>`;
    case 'horn':
      return `<svg viewBox="0 0 24 24"><polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/><path d="M15.54 8.46a5 5 0 0 1 0 7.07"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14"/></svg>`;
    case 'brake':
      return `<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="8"/><path d="M12 8v4"/><path d="M12 16h.01"/><path d="M4 12a8 8 0 0 1 .5-2.8"/><path d="M20 12a8 8 0 0 0-.5-2.8"/></svg>`;
    case 'left_turn':
      return `<svg viewBox="0 0 24 24"><polygon points="11 19 2 12 11 5 11 9 22 9 22 15 11 15 11 19"/></svg>`;
    case 'right_turn':
      return `<svg viewBox="0 0 24 24"><polygon points="13 5 22 12 13 19 13 15 2 15 2 9 13 9 13 5"/></svg>`;
    case 'brake_turn_l':
      return `<svg viewBox="0 0 24 24"><circle cx="14" cy="12" r="7"/><path d="M7 12l-4-3v6l4-3z"/></svg>`;
    case 'brake_turn_r':
      return `<svg viewBox="0 0 24 24"><circle cx="10" cy="12" r="7"/><path d="M17 12l4-3v6l-4-3z"/></svg>`;
    case 'heated':
      return `<svg viewBox="0 0 24 24"><path d="M6 3v6a6 6 0 0 0 12 0V3"/><line x1="12" y1="15" x2="12" y2="21"/><path d="M9 18l3 3 3-3"/></svg>`;
    default:
      return `<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><line x1="4.93" y1="4.93" x2="19.07" y2="19.07"/></svg>`;
  }
}

// Inicialización general
document.addEventListener('DOMContentLoaded', () => {
  setupTabs();
  buildFunctionMatrix();
  updateChannelCardsUI();
  setupSettingsListeners();
  setupHandlebarSimulator();
  startSimulationTicker();
  logCanFrame('0x500', 8, '00 00 00 00 00 00 00 00', 'Sensor Iluminación TFT: Modo DÍA');
  logCanFrame('0x120', 8, '16 A8 00 00 00 00 00 00', 'Motor KTM LC8 1290: 1,450 RPM');
});

// Navegación de pestañas
function setupTabs() {
  const tabs = document.querySelectorAll('.tab-btn');
  tabs.forEach(btn => {
    btn.addEventListener('click', () => {
      tabs.forEach(b => {
        b.classList.remove('active');
        b.setAttribute('aria-selected', 'false');
      });
      document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));

      btn.classList.add('active');
      btn.setAttribute('aria-selected', 'true');
      const targetId = btn.getAttribute('data-tab');
      const targetPanel = document.getElementById(targetId);
      if (targetPanel) targetPanel.classList.add('active');
    });
  });
}

// Construcción de la matriz de funciones
function buildFunctionMatrix() {
  const grid = document.getElementById('functionsGrid');
  grid.innerHTML = '';

  FUNCTIONS.forEach(fn => {
    const tile = document.createElement('div');
    tile.className = `fn-tile ${fn.colorClass}`;
    tile.id = `fnTile_${fn.key}`;
    tile.innerHTML = `
      ${fn.icon}
      <span class="fn-label">${fn.name}</span>
    `;

    tile.addEventListener('click', () => {
      assignFunctionToChannel(state.selectedChannel, fn.key);
    });

    grid.appendChild(tile);
  });
}

// Asignar función al canal seleccionado
function assignFunctionToChannel(channelIdx, functionKey) {
  state.channels[channelIdx].functionKey = functionKey;
  updateChannelCardsUI();
  updateLightsSimulation();
  
  const chNames = ['Blanco', 'Amarillo', 'Azul', 'Rojo'];
  logCanFrame('0x000', 4, `0${channelIdx} ${functionKey.slice(0,6)}`, `Config: Circuito ${chNames[channelIdx]} -> ${functionKey}`);
}

// Actualizar tarjetas de los 4 canales
function updateChannelCardsUI() {
  const chCards = document.querySelectorAll('.circuit-card');
  chCards.forEach((card, idx) => {
    card.classList.toggle('active-card', idx === state.selectedChannel);
    card.onclick = () => {
      state.selectedChannel = idx;
      updateChannelCardsUI();
    };

    const ch = state.channels[idx];
    const fnDef = FUNCTIONS.find(f => f.key === ch.functionKey) || FUNCTIONS[0];

    const previewIcon = document.getElementById(`previewIcon${idx}`);
    const previewName = document.getElementById(`previewName${idx}`);
    const meterFill = document.getElementById(`meterFill${idx}`);
    const meterDuty = document.getElementById(`meterDuty${idx}`);
    const meterAmps = document.getElementById(`meterAmps${idx}`);

    if (previewIcon) previewIcon.innerHTML = fnDef.icon;
    if (previewName) previewName.textContent = fnDef.name;
    if (meterFill) meterFill.style.width = `${ch.duty}%`;
    if (meterDuty) meterDuty.textContent = `PWM: ${ch.duty}%`;
    if (meterAmps) meterAmps.textContent = `${ch.amps.toFixed(1)} A`;
  });

  const selectedChObj = state.channels[state.selectedChannel];
  const lbl = document.getElementById('selectedChannelLabel');
  if (lbl) lbl.textContent = `Circuito ${selectedChObj.name}`;

  // Resaltar en la matriz de abajo la función activa del canal actual
  document.querySelectorAll('.fn-tile').forEach(tile => {
    tile.classList.toggle('selected', tile.id === `fnTile_${selectedChObj.functionKey}`);
  });
}

// Listeners de los deslizadores y toggles de la pestaña 2
function setupSettingsListeners() {
  // SET 1 Toggle & Sliders
  const set1Toggle = document.getElementById('set1EnabledToggle');
  set1Toggle.addEventListener('change', (e) => {
    state.settings.set1Enabled = e.target.checked;
    updateLightsSimulation();
  });

  const sliderSet1Day = document.getElementById('sliderSet1Day');
  sliderSet1Day.addEventListener('input', (e) => {
    state.settings.set1Day = parseInt(e.target.value);
    document.getElementById('valSet1Day').textContent = `${state.settings.set1Day}%`;
    updateLightsSimulation();
  });

  const sliderSet1Night = document.getElementById('sliderSet1Night');
  sliderSet1Night.addEventListener('input', (e) => {
    state.settings.set1Night = parseInt(e.target.value);
    document.getElementById('valSet1Night').textContent = `${state.settings.set1Night}%`;
    updateLightsSimulation();
  });

  const sliderSet1High = document.getElementById('sliderSet1High');
  sliderSet1High.addEventListener('input', (e) => {
    state.settings.set1High = parseInt(e.target.value);
    document.getElementById('valSet1High').textContent = `${state.settings.set1High}%`;
    updateLightsSimulation();
  });

  // SET 2 (Nieblas) Toggle & Sliders
  const set2Toggle = document.getElementById('set2EnabledToggle');
  set2Toggle.addEventListener('change', (e) => {
    state.settings.set2Enabled = e.target.checked;
    updateLightsSimulation();
  });

  const sliderSet2Day = document.getElementById('sliderSet2Day');
  sliderSet2Day.addEventListener('input', (e) => {
    state.settings.set2Day = parseInt(e.target.value);
    document.getElementById('valSet2Day').textContent = `${state.settings.set2Day}%`;
    updateLightsSimulation();
  });

  const sliderSet2Night = document.getElementById('sliderSet2Night');
  sliderSet2Night.addEventListener('input', (e) => {
    state.settings.set2Night = parseInt(e.target.value);
    document.getElementById('valSet2Night').textContent = `${state.settings.set2Night}%`;
    updateLightsSimulation();
  });

  // Botón Guardar
  document.getElementById('btnSaveConfig').addEventListener('click', () => {
    const btn = document.getElementById('btnSaveConfig');
    const oldText = btn.innerHTML;
    btn.innerHTML = '✔ ¡Guardado con Éxito!';
    btn.style.background = 'var(--accent-green)';
    setTimeout(() => {
      btn.innerHTML = oldText;
      btn.style.background = 'var(--ktm-orange)';
    }, 1600);
  });

  // Botón Reset
  document.getElementById('btnResetDefaults').addEventListener('click', () => {
    if (confirm('¿Restaurar la configuración predeterminada de fábrica para KTM 1290?')) {
      state.settings.set1Day = 40;
      state.settings.set1Night = 20;
      state.settings.set1High = 100;
      state.settings.set2Day = 50;
      state.settings.set2Night = 30;
      sliderSet1Day.value = 40;
      sliderSet1Night.value = 20;
      sliderSet1High.value = 100;
      sliderSet2Day.value = 50;
      sliderSet2Night.value = 30;
      document.getElementById('valSet1Day').textContent = '40%';
      document.getElementById('valSet1Night').textContent = '20%';
      document.getElementById('valSet1High').textContent = '100%';
      document.getElementById('valSet2Day').textContent = '50%';
      document.getElementById('valSet2Night').textContent = '30%';
      updateLightsSimulation();
    }
  });

  // Limpiar consola
  document.getElementById('btnClearLog').addEventListener('click', () => {
    document.getElementById('canLogBody').innerHTML = '';
  });
}

// Simulador interactivo de la Piña KTM 1290
function setupHandlebarSimulator() {
  const btnTurnCancel = document.getElementById('btnTurnCancel');
  const btnPassHoldUp = document.getElementById('btnPassHoldUp');
  const btnPassHoldDown = document.getElementById('btnPassHoldDown');
  const btnPassFlash = document.getElementById('btnPassFlash');
  const btnDpadUp = document.getElementById('btnDpadUp');
  const btnDpadDown = document.getElementById('btnDpadDown');
  const btnTurnLeft = document.getElementById('btnTurnLeft');
  const btnTurnRight = document.getElementById('btnTurnRight');
  const btnHorn = document.getElementById('btnHorn');
  const btnToggleDayNight = document.getElementById('btnToggleDayNight');

  // Cancelar Intermitente: Detección de TRIPLE CLIC y HOLD 3s
  btnTurnCancel.addEventListener('mousedown', () => {
    const now = Date.now();
    state.cancelHoldTimer = setTimeout(() => {
      // Sostenido 3 segundos -> Conmutar Light Set 1
      state.settings.set1Enabled = !state.settings.set1Enabled;
      document.getElementById('set1EnabledToggle').checked = state.settings.set1Enabled;
      flashLightsFeedback('SET1');
      logCanFrame('0x240', 8, '00 04 00 00 00 00 00 00', `Mandos KTM: Pulsación 3s Cancel -> Set 1: ${state.settings.set1Enabled ? 'ENCENDIDO' : 'APAGADO'}`);
      updateLightsSimulation();
    }, 2800);
  });

  btnTurnCancel.addEventListener('mouseup', () => {
    clearTimeout(state.cancelHoldTimer);
    const now = Date.now();
    if (now - state.lastCancelClickTime < 600) {
      state.cancelClickCount++;
    } else {
      state.cancelClickCount = 1;
    }
    state.lastCancelClickTime = now;

    if (state.cancelClickCount === 3) {
      // Triple Clic -> Conmutar Nieblas (Light Set 2)
      state.settings.set2Enabled = !state.settings.set2Enabled;
      document.getElementById('set2EnabledToggle').checked = state.settings.set2Enabled;
      flashLightsFeedback('SET2');
      logCanFrame('0x240', 8, '00 04 00 00 00 00 00 00', `Mandos KTM: Triple Clic Cancel -> Nieblas (Set 2): ${state.settings.set2Enabled ? 'ENCENDIDAS' : 'APAGADAS'}`);
      state.cancelClickCount = 0;
      updateLightsSimulation();
    }
  });

  // Gatillo Ráfagas (Pass) Hold Up (3s) -> Dimmer SET 1
  btnPassHoldUp.addEventListener('click', () => {
    enterDimmerMode('SET1');
    logCanFrame('0x240', 8, '03 00 00 00 00 00 00 00', 'Mandos KTM: Gatillo Ráfagas Arriba 3s -> Modo DIMMER Set 1');
  });

  // Gatillo Ráfagas (Pass) Hold Down (3s) -> Dimmer SET 2
  btnPassHoldDown.addEventListener('click', () => {
    enterDimmerMode('SET2');
    logCanFrame('0x240', 8, '02 00 00 00 00 00 00 00', 'Mandos KTM: Gatillo Ráfagas Abajo 3s -> Modo DIMMER Nieblas (Set 2)');
  });

  // Ráfaga rápida
  btnPassFlash.addEventListener('mousedown', () => {
    state.bike.passTrigger = true;
    updateLightsSimulation();
    logCanFrame('0x240', 8, '02 00 00 00 00 00 00 00', 'Mandos KTM: Ráfaga de luces largas (Flash)');
  });
  btnPassFlash.addEventListener('mouseup', () => {
    state.bike.passTrigger = false;
    updateLightsSimulation();
  });

  // Cruceta D-Pad '+' (Subir Brillo)
  btnDpadUp.addEventListener('click', () => {
    if (state.dimmer.active) {
      resetDimmerTimer();
      if (state.dimmer.target === 'SET1') {
        state.settings.set1Day = Math.min(state.settings.set1Day + 10, 100);
        document.getElementById('sliderSet1Day').value = state.settings.set1Day;
        document.getElementById('valSet1Day').textContent = `${state.settings.set1Day}%`;
      } else {
        state.settings.set2Day = Math.min(state.settings.set2Day + 10, 100);
        document.getElementById('sliderSet2Day').value = state.settings.set2Day;
        document.getElementById('valSet2Day').textContent = `${state.settings.set2Day}%`;
      }
      updateLightsSimulation();
      logCanFrame('0x420', 8, '01 00 00 00 00 00 00 00', `Cruceta KTM (+): Brillo ajustado a ${state.dimmer.target === 'SET1' ? state.settings.set1Day : state.settings.set2Day}%`);
    }
  });

  // Cruceta D-Pad '-' (Bajar Brillo)
  btnDpadDown.addEventListener('click', () => {
    if (state.dimmer.active) {
      resetDimmerTimer();
      if (state.dimmer.target === 'SET1') {
        state.settings.set1Day = Math.max(state.settings.set1Day - 10, 10);
        document.getElementById('sliderSet1Day').value = state.settings.set1Day;
        document.getElementById('valSet1Day').textContent = `${state.settings.set1Day}%`;
      } else {
        state.settings.set2Day = Math.max(state.settings.set2Day - 10, 10);
        document.getElementById('sliderSet2Day').value = state.settings.set2Day;
        document.getElementById('valSet2Day').textContent = `${state.settings.set2Day}%`;
      }
      updateLightsSimulation();
      logCanFrame('0x420', 8, '02 00 00 00 00 00 00 00', `Cruceta KTM (-): Brillo bajado a ${state.dimmer.target === 'SET1' ? state.settings.set1Day : state.settings.set2Day}%`);
    }
  });

  // Intermitente Izquierdo
  btnTurnLeft.addEventListener('click', () => {
    state.bike.turnLeft = !state.bike.turnLeft;
    state.bike.turnRight = false;
    btnTurnLeft.style.borderColor = state.bike.turnLeft ? 'var(--accent-yellow)' : '';
    btnTurnRight.style.borderColor = '';
    updateLightsSimulation();
    logCanFrame('0x240', 8, state.bike.turnLeft ? '00 01 00 00 00 00 00 00' : '00 00 00 00 00 00 00 00', `Intermitente Izquierdo: ${state.bike.turnLeft ? 'ACTIVO' : 'OFF'}`);
  });

  // Intermitente Derecho
  btnTurnRight.addEventListener('click', () => {
    state.bike.turnRight = !state.bike.turnRight;
    state.bike.turnLeft = false;
    btnTurnRight.style.borderColor = state.bike.turnRight ? 'var(--accent-yellow)' : '';
    btnTurnLeft.style.borderColor = '';
    updateLightsSimulation();
    logCanFrame('0x240', 8, state.bike.turnRight ? '00 02 00 00 00 00 00 00' : '00 00 00 00 00 00 00 00', `Intermitente Derecho: ${state.bike.turnRight ? 'ACTIVO' : 'OFF'}`);
  });

  // Bocina / Claxon (Estroboscópico de alerta)
  let strobeInterval = null;
  btnHorn.addEventListener('mousedown', () => {
    state.bike.horn = true;
    logCanFrame('0x240', 8, '04 00 00 00 00 00 00 00', '¡BOCINA PULSADA! Activando Estroboscópico de Emergencia a 12Hz');
    strobeInterval = setInterval(() => {
      toggleStrobePhase();
    }, 70);
  });

  btnHorn.addEventListener('mouseup', () => {
    state.bike.horn = false;
    clearInterval(strobeInterval);
    updateLightsSimulation();
  });

  // Alternar Día / Noche (Sensor TFT)
  btnToggleDayNight.addEventListener('click', () => {
    state.bike.isNightMode = !state.bike.isNightMode;
    document.getElementById('lblTftMode').textContent = state.bike.isNightMode ? 'NOCHE' : 'DÍA';
    document.getElementById('lblTftMode').style.color = state.bike.isNightMode ? 'var(--accent-cyan)' : 'var(--accent-yellow)';
    updateLightsSimulation();
    logCanFrame('0x500', 8, state.bike.isNightMode ? '01 00 00 00 00 00 00 00' : '00 00 00 00 00 00 00 00', `Sensor Luz Ambiental TFT: Modo ${state.bike.isNightMode ? 'NOCHE' : 'DÍA'}`);
  });
}

// Activar Modo Dimmer
function enterDimmerMode(targetSet) {
  state.dimmer.active = true;
  state.dimmer.target = targetSet;
  flashLightsFeedback(targetSet);

  const banner = document.getElementById('dimmerAlertBanner');
  banner.classList.add('active');
  document.getElementById('dimmerBannerText').textContent = 
    `Ajustando ${targetSet === 'SET1' ? 'Luces Principales (Set 1)' : 'Luces de Niebla (Set 2)'} con teclas + / - ...`;

  resetDimmerTimer();
}

function resetDimmerTimer() {
  clearTimeout(state.dimmer.timer);
  state.dimmer.countdownSec = 5;
  const fill = document.getElementById('dimmerCountdown');
  fill.style.width = '100%';

  const stepInterval = setInterval(() => {
    state.dimmer.countdownSec -= 0.1;
    const pct = (state.dimmer.countdownSec / 5) * 100;
    if (fill) fill.style.width = `${pct}%`;
    if (state.dimmer.countdownSec <= 0) {
      clearInterval(stepInterval);
    }
  }, 100);

  state.dimmer.timer = setTimeout(() => {
    state.dimmer.active = false;
    state.dimmer.target = null;
    document.getElementById('dimmerAlertBanner').classList.remove('active');
    logCanFrame('0x000', 0, '', 'Modo DIMMER finalizado: Nuevos ajustes guardados en memoria NVS.');
  }, 5000);
}

// Destello leve de confirmación visual
function flashLightsFeedback(target) {
  const glowL = document.getElementById('glowLightLeft');
  const glowR = document.getElementById('glowLightRight');
  const glowFogL = document.getElementById('glowFogLeft');
  const glowFogR = document.getElementById('glowFogRight');

  if (target === 'SET1') {
    glowL.style.opacity = '1';
    glowR.style.opacity = '1';
    setTimeout(() => updateLightsSimulation(), 120);
  } else {
    glowFogL.style.opacity = '1';
    glowFogR.style.opacity = '1';
    setTimeout(() => updateLightsSimulation(), 120);
  }
}

let strobeToggle = false;
function toggleStrobePhase() {
  strobeToggle = !strobeToggle;
  const glowL = document.getElementById('glowLightLeft');
  const glowR = document.getElementById('glowLightRight');
  glowL.style.opacity = strobeToggle ? '1' : '0';
  glowR.style.opacity = strobeToggle ? '0' : '1';
}

// Cálculo del brillo en tiempo real
function updateLightsSimulation() {
  if (state.bike.horn) return; // Si la bocina está sonando, manda el estroboscópico

  // Set 1 (Luces Principales)
  let set1Duty = 0;
  if (state.settings.set1Enabled && state.bike.ignition) {
    if (state.bike.highBeam || state.bike.passTrigger) {
      set1Duty = state.settings.set1High;
    } else {
      set1Duty = state.bike.isNightMode ? state.settings.set1Night : state.settings.set1Day;
    }
  }

  // Set 2 (Nieblas)
  let set2Duty = 0;
  if (state.settings.set2Enabled && state.bike.ignition) {
    set2Duty = state.bike.isNightMode ? state.settings.set2Night : state.settings.set2Day;
  }

  // Corte por intermitente activo (Turn signal cutoff)
  let leftCutoff = state.settings.turnCutoff && state.bike.turnLeft;
  let rightCutoff = state.settings.turnCutoff && state.bike.turnRight;

  let finalSet1L = leftCutoff ? 0 : set1Duty;
  let finalSet1R = rightCutoff ? 0 : set1Duty;

  // Actualizar UI del escenario visual
  applyLightUi('glowLightLeft', 'pctLightLeft', finalSet1L, state.settings.set1Enabled);
  applyLightUi('glowLightRight', 'pctLightRight', finalSet1R, state.settings.set1Enabled);
  applyLightUi('glowFogLeft', 'pctFogLeft', set2Duty, state.settings.set2Enabled);
  applyLightUi('glowFogRight', 'pctFogRight', set2Duty, state.settings.set2Enabled);

  // Faro central KTM
  const textHeadlight = document.getElementById('textMainHeadlight');
  const glowHeadlight = document.getElementById('glowMainHeadlight');
  if (state.bike.highBeam || state.bike.passTrigger) {
    textHeadlight.textContent = 'Luz Larga (100%)';
    glowHeadlight.style.opacity = '1';
    glowHeadlight.style.transform = 'translate(-50%, -50%) scale(1.3)';
  } else {
    textHeadlight.textContent = state.bike.isNightMode ? 'Cruce (Noche)' : 'Cruce (Día)';
    glowHeadlight.style.opacity = '0.5';
    glowHeadlight.style.transform = 'translate(-50%, -50%) scale(1)';
  }

  // Actualizar indicadores de canal en Pestaña 1
  state.channels[0].duty = finalSet1L;
  state.channels[1].duty = finalSet1R;
  state.channels[2].duty = set2Duty;
  state.channels[0].amps = (finalSet1L / 100) * 2.8;
  state.channels[1].amps = (finalSet1R / 100) * 2.8;
  state.channels[2].amps = (set2Duty / 100) * 3.5;

  const totalAmps = state.channels[0].amps + state.channels[1].amps + state.channels[2].amps;
  const monAmps = document.getElementById('monTotalAmps');
  if (monAmps) monAmps.textContent = `${totalAmps.toFixed(1)} A / 25A`;

  updateChannelCardsUI();
}

function applyLightUi(glowId, pctId, duty, isEnabled) {
  const glow = document.getElementById(glowId);
  const text = document.getElementById(pctId);
  if (!glow || !text) return;

  if (!isEnabled || duty === 0) {
    glow.style.opacity = '0';
    text.textContent = 'OFF';
    text.style.color = 'var(--text-dim)';
  } else {
    glow.style.opacity = `${duty / 100}`;
    glow.style.transform = `translate(-50%, -50%) scale(${0.7 + (duty / 100) * 0.5})`;
    text.textContent = `${duty}%`;
    text.style.color = '#fff';
  }
}

// Registro en el Monitor CAN
function logCanFrame(canId, dlc, bytes, desc) {
  const logBody = document.getElementById('canLogBody');
  if (!logBody) return;

  const now = new Date();
  const timeStr = `${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}:${String(now.getSeconds()).padStart(2,'0')}.${String(now.getMilliseconds()).padStart(3,'0')}`;

  const row = document.createElement('div');
  row.className = 'log-entry';
  row.innerHTML = `
    <span class="log-time">${timeStr}</span>
    <span class="log-id">${canId}</span>
    <span class="log-dlc">d:${dlc}</span>
    <span class="log-bytes">${bytes}</span>
    <span class="log-desc">// ${desc}</span>
  `;

  logBody.prepend(row);
  if (logBody.children.length > 50) {
    logBody.removeChild(logBody.lastChild);
  }
}

// Bucle de telemetría de fondo (simula heartbeat de la KTM 1290)
function startSimulationTicker() {
  setInterval(() => {
    // Variación leve de RPM y voltaje
    state.bike.rpm = 1420 + Math.floor(Math.random() * 45);
    const rpmEl = document.getElementById('monRpm');
    if (rpmEl) rpmEl.textContent = `${state.bike.rpm.toLocaleString()} RPM`;

    // Pulso CAN aleatorio simulado
    const canPulse = document.getElementById('canPulse');
    if (canPulse) {
      canPulse.style.opacity = canPulse.style.opacity === '0.4' ? '1' : '0.4';
    }
  }, 1000);
}
