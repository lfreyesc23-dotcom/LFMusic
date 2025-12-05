# 📚 Frutilla Studio - Guía de Usuario

## Bienvenido a Frutilla Studio v2.0

Frutilla Studio es una estación de trabajo de audio digital (DAW) profesional inspirada en FL Studio, con capacidades de producción musical de nivel profesional.

---

## 🚀 Guía de Inicio Rápido

### Instalación y Configuración Inicial

1. **Requisitos del Sistema**
   - macOS 10.15 o superior (Intel/Apple Silicon)
   - Windows 10 o superior
   - 8GB RAM mínimo (16GB recomendado)
   - 2GB espacio en disco
   - Interfaz de audio (opcional pero recomendado)

2. **Primera Ejecución**
   ```bash
   npm install
   npm run dev
   ```

3. **Configuración de Audio**
   - Ve a `Configuración → Audio Settings`
   - Selecciona tu interfaz de audio
   - Ajusta el buffer size (512 samples recomendado)
   - Configura tu sample rate (48 kHz recomendado)

---

## 🎹 Tutorial: Pattern Workflow (Flujo de Trabajo con Patrones)

### ¿Qué es un Pattern?

Un **Pattern** es una secuencia musical contenida que puedes reutilizar, clonar y organizar en la **Playlist**. Este es el corazón del flujo de trabajo de Frutilla Studio.

### Crear tu Primer Pattern

1. **Abrir la Vista de Playlist**
   - Clic en la pestaña `Playlist` en el navbar
   - Verás la lista de patterns a la derecha

2. **Crear un Nuevo Pattern**
   - Clic en el botón `➕ New Pattern`
   - Dale un nombre significativo (ejemplo: "Kick Pattern")
   - Define la longitud en beats (16 beats por defecto)

3. **Agregar Contenido al Pattern**
   - Abre el **Channel Rack** (pestaña Channel Rack)
   - Agrega canales con el botón `➕ Add Channel`
   - Programa tu ritmo en el step sequencer de 16 pasos

4. **Colocar el Pattern en la Playlist**
   - Vuelve a la vista de Playlist
   - **Arrastra** el pattern desde la lista hacia la grid
   - Suéltalo en el track y posición deseados
   - El snap to grid está activado por defecto

### Técnicas Avanzadas de Patterns

#### Clonar Patterns
```
1. Selecciona un pattern en la lista
2. Clic en botón "📋 Clone"
3. Modifica el clon sin afectar el original
```

#### Pattern Único (Make Unique)
```
- Cuando un pattern está en múltiples posiciones
- "Make Unique" crea una versión independiente
- Úsalo para crear variaciones
```

#### Dividir Patterns (Split)
```
1. Selecciona un pattern colocado en la playlist
2. Clic en "✂️ Split"
3. Ingresa el beat donde dividir (ejemplo: 8)
4. Resultado: dos patterns independientes
```

#### Fusionar Patterns (Merge)
```
1. Selecciona múltiples patterns (Ctrl+Click)
2. Clic en "🔗 Merge"
3. Se combinarán en un solo pattern
```

---

## 🥁 Tutorial: Channel Rack (Secuenciador por Pasos)

### Interfaz del Channel Rack

El **Channel Rack** es un secuenciador de 16 pasos estilo FL Studio, perfecto para crear ritmos y melodías.

### Crear un Beat Básico

#### Paso 1: Agregar Canales
```
1. Clic en "➕ Add Channel"
2. Nombra tu canal (ejemplo: "Kick")
3. Repite para Snare, Hi-Hat, etc.
```

#### Paso 2: Programar Pasos
```
- Clic en los cuadrados de la grid para activar/desactivar pasos
- Los pasos activos se iluminan en azul
- Durante reproducción, el paso actual se marca con borde dorado
```

#### Paso 3: Ajustar Parámetros por Canal
```
- M button: Mute (silenciar canal)
- S button: Solo (solo ese canal suena)
- Volume slider: Ajustar volumen individual
```

### Presets Rítmicos Incluidos

| Preset | Descripción | Uso |
|--------|-------------|-----|
| **4-on-Floor** | Bombo en cada beat | House, Techno |
| **Backbeat** | Snare en beats 2 y 4 | Rock, Pop |
| **Hi-Hat 8th** | Hi-hats en octavos | General |
| **Breakbeat** | Patrón sincopado | Drum & Bass, Hip Hop |

### Capas de Canales (Layers)

```javascript
// Crear capas para sonidos complejos
1. Selecciona un canal
2. Clic en "Add Layer"
3. Carga diferentes samples en cada capa
4. Ajusta velocity y volumen independiente
```

---

## 🎛️ Tutorial: Síntesis de Wavetable

### ¿Qué es Síntesis de Wavetable?

La **síntesis de wavetable** usa formas de onda pre-calculadas que puedes morphear para crear sonidos únicos.

### Crear un Sonido de Bass

1. **Cargar el Wavetable Synth**
   ```
   - Agrega un nuevo canal en Channel Rack
   - Selecciona "Wavetable Synth" como instrumento
   ```

2. **Seleccionar Waveform Base**
   ```
   - Waveform: "Saw" (para bass agresivo)
   - Waveform Position: 0.0 (inicio del wavetable)
   ```

3. **Configurar Unison**
   ```
   - Unison Voices: 4 (más voices = más grueso)
   - Detune: 0.15 (desafina ligeramente las voces)
   - Spread: 0.7 (panoramiza las voces)
   ```

4. **Ajustar Filter**
   ```
   - Filter Type: Low Pass
   - Cutoff: 800 Hz
   - Resonance: 0.5
   ```

5. **ADSR Envelope**
   ```
   - Attack: 0.01s (ataque rápido)
   - Decay: 0.3s
   - Sustain: 0.6
   - Release: 0.5s
   ```

### Presets Recomendados

#### Massive Bass
```
Waveform: Square
Unison: 8 voices
Detune: 0.25
Filter: Lowpass 600Hz, Res 0.7
```

#### Bright Lead
```
Waveform: Saw
Unison: 6 voices
Detune: 0.10
Filter: Bandpass 2000Hz, Res 0.4
```

#### Lush Pad
```
Waveform: Sine
Unison: 16 voices
Detune: 0.05
Filter: Lowpass 1500Hz, Res 0.3
Attack: 0.5s, Release: 2.0s
```

---

## 🎹 Tutorial: FM Synthesis (Síntesis FM)

### ¿Qué es Síntesis FM?

La **síntesis FM** (Frequency Modulation) usa operadores que modulan las frecuencias de otros operadores para crear timbres complejos.

### Anatomía del FM Synth

```
- 6 Operadores (cada uno es un oscilador)
- Cada operador tiene: Ratio, Level, Feedback, ADSR
- Los algoritmos determinan cómo se conectan
```

### Crear un Electric Piano (Preset DX7)

1. **Seleccionar Algoritmo**
   ```
   - Algorithm: DX7-1 (clásico E.Piano)
   - Este algoritmo usa modulación en cascada
   ```

2. **Configurar Operadores**
   ```
   Operator 1 (Carrier):
   - Ratio: 1.0
   - Level: 0.9
   - Attack: 0.001s, Decay: 0.5s

   Operator 2 (Modulator):
   - Ratio: 14.0
   - Level: 0.7
   - Attack: 0.001s, Decay: 0.3s

   Operator 3-6: Levels bajos para armónicos
   ```

3. **Ajustar Feedback**
   ```
   - Operator 1 Feedback: 0.3
   - Da carácter metálico al sonido
   ```

### Otros Presets FM

#### Brass (Bronce)
```
Algorithm: Stack
Op1: Ratio 1.0, Level 0.8
Op2: Ratio 2.0, Level 0.7, Feedback 0.5
Op3-6: Ratios 3.0, 4.0, 5.0, 6.0 - Levels decrecientes
```

#### Bell (Campana)
```
Algorithm: DX7-32
Op1: Ratio 1.0, Level 0.9
Op2: Ratio 3.5, Level 0.6
Op3: Ratio 7.1, Level 0.4
Todos con Decay largo (2.0s+)
```

---

## ✨ Tutorial: Efectos Creativos

### Flanger
```
Descripción: Efecto de barrido metálico
Uso: Guitarras, synths, voces

Parámetros clave:
- Rate: 0.5 Hz (velocidad del LFO)
- Depth: 0.7 (intensidad del efecto)
- Feedback: 0.5 (resonancia)
- Mix: 0.5 (50% wet/dry)

Tip: Rate bajo + Depth alto = efecto "jet plane"
```

### Phaser
```
Descripción: Efecto de fase ondulante
Uso: Teclados, guitarras eléctricas

Parámetros clave:
- Stages: 6 (más stages = más resonancia)
- Rate: 0.3 Hz
- Depth: 0.6
- Feedback: 0.4
- Mix: 0.4

Tip: Ideal para sonidos retro 70s/80s
```

### Chorus
```
Descripción: Engrosa el sonido simulando múltiples voces
Uso: Cuerdas, pads, guitarra acústica

Parámetros clave:
- Voices: 3 (más voces = más grueso)
- Rate: 0.8 Hz
- Depth: 0.5
- Spread: 0.7 (panoramización)
- Mix: 0.35

Tip: Mix bajo para efecto sutil en voces
```

### Vocoder
```
Descripción: Efecto robotizado de voz
Uso: Voces, synths lead

Parámetros clave:
- Bands: 16 (más bandas = más definición)
- Mix: 0.8 (efecto muy presente)

Tip: Requiere señal carrier (synth) y modulator (voz)
```

### Stutter/Glitch
```
Descripción: Repeticiones rítmicas sincronizadas con tempo
Uso: Drops, transiciones, efectos creativos

Parámetros clave:
- Division: 1/16 (subdivisión rítmica)
- Repeat: 4 (número de repeticiones)
- Decay: 0.8 (desvanecimiento)
- Mix: 1.0 (efecto completo cuando activo)

Tip: Automatiza el "trigger" para crear breaks
```

---

## 🎚️ Tutorial: Mezcla y Exportación

### Workflow de Mezcla

1. **Balancear Niveles**
   ```
   - Empieza con todos los faders a -6dB
   - Sube el fader master solo al final
   - Deja headroom para mastering
   ```

2. **Panoramización (Panning)**
   ```
   - Centro: Kick, Snare, Bass, Lead Vocal
   - Izquierda/Derecha: Hi-Hats, Percusión, Synths
   - Wide: Pads, Strings, Efectos ambientales
   ```

3. **EQ Básico**
   ```
   - High-pass todo excepto kick/bass (80-120 Hz)
   - Corta frecuencias conflictivas
   - Booste con moderación
   ```

4. **Compresión**
   ```
   - Drums: Ratio 4:1, Attack rápido
   - Voces: Ratio 3:1, Attack medio
   - Bus: Ratio 2:1, Attack lento (glue compression)
   ```

### Exportar tu Proyecto

#### Exportación Master
```
1. File → Export → Master Mix
2. Format: WAV 24-bit, 48kHz
3. Dithering: Activado si exportas a 16-bit
4. Normalize: Desactivado (preserva dynamics)
```

#### Exportación de Stems
```
1. File → Export → Stems
2. Selecciona tracks a exportar
3. Nombrado automático: "ProjectName_TrackName.wav"
4. Usa para colaboración o mastering externo
```

#### Exportar MIDI
```
1. Selecciona patterns con contenido MIDI
2. File → Export → MIDI
3. Útil para compartir con otros DAWs
```

### Performance Tips

#### Freeze Tracks
```
// Cuando tu CPU está al límite
1. Click derecho en track → Freeze
2. El track se renderiza temporalmente
3. Libera CPU pero no puedes editar
4. Unfreeze cuando necesites cambios
```

#### Smart Disable
```
// El sistema desactiva plugins no usados automáticamente
- Plugins inactivos en tracks muteados
- Efectos con mix al 0%
- Revísalo en: Performance → Smart Disable
```

---

## 🔧 Atajos de Teclado Esenciales

### Navegación
```
Tab               → Cambiar entre Playlist/Channel Rack
Space             → Play/Pause
Ctrl + S          → Guardar proyecto
Ctrl + Z          → Deshacer
Ctrl + Y          → Rehacer
```

### Edición
```
Ctrl + C          → Copiar
Ctrl + V          → Pegar
Ctrl + X          → Cortar
Ctrl + D          → Duplicar selección
Delete            → Eliminar selección
```

### Patterns
```
Ctrl + N          → Nuevo pattern
Ctrl + Shift + C  → Clonar pattern
Ctrl + U          → Make pattern unique
Alt + S           → Split pattern
Alt + M           → Merge patterns
```

### Transport
```
Enter             → Play desde inicio
Shift + Enter     → Play desde cursor
R                 → Toggle recording
F12               → Panic (stop all sounds)
```

---

## 🎯 Workflows Recomendados

### Workflow 1: Crear un Beat desde Cero

```
1. Nuevo Proyecto → Set BPM (ejemplo: 128)
2. Channel Rack → Agregar Kick, Snare, Hi-Hat
3. Programar patrón de 16 pasos
4. Aplicar preset "4-on-Floor" al kick
5. Playlist → Arrastrar pattern 8 veces (2 compases)
6. Crear variación (clonar + modificar)
7. Agregar canal con Bass Synth
8. Programar línea de bajo
9. Exportar stems para mezclar
```

### Workflow 2: Sound Design con Synths

```
1. Channel Rack → Add Wavetable Synth
2. Seleccionar waveform base
3. Experimentar con Unison (4-8 voices)
4. Ajustar Filter cutoff + resonance
5. Configurar ADSR envelope
6. Agregar efecto Chorus o Flanger
7. Grabar automación de filter cutoff
8. Guardar como preset personalizado
```

### Workflow 3: Producción Completa

```
Día 1: Drums & Percussion
- Channel Rack patterns
- 4-8 compases de groove

Día 2: Bass & Harmony
- Wavetable synth para bass
- FM synth para chords/pads

Día 3: Melodía & Efectos
- Lead synth
- Efectos creativos en transiciones

Día 4: Mezcla
- Balance, panning, EQ
- Compresión en bus

Día 5: Mastering & Export
- Limiting en master
- Export stems + master
```

---

## 📊 Solución de Problemas Comunes

### Audio Crackling / Glitches
```
Problema: Audio entrecortado o glitches
Solución:
1. Aumenta buffer size (1024 samples)
2. Freeze tracks pesados
3. Activa Smart Disable
4. Cierra aplicaciones innecesarias
```

### High CPU Usage
```
Problema: CPU al 100%
Solución:
1. Performance → CPU Monitor
2. Identifica plugins pesados
3. Freeze tracks no editables
4. Reduce unison voices en synths
5. Usa Low Latency Mode solo al grabar
```

### MIDI Not Playing
```
Problema: Notas MIDI no suenan
Solución:
1. Verifica canal no está muteado
2. Revisa volumen del canal
3. Asegura instrumento asignado
4. Check MIDI output routing
```

### Exports Sound Different
```
Problema: Export suena diferente al proyecto
Solución:
1. Desactiva master limiter temporalmente
2. Verifica plugins no tienen "PDC" (Plugin Delay Compensation) desactivado
3. Export con "Normalize" desactivado
4. Asegura export a mismo sample rate
```

---

## 🚀 Recursos Adicionales

### Tutoriales en Video (Próximamente)
- Basic Workflow Walkthrough (15 min)
- Creating Professional Beats (30 min)
- Wavetable Sound Design Masterclass (45 min)
- FM Synthesis Deep Dive (60 min)

### Comunidad y Soporte
- Discord: [discord.gg/frutillastudio](#)
- Forum: [community.frutillastudio.com](#)
- YouTube: [youtube.com/@frutillastudio](#)

### Packs de Contenido
- Drum Kits Vol. 1-3
- Wavetable Presets (200+)
- FM Presets (150+)
- Channel Rack Templates

---

## 📝 Notas Finales

Esta guía cubre las funcionalidades principales de **Frutilla Studio v2.0**. El software está en constante desarrollo, y nuevas funciones se agregan regularmente.

**¡Feliz producción musical!** 🎵🍓

---

*Última actualización: 2024*
*Versión de la guía: 2.0*
