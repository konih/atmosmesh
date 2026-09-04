# Elektronik-Inventar

> Stand: 25. August 2026, ergänzt am 28. August 2026 und am 4. September 2026
> Dieses Inventar basiert auf der gemeinsamen Durchsicht und Identifikation. Unsichere Stückzahlen bzw. Varianten sind entsprechend markiert.

![[Images/Elektronik-OLED-und-Zweipol-Bauteil-2026-08-17.jpg|480]]

## Computer & Mikrocontroller

- 1× Raspberry Pi 1 Model B (Modell am 4. September 2026 bestätigt; nur ein Pi 1 insgesamt)
- 1× Raspberry Pi 3 Model B Rev. 1.2
- 1× Raspberry Pi Zero 2 W
- 1× Arduino Uno
- Arduino Nano-kompatible Boards – mehrere vorhanden, genaue Anzahl später zählen
- 2× STM32 „Blue Pill“ / STM32F103C8T6
- 1× ESP32-CAM
- 1× ESP32-CAM-MB – Programmier-/USB-Board für ESP32-CAM
- 3× ESP32-Devboards insgesamt
  - 1× ESP32-Devboard mit ESP-WROOM-32
    - VIN/5V-Eingang
    - Onboard-3,3-V-Regler
    - 3V3-Ausgang
    - konkrete Board-Ausführung per Foto identifiziert
  - 2× weitere ESP32-Devboards
    - genaue Varianten später bestimmen
- 2× ideaspark-ESP32-Boards mit Display oben, ESP-WROOM-32 (Ergänzung 4. September 2026)
  - 1× ideaspark ESP32 1,14"-TFT-LCD-Board: der Controller des AtmosMesh-Room-Aufbaus
    (`hardware/kicad/atmosmesh-room/README.md`, Pinout in
    `hardware/ideaspark-esp32-tft-pinout.png`)
  - 1× weiteres Board derselben Bauart, vom Operator als „mit OLED oben“ bezeichnet. ideaspark
    liefert diese Boards mit 0,96"-OLED (monochrom, I²C) **oder** 1,14"-TFT (Farbe, SPI); welche
    Variante das ist, am Display und am Aufdruck prüfen. Nur die TFT-Variante ist pinkompatibel zum
    Room-Träger
  - möglicherweise sind das zwei der oben als „2× weitere ESP32-Devboards“ geführten Boards;
    beim nächsten Zählen auflösen
- 1× Heltec WiFi LoRa 32 V2
  - LoRa 866–915 MHz
  - 0,96" OLED
  - 128×64 Pixel
- ESP8266MOD / ESP8266-Boards – mehrere vorhanden, genaue Varianten und Anzahl später zählen
- 2× GR-Sakura / Renesas RX63N Boards – unterschiedliche Ausführungen
- 1× unbekanntes Board „WCBN64“ – noch zu identifizieren

## Sensoren

- 4× SHT41 (Stückzahl korrigiert am 28. August 2026, zuvor 1×)
  - digitaler Temperatur- und Luftfeuchtigkeitssensor
  - I²C, feste Adresse **0x44** (Variante SHT4x-Bxxx: 0x45)
  - Breakout-Pinreihenfolge operator-bestätigt: **VIN / GND / SCL / SDA**
  - am 25. August 2026 als eingetroffen erfasst
- 4× VEML7700 (Ergänzung 28. August 2026)
  - Umgebungslichtsensor, Lux
  - I²C, **feste Adresse 0x10**
  - Breakout-Pinreihenfolge operator-bestätigt: **VIN / 3Vo / GND / SCL / SDA**; `3Vo` ist der
    Reglerausgang des Breakouts und bleibt unbeschaltet

> **Beide Typen haben eine feste I²C-Adresse.** Zwei VEML7700 oder zwei SHT41 der gleichen Variante
> können deshalb *nicht* an denselben Bus. Die Mehrfachbestände sind Ersatz beziehungsweise
> ermöglichen parallele Aufbauten — Room, Aqua und Bench — nicht mehrere gleiche Sensoren an einem
> Bus. Für mehrere Sensoren desselben Typs wäre ein I²C-Multiplexer nötig; ein solcher ist im
> Bestand nicht erfasst.
- 2× ENS160 + AHT20 Kombimodul (Ergänzung 4. September 2026, vom Operator abgelesen, nicht per
  Foto bestätigt)
  - ScioSense ENS160: digitaler Metalloxid-Multigassensor, liefert TVOC, einen Luftqualitätsindex
    und einen **eCO₂-Schätzwert**. eCO₂ wird aus dem VOC-Signal abgeleitet und ist **keine
    CO₂-Messung**; als CO₂ darf nur der SCD41 bezeichnet werden. Gleiche Regel wie beim MQ135
  - AHT20: Temperatur- und Luftfeuchtigkeitssensor; liefert dem ENS160 die Kompensationswerte
  - I²C. ENS160 auf 0x52 oder 0x53 (ADDR-Pin), AHT20 **fest auf 0x38**. Zwei dieser Module an
    einem Bus kollidieren daher beim AHT20; die zwei Exemplare sind für zwei Aufbauten, nicht für
    einen Bus
  - Operator: 8 Pins. Übliche Belegung dieser Module ist VIN, 3V3, GND, SCL, SDA, CS, ADD, INT;
    am Modul vor dem Anschluss ablesen
  - Versorgung: der ENS160-Kern läuft mit 1,8 V, Module tragen dafür einen eigenen Regler. Bis
    Regler und Pull-ups am Modul geprüft sind, gilt **3V3** wie bei allen I²C-Breakouts
  - der ENS160 heizt: nach dem Einschalten Aufwärmzeit im Minutenbereich, beim ersten Betrieb ein
    deutlich längerer Einlauf; Werte dazu aus dem Datenblatt übernehmen, nicht raten
  - alle Angaben außer Stückzahl und Pinzahl sind Typenwissen, keine Prüfung am Exemplar
- 1× Nova Fitness SDS011 Feinstaubsensor
  - PM2.5 / PM10
  - zusammen mit USB-Adapter/Kabel USB2TT004
- 1× MQ135 Gassensor
- 1× MQ-3 Gassensor
  - primär empfindlich auf Alkohol-/Ethanoldämpfe
  - nur für relative Trends und Experimente vorgesehen; kein kalibriertes Messgerät
- 1× MQ-7 Gassensor
  - primär für Kohlenmonoxid vorgesehen
  - korrekter Betrieb erfordert einen zyklisch angesteuerten Heizbetrieb; konkrete Modulschaltung noch prüfen
  - nicht als Ersatz für einen zertifizierten CO-Warnmelder verwenden
- 1× AM2302 / DHT22
  - Temperatur
  - Luftfeuchtigkeit
- 1× DHT11
  - Temperatur
  - Luftfeuchtigkeit
  - physisch vorhanden; möglicherweise identisch mit dem B24-Modul aus dem 4duino SensorKit
- 1× YL-69 / Y69 Bodenfeuchtesensor
- 6× BME280 Breakout (Ergänzung 4. September 2026, abgelesen, nicht per Foto bestätigt; fünf
  aus der Zählung vom selben Tag plus ein weiteres gefundenes Exemplar, Gesamtzahl bestätigen)
  - Bosch BME280: Luftfeuchtigkeit, Luftdruck und Temperatur in einem Chip; damit der einzige
    Sensor im Bestand, der alle drei Größen liefert. Nicht mit dem BMP280 verwechseln, der keine
    Feuchte misst
  - I²C-Adresse 0x76 oder 0x77 über SDO, wie beim BMP280; auch SPI möglich
  - Breakout-Variante, Versorgung, Pull-ups und Pinfolge am Modul ablesen; bis dahin **3V3**
  - Typenwissen, nicht am Exemplar geprüft
- 1× GY-BMP280
  - Luftdruck
  - Temperatur
  - keine Luftfeuchtigkeitsmessung
  - 6-Pin-Breakout mit VCC, GND, SCL, SDA, CSB und SDO
  - unterstützt I²C und SPI; für AtmosMesh ist I²C vorgesehen
  - geplante I²C-Beschaltung: CSB an 3,3 V, SDO an GND → Adresse `0x76`
  - noch prüfen, ob das konkrete Breakout CSB/SDO oder I²C-Pull-ups bereits onboard beschaltet
- 1× BMP180-Modul
  - Luftdruck
  - Temperatur
  - I²C-Adresse üblicherweise `0x77`; konkrete Breakout-Versorgung und Pegel noch prüfen
- 1× Water Sensor, 3 Pins
- 1× PIR-Bewegungsmelder-Modul
  - Hersteller/Markierung: D-SUN
  - Anschlüsse: VCC, OUT, GND
  - weiße Fresnel-Kuppel
  - genaue Modellnummer unbekannt
- 1× Mikrofon-/Sound-Sensor-Modul
  - Markierung: HC-20
  - genaue Variante, Pinbelegung und Versorgung noch bestimmen
- 1× Fotowiderstand / LDR, lose bedrahtete Ausführung
  - durch Nahaufnahme eindeutig anhand der mäanderförmigen Fotoleiterbahn identifiziert
  - kleines orangefarbenes Scheibenbauteil, ungefähr 5-mm-Bauform, mit zwei langen Anschlussdrähten
  - Widerstand sinkt typischerweise bei zunehmender Beleuchtung
  - vermutlich GL55xx-Familie; genaue Variante, beispielsweise GL5528, und Kennwerte noch bestimmen
  - Foto: ![[Images/Elektronik-Fotowiderstand-LDR-2026-08-17.jpg|240]]
- 1× Thermoelement / Thermocouple – genauer Typ noch unbekannt
- 1× ZS-100 – vermutlich optischer/Lichtsensor; genaue Identifikation noch offen
- diverse IR-/Infrarot-Sensormodule
- diverse Hall-Sensoren
- diverse Neigungs-/Schalt-/Sensorboards aus dem 4duino SensorKit

## Displays & LEDs

- 2× 3,5" SPI TFT LCD für Raspberry Pi
  - Markierung/Hersteller: KDI
  - 480×320 Pixel
- 1× kleines monochromes OLED-Display, blau
  - 128×64 Pixel
  - 4-Pin-I²C-Modul
  - auf dem fotografierten Modul ist eine Adressauswahl `0x78` / `0x7A` aufgedruckt; dies entspricht üblicherweise den 7-Bit-I²C-Adressen `0x3C` / `0x3D`
  - Controller vermutlich SSD1306-kompatibel; vor Einsatz per I²C-Test beziehungsweise Bibliothek prüfen
  - konkrete Pinreihenfolge am Header vor Anschluss anhand der Vorderseitenbeschriftung prüfen
  - am 17. August 2026 neu und separat aufgenommen
- 1× kleines OLED-Display
  - 128×32 Pixel
  - bereits in einem Gerät verbaut
  - Schnittstelle, Controller und konkrete Pinreihenfolge noch prüfen
- 1× LCD 1602 I²C V1
  - 16×2 Zeichen
- 1× 4-stelliges 7-Segment-Display
- 1× 8×8 LED-Matrix
- 1× 4-fach 8×8 LED-Matrix-Modul
  - vier verkettete 8×8-Matrizen
  - insgesamt 32×8 / 256 LEDs
  - vermutlich monochrom
- 10× RGB-LED-Dot-/Pixel-Module
  - kleine runde PCBs
  - genauer LED-/Controller-Typ später bestimmen
- weitere RGB-LED-Module, u. a. 4-Pin; genaue zusätzliche Stückzahl noch zählen
- weitere LED-Module aus dem 4duino SensorKit
- Sortiment bedrahteter Einzel-LEDs (Ergänzung 28. August 2026)
  - gängige Farben, 3 mm und 5 mm
  - größere Stückzahlen je Farbe
  - genaue Farbaufteilung, Flussspannungen und Stückzahlen später zählen
  - Vorwiderstand ist immer separat zu wählen; die E24-Widerstandsreihe liegt vollständig vor

## ADC / Analog

- 2× ADS1115 ADC + PGA Breakout (Stückzahl am 4. September 2026 gezählt; zuvor „mehrere
  vorhanden“)
  - 16 Bit, 4 Eingänge (single-ended) oder 2 Differenzpaare, programmierbarer Verstärker (PGA)
  - I²C, Adresse 0x48 bis 0x4B über den ADDR-Pin (an GND, VDD, SDA oder SCL); beide Module
    können an einem Bus sitzen
  - Versorgung 2,0–5,5 V; für ESP32-Aufbauten **3V3**, und Eingangsspannungen dürfen VDD nicht
    überschreiten
  - für das Room-Board nicht vorgesehen (D-032 in `agent-context/decisions.md`)
- Potentiometer-Module
- Joystick-Controller-Board, Keyes-SJoys, 5 Pins

## Strom- und Leistungsmessung

- 1× CJMCU-226 Modul mit INA226 (Ergänzung 4. September 2026, abgelesen, nicht per Foto
  bestätigt; Stückzahl 1 angenommen)
  - Texas Instruments INA226: bidirektionaler Strom-, Spannungs- und Leistungsmonitor, 16 Bit,
    I²C. Misst die Spannung über einem Shunt (±81,92 mV Vollausschlag) und die Busspannung
    0–36 V; Strom und Leistung werden im Chip berechnet. Typenwissen, nicht am Exemplar geprüft
  - I²C-Adresse über A0/A1 wählbar, 0x40 bis 0x4F; mehrere Module an einem Bus sind damit möglich
  - Versorgung 2,7–5,5 V; für ESP32-Aufbauten **3V3**, damit die I²C-Pegel stimmen
  - Operator: 8 Pins. Übliche Belegung dieser Module: VCC, GND, SDA, SCL, ALE (Alert), VBS
    (Busspannung), IN+, IN−; am Modul ablesen
  - **Shuntwert am Modul ablesen.** CJMCU-226-Boards tragen meist 0,1 Ω (Aufdruck `R100`), was
    ±0,8 A Messbereich bedeutet; manche 0,01 Ω für ±8 A. Ohne den Wert ist jede Stromangabe
    falsch skaliert
  - Messung high-side oder low-side möglich, Gleichtaktbereich 0–36 V

  **Konstruktive Folge:** Der tatsächliche Strom der 5-V-Domäne auf dem Room-Board (SDS011-Lüfter
  und -Laser, Tastverhältnis) lässt sich damit im Betrieb loggen statt nur am Labornetzteil
  abzulesen. Die Aussage in `hardware/spec-comparison.md` zu rund 650 mA Spitzenkoinzidenz wäre
  damit messbar, nicht nur gerechnet.

## Funk & Kommunikation

- 1× RFID-RC522-Modul
  - RFID-/NFC-Lesemodul auf Basis MFRC522
- 1× 315/433-MHz-RF-Sender-/Empfänger-Set
  - Empfänger: XY-MK-5V
  - Sender: FS1000A beziehungsweise kompatible Variante
  - einfache ASK-/OOK-Funkmodule
  - auf den Modulen finden sich Angaben zu 315 beziehungsweise 433 MHz; konkrete Frequenzzuordnung vor Einsatz prüfen
- 1× RFM12S V4.0
  - HopeRF-Funk-Transceiver (sendet und empfängt), **FSK**, gesteuert per SPI, **3,3 V**; keine
    5-V-Logik anschließen
  - Band ist je Modul fest (433, 868 oder 915 MHz) und meist aufgedruckt; am Modul ablesen
  - spricht wegen der anderen Modulation **nicht** mit den ASK-Modulen FS1000A/XY-MK-5V oder
    H34A/H3V4F, nur mit einem weiteren Modul der RFM12-Familie
- 1× 433-MHz-Set bestätigt (4. September 2026): Tüte mit FS1000A und XY-MK-5V trägt die Angabe
  433 MHz; die Frequenzfrage zu diesem Set ist damit beantwortet
- 2× H34A Sender + 2× H3V4F Empfänger, 433 MHz (Stückzahl und Zuordnung am 4. September 2026
  vom Operator bestätigt; zuvor 1× H34A mit offener Funktion)
  - H34A: ASK/OOK-Sender, 433,92 MHz. Der aufgelötete NDR4208 ist der zugehörige SAW-Resonator
    und legt die Frequenz fest
  - H3V4F: der passende ASK/OOK-Empfänger, 433 MHz
  - zweites 433-MHz-Set neben dem XY-MK-5V/FS1000A-Paar; beide Sets sind untereinander
    kompatibel, da gleiche Modulation und gleiches Band. Typenwissen, nicht am Exemplar geprüft
  - Versorgung und Pinbelegung (typisch VCC, DATA, GND, ANT) am Modul ablesen; Antennenlänge
    für 433 MHz rund 17 cm Draht
  - kein CO₂- oder anderer Raumluftsensor
- mehrere ESP8266MOD – siehe Mikrocontroller
- 1× Heltec WiFi LoRa 32 V2 – siehe Mikrocontroller

## Relais & Schalten

- mehrere Relaismodule
- mindestens 1× QIANJI/QIANJE JQC-3F (T73) Relaismodul
- 1× 8-Kanal-Relaismodul
  - mikrocontroller-/Arduino-kompatibel
  - acht einzeln schaltbare Relais
  - genaue Relais-, Trigger- und Boardvariante noch bestimmen
- weitere 5-V-Relaismodule aus dem Sensorsortiment

## Stromversorgung

Stückzahlen am 4. September 2026 vom Operator gezählt und abgelesen, nicht per Foto bestätigt.

- 3× kleines offenes AC/DC-Schaltnetzteil, Kennzeichnung 5V07 / 12V04 (zuvor 1×; das dritte
  Exemplar wurde als „5D07“ abgelesen, vermutlich dieselbe Familie, Aufdruck prüfen)
  - geplanter Einsatz: 5-V-Versorgung
  - die Familie gibt es als 5 V / 0,7 A **oder** 12 V / 0,4 A; welches jedes Board ist, steht
    nicht sicher fest. Ausgangsspannung und Anschlussbelegung **je Exemplar** vor Einsatz per
    Multimeter verifizieren und auf dem Board vermerken
- 4× SANMIM AC/DC-Schaltnetzteil SM-PLG06A, Variante SM-104-3.3V-02 (Stückzahl bestätigt; zuvor
  „mindestens 4“)
  - Ausgang: 3,3 V
  - als 3,3-V-Netzteil identifiziert; nicht mehr als unbekannt führen
- 2× SANMIM Wandlerboard, abgelesen als SM-PLB03A, Aufdruck „1603“ (vermutlich Datecode)
  - große Drossel mit Aufschrift „SM001“. Das ist die Spule, nicht das Board: der frühere Eintrag
    „Mini-Stromversorgungsmodul 5M001“ meinte dieses Board und ist damit erledigt
  - gleiche Baureihe wie das SM-PLG06A, also mit hoher Wahrscheinlichkeit ein **offenes
    AC/DC-Modul mit Netzspannungsseite**; bis zum Ablesen von Eingang und Ausgang so behandeln
  - Ausgangsspannung und -strom vom Aufdruck ablesen, sonst messen
- 1× Breadboard-Netzteilmodul (Bauart MB102), DC/DC, linear
  - Eingang: Hohlstecker, beschriftet 9 V (typisch 6,5–12 V), Ein/Aus-Taster
  - Ausgänge: 3,3 V und 5 V, je Schiene per Jumper wählbar; zwei AMS1117 (5,0 V und 3,3 V) auf
    dem Board, kein Schaltregler
  - Linearregler: bei 9 V Eingang fallen auf der 5-V-Schiene 4 V × I als Wärme ab, über etwa
    200–300 mA wird der Regler heiß. Nicht für den SDS011-Lüfter oder Servos vorsehen
  - keine Netzspannung, darf offen auf dem Breadboard betrieben werden
- weitere Stromversorgungsboards mit großen Spulen und Kondensatoren: nach dieser Zählung keine
  unzugeordneten mehr bekannt; taucht eines auf, hier eintragen

- 4× AMS1117-3.3 Linearregler, SOT-223 (Ergänzung 4. September 2026, abgelesen, nicht per Foto
  bestätigt); Aufdruck laut Operator „AMS1117 3.3 GMTC 1608“, wobei `1608` nach einem Datecode
  aussieht und `GMTC` Hersteller- oder Loskennung sein kann
  - 3,3-V-Festspannungs-LDO, 1 A, Dropout rund 1,1–1,3 V bei Volllast, Eingang bis 15 V.
    Typenwissen, nicht am Exemplar geprüft; unter diesem Namen laufen viele Nachbauten
  - Pinfolge SOT-223 mit Blick auf die Beschriftung: GND (Adjust), VOUT, VIN; die Fahne liegt auf
    VOUT. Am Datenblatt gegenprüfen, eine Verwechslung von VIN und VOUT legt 5 V auf die
    3,3-V-Schiene
  - braucht am Ausgang einen Kondensator mit ausreichender ESR-Lage (Datenblatt: 22 µF Tantal
    oder Elko); ein 100-nF-Keramik allein reicht nicht für Stabilität
  - **thermisch begrenzt:** P ≈ (5 V − 3,3 V) × I. Bei 500 mA sind das 0,85 W, in SOT-223 ohne
    Kupferfläche zu viel. Für Dauerlasten über etwa 300 mA Kupferfläche oder Kühlkörper vorsehen
  - Einsatz: Ersatz für den DevKit-Regler (Vorfall vom 17. August 2026,
    `hardware/incident-2026-08-17-ldo.md`) und als separater 5-V-auf-3,3-V-Regler, wenn die
    3,3-V-Last nicht mehr an das Devboard-`3V3` gehängt werden soll (siehe `hardware/power.md`)
- Sicherungshalter mit Sicherungen, Aufschrift laut Operator „D63“ (Ergänzung 4. September 2026,
  abgelesen, nicht per Foto bestätigt)
  - 10× mit 5-A-Sicherung, 10× mit 1-A-Sicherung
  - Sicherungsformat (vermutlich 5×20 mm), Bauform des Halters (Leiterplatte, Kabel, Panel) und
    ob „D63“ den Halter oder die Sicherung bezeichnet: am Teil ablesen
  - **Nennspannung des Halters ablesen, bevor er auf der Netzseite eingesetzt wird.** Ein Halter
    für 32 V DC auf der 230-V-Primärseite ist keine Absicherung, sondern ein Isolationsfehler
  - Auslösecharakteristik (flink/träge) der Sicherungen ablesen; sie steht nicht in der
    Stromangabe
  - naheliegende Zuordnung: die beiden 230-V/5-V-Netzteile mit 5 A und 1 A. Der Sicherungswert
    gehört zur abgesicherten Last, nicht zur Netzteilbezeichnung; eine 5-A-Sicherung vor dem
    1-A-Netzteil schützt nichts

> [!danger] Offene Netzspannungs-Netzteile
> Das 5V07/12V04-Board und das SANMIM SM-PLG06A sind offene AC/DC-Schaltnetzteile mit einer Netzspannungsseite. Sie sind **keine gewöhnlichen Breadboard-DC/DC-Module** und dürfen nicht offen oder ungeschützt wie Niederspannungsmodule verwendet werden. Messung und Betrieb nur in einem geeigneten berührungssicheren Aufbau durch eine entsprechend qualifizierte Person.

## Adapter & Schnittstellen

- USB-to-TTL-Adapter
- V-USB-TTL USB-to-TTL-Stick
- USB2TT004 USB-Adapter/Kabel beim SDS011
- Dupont-Steckverbinder-Sortiment, männlich und weiblich (Ergänzung 28. August 2026, präzisiert
  am 4. September 2026)
  - Stiftleisten männlich, 2,54 mm, umbrechbar; darunter 40-polige Stiftleisten
  - Buchsenleisten weiblich, 2,54 mm, umbrechbar
  - Crimpkontakte männlich und weiblich sowie einreihige Gehäuse mit etwa 1 bis 10 Positionen
  - Bestand als ausreichend bestätigt; genaue Stückzahlen und Reihenlängen nicht gezählt
  - vor Einsatz je Verbindung prüfen: Sitz der Crimpung, Strombedarf und Pinzuordnung. Ein
    Dupont-Kontakt ist für Signale und kleine Ströme gedacht, nicht für die 5-V-Versorgung eines
    Lüfters oder Servos ohne Nachrechnen
  - für den Room-Aufbau relevant: U1 braucht zwei 1×15-Buchsenleisten im Rastermaß 2,54 mm

## Eingabe / Bedienelemente

- 1× 4×4 Tastenmatrix / Button-Raster
- 1× Keyes-SJoys Joystick-Modul, 5 Pins
- 1× Joystick-/Taster-Modul, 4 Pins
  - Hersteller-/Bauteilmarkierung ungefähr „XINDA“
  - genaue Variante und Pinbelegung später bestimmen
- Potentiometer-Module
- weitere Taster/Drehgeber aus dem SensorKit
- Sortiment kleiner Schalter und Taster (Ergänzung 28. August 2026)
  - Miniatur-Drucktaster, THT
  - kleine Schiebe-/Kippschalter
  - genaue Typen, Polzahl und Stückzahlen später bestimmen
  - Entprellung ist nicht vorausgesetzt; sie muss je Anwendung in Hardware oder Firmware erfolgen
- Drehgeber / Rotary Encoder, mehrere Stück (Ergänzung 28. August 2026)
  - inkrementell, mit Rastung und meist integriertem Taster
  - genaue Typen, Impulszahl je Umdrehung und Stückzahl später bestimmen
  - benötigen Pull-ups und Entprellung; Pinbelegung vor dem Anschluss prüfen
- Einzel-Potentiometer, bedrahtet (Ergänzung 28. August 2026)
  - ergänzend zu den Potentiometer-Modulen und den Trimmpotentiometern
  - Widerstandswerte und Kennlinie linear/logarithmisch später bestimmen

## Motoren & Motorsteuerung

- 1× L298N Dual-H-Bridge-Motortreiber-Modul
- 1× DK Electronics Motor Controller Shield für Arduino
  - vermutlich L293D-basierter Typ
  - Anschlüsse für bis zu 4 DC-Motoren beziehungsweise 2 Schrittmotoren
  - Servoanschlüsse vorhanden
- 2× Arduino Motor Shield / Motor Controller Shield, Klon
  - eines mit Kennzeichnung/Quelle Sunsunmall
  - L293D-artiger Typ
  - Anschlüsse für 2 Servos
  - vorgesehen für 2 Schrittmotoren beziehungsweise 4 DC-Motoren
- 2× ULN2003A Schrittmotor-Treiberboards
  - ULN2003A Darlington-Treiber
  - typische Treiberplatinen beispielsweise für 28BYJ-48-Schrittmotoren
  - 7 Eingänge IN1–IN7 vorhanden
  - ULN2003A auf den fotografierten Boards erkennbar
- Kleinservos, mehrere Stück (Ergänzung 28. August 2026)
  - Bauform vom Typ SG90/MG90 beziehungsweise vergleichbar
  - Ansteuerung über 50-Hz-PWM, Signalpegel 3,3 V am ESP32 in der Regel ausreichend
  - **Stromversorgung getrennt führen.** Der Anlaufstrom eines Kleinservos bricht die 5-V-Schiene
    ein; nicht aus dem ESP32-`3V3` und nicht aus einer gemeinsamen USB-5-V-Schiene ohne Reserve
    speisen. Siehe [hardware/power.md](hardware/power.md)
  - genaue Typen und Stückzahl später bestimmen

## Thermoelektrik

- 1× Peltier-Element / thermoelektrisches Kühl-/Heizelement
  - keramisches Plattenelement
  - eine Seite wird bei Bestromung kalt, die andere warm
  - genaue Typnummer, Spannung und Leistung noch bestimmen
  - vor Einsatz nur mit geeignetem Kühlkörper, Strombegrenzung und Temperaturüberwachung betreiben

## THT-Bauteilsortimente

### Widerstände

- umfangreiches THT-Widerstandssortiment
- genaue Einzelwerte nicht gezählt; Sortiment wird als weitgehend vollständig geführt

### Trimmpotentiometer

15 Werte:

- 100 Ω
- 220 Ω
- 470 Ω
- 1 kΩ
- 3,3 kΩ
- 4,7 kΩ
- 10 kΩ
- 20 kΩ
- 30 kΩ
- 47 kΩ
- 100 kΩ
- 200 kΩ
- 300 kΩ
- 500 kΩ
- 1 MΩ

### Keramikkondensatoren

10 Werte:

- 10 pF
- 20 pF
- 30 pF
- 47 pF
- 56 pF
- 68 pF
- 100 pF
- 1 nF
- 10 nF
- 100 nF

### Elektrolytkondensatoren

- Sortiment radialer Aluminium-Elektrolytkondensatoren — **Etikett per Foto bestätigt am
  4. September 2026**
  - 925 Stück in 36 Werten
  - Bereich: 1 µF bis 1500 µF
  - Spannungsfestigkeiten 10 V bis 63 V
  - gepolt: Polarität und Nennspannung an jedem Exemplar ablesen; nie verpolen, nie über der
    Nennspannung betreiben
  - Qualität, ESR und Ripplestrom-Belastbarkeit sind dem Etikett nicht zu entnehmen; für die
    SDS011-Welligkeitsgrenze (< 20 mV) zählt das Messergebnis am Oszilloskop, nicht der Sortimentsname
- frühere Erfassung (25. August 2026): „24-Werte-Sortiment, 0,1 µF bis 1000 µF“. Ob das dieselbe
  Box mit damals ungenau erinnertem Etikett oder ein zweites Sortiment ist, ist offen; bis zur
  Klärung gilt die per Foto bestätigte Angabe

### Dioden

Zwei Erfassungen mit unterschiedlicher Zusammensetzung liegen vor. Ob es sich um zwei Sortimente
oder um zwei Lesungen desselben handelt, ist offen (siehe „Noch offen“).

**Erfassung 1** — 10-Werte-Sortiment, auf der Verpackung erkennbar (25. August 2026):

- 1N4001
- 1N4002
- 1N4003
- 1N4004
- 1N4005
- 1N4006
- 1N4007
- „1N58…“ – vollständige Nummer auf Foto nicht sicher lesbar
- 1N5818
- 1N5819

**Erfassung 2** — 100 Stück in 8 Typen, vom Operator abgelesen am 4. September 2026, nicht per
Foto bestätigt:

| Typ | Stück | Familie |
| --- | ---: | --- |
| 1N4148 | 25 | Kleinsignal-Schaltdiode |
| 1N4007 | 25 | Gleichrichter 1 A / 1000 V |
| 1N5819 | 10 | Schottky 1 A / 40 V |
| 1N5408 | 5 | Gleichrichter 3 A / 1000 V |
| 1N5399 | 10 | Gleichrichter 1,5 A / 1000 V |
| FR107 | 10 | schneller Gleichrichter 1 A |
| FR207 | 10 | schneller Gleichrichter 2 A |
| 1N5822 | 5 | Schottky 3 A / 40 V |

Die Typen sind nicht untereinander austauschbar: vor Einsatz Typ, Kathodenring und die zulässigen
Werte für Spannung, Strom und Schaltverhalten am Bauteil und im passenden Datenblatt prüfen.
Die Familienangaben in der Tabelle sind Typenwissen, keine Messung am vorhandenen Exemplar.

**Konstruktive Folge:** Mit 1N5819 und 1N5822 sind Schottky-Dioden für Verpolschutz oder
Freilauf an 5 V vorhanden; die 1N4148 deckt Kleinsignal- und Logikaufgaben ab. Ein Zener-Clamp für
3,3-V-GPIOs bleibt weiterhin ausgeschlossen, siehe Zenerdioden.

### Zenerdioden

- 10-Werte-Sortiment, 1N47xx-Serie — **vollständig erfasst am 28. August 2026**

| Typ | Zenerspannung |
| --- | ---: |
| 1N4733 | 5,1 V |
| 1N4738 | 8,2 V |
| 1N4739 | 9,1 V |
| 1N4740 | 10 V |
| 1N4741 | 11 V |
| 1N4742 | 12 V |
| 1N4744 | 15 V |
| 1N4745 | 16 V |
| 1N4746 | 18 V |
| 1N4748 | 22 V |

**Konstruktive Folge: für 3,3-V-GPIOs ist nichts davon brauchbar.** Der niedrigste Wert ist
1N4733 mit 5,1 V und klemmt damit weit oberhalb der zulässigen GPIO-Spannung. Ein 1N4728 (3,3 V)
oder 1N4730 (3,9 V) ist **nicht** vorhanden. Die Regel „kein Zener-Clamp" im Room-Design
(`hardware/kicad/atmosmesh-room/README.md`) steht damit auf einer vollständigen Bestandsliste und
nicht mehr auf einer Vermutung.

### Transistoren

15-Typen-Sortiment:

- 2N2222
- 2N3904
- 2N3906
- 2N5401
- 2N5551
- A1015
- C1815
- C945
- S8050
- S8550
- S9012
- S9013
- S9014
- S9015
- S9018

### MOSFETs

- 10× IRLZ34 / IRLZ34N (Ergänzung 28. August 2026)
  - N-Kanal, **logic-level**: schaltet bereits mit 3,3 V Gate-Spannung durch
  - damit für Low-Side-Schaltaufgaben direkt an einem ESP32-GPIO geeignet, ohne Treiberstufe
  - typische Anwendung hier: Lasten auf der 5-V-Schiene schalten, etwa das Tastverhältnis des
    SDS011-Lasers gegen dessen 8000-h-Lebensdauer
  - Gate-Vorwiderstand und Gate-Pulldown sind trotzdem vorzusehen; ein offenes Gate ist undefiniert
  - genaue Typvariante (IRLZ34 gegenüber IRLZ34N) am Bauteil ablesen
- 10× IRLB8721, TO-220 (Ergänzung 4. September 2026, abgelesen, nicht per Foto bestätigt)
  - N-Kanal, **logic-level**; Datenblattwerte: 30 V, R_DS(on) rund 9 mΩ bei 10 V und rund
    16 mΩ bei 4,5 V Gate-Spannung, Schwellenspannung 1,35–2,35 V. Typenwissen, nicht am Exemplar
    geprüft
  - schaltet an einem 3,3-V-GPIO sauber durch und hat deutlich weniger Durchlasswiderstand als der
    IRLZ34; für Lasten von einigen Ampere die bessere Wahl
  - Pinfolge TO-220 mit Blick auf die Beschriftung: Gate, Drain, Source; die Kühlfahne liegt auf
    Drain. Vor dem Einlöten am Datenblatt gegenprüfen
  - nur 30 V Sperrspannung: für 5-V- und 12-V-Lasten reichlich, nicht für höhere Spannungen
  - Gate-Vorwiderstand und Gate-Pulldown auch hier vorsehen; induktive Lasten mit Freilaufdiode
    (1N5819/1N5822 aus dem Diodensortiment)

## ICs / Chips

Am 25. August 2026 anhand der abgelesenen Gehäusebeschriftungen aufgenommen. Getrennte Fertigungs-, Hersteller- oder Datecodes können in der diktierten Zeichenfolge enthalten sein; unsichere Lesungen vor Einsatz anhand von Fotos prüfen.

- 1× MB9124B – Beschriftung unsicher; diktiert als `MB9 124 B`
- 1× 74LS154 – zusätzliche Beschriftung vermutlich `M1`
- 1× IC mit Beschriftung `CA G046` – genaue Typtrennung und Identifikation offen
- 1× RCA-IC mit Beschriftung `906` – vollständige Typnummer offen
- 1× KM4164E – genaue Suffix-/Datecode-Zeichen noch prüfen
- mehrere CD4556BE
  - CMOS-Doppeldecoder/-Demultiplexer: zwei unabhängige 2-zu-4-Decoder mit aktiv-low Ausgängen
  - 16-poliges DIP-Gehäuse bei der `E`-Variante
  - genaue Stückzahl noch zählen
- 1× IC mit Beschriftung `LCA314` – Lesung und Herstellerpräfix unsicher
- 1× IC mit Beschriftung ungefähr `MCP29315B7 SP` – genaue Gruppierung/Typnummer offen
- 1× CA3081
- 1× vermutlich MC14086B / CD4086B-kompatibler CMOS-Logikbaustein
  - diktiert ungefähr als `NCI-C086` beziehungsweise zuvor `CAC086`
  - Funktion der 4086-Familie: erweiterbares 4-fach-2-Eingang-AND-OR-INVERT-Gatter
  - genaue Herstelleraufschrift, Suffixe und Pinzahl noch per Foto bestätigen
- 1× CA304A – Typnummer unvollständig oder unsicher
- 2× CA3086 – die Typnummer wurde zweimal genannt; Stückzahl physisch bestätigen

## 4duino SensorKit 40 in 1

> [!warning] Unklare Module vor Einsatz prüfen
> Bei funktional erfassten oder noch nicht eindeutig identifizierten Modulen müssen Pinbelegung, Versorgungsspannung und genaue Bauteilvariante vor einem tatsächlichen Einsatz erneut anhand der Platinenbeschriftung, eines Fotos oder des zugehörigen Datenblatts geprüft werden.

Vorhandenes Sensor-Kit laut hochgeladener Dokumentation. Die Dokumentation führt folgende Module bzw. Modultypen auf:

- B01 Joystick Modul (XY-Achsen)
- B02 5V Relay Modul
- B03 Mikrofon Sensor Modul
- B04 IR Optical Detection / IR-Erkennung
- B05 Flame Sensor / Flammensensor
- B06 Hall TTL Sensor
- B07/B31 NTC Threshold TTL / NTC 10k
- B08 Touch Sensor
- B09 RGB LED
- B10/B11 Zweifarbige LED 5/3 mm
- B12/B13 Reed Sensor / Magnetkontakt-Sensor
- B14 Button / Taster
- B15 Tilt Sensor / Neigungssensor
- B16 Rotary Encoder / kodierter Drehschalter
- B18 Light Barrier / Lichtschranke
- B19 Potentiometer / Analog Hall
- B20 Temperatur-Sensormodul, 1-Wire
- B21 IR LED / Infrarot-LED
- B22 IR Receiver 38 kHz
- B23 Shock Sensor / Schocksensor
- B24 Temperature & Humidity / DHT11
- B25 1-Watt-LED-Modul
- B26 Piezo Speaker
- B27 Buzzer
- B28 Flash LED
- B29 Heartbeat
- B30 Photoresistor / Lichtsensor
- B32 5V Step-engine with driver PCB / Schrittmotor-Treiberboard
- B33 Gas MQ2 Sensor
- B34 Linear Voltage Regulator
- B35 Voltage Regulator
- B36 Motion Detection / Bewegungsmelder
- B37 8 LED PCB / 8-fach LED PCB
- B38 Temperature I²C / I²C-Temperatursensor
- B39 Vibration Sensor

**Hinweis:** Diese Liste beschreibt den Inhalt des dokumentierten 40-in-1-Sets. Noch nicht jedes einzelne Modul wurde physisch gegengeprüft.

## Messtechnik und Hilfsmittel (Ergänzung 28. August 2026)

- 1× HANTEK DSO2D15 Digitaloszilloskop
  - vom Operator als `DS02D15` genannt; die Hantek-Typbezeichnung lautet DSO2D15
  - Serie DSO2000, zwei Kanäle, `D`-Variante mit integriertem Funktionsgenerator
  - konkrete Bandbreite, Abtastrate und Generatorumfang am Gerät ablesen und hier nachtragen

  **Konstruktive Folge: zwei offene Punkte des Room-Designs sind damit tatsächlich messbar.**

  1. Die SDS011-Welligkeitsgrenze von < 20 mV gegen den laufenden Lüfter. Diese Prüfung war bisher
     als „mit einem Oszilloskop messen" formuliert, ohne dass ein Gerät erfasst war.
  2. Die I²C-Flankenzeit. `R_PU_SDA`/`R_PU_SCL` wurden mit 3,3 kΩ gegen eine **angenommene**
     Buskapazität von rund 200 pF ausgelegt (Widerstandstabelle in
     `hardware/kicad/atmosmesh-room/wiring.md`). Am fertigen Lochrasteraufbau lässt sich die
     tatsächliche Anstiegszeit messen und die Annahme bestätigen oder korrigieren.

- 1× Labornetzteil mit einstellbarer Spannung und einstellbarer Strombegrenzung
  - genaue Typbezeichnung, Spannungs- und Strombereich am Gerät ablesen und hier nachtragen

  **Konstruktive Folge: das Room-Board bekommt dafür eine umschaltbare 5-V-Quelle (D-031).**

  1. **Erstinbetriebnahme mit Strombegrenzung.** Der Grenzwert wird knapp über den erwarteten
     Strom gesetzt, nicht auf das Maximum. Ein Verdrahtungsfehler führt dann zu einem Netzteil,
     das schlicht nicht liefert, statt zu einem zerstörten Bauteil. Dieses Projekt hat bereits
     einmal einen LDO verloren (`hardware/incident-2026-08-17-ldo.md`).
  2. **Das Problem der gemeinsamen 5-V-Schiene entfällt.** `hardware/spec-comparison.md` nennt
     rund 650 mA Spitzenkoinzidenz auf der USB-Schiene noch ohne den SDS011-Lüfter. Mit
     `JP_5V_SRC` auf 2–3 hängt der Lüfter gar nicht an dieser Schiene.
  3. Der tatsächliche Stromverbrauch beider 5-V-Domänen lässt sich am Netzteil direkt ablesen.

- IC-Fassungssortiment (DIP)
  - verschiedene Polzahlen
  - für das Room-Design derzeit nicht erforderlich; es enthält keinen DIP-Baustein
  - relevant, sobald ein DIP-IC verbaut wird: Fassung statt direktem Einlöten erlaubt Tausch ohne
    Auslöten

- Schrumpfschlauch-Sortiment
  - verschiedene Durchmesser
  - **für den Lochrasteraufbau direkt relevant:** alle 5-V-führenden Litzen und Modul-Anschlussdrähte
    isolieren und zugentlasten. Die 5-V-Domäne ist auf dem Room-Board bewusst räumlich abgetrennt;
    ein abgerutschter 5-V-Draht auf einen GPIO ist genau der Fehler, gegen den das gesamte
    Schutzkonzept ausgelegt ist

## Noch offen / später prüfen

- Elektrolytkondensatoren: klären, ob das per Foto bestätigte 36-Werte-Sortiment (925 Stück,
  1–1500 µF) dieselbe Box wie die frühere „24 Werte, 0,1–1000 µF“ ist oder ein zweites Sortiment
- ENS160 + AHT20: Pinbelegung und Reglerbestückung am Modul per Foto bestätigen; ADDR-Pin-Zustand
  ab Werk prüfen
- Sicherungshalter „D63“: Halter-Nennspannung, Sicherungsformat und Auslösecharakteristik per
  Foto bestätigen
- CJMCU-226: Shuntwert und Pinbelegung am Modul per Foto bestätigen; Stückzahl bestätigen
- BME280: Gesamtstückzahl (6?) bestätigen; Breakout-Variante und Pinfolge per Foto
- Dioden: klären, ob Erfassung 1 (1N4001–1N4007, 1N5818/1N5819) und Erfassung 2 (8 Typen,
  100 Stück) zwei Sortimente sind; Erfassung 2 per Foto des Etiketts bestätigen
- Fotos und Pinzahl/Gehäuseform der neu erfassten ICs aufnehmen; insbesondere `MB9124B`, `CA G046`, `RCA 906`, `LCA314`, `MCP29315B7 SP`, den vermutlich als `MC14086B` gelesenen Chip und `CA304A` eindeutig identifizieren
- Stückzahlen von CD4556BE und CA3086 physisch bestätigen
- genaue Anzahl und Varianten der ESP8266-Boards
- genaue Anzahl der Arduino-Nano-kompatiblen Boards
- weitere Arduino-Nano-/Mini-/Pro-Mini-artige Boards identifizieren; bis dahin nicht zur Stückzahl addieren
- genaue Varianten der zwei zusätzlichen ESP32-Devboards; klären, ob die beiden ideaspark-Boards
  darunter sind
- zweites ideaspark-Board: OLED- oder TFT-Variante am Gerät prüfen
- Controller und konkrete Pinreihenfolge des neuen 128×64-I²C-OLEDs prüfen
- Schnittstelle, Controller und konkrete Pinreihenfolge des bereits verbauten 128×32-OLEDs prüfen
- genaue GL55xx-Variante und Kennwerte des losen Fotowiderstands bestimmen
- Thermoelement-Typ und eventuell vorhandenes Verstärkermodul
- genaue Identifikation des Boards „WCBN64“
- genaue Identifikation von ZS-100
- genaue Modellnummer und Versorgung des D-SUN-PIR-Moduls
- genaue Variante, Pinbelegung und Versorgung des HC-20-Sound-Moduls
- genaue Pinbelegung und Variante des 4-Pin-Joystick-/Taster-Moduls „XINDA“
- Pinbelegung des XY-MK-5V-/FS1000A-Funksets am Modul ablesen (Band 433 MHz ist bestätigt)
- RFM12S: Band vom Aufdruck ablesen
- genaue Relais-, Trigger- und Versorgungsvariante des 8-Kanal-Relaismoduls
- genaue Varianten der drei L293D-artigen Motor-Shields
- genaue LED-/Controller-Typen und Gesamtstückzahl der RGB-LED-Module
- genaue Typnummer, Spannung und Leistung des Peltier-Elements
- Ausgangsspannung und Anschlussbelegung der drei AC/DC-Netzteile 5V07 / 12V04 je Exemplar per
  Multimeter verifizieren; Aufdruck des als „5D07“ gelesenen Exemplars prüfen
- SM-PLB03A: Eingang, Ausgangsspannung und -strom vom Aufdruck ablesen; Netzspannungsseite bestätigen
- Versorgung und Pinbelegung der H34A-Sender und H3V4F-Empfänger am Modul ablesen
- physischer Abgleich des kompletten 4duino SensorKit 40-in-1
