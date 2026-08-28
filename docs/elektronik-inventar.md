# Elektronik-Inventar

> Stand: 25. August 2026, ergänzt am 28. August 2026
> Dieses Inventar basiert auf der gemeinsamen Durchsicht und Identifikation. Unsichere Stückzahlen bzw. Varianten sind entsprechend markiert.

![[Images/Elektronik-OLED-und-Zweipol-Bauteil-2026-08-17.jpg|480]]

## Computer & Mikrocontroller

- 1× Raspberry Pi 1
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
- 1× Heltec WiFi LoRa 32 V2
  - LoRa 866–915 MHz
  - 0,96" OLED
  - 128×64 Pixel
- ESP8266MOD / ESP8266-Boards – mehrere vorhanden, genaue Varianten und Anzahl später zählen
- 2× GR-Sakura / Renesas RX63N Boards – unterschiedliche Ausführungen
- 1× unbekanntes Board „WCBN64“ – noch zu identifizieren

## Sensoren

- 1× SHT41
  - digitaler Temperatur- und Luftfeuchtigkeitssensor
  - I²C
  - am 25. August 2026 als eingetroffen erfasst
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

- ADS1115 ADC + PGA – mehrere vorhanden
  - 16 Bit
  - I²C
  - PGA
- Potentiometer-Module
- Joystick-Controller-Board, Keyes-SJoys, 5 Pins

## Funk & Kommunikation

- 1× RFID-RC522-Modul
  - RFID-/NFC-Lesemodul auf Basis MFRC522
- 1× 315/433-MHz-RF-Sender-/Empfänger-Set
  - Empfänger: XY-MK-5V
  - Sender: FS1000A beziehungsweise kompatible Variante
  - einfache ASK-/OOK-Funkmodule
  - auf den Modulen finden sich Angaben zu 315 beziehungsweise 433 MHz; konkrete Frequenzzuordnung vor Einsatz prüfen
- 1× RFM12S V4.0
  - Funk-Transceiver-Modul
- 1× Board mit Kennzeichnung „H34A“ und NDR4208
  - NDR4208 als 433,92-MHz-SAW-Resonator identifiziert
  - vermutlich Teil eines einfachen 433-MHz-Senders oder einer Funkfernbedienung
  - genaue Funktion, Versorgung und Pinbelegung des H34A-Boards noch offen
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

- Mini-Stromversorgungsmodul, Beschriftung ungefähr „5M001“ – noch genauer identifizieren
- 1× kleines offenes AC/DC-Schaltnetzteil, Kennzeichnung 5V07 / 12V04
  - geplanter Einsatz: 5-V-Versorgung
  - tatsächliche Ausgangsspannung und Anschlussbelegung vor endgültiger Erfassung noch per Multimeter verifizieren
- 1× SANMIM AC/DC-Schaltnetzteil
  - Platinenkennzeichnung: SM-PLG06A
  - Variante: SM-104-3.3V-02
  - Ausgang: 3,3 V
  - als 3,3-V-Netzteil identifiziert; nicht mehr als unbekannt führen
- weitere ähnliche SANMIM-Stromversorgungsmodule vorhanden
  - frühere Sichtung: mindestens 4 Stück einer SANMIM-Ausführung
  - genaue Zuordnung und Stückzahl der zusätzlichen Exemplare später prüfen
- weitere Stromversorgungsboards mit großen Spulen und Kondensatoren

> [!danger] Offene Netzspannungs-Netzteile
> Das 5V07/12V04-Board und das SANMIM SM-PLG06A sind offene AC/DC-Schaltnetzteile mit einer Netzspannungsseite. Sie sind **keine gewöhnlichen Breadboard-DC/DC-Module** und dürfen nicht offen oder ungeschützt wie Niederspannungsmodule verwendet werden. Messung und Betrieb nur in einem geeigneten berührungssicheren Aufbau durch eine entsprechend qualifizierte Person.

## Adapter & Schnittstellen

- USB-to-TTL-Adapter
- V-USB-TTL USB-to-TTL-Stick
- USB2TT004 USB-Adapter/Kabel beim SDS011
- Dupont-Steckverbinder-Sortiment, männlich und weiblich (Ergänzung 28. August 2026)
  - Stiftleisten männlich, 2,54 mm, umbrechbar
  - Buchsenleisten weiblich, 2,54 mm, umbrechbar
  - größere Stückzahlen; genaue Reihenlängen später zählen
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

- 24-Werte-Sortiment
- Bereich: 0,1 µF bis 1000 µF
- unterschiedliche Spannungsfestigkeiten gemäß Sortimentsbeschriftung

### Dioden

10-Werte-Sortiment, auf der Verpackung erkennbar:

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

## Noch offen / später prüfen

- Fotos und Pinzahl/Gehäuseform der neu erfassten ICs aufnehmen; insbesondere `MB9124B`, `CA G046`, `RCA 906`, `LCA314`, `MCP29315B7 SP`, den vermutlich als `MC14086B` gelesenen Chip und `CA304A` eindeutig identifizieren
- Stückzahlen von CD4556BE und CA3086 physisch bestätigen
- genaue Anzahl und Varianten der ESP8266-Boards
- genaue Anzahl der Arduino-Nano-kompatiblen Boards
- weitere Arduino-Nano-/Mini-/Pro-Mini-artige Boards identifizieren; bis dahin nicht zur Stückzahl addieren
- genaue Varianten der zwei zusätzlichen ESP32-Devboards
- Controller und konkrete Pinreihenfolge des neuen 128×64-I²C-OLEDs prüfen
- Schnittstelle, Controller und konkrete Pinreihenfolge des bereits verbauten 128×32-OLEDs prüfen
- genaue GL55xx-Variante und Kennwerte des losen Fotowiderstands bestimmen
- Thermoelement-Typ und eventuell vorhandenes Verstärkermodul
- genaue Identifikation des Boards „WCBN64“
- genaue Identifikation von ZS-100
- genaue Modellnummer und Versorgung des D-SUN-PIR-Moduls
- genaue Variante, Pinbelegung und Versorgung des HC-20-Sound-Moduls
- genaue Pinbelegung und Variante des 4-Pin-Joystick-/Taster-Moduls „XINDA“
- genaue Frequenzzuordnung und Pinbelegung des XY-MK-5V-/FS1000A-Funksets
- genaue Relais-, Trigger- und Versorgungsvariante des 8-Kanal-Relaismoduls
- genaue Varianten der drei L293D-artigen Motor-Shields
- genaue LED-/Controller-Typen und Gesamtstückzahl der RGB-LED-Module
- genaue Typnummer, Spannung und Leistung des Peltier-Elements
- genaue Identifikation des Moduls „5M001“
- Ausgangsspannung und Anschlussbelegung des AC/DC-Netzteils 5V07 / 12V04 sicher per Multimeter verifizieren
- genaue Identifikation, Funktion und Pinbelegung des H34A-Boards mit NDR4208
- genaue Zuordnung und Stückzahl der zusätzlichen SANMIM-Stromversorgungsmodule
- physischer Abgleich des kompletten 4duino SensorKit 40-in-1
