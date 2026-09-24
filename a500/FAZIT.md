# Amiga 500 (UAE4all) auf ESP32-S3 – Experiment & Fazit

## 1. Überblick & Ausgangslage
Dieses Verzeichnis (`a500` sowie `components/a500`) enthält die experimentelle Portierung des Amiga 500 Emulators (basierend auf UAE4all / FAME-C 68000) in das **Retro-Go** Framework für das Target **ESP32-S3 (DevKitC-1 N16R8: 16 MB Flash, 8 MB Octal PSRAM @ 240 MHz)**.

Ziel des Experiments war es, Commodore Amiga 500 OCS-Spiele (im ADF-Format) mit Kickstart 1.3 direkt auf dem Handheld lauffähig zu machen.

---

## 2. Erreichte Meilensteine (Was funktioniert)
* **Vollständige Retro-Go Integration:**
  * Der Core ist in `rg_tool.py` und im `launcher` als eigenes System ("Amiga 500", Dateiendung `.adf`) registriert.
  * Automatische Erkennung des Kickstart 1.3 ROMs auf der SD-Karte (`/sd/retro-go/bios/kick13.rom` oder `/sd/roms/a500/kick13.rom`).
  * Bootet sauber in den Kickstart 1.3 Startbildschirm („Hand mit Diskette“), wenn kein ADF geladen ist.
* **Display & Grafik-Pipeline:**
  * Display-Ausgabe über Retro-Go Surfaces (320x240 RGB565) mit Triple-Buffering.
  * Asynchroner SPI-DMA Transfer auf Core 1 entlastet den Emulations-Thread auf Core 0.
* **Eingabe (Gamepad & Maus):**
  * Joystick Port 2 Gray-Code-Emulation (Amiga `JOY1DAT` Quadratur-Kodierung) für D-Pad und Buttons (A/B).
  * Umschaltbarer Mausmodus über `SELECT` (D-Pad steuert Mauszeiger, A/B Mausklicks) für Cracktros, Menüs und Desktop.
  * Tastatur-Shortcuts (Space, Return, ESC) auf den Funktionstasten.
* **Audio:**
  * Paula 4-Kanal DMA-Sound wird in den Retro-Go I2S Ringpuffer gestreamt.

---

## 3. Technische Hürden & Flaschenhälse (Warum es hakt)

Trotz intensiver Optimierungsversuche erreicht die Emulation bei anspruchsvollen Spielen keine stabilen 50 FPS PAL und keinen ruckelfreien Ton. Die Ursachen sind hardware-architektonischer Natur:

### A. Das PSRAM-Latenz- und Bandbreiten-Problem (Hauptursache)
* Der ESP32-S3 verfügt pro Xtensa LX7 Core nur über **32 KB L1 Daten-Cache** und **32 KB L1 Instruction-Cache**. Er besitzt keinen L2-Cache.
* Der Amiga 500 benötigt mindestens 512 KB Chip-RAM, 512 KB Kickstart-ROM sowie Line- und Bitplane-Puffer. Dieser gesamte Speicherraum (über 1 MB) liegt im **externen PSRAM**, der über einen seriellen SPI-Bus angebunden ist.
* Jeder L1-Cache-Miss kostet die CPU **30 bis 50 Taktzyklen Wartezeit**. Da der Amiga-Speicherraum den 32 KB Cache um ein Vielfaches übersteigt, kommt es zu permanentem Cache-Thrashing.
* Auf Plattformen, für die UAE4all ursprünglich optimiert wurde (ARM Cortex mit 32-Bit breitem internen DDR-RAM-Bus und L2-Cache), existiert dieser Flaschenhals nicht.

### B. Rechenaufwand der Amiga-Custom-Chips
* Der Amiga 500 ist kein einfacher Spielkonsolen-Chip, sondern ein komplexes Multiprozessor-System (Motorola 68000 @ 7.09 MHz, Agnus mit Blitter und Copper, Denise mit planarer Bitplane-Grafik, Paula mit 4-Kanal-DMA-Sound und CIAs).
* Pro PAL-Frame (20 ms) müssen neben Millionen CPU-Zyklen auch alle Custom-Chip-Register zeilenweise synchronisiert werden.
* Auf dem ESP32-S3 @ 240 MHz benötigt der UAE4all-Core für einen einzigen Amiga-Frame in der Praxis **33 bis 40 ms Rechenzeit**.
* Das physikalische Limit des Systems liegt dadurch bei ca. **25 bis 28 Frames pro Sekunde**.

### C. Audio-Stottern durch Framerate-Defizit
* Da der Emulator nur ~25–28 PAL-Frames pro Sekunde berechnen kann, erzeugt auch der Paula-Soundchip pro realer Sekunde nur Audiodaten für 25–28 Frames anstelle der geforderten 50 Frames.
* Der I2S-DMA-Puffer läuft dadurch unweigerlich leer (Buffer Underrun), was zu ständigen Tonaussetzern, Knacken und Verzögerungen führt.

### D. Blitter- und Scanline-Timing (Kompatibilitätsprobleme)
* Versuche, den Blitter künstlich zu beschleunigen (*Immediate Blits*), brechen Spiele, die Cycle-Exact auf den Blitter warten oder den Blitter über den Copper synchronisieren (z. B. *1943*, das beim Booten hängenbleibt, oder Sprite-Glitches in anderen Spielen).
* Das Überspringen von VBLANK-Rasterzeilen (`m68k_speed > 1`) zerstört das Interrupt-Timing vieler Spieletitel.

---

## 4. Gesamtfazit & Realistische Bewertung
Die Portierung des Amiga 500 auf den ESP32-S3 ist ein **faszinierendes und technisch lehrreiches Experiment**, das beweist, dass selbst hochkomplexe 16-Bit-Computer-Architekturen auf einem modernen Mikrocontroller grundsätzlich booten und lauffähig sind.

Für den alltäglichen, flüssigen Spielbetrieb (wie ihn Retro-Go für NES, Game Boy, SMS, Genesis, Doom etc. bietet) ist der ESP32-S3 aufgrund der begrenzten Speicherbandbreite und des Cache-Miss-Overheads zum PSRAM jedoch **leistungsmäßig überfordert**. Vollwertiges, flüssiges PAL-Gaming mit 50 Hz und synchronem Ton ist auf dieser Hardwareklasse mit der vorliegenden UAE4all-Architektur nicht ohne einen grundlegenden, monatelangen Neuentwurf (wie z. B. handgeschriebener Xtensa-Assembler-Core) erreichbar.

Der Amiga-Core verbleibt daher als **experimenteller Proof-of-Concept** im Projekt.
