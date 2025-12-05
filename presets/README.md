# 🎵 Preset Library - Frutilla Studio

## 📁 Estructura de Carpetas

```
presets/
├── wavetable/          # Presets de Wavetable Synth
│   ├── Bass/
│   ├── Lead/
│   ├── Pad/
│   └── Pluck/
├── fm/                 # Presets de FM Synth
│   ├── Keys/
│   ├── Brass/
│   ├── Bass/
│   └── Percussive/
├── channelRack/        # Patterns de Channel Rack
│   ├── Kick/
│   ├── Snare/
│   ├── HiHat/
│   └── Full/
└── mixer/              # Templates de Mixer
    ├── Mastering/
    └── Vocal/
```

---

## 🎹 Wavetable Synth Presets

### Bass (5 presets)

#### 1. **Massive Bass**
- **Uso:** Sub bass agresivo para EDM, Dubstep
- **Características:** 8 voces unison, detune alto, filter resonante
- **Tags:** `bass`, `edm`, `aggressive`, `sub`

#### 2. **Sub Bass**
- **Uso:** Bass puro de sub-frecuencias
- **Características:** 2 voces, casi sin detune, filter lowpass a 200Hz
- **Tags:** `bass`, `sub`, `clean`, `low`

### Lead (1 preset)

#### 3. **Bright Lead**
- **Uso:** Melodías cortantes, leads principales
- **Características:** 6 voces, bandpass filter, LFO en cutoff
- **Tags:** `lead`, `bright`, `melody`, `edm`

### Pad (1 preset)

#### 4. **Lush Pad**
- **Uso:** Fondos atmosféricos, texturas ambientales
- **Características:** 16 voces, stereo wide, attack/release largos
- **Tags:** `pad`, `ambient`, `lush`, `wide`

### Pluck (1 preset)

#### 5. **Pluck Lead**
- **Uso:** Arpeggios, secuencias rítmicas
- **Características:** Attack instantáneo, decay corto, sin sustain
- **Tags:** `pluck`, `arp`, `short`, `edm`

---

## 🎛️ FM Synth Presets

### Keys (1 preset)

#### 1. **DX7 Electric Piano**
- **Algoritmo:** DX7-1 (clásico)
- **Uso:** Piano eléctrico estilo Yamaha DX7
- **Operadores:** Op1 (1.0) + Op2 (14.0) ratio para brillo
- **Tags:** `epiano`, `dx7`, `keys`, `classic`

### Brass (1 preset)

#### 2. **FM Brass**
- **Algoritmo:** Stack
- **Uso:** Sección de vientos, leads potentes
- **Operadores:** 6 operadores en ratios armónicos (1.0-6.0)
- **Tags:** `brass`, `horn`, `stack`, `powerful`

### Percussive (1 preset)

#### 3. **Bell**
- **Algoritmo:** DX7-32
- **Uso:** Campanas, sonidos metálicos
- **Operadores:** Ratios inarmónicos (3.5, 7.1, 11.0, 14.3)
- **Tags:** `bell`, `metallic`, `percussion`, `long`

### Bass (1 preset)

#### 4. **FM Bass**
- **Algoritmo:** Parallel
- **Uso:** Bass con carácter FM, growl
- **Operadores:** Mezcla de ratios bajos (0.5, 1.0, 1.5, 2.0)
- **Tags:** `bass`, `growl`, `fm`, `aggressive`

---

## 🥁 Channel Rack Patterns

### Kick Patterns (1 preset)

#### 1. **4-on-Floor**
- **Steps:** 4 kicks en beats 1, 2, 3, 4
- **BPM:** 120-130 (House/Techno)
- **Velocities:** 127 (uniforme)
- **Géneros:** House, Techno, Trance

### Snare Patterns (1 preset)

#### 2. **Backbeat**
- **Steps:** Snare en beats 2 y 4
- **BPM:** 80-140 (Universal)
- **Velocities:** 120
- **Géneros:** Rock, Pop, R&B, Hip Hop

### Hi-Hat Patterns (1 preset)

#### 3. **Hi-Hat 8ths**
- **Steps:** 8 hi-hats (8th notes)
- **BPM:** 80-160 (Universal)
- **Velocities:** Alternadas (100 / 80) para groove
- **Géneros:** Universal

### Full Patterns (1 preset)

#### 4. **Breakbeat**
- **Channels:** Kick + Snare + Hi-Hat
- **Steps:** Patrón sincopado inspirado en Amen Break
- **BPM:** 170 (Drum & Bass óptimo)
- **Géneros:** DnB, Jungle, Breakbeat, Hip Hop

---

## 🎚️ Mixer Templates (Próximamente)

### Mastering Chain
```
Coming soon:
- EQ (High-pass, notch, shelf)
- Multiband Compressor
- Exciter
- Limiter
```

### Vocal Chain
```
Coming soon:
- De-esser
- EQ (correction)
- Compressor
- Reverb (send)
- Delay (send)
```

---

## 📝 Formato de Archivo JSON

### Wavetable Synth Preset Format
```json
{
  "presetName": "String",
  "category": "String",
  "author": "String",
  "version": "String",
  "description": "String",
  "tags": ["array", "of", "tags"],
  
  "wavetable": {
    "waveformType": "Sine|Saw|Square|Triangle",
    "waveformPosition": 0.0-1.0,
    "morphAmount": 0.0-1.0
  },
  
  "unison": {
    "voices": 1-16,
    "detune": 0.0-1.0,
    "spread": 0.0-1.0,
    "blend": 0.0-1.0
  },
  
  "filter": {
    "type": "lowpass|highpass|bandpass",
    "cutoff": 20-20000 (Hz),
    "resonance": 0.0-1.0,
    "drive": 0.0-1.0
  },
  
  "envelope": {
    "attack": 0.0+ (seconds),
    "decay": 0.0+,
    "sustain": 0.0-1.0,
    "release": 0.0+
  },
  
  "lfo": {
    "rate": 0.0+ (Hz),
    "depth": 0.0-1.0,
    "destination": "cutoff|pitch|volume"
  },
  
  "output": {
    "volume": 0.0-1.0,
    "pan": 0.0-1.0 (0.5 = center)
  }
}
```

### FM Synth Preset Format
```json
{
  "presetName": "String",
  "category": "String",
  "algorithm": "Stack|Parallel|DX7-1|DX7-32",
  
  "operators": [
    {
      "id": 1-6,
      "ratio": 0.25-16.0,
      "level": 0.0-1.0,
      "feedback": 0.0-1.0,
      "envelope": {
        "attack": 0.0+,
        "decay": 0.0+,
        "sustain": 0.0-1.0,
        "release": 0.0+
      }
    }
  ]
}
```

### Channel Rack Pattern Format
```json
{
  "presetName": "String",
  "category": "String",
  "steps": 16,
  
  "pattern": [
    {
      "step": 0-15,
      "velocity": 0-127,
      "note": "MIDI Note String (C1, D#2, etc)"
    }
  ],
  
  "channelSettings": {
    "volume": 0.0-1.0,
    "pan": 0.0-1.0,
    "mute": boolean,
    "solo": boolean
  }
}
```

---

## 🚀 Cómo Usar los Presets

### En Wavetable/FM Synth
```
1. Abre el synth en un canal
2. Click en "Load Preset"
3. Navega a la carpeta correspondiente
4. Selecciona el preset .json
5. El synth cargará todos los parámetros
```

### En Channel Rack
```
1. Selecciona un canal
2. Click en "Load Preset" (botón de carpeta)
3. Selecciona el pattern .json
4. Los steps se cargarán automáticamente
```

---

## ✨ Crear tus Propios Presets

### Wavetable/FM Synth
```
1. Diseña tu sonido en el synth
2. Click en "Save Preset"
3. Dale un nombre y categoría
4. Agrega tags descriptivos
5. El preset se guarda en JSON
```

### Channel Rack Patterns
```
1. Programa tu pattern
2. Click derecho → "Save as Preset"
3. Nombra el pattern
4. Se guarda en presets/channelRack/
```

---

## 📊 Estadísticas de Presets

- **Total Presets:** 12+
- **Wavetable:** 5 presets
- **FM Synth:** 4 presets
- **Channel Rack:** 4 presets
- **Mixer Templates:** 0 (próximamente)

---

## 🔮 Presets Futuros

### En Desarrollo
- Vocal processing chains
- Mastering templates
- More drum patterns (trap, reggaeton, afrobeat)
- Experimental wavetable sounds
- Complex FM patches (organs, strings)

### Community Presets
Se planea un sistema de compartir presets creados por la comunidad:
- Upvoting system
- Categories and tags
- Download statistics
- Rating system

---

*Última actualización: 2024*
*Versión: 1.0*
