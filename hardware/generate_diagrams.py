"""
Generador de los diagramas de cableado KTM 1290 CANsmart.

Produce 4 hojas en hardware/diagramas/ (cada una en .svg y .png, mismo dibujo):
  01_alimentacion, 02_bus_can, 03_senales_pwm, 04_salidas_conectores

Reglas de diseño (para que se entienda dónde empieza y termina cada cable):
  * Cada cable tiene un ID (W1..W26) y una etiqueta sobre el propio cable.
  * Ningún cable se solapa ni se cruza con otro: cada uno tiene su propio carril.
  * Cada extremo termina en un terminal (círculo) dibujado sobre el borne real.
  * Cada hoja incluye su lista de cables (de -> a, calibre, color).

Uso:  python hardware/generate_diagrams.py
"""
from pathlib import Path
from xml.sax.saxutils import escape

HERE = Path(__file__).resolve().parent
OUT = HERE / "diagramas"
FONT = "'Segoe UI', Roboto, Helvetica, Arial, sans-serif"

# Colores
BG = "#0e1117"
PANEL = "#161b26"
TXT = "#e5e7eb"
MUTED = "#9ca3af"
ORANGE = "#ff6a13"
RED = "#ef4444"
BLUE = "#3b82f6"
CYAN = "#22d3ee"
GREEN = "#10b981"
AMBER = "#f59e0b"
YELLOW = "#facc15"
GREY = "#6b7280"
CAN_H = "#f97316"
CAN_L = "#eab308"


class Sheet:
    def __init__(self, slug, num, title, subtitle, w, h):
        self.slug, self.num, self.title, self.subtitle = slug, num, title, subtitle
        self.w, self.h = w, h
        self.p, self.wires, self.terms, self.tags = [], [], [], []

    # ---- primitivas
    def raw(self, s):
        self.p.append(s)

    def text(self, x, y, s, size=14, fill=TXT, weight="normal", anchor="start"):
        self.raw(
            f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" '
            f'font-weight="{weight}" text-anchor="{anchor}">{escape(s)}</text>'
        )

    def box(self, x, y, w, h, title, color, hh=34):
        self.raw(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" fill="{PANEL}" stroke="{color}" stroke-width="2.5"/>')
        self.raw(f'<rect x="{x}" y="{y}" width="{w}" height="{hh}" rx="10" fill="{color}"/>')
        self.raw(f'<rect x="{x}" y="{y + hh - 10}" width="{w}" height="10" fill="{color}"/>')
        self.text(x + 14, y + hh / 2 + 6, title, 16, "#ffffff", "bold")

    def cell(self, x, y, w, h, color, dashed=False):
        dash = ' stroke-dasharray="6 4"' if dashed else ""
        self.raw(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" fill="#0c1018" stroke="{color}" stroke-width="2"{dash}/>')

    def badge(self, cx, cy, label, color, r=13):
        self.raw(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{color}"/>')
        self.text(cx, cy + 5, label, 14, "#ffffff", "bold", "middle")

    def wire(self, pts, color, width=5, tag=None, tagpos=None):
        d = "M " + " L ".join(f"{x} {y}" for x, y in pts)
        self.wires.append(
            f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width}" '
            f'stroke-linecap="round" stroke-linejoin="round"/>'
        )
        if tag:
            self.tags.append((tagpos, tag, color))

    def term(self, x, y, color):
        self.terms.append(
            f'<circle cx="{x}" cy="{y}" r="8" fill="#0b0f16" stroke="{color}" stroke-width="3.5"/>'
            f'<circle cx="{x}" cy="{y}" r="2.5" fill="{color}"/>'
        )

    def junction(self, x, y, color):
        self.terms.append(f'<circle cx="{x}" cy="{y}" r="7" fill="{color}" stroke="#0b0f16" stroke-width="2.5"/>')

    def table(self, x, y, cols, rows, title, rh=36):
        self.text(x, y - 14, title, 17, YELLOW, "bold")
        total = sum(c[1] for c in cols)
        self.raw(f'<rect x="{x}" y="{y}" width="{total}" height="{rh}" fill="#222a3a"/>')
        cx = x
        for name, wd in cols:
            self.text(cx + 16, y + rh / 2 + 5, name, 13, MUTED, "bold")
            cx += wd
        for i, (wid, desc, cable, color) in enumerate(rows):
            yy = y + rh * (i + 1)
            self.raw(f'<rect x="{x}" y="{yy}" width="{total}" height="{rh}" fill="{"#121722" if i % 2 == 0 else "#171d2b"}"/>')
            self.raw(f'<rect x="{x}" y="{yy}" width="6" height="{rh}" fill="{color}"/>')
            self.text(x + 18, yy + rh / 2 + 5, wid, 15, color, "bold")
            self.text(x + cols[0][1] + 16, yy + rh / 2 + 5, desc, 14, TXT)
            self.text(x + cols[0][1] + cols[1][1] + 16, yy + rh / 2 + 5, cable, 14, MUTED)

    def legend(self, x, y, items):
        cx = x
        for color, label in items:
            self.raw(f'<rect x="{cx}" y="{y - 5}" width="34" height="8" rx="4" fill="{color}"/>')
            self.text(cx + 44, y + 5, label, 13, MUTED)
            cx += 44 + len(label) * 8 + 34

    def note(self, x, y, w, lines, color=AMBER, title=None):
        h = 24 + 24 * (len(lines) + (1 if title else 0))
        self.raw(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="#1a1710" stroke="{color}" stroke-width="1.8"/>')
        yy = y + 28
        if title:
            self.text(x + 16, yy, title, 15, color, "bold")
            yy += 24
        for ln in lines:
            self.text(x + 16, yy, ln, 14, "#e5e7eb")
            yy += 24

    # ---- salida
    def svg(self):
        out = [
            f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {self.w} {self.h}" '
            f'width="{self.w}" height="{self.h}" font-family="{FONT}">',
            f'<rect width="{self.w}" height="{self.h}" fill="{BG}"/>',
            f'<rect width="{self.w}" height="70" fill="#121722"/>',
            f'<rect width="8" height="70" fill="{ORANGE}"/>',
            f'<text x="30" y="32" font-size="24" font-weight="bold" fill="{ORANGE}">KTM 1290 CANsmart · HOJA {self.num}/4 — {escape(self.title)}</text>',
            f'<text x="30" y="56" font-size="14" fill="{MUTED}">{escape(self.subtitle)}</text>',
        ]
        out += self.p
        out += self.wires
        out += self.terms
        for pos, label, color in self.tags:
            x, y = pos
            w = len(label) * 9 + 20
            out.append(f'<rect x="{x - w / 2}" y="{y - 12}" width="{w}" height="24" rx="6" fill="#0b0f16" stroke="{color}" stroke-width="2"/>')
            out.append(f'<text x="{x}" y="{y + 5}" font-size="14" font-weight="bold" fill="{color}" text-anchor="middle">{label}</text>')
        out.append("</svg>")
        return "\n".join(out)


COLS = [("ID", 70), ("DE  →  A", 500), ("CABLE", 270)]


# =====================================================================
# HOJA 1 - ALIMENTACIÓN
# =====================================================================
def sheet1():
    PURPLE = "#8b5cf6"
    s = Sheet("01_alimentacion", 1, "ALIMENTACIÓN, RELÉ DE CONTACTO Y MASAS",
              "Batería → fusible 15 A → relé (solo con contacto) → placa MOSFET y convertidor 5 V → LILYGO", 1800, 1500)

    # Batería
    s.box(60, 150, 280, 230, "BATERÍA MOTO 12 V", RED)
    s.text(322, 275, "(+) 12 V", 14, "#fca5a5", "bold", "end")
    s.text(78, 195, "(−) GND  → W4", 14, "#93c5fd", "bold")
    s.text(78, 255, "(−) GND  → W5", 14, "#93c5fd", "bold")
    s.text(200, 335, "W4 y W5 van al MISMO borne (−)", 12, MUTED, "normal", "middle")
    s.text(200, 355, "(anilla M6 doble)", 12, MUTED, "normal", "middle")

    # Portafusible principal 15 A
    s.box(390, 230, 160, 80, "PORTAFUSIBLE", GREEN, hh=28)
    s.text(402, 280, "IN", 13, "#6ee7b7", "bold")
    s.text(538, 280, "OUT", 13, "#6ee7b7", "bold", "end")
    s.text(470, 298, "fusible 15 A (al FINAL)", 12, MUTED, "normal", "middle")

    # Relé de contacto
    s.box(660, 150, 220, 230, "RELÉ 12 V · 30 A", PURPLE)
    s.text(674, 275, "30", 14, "#ddd6fe", "bold")
    s.text(674, 205, "86 (+ bobina)", 13, "#ddd6fe", "bold")
    s.text(868, 275, "87", 14, "#ddd6fe", "bold", "end")
    s.text(770, 362, "85 (− bobina)", 13, "#ddd6fe", "bold", "middle")
    s.text(770, 318, "Diodo D1 1N4007 en la bobina", 12, MUTED, "normal", "middle")
    s.text(770, 336, "(cátodo a 86, ánodo a 85)", 12, MUTED, "normal", "middle")

    # Fusible convertidor 3 A
    s.box(990, 230, 140, 80, "FUSIBLE 3 A", GREEN, hh=28)
    s.text(1002, 280, "IN", 13, "#6ee7b7", "bold")
    s.text(1118, 280, "OUT", 13, "#6ee7b7", "bold", "end")
    s.text(1060, 298, "portafusible en línea", 12, MUTED, "normal", "middle")

    # Convertidor
    s.box(1200, 150, 250, 230, "CONVERTIDOR 12 V → 5 V", "#2563eb")
    s.text(1325, 210, "IN (−)", 13, "#93c5fd", "bold", "middle")
    s.text(1214, 275, "IN (+)", 13, "#fca5a5", "bold")
    s.text(1435, 235, "OUT (+) 5 V", 13, "#67e8f9", "bold", "end")
    s.text(1435, 325, "OUT (−) GND", 13, "#cbd5e1", "bold", "end")
    s.text(1325, 365, "3 A · sellado IP67", 12, MUTED, "normal", "middle")

    # LILYGO
    s.box(1520, 150, 250, 230, "LILYGO T-CAN485", GREEN)
    s.text(1536, 235, "VIN (+5 V)", 13, "#67e8f9", "bold")
    s.text(1536, 325, "GND", 13, "#cbd5e1", "bold")
    s.text(1645, 365, "CAN → Hoja 2 · GPIO → Hoja 3", 12, MUTED, "normal", "middle")

    # Driver de contacto
    s.box(100, 470, 240, 220, "DRIVER DE CONTACTO", AMBER)
    s.text(220, 530, "C → relé 85", 13, "#fde68a", "bold", "middle")
    s.text(330, 565, "IN ← KL15", 13, "#fde68a", "bold", "end")
    s.text(112, 665, "E → GND", 13, "#93c5fd", "bold")
    s.text(220, 590, "NPN BC337", 13, "#ffffff", "bold", "middle")
    s.text(220, 612, "R1 10 kΩ en la base", 12, MUTED, "normal", "middle")
    s.text(220, 632, "R2 100 kΩ base–GND", 12, MUTED, "normal", "middle")
    s.text(350, 540, "← Euro 5 pin 3 (Hoja 2)", 12, "#fde68a")

    # Placa MOSFET
    s.box(500, 560, 640, 240, "PLACA MOSFET 4 CANALES (entrada PNP 3,3–5 V · salida High-Side)", "#1d4ed8")
    s.text(925, 625, "VCC (+12 V)", 14, "#fca5a5", "bold", "middle")
    s.text(518, 750, "GND (−)", 14, "#93c5fd", "bold")
    s.text(820, 690, "Entradas X1–X4: Hoja 3   ·   Salidas Y1–Y4: Hoja 4", 14, MUTED, "normal", "middle")
    s.text(820, 720, "Fusible en línea 3–5 A por salida (ver Hoja 4)", 14, MUTED, "normal", "middle")

    # Cables
    s.wire([(340, 270), (390, 270)], RED, 6, "W1", (365, 270))
    s.wire([(550, 270), (660, 270)], RED, 6, "W2", (625, 270))
    s.wire([(590, 270), (590, 200), (660, 200)], RED, 3, "W27", (625, 200))
    s.wire([(925, 270), (925, 560)], RED, 6, "W3", (925, 420))
    s.wire([(880, 270), (990, 270)], RED, 4, "W28", (958, 270))
    s.wire([(1130, 270), (1200, 270)], RED, 4, "W29", (1165, 270))
    s.wire([(60, 190), (20, 190), (20, 105), (1325, 105), (1325, 150)], BLUE, 4, "W4", (700, 105))
    s.wire([(60, 250), (20, 250), (20, 745), (500, 745)], BLUE, 6, "W5", (20, 450))
    s.wire([(1450, 230), (1520, 230)], CYAN, 4, "W6", (1485, 230))
    s.wire([(1450, 320), (1520, 320)], GREY, 4, "W7", (1485, 320))
    s.wire([(100, 660), (20, 660)], BLUE, 3, "W30", (60, 660))
    s.wire([(770, 380), (770, 430), (220, 430), (220, 470)], AMBER, 3, "W31", (500, 430))
    s.wire([(340, 560), (440, 560)], AMBER, 3, "W32", (390, 560))

    # Terminales
    for x, y, c in [(340, 270, RED), (390, 270, GREEN), (550, 270, GREEN), (660, 270, PURPLE), (660, 200, PURPLE),
                    (880, 270, PURPLE), (770, 380, PURPLE), (990, 270, GREEN), (1130, 270, GREEN),
                    (1200, 270, RED), (925, 560, RED), (60, 190, BLUE), (60, 250, BLUE), (1325, 150, BLUE),
                    (1450, 230, CYAN), (1450, 320, GREY), (1520, 230, CYAN), (1520, 320, GREY),
                    (500, 745, BLUE), (100, 660, AMBER), (220, 470, AMBER), (340, 560, AMBER), (440, 560, AMBER)]:
        s.term(x, y, c)
    s.junction(590, 270, RED)
    s.junction(925, 270, RED)
    s.junction(20, 660, BLUE)

    rows = [
        ("W1", "Batería (+)  →  Portafusible ENTRADA", "AWG 14 rojo · anilla M6", RED),
        ("W2", "Portafusible SALIDA  →  Relé pin 30", "AWG 14 rojo", RED),
        ("W27", "Portafusible SALIDA  →  Relé pin 86 (+ bobina)", "AWG 20 rojo", RED),
        ("W3", "Relé pin 87  →  Placa MOSFET VCC", "AWG 14 rojo", RED),
        ("W28", "Relé pin 87  →  Fusible 3 A ENTRADA", "AWG 18 rojo", RED),
        ("W29", "Fusible 3 A SALIDA  →  Convertidor IN (+)", "AWG 18 rojo", RED),
        ("W4", "Batería (−)  →  Convertidor IN (−)", "AWG 18 negro · anilla M6", BLUE),
        ("W5", "Batería (−)  →  Placa MOSFET GND", "AWG 14 negro · anilla M6", BLUE),
        ("W6", "Convertidor OUT (+) 5 V  →  LILYGO VIN", "AWG 20 rojo", CYAN),
        ("W7", "Convertidor OUT (−)  →  LILYGO GND", "AWG 20 negro", GREY),
        ("W30", "Driver emisor (E)  →  GND (empalme en W5)", "AWG 22 negro", BLUE),
        ("W31", "Relé pin 85 (− bobina)  →  Driver colector (C)", "AWG 22 amarillo", AMBER),
        ("W32", "Euro 5 pin 3 (KL15)  →  Driver IN (R1 a la base)", "AWG 22 amarillo", AMBER),
    ]
    s.table(60, 950, COLS, rows, "LISTA DE CABLES · HOJA 1")
    s.note(1180, 560, 590, [
        "1. Desconecta el borne (−) de la batería antes de empezar.",
        "2. Portafusible SIN fusible hasta terminar y revisar.",
        "3. Mide el pin 3 del Euro 5: 0 V con contacto OFF y",
        "   ≈ 12 V con contacto ON (si no, no es KL15).",
        "4. Sin contacto el relé corta todo: consumo en reposo ≈ 0.",
    ], AMBER, "ORDEN DE MONTAJE")
    s.note(1180, 740, 590, [
        "• Fusible 15 A: protege el AWG 14 hasta el relé y la placa.",
        "• Fusible 3 A: protege el AWG 18 del convertidor.",
        "• Fusibles de 3–5 A por salida Y1–Y4 (Hoja 4).",
        "• Recomendado: TVS SMBJ24A en IN del convertidor.",
    ], RED, "PROTECCIONES")
    s.legend(60, 860, [(RED, "+12 V protegido"), (BLUE, "GND / masa batería"), (CYAN, "+5 V"), (GREY, "GND lógico"), (AMBER, "señal contacto / bobina")])
    return s


# =====================================================================
# HOJA 2 - BUS CAN
# =====================================================================
def sheet2():
    s = Sheet("02_bus_can", 2, "BUS CAN (SOLO ESCUCHA)",
              "Conector rojo Euro 5 de la moto → bornera CAN de la LILYGO · 500 kbps · modo LISTEN-ONLY", 1800, 900)
    s.box(60, 150, 1000, 220, "CLAVIJA ROJA EURO 5 (ISO 19689) · 6 pines · bajo el asiento del pasajero", RED)
    s.box(60, 560, 1000, 240, "", GREEN)
    s.text(1046, 584, "LILYGO T-CAN485 · bornera de tornillo", 16, "#ffffff", "bold", "end")

    pins = [
        ("1", "CAN-H", "Naranja / Negro", CAN_H, "→ W8"),
        ("2", "CAN-L", "Naranja / Marrón", CAN_L, "→ W9"),
        ("3", "+12 V", "contacto (KL15)", AMBER, "→ W32 (solo señal)"),
        ("4", "GND", "masa", "#94a3b8", "→ W10"),
        ("5", "K-Line", "diagnóstico", GREY, "NO CONECTAR"),
        ("6", "Aux", "diagnóstico", GREY, "NO CONECTAR"),
    ]
    terms = [
        ("CAN_H", "recibe W8", CAN_H, False),
        ("CAN_L", "recibe W9", CAN_L, False),
        ("120 Ω", "jumper: dejar OFF", GREY, True),
        ("GND", "recibe W10", "#94a3b8", False),
        ("VIN", "ver Hoja 1", GREY, True),
        ("GND", "ver Hoja 1", GREY, True),
    ]
    for i, (n, name, sub, col, status) in enumerate(pins):
        x0 = 90 + i * 160
        dashed = col == GREY
        s.cell(x0, 215, 140, 100, col, dashed)
        s.badge(x0 + 24, 241, n, col if not dashed else "#4b5563")
        s.text(x0 + 46, 247, name, 16, "#ffffff", "bold")
        s.text(x0 + 14, 276, sub, 12, MUTED)
        s.text(x0 + 14, 301, status, 13, "#f87171" if dashed else "#86efac", "bold")
    for i, (name, sub, col, dashed) in enumerate(terms):
        x0 = 90 + i * 160
        s.cell(x0, 606, 140, 100, col, dashed)
        s.text(x0 + 70, 644, name, 17, "#ffffff", "bold", "middle")
        s.text(x0 + 70, 672, sub, 12, MUTED, "normal", "middle")

    s.cell(410, 440, 140, 70, AMBER)
    s.text(480, 470, "DRIVER CONTACTO", 13, "#ffffff", "bold", "middle")
    s.text(480, 492, "Hoja 1 · R1 10 kΩ", 12, MUTED, "normal", "middle")
    s.wire([(480, 315), (480, 440)], AMBER, 4, "W32", (480, 380))
    s.term(480, 315, AMBER)
    s.term(480, 440, AMBER)
    s.text(80, 735, "Transceptor SN65HVD231 interno · TWAI a 500 kbps · RX = GPIO 26, TX = GPIO 27 (ya cableados en la placa)", 13, MUTED)
    s.text(80, 760, "El firmware activa GPIO 16 = HIGH al arrancar (alimenta el transceptor).", 13, MUTED)

    for i, (wid, col) in [(0, ("W8", CAN_H)), (1, ("W9", CAN_L)), (3, ("W10", "#94a3b8"))]:
        x = 160 + i * 160
        s.wire([(x, 315), (x, 606)], col, 5, wid, (x, 452))
        s.term(x, 315, col)
        s.term(x, 606, col)

    rows = [
        ("W8", "Euro 5 Pin 1 (CAN-H)  →  LILYGO CAN_H", "AWG 22 · naranja/negro", CAN_H),
        ("W9", "Euro 5 Pin 2 (CAN-L)  →  LILYGO CAN_L", "AWG 22 · naranja/marrón", CAN_L),
        ("W10", "Euro 5 Pin 4 (GND)  →  LILYGO GND (bornera CAN)", "AWG 22 · negro", "#94a3b8"),
        ("W32", "Euro 5 Pin 3 (KL15)  →  Driver de contacto (Hoja 1)", "AWG 22 · amarillo", AMBER),
    ]
    s.table(1090, 185, [("ID", 70), ("DE  →  A", 390), ("CABLE", 210)], rows, "LISTA DE CABLES · HOJA 2")
    s.note(1090, 400, 660, [
        "• Pin 3 (KL15): SOLO como señal hacia el driver (W32), NUNCA potencia.",
        "  Verifica: 0 V con contacto OFF y ≈ 12 V con ON.",
        "• Pines 5 y 6 (K-Line/aux): sin conectar.",
        "• Jumper/interruptor de 120 Ω de la LILYGO en OFF:",
        "  la moto ya trae sus dos resistencias de terminación.",
        "• Con la batería desconectada, entre CAN_H y CAN_L",
        "  debes medir ≈ 60 Ω.",
        "• Solo escucha: el firmware nunca transmite al bus.",
    ], RED, "IMPORTANTE")
    s.legend(60, 855, [(CAN_H, "CAN-H"), (CAN_L, "CAN-L"), ("#94a3b8", "GND"), (GREY, "no conectar")])
    return s


# =====================================================================
# HOJA 3 - SEÑALES PWM
# =====================================================================
def sheet3():
    s = Sheet("03_senales_pwm", 3, "SEÑALES PWM  (LILYGO → PLACA MOSFET)",
              "Cinco cables rectos entre el header de la LILYGO y las entradas de la placa MOSFET · señal 3,3 V", 1800, 960)
    ys = [250, 330, 410, 490, 570]
    s.box(60, 140, 640, 480, "LILYGO T-CAN485 · header GPIO (salidas PWM)", GREEN)
    s.box(1100, 140, 640, 480, "PLACA MOSFET 4 CANALES · entradas de señal", "#1d4ed8")

    left = [("GPIO 25", "Canal 1 · foco izquierdo"), ("GPIO 32", "Canal 2 · foco derecho"),
            ("GPIO 33", "Canal 3 · faros de niebla"), ("GPIO 18", "Canal 4 · bocina / freno / aux"),
            ("GND", "masa lógica")]
    right = [("X1", "→ Y1"), ("X2", "→ Y2"), ("X3", "→ Y3"), ("X4", "→ Y4"), ("COM", "común de entradas (a GND)")]
    cols = [TXT, YELLOW, "#60a5fa", "#f87171", GREY]
    ids = ["W11", "W12", "W13", "W14", "W15"]
    for y, (a, b), (c, d), col, wid in zip(ys, left, right, cols, ids):
        s.text(684, y + 5, a, 16, "#ffffff", "bold", "end")
        s.text(684, y + 25, b, 12, MUTED, "normal", "end")
        s.text(1118, y + 5, c, 16, "#ffffff", "bold")
        s.text(1118, y + 25, d, 12, MUTED)
        s.wire([(700, y), (1100, y)], col, 5, wid, (900, y))
        s.term(700, y, col)
        s.term(1100, y, col)

    s.text(80, 215, "PWM 3,3 V · 1 kHz · activo a nivel ALTO", 13, MUTED)
    s.text(80, 600, "Sin cables de potencia aquí: solo señal.", 13, MUTED)

    rows = [
        ("W11", "LILYGO GPIO 25  →  Placa MOSFET X1", "AWG 24 · blanco", TXT),
        ("W12", "LILYGO GPIO 32  →  Placa MOSFET X2", "AWG 24 · amarillo", YELLOW),
        ("W13", "LILYGO GPIO 33  →  Placa MOSFET X3", "AWG 24 · azul", "#60a5fa"),
        ("W14", "LILYGO GPIO 18  →  Placa MOSFET X4", "AWG 24 · rojo", "#f87171"),
        ("W15", "LILYGO GND (header)  →  Placa MOSFET COM", "AWG 24 · negro", GREY),
    ]
    s.table(60, 680, [("ID", 70), ("DE  →  A", 640), ("CABLE", 300)], rows, "LISTA DE CABLES · HOJA 3")
    s.note(1130, 680, 610, [
        "• Placa eletechsup OPMSA04_PNP: bornes X1–X4 + COM.",
        "• Puentes de la placa: Input Level = PNP",
        "  y Trigger Voltage = 5V (señal 3,3 V del ESP32).",
        "• Con nivel PNP: COM a GND (W15, obligatorio) y",
        "  señal ALTA en X1–X4 activa el canal.",
        "• Recomendado: resistencia 10 kΩ de cada X1–X4 a COM",
        "  (evita parpadeos mientras la LILYGO arranca).",
    ], AMBER, "NOTAS")
    return s


# =====================================================================
# HOJA 4 - SALIDAS A CONECTORES
# =====================================================================
def sheet4():
    s = Sheet("04_salidas_conectores", 4, "SALIDAS A LOS CONECTORES SUPERSEAL",
              "2 conectores de 2 pines + 2 conectores de 3 pines · cada pin con su cable, ninguno al aire", 1800, 1440)

    # Placa MOSFET (izquierda)
    s.box(60, 110, 420, 814, "PLACA MOSFET 4 CANALES · salidas", "#1d4ed8")
    outs = [(175, "Y1", "Canal 1"), (355, "Y2", "Canal 2"), (535, "Y3", "Canal 3"), (777, "Y4", "Canal 4")]
    for y, a, b in outs:
        s.text(462, y + 5, a + " · +12 V PWM", 16, "#ffffff", "bold", "end")
        s.text(462, y + 25, b + " · fusible 3–5 A", 12, MUTED, "normal", "end")
    s.text(270, 640, "Alimentación VCC / GND: Hoja 1", 13, MUTED, "normal", "middle")
    s.text(270, 665, "Entradas X1–X4: Hoja 3", 13, MUTED, "normal", "middle")

    # Conectores
    conns = [
        (110, 150, "CONECTOR 1 · 2 PINES · FOCO IZQUIERDO (SET 1)", YELLOW, [175, 231],
         [("1", "+12 V PWM · Y1", "Foco izquierdo · rojo AWG 16", RED), ("2", "GND (masa común)", "desde regleta GND · negro AWG 16", BLUE)]),
        (290, 150, "CONECTOR 2 · 2 PINES · FOCO DERECHO (SET 1)", YELLOW, [355, 411],
         [("1", "+12 V PWM · Y2", "Foco derecho · rojo AWG 16", RED), ("2", "GND (masa común)", "desde regleta GND · negro AWG 16", BLUE)]),
        (470, 212, "CONECTOR 3 · 3 PINES · FAROS DE NIEBLA (SET 2)", CYAN, [535, 591, 647],
         [("1", "+12 V PWM · Y3", "Faro de niebla · rojo AWG 16", RED), ("2", "GND (masa común)", "desde regleta GND · negro AWG 16", BLUE),
          ("3", "+12 V PWM · puente con Pin 1", "Aro DRL del foco · amarillo AWG 18", AMBER)]),
        (712, 212, "CONECTOR 4 · 3 PINES · AUX / LUZ DE FRENO", "#ec4899", [777, 833, 889],
         [("1", "+12 V PWM · Y4", "Aux / luz freno · rojo AWG 16 (bocina: ver nota)", RED), ("2", "GND (masa común)", "desde regleta GND · negro AWG 16", BLUE),
          ("3", "+12 V PWM · puente con Pin 1", "Luz de posición / aux · amarillo AWG 18", AMBER)]),
    ]
    pin_y = {}
    for ci, (y, h, title, color, yc, pins) in enumerate(conns, start=1):
        s.box(820, y, 480, h, title, color, hh=36)
        for yy, (n, a, b, pc) in zip(yc, pins):
            s.cell(830, yy - 23, 460, 46, pc)
            s.badge(860, yy, n, pc)
            s.text(886, yy - 2, a, 15, "#ffffff", "bold")
            s.text(886, yy + 15, b, 12, MUTED)
            pin_y[(ci, n)] = yy

    # Cables OUT -> Pin 1
    red_tags = {1: (700, 175), 2: (700, 355), 3: (720, 535), 4: (720, 777)}
    for ci, (y, _, a) in enumerate(outs, start=1):
        yy = pin_y[(ci, "1")]
        s.wire([(480, yy), (830, yy)], RED, 5, f"W{15 + ci}", red_tags[ci])
        s.term(480, yy, RED)
        s.term(830, yy, RED)

    # Puentes a Pin 3 (mismo canal)
    for ci, wid in [(3, "W20"), (4, "W21")]:
        y1, y3 = pin_y[(ci, "1")], pin_y[(ci, "3")]
        s.wire([(620, y1), (620, y3), (830, y3)], AMBER, 4, wid, (620, (y1 + y3) // 2))
        s.junction(620, y1, AMBER)
        s.term(830, y3, AMBER)
    s.text(632, pin_y[(3, "1")] + 30, "puente", 12, AMBER)
    s.text(632, pin_y[(4, "1")] + 30, "puente", 12, AMBER)

    # Regleta GND (derecha)
    s.raw(f'<rect x="1440" y="200" width="20" height="660" rx="6" fill="{BLUE}" stroke="#93c5fd" stroke-width="2"/>')
    s.text(1450, 168, "REGLETA GND", 15, "#93c5fd", "bold", "middle")
    s.text(1450, 186, "(Wago 5 vías)", 12, MUTED, "normal", "middle")
    for ci, wid in zip([1, 2, 3, 4], ["W23", "W24", "W25", "W26"]):
        yy = pin_y[(ci, "2")]
        s.wire([(1290, yy), (1450, yy)], BLUE, 4, wid, (1370, yy))
        s.term(1290, yy, BLUE)
        s.junction(1450, yy, "#93c5fd")

    # Batería (−)
    s.box(1580, 480, 190, 120, "BATERÍA (−)", RED, hh=30)
    s.text(1675, 552, "borne negativo", 13, "#93c5fd", "bold", "middle")
    s.wire([(1580, 540), (1450, 540)], BLUE, 6, "W22", (1515, 540))
    s.term(1580, 540, BLUE)
    s.junction(1450, 540, "#93c5fd")

    rows = [
        ("W16", "Placa MOSFET Y1  →  Conector 1 · Pin 1", "AWG 16 rojo · fusible 3–5 A", RED),
        ("W17", "Placa MOSFET Y2  →  Conector 2 · Pin 1", "AWG 16 rojo · fusible 3–5 A", RED),
        ("W18", "Placa MOSFET Y3  →  Conector 3 · Pin 1", "AWG 16 rojo · fusible 3–5 A", RED),
        ("W19", "Placa MOSFET Y4  →  Conector 4 · Pin 1", "AWG 16 rojo · fusible 3–5 A", RED),
        ("W20", "Conector 3 · Pin 1  →  Conector 3 · Pin 3 (puente, mismo canal)", "AWG 18 amarillo", AMBER),
        ("W21", "Conector 4 · Pin 1  →  Conector 4 · Pin 3 (puente, mismo canal)", "AWG 18 amarillo", AMBER),
        ("W22", "Batería (−)  →  Regleta GND", "AWG 14 negro · anilla M6", BLUE),
        ("W23", "Regleta GND  →  Conector 1 · Pin 2", "AWG 16 negro", BLUE),
        ("W24", "Regleta GND  →  Conector 2 · Pin 2", "AWG 16 negro", BLUE),
        ("W25", "Regleta GND  →  Conector 3 · Pin 2", "AWG 16 negro", BLUE),
        ("W26", "Regleta GND  →  Conector 4 · Pin 2", "AWG 16 negro", BLUE),
    ]
    s.table(60, 1000, [("ID", 70), ("DE  →  A", 700), ("CABLE", 280)], rows, "LISTA DE CABLES · HOJA 4", rh=32)
    s.note(1190, 1000, 560, [
        "• El Pin 3 va en PARALELO con el Pin 1 (mismo canal):",
        "  se apaga con la moto y respeta el dimmer.",
        "• NUNCA lo conectes a +12 V permanente: dejaría el",
        "  aro encendido y descargaría la batería.",
        "• Pin 1 + Pin 3 comparten el límite de 5 A del canal.",
        "• Fusible en línea de 3–5 A en cada cable rojo (W16–W19).",
        "• Bocina de aire (> 5 A): Y4 solo excita la bobina de un relé",
        "  (con diodo) y el contacto del relé lleva su propio fusible.",
    ], AMBER, "NOTAS DE SALIDAS")
    s.legend(60, 950, [(RED, "+12 V PWM (salida MOSFET)"), (AMBER, "puente al Pin 3"), (BLUE, "GND / masa")])
    return s


def render_png(sheets):
    from playwright.sync_api import sync_playwright

    with sync_playwright() as p:
        browser = p.chromium.launch()
        for sh in sheets:
            page = browser.new_page(viewport={"width": sh.w, "height": sh.h}, device_scale_factor=1.5)
            page.goto((OUT / f"{sh.slug}.svg").as_uri())
            page.wait_for_timeout(300)
            page.screenshot(path=str(OUT / f"{sh.slug}.png"))
            page.close()
        browser.close()


def main():
    OUT.mkdir(exist_ok=True)
    sheets = [sheet1(), sheet2(), sheet3(), sheet4()]
    for sh in sheets:
        (OUT / f"{sh.slug}.svg").write_text(sh.svg(), encoding="utf-8")
        print("SVG:", OUT / f"{sh.slug}.svg")
    render_png(sheets)
    for sh in sheets:
        print("PNG:", OUT / f"{sh.slug}.png")


if __name__ == "__main__":
    main()
