# Plan Técnico — Implementación FL-Grade

## Módulos clave
- `engine` (Audio/Core): grafo `AudioProcessorGraph`, PDC, oversampling por nodo, colas lock-free, pools de memoria.
- `ml` (ONNX): loader de modelos, providers (CoreML/DirectML/CUDA/CPU), scheduler de trabajos con prioridades, cache de fingerprint de audio.
- `content`: índices SQLite + embeddings (e5-small) para loops/samples/presets; cache en `~/.frutilla/cache`.
- `ui`: JUCE + OpenGL fallback; paletas por dominio; paneles con animación escalonada.
- `io`: drivers audio/MIDI, OSC/WebSocket para control remoto, recorder WAV.
- `persistence`: proyectos `.omg`, snapshots, migrador, autosave incrementales.
- `licensing`: lifetime key y gestor de updates.

## Servicios y contratos
- `StemSeparationService`
  - Entrada: `AudioClipRef`, opciones (stems: vocal/drums/bass/other, chunkSize, overlap, provider).
  - Pipeline: normalizar -> chunk OLA -> ORT session -> post-EQ -> write stems.
  - Salida: paths a stems + metadatos SNR/latencia; evento para playlist lanes.
- `DenoiseService`
  - ONNX (RNNoise/FullSubNet), cola background; control de artefactos.
- `LoopStarterService`
  - Query por tags (genre/BPM/key/mood); generador Markov/transformer pequeño para estructura; coloca patterns 8–16 compases en playlist.
- `ChordGenerator`
  - API: `generate(key, scale, density, voiceLeading=true)` -> MIDI clip con voicings drop-2/4; modos: acompañamiento/lead.
- `GopherAssistant`
  - Herramientas: explain routing, gain staging, sugerir voicings, crear sidechain. Operan sobre grafo y devuelven acciones aplicables.
- `RemoteControl`
  - Servidor WebSocket `/remote`; mensajes OSC-like; PWA con transport, mixer, pads; pairing code + mTLS opcional.
- `RackProcessor`
  - Slots dinámicos con drag/drop; dry/wet por slot; soporta AUv3 bridge/iOS y FX internos.

## Concurrencia y seguridad RT
- Audio thread: sin locks, sin alloc; usa pools y FIFOs. ML solo en workers.
- Workers: cola lock-free MPMC; prioridades (RT-adjacent para stems/denoise; normal para búsqueda semántica).
- Smoothing: parámetros con ramp y oversampling opcional.

## Almacenamiento y assets
- `Assets/Models`: ONNX (stems, denoise, pitch). Versionados con checksum y proveedor recomendado.
- `Assets/Content`: samples/presets/templates con metadatos JSON (BPM/key/genre/tags, hash, loudness).
- Project tree: `Audio/`, `MIDI/`, `Stems/`, `Cache/`, `Renders/`.

## Testing
- Audio golden: impulso/step para FX; budget CPU por bloque.
- ML: SNR/SDR para stems; MOS simulado para denoise.
- MIDI: roundtrip latency y jitter; MPE lanes.
- UI: snapshots de paneles; comandos de Gopher en dry-run.
- Host: fuzzing de plugins y crash-guard.

## Integración CMake/JUCE
- ORT opcional (`-DENABLE_ORT=ON`); buscar provider según plataforma.
- Build macOS: CoreML EP + NEON; Windows: DirectML/CUDA si disponible.
- Flags: `ENABLE_STEMS`, `ENABLE_DENOISE`, `ENABLE_REMOTE`, `ENABLE_AI_ASSISTANT` para compilar selectivamente.

## Código añadido (stubs iniciales)
- `Source/Audio/AI/StemSeparation.*`: separación mejorada por bandas con énfasis de transientes y normalización opcional.
- `Source/Audio/AI/OnnxRuntimeWrapper.*`: envoltura mínima para sesiones ORT (opcional por flag).
- `Source/Audio/AI/AIJobQueue.h`: cola de trabajos con `ThreadPool` para tareas AI.
- `Source/Audio/AI/AIServiceStubs.h`: esqueletos para Loop Starter y Gopher Assistant con hooks de progreso/acciones y generación básica de clips.
- `Source/Workflow/LoopStarterIntegration.h`: integra LoopStarter al PlaylistEngine (crea patrones/instancias por tipo de clip).
- `Source/Workflow/GopherCommands.h`: registra comandos por defecto (listar, generar loops base) sobre Playlist.
- `Source/Audio/Effects/PremiumFX.*`: FX prioritarios (Emphasis, Emphasizer, LuxeVerb, PitchShifter, TransientProcessor, GrossBeatLite).
- `Source/Remote/RemoteServer.*`: servidor TCP simple para control remoto (placeholder, 30 Hz polling).
- `Source/Mobile/MobileRackBridge.h`: stub de rack móvil/AUv3 bridge.
- `Source/Remote/RemoteAPI.h`: manejador JSON de comandos (transport, mixer, pads) para el socket remoto.
