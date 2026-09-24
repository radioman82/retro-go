# Amiga 500 (UAE4all / Musashi) auf ESP32-S3 – Experiment, Entwicklungshistorie & Fazit

## 1. Überblick & Ausgangslage
Dieses Repository dokumentiert die experimentelle Portierung und Optimierung eines Amiga 500 Emulators für das **Retro-Go** Framework auf dem **ESP32-S3 (DevKitC-1 N16R8: 16 MB Flash, 8 MB Octal PSRAM @ 240 MHz)**.

Ziel des Projekts war es, Amiga 500 OCS-Spiele (im ADF-Format) mit Kickstart 1.3 direkt auf einem kompakten Handheld lauffähig zu machen. Im Laufe der Entwicklung wurden **zwei grundlegend verschiedene technische Architekturen/Cores** evaluiert.

---

## 2. Die beiden evaluierten technischen Architekturen

### Basis 1: Der Musashi-Core (`amiga500-esp32` / MAME-basiert)
* **Technischer Ansatz:**
  * Verwendung des weit verbreiteten, extrem akkuraten C-Kerns **Musashi** (`m68kcpu.c`, generiertes `m68kops.c` mit > 800 KB Code) kombiniert mit einer portierten OCS-Emulation (`blitter.cpp`, `custom.cpp`, `paula.cpp`, `video.cpp`).
  * Gesichert im Projektarchiv unter `backup_stand_a500_musashi_frozen/`.
* **Ergebnisse & Probleme von Basis 1:**
  * *Speicherlimitierungen:* Musashi erzeugt gigantische C-Switch-/Jump-Tabellen. Frühere Versuche unter Arduino scheiterten am internen DRAM-Limit des ESP32.
  * *Mangelnde Performance:* Da Musashi für absolute Cycle-Genauigkeit auf Desktop-CPUs (MAME) ausgelegt ist und jeden Speicherzugriff über tiefe C-Callbacks abwickelt, brach die Framerate auf dem ESP32-S3 auf unspielbare **~5 bis 10 FPS** ein.
  * *Entscheidung:* Basis 1 wurde als zu schwerfällig für eine 240 MHz MCU eingefroren.

### Basis 2: Der UAE4all-Core mit FAME-C (Embedded-Optimiert)
* **Technischer Ansatz:**
  * Umstieg auf **UAE4all** mit dem für Handhelds (GP2X, Dingoo, Dreamcast) geschriebenen **FAME-C 68000 Core** (`famec.cpp` von Fox68k).
  * FAME-C nutzt eine kompakte Jump-Table (64k Einträge), direkte Speicher-Makros (`maccess.h`) und Scanline-orientiertes Custom-Chip-Scheduling.
* **Ergebnisse von Basis 2:**
  * Signifikanter Performance-Sprung gegenüber Musashi: Framerate stieg von <10 FPS auf **~25 bis 30 FPS**.
  * Ton (Paula DMA über I2S), Kickstart 1.3 und Retro-Go Menüintegration funktionierten grundsätzlich.
  * ADFs konnten geladen werden, Maus/Joystick-Umschaltung war erfolgreich.

---

## 3. Erreichte Meilensteine (Aktueller Stand Basis 2)
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

## 4. Technische Hürden & Flaschenhälse (Warum auch Basis 2 an Grenzen stößt)

Trotz des deutlichen Fortschritts von FAME-C gegenüber Musashi erreicht auch Basis 2 keine stabilen 50 FPS PAL und keinen ruckelfreien Ton. Die Ursachen sind hardware-architektonischer Natur:

### A. Das PSRAM-Latenz- und Bandbreiten-Problem (Hauptursache)
* Der ESP32-S3 verfügt pro Xtensa LX7 Core nur über **32 KB L1 Daten-Cache** und **32 KB L1 Instruction-Cache**. Er besitzt keinen L2-Cache.
* Der Amiga 500 benötigt mindestens 512 KB Chip-RAM, 512 KB Kickstart-ROM sowie Line- und Bitplane-Puffer. Dieser gesamte Speicherraum (über 1 MB) liegt im **externen PSRAM**, der über einen seriellen SPI-Bus angebunden ist.
* Jeder L1-Cache-Miss kostet die CPU **30 bis 50 Taktzyklen Wartezeit**. Da der Amiga-Speicherraum den 32 KB Cache um das 30-fache übersteigt, kommt es zu permanentem Cache-Thrashing.
* Auf Plattformen, für die UAE4all ursprünglich flüssig lief (ARM Cortex mit 32-Bit breitem internen DDR-RAM-Bus und L2-Cache), existiert dieser Flaschenhals nicht.

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

## 5. Vergleich der beiden Cores

| Eigenschaft | Basis 1: Musashi (`amiga500-esp32`) | Basis 2: UAE4all / FAME-C (Aktuell) |
| :--- | :--- | :--- |
| **Zielplattform** | Desktop (MAME), hohe Genauigkeit | Embedded (GP2X, Dreamcast) |
| **Code-Größe** | Sehr groß (>800 KB allein `m68kops.c`) | Kompakt, 64k Opcode-JumpTable |
| **Speicher-Overhead** | Extrem hoch (DRAM-Überlauf) | Ausreichend für ESP32-S3 |
| **Framerate (ESP32-S3)** | **~5 – 10 FPS** (unspielbar) | **~25 – 30 FPS** (teilw. spielbar) |
| **Retro-Go Integration** | Isoliert / Standalone | Vollständig integriert mit UI & Audio |
| **Status** | Eingefroren (`backup_stand_a500_musashi_frozen`) | Aktiver Proof-of-Concept |

---

## 6. Gesamtfazit & Realistische Bewertung
Die Portierung des Amiga 500 auf den ESP32-S3 ist ein **faszinierendes und technisch lehrreiches Experiment**:
1. Der Wechsel von **Musashi** zu **FAME-C/UAE4all** brachte eine Verdreifachung der Geschwindigkeit (von 8 FPS auf ~27 FPS) und machte den Amiga im Retro-Go Menü überhaupt erst lauffähig.
2. Für den alltäglichen, flüssigen Spielbetrieb (wie ihn Retro-Go für NES, Game Boy, SMS, Genesis, Doom etc. bietet) ist der ESP32-S3 jedoch aufgrund der begrenzten Speicherbandbreite und des Cache-Miss-Overheads zum seriellen PSRAM **leistungsmäßig überfordert**. Vollwertiges, flüssiges PAL-Gaming mit 50 Hz und synchronem Ton ist auf dieser Hardwareklasse mit C-basierten Emulatoren nicht ohne monatelangen Neuentwurf (wie handgeschriebener Xtensa-Assembler-Core) erreichbar.

Der Amiga-Core verbleibt daher als **experimenteller Proof-of-Concept** im Projekt archiviert.
