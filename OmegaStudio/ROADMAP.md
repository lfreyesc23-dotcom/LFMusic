# OmegaStudio Roadmap — Entrega FL-Studio-Grade

## Principios
- Calidad de audio y RT-safety primero (sin bloqueos en audio thread; lock-free, pools, SIMD).
- Paridad de UX FL con mejoras modernas: flujos rápidos, accesos de teclado, tooltips con intención.
- ML acelerado (CoreML/DirectML/CUDA) con degradación a CPU y flags de capacidad.
- Entregas incrementales con compatibilidad hacia adelante; snapshots de proyecto y migradores.

## Fase M1 — Núcleo y ML base (semanas 1-4)
- Infra: integrar ONNX Runtime (CoreML macOS, DirectML Win) y sistema de jobs RT-safe (cola lock-free + workers).
- Stems: `StemSeparationProcessor` con OLA, cache por fingerprint, export a lanes dedicadas.
- Denoise: RNNoise/FullSubNet ONNX en background con control de artefactos.
- Grabación avanzada: commit atómico de WAV, slip, fades no destructivas, time-sig por clip.
- Playlist: lanes audio/MIDI/automation; marcadores de compás/tempo; ghost notes.
- Mixer: PDC, mid/side, oversampling por plugin, medidores LUFS/TP.
- Piano Roll: escalas, guías de acorde, cuantización avanzada, strum/humanize, MPE.
- Calidad: tests golden audio (impulso/step), benchmarks SNR stems, latencia MIDI roundtrip.

## Fase M2 — Creatividad y asistente (semanas 5-8)
- Loop Starter: servicio con DB de loops (BPM/tono/género/tags) + generador 8-16 compases auto-arrange.
- Gopher AI: paleta de comandos + chat dock; herramientas: explicar ruteo, gain staging, voicings.
- Chord Generator: motor armónico (circle-of-fifths, drop-2/4, inversiones adaptativas), export a MIDI clip.
- Automatización: splines cúbicos, LFO tempo-sinc, modo step, smoothing por parámetro.
- Remote: servidor `/remote` WebSocket + PWA (transport, mixer faders, pads) con pairing code y opcional mTLS.
- UX: tema audaz con codificación de color por dominio, animaciones de entrada escalonadas en paneles pesados.
- Calidad: snapshots UI, fuzzing host de plugins, integración grabar→editar→mezclar→exportar.

## Fase M3 — FX e instrumentos estrella (semanas 9-14)
- FX premium: Emphasis (master comp/limiter multi-stage), Emphasizer (tilt/air), LuxeVerb (FDN/plate), PitchShifter (PSOLA/Elastique), Transient Processor, GrossBeat-like, Patcher modular.
- FX core “Fruity”: banco de delays, filtros, moduladores, dynamics como bloques reutilizables.
- ML mastering: Assistant/SmartEQ/Mix Analyzer refinados con perfiles por género.
- Instrumentos: Harmor (aditivo+resíntesis), Sytrus/Toxic (FM/híbrido), FLEX-style rompler, Fruity Slicer 2, DirectWave sampler (disk streaming, round robins), Drumaxx/3xOSC/GMS.
- Plugin hosting: sandbox helper-process VST3/AU/CLAP, crash guard + autosave de estado, oversampling por plugin.
- Contenido: librería de samples/presets con metadatos (BPM/key/genre/tags) indexada en SQLite + búsqueda semántica cacheada.

## Fase M4 — Experiencia avanzada y mobile (semanas 15-20)
- FL Studio Mobile Rack + FX: rack modular `RackProcessor` con drag/drop, dry/wet por slot; bridge AUv3/IPC.
- Remote avanzado: WebRTC opcional para baja latencia; control de pads, mixer y macros.
- Editor “Newtone”/“Newtime”: pitch-correct + time-align con formantes; warping elástico por clip; detecta key/scale.
- Video/visuals: reproductor de video en timeline y visualizador reactivo (Wave Candy-style).
- Licenciamiento y updates: lifetime key, gestor de actualizaciones y migrador de proyectos.
- Resiliencia: autosave por bloques, recuperación de crash, watchdog de latencia, telemetría opt-in.

## Riesgos y mitigaciones
- Carga ML alta → uso de progresos + cancelación y downsample adaptativo.
- Sandbox plugins → IPC ligero y snapshots de estado al crashear.
- UX compleja → densidad ajustable, presets de layout, teclado completo.

## QA continua
- CI matriz macOS/Windows con/sin aceleración ML.
- Golden tests de audio por versión; budget de CPU por bloque.
- Benchmarks de stems/denoise; fuzzing de host; pruebas de regresión de proyectos.
