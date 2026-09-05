# AtmosMesh vault snapshot (13–14 August 2026)

> Indoor Air Quality Station — local sensing, connected as a mesh.

> [!IMPORTANT]
> This is a historical planning snapshot moved from the personal Obsidian vault. It contains
> superseded pins, scope, and acceptance text. Do not use it for wiring or current project status.
> The current sources of truth are the [hardware inventory](../hardware/inventory.md),
> [power rules](../hardware/power.md), and [roadmap](../../agent-context/roadmap.md).

## Outcome

- 🌬️ Eine dauerhaft laufende Raumluftstation misst Feinstaub, Temperatur, Luftfeuchtigkeit, Luftdruck und einen ausdrücklich als **Gas-/Luftqualitätstrend** bezeichneten MQ135-Wert.
- 🖥️ Ein kleines I²C-OLED zeigt die wichtigsten lokalen Werte und den Verbindungsstatus an.
- 📡 Der ESP32 überträgt Messwerte per WLAN und MQTT an die Home-Automation-Plattform im Kubernetes-Cluster.
- 📊 Home Assistant und Grafana machen aktuelle Werte, Verläufe und Warnzustände sichtbar.
- 🧩 Die Lösung bleibt so modular, dass später ein echter NDIR-CO₂-Sensor ergänzt werden kann.

## Context

- Das Projekt verwendet zunächst vorhandene Teile aus dem [Elektronik-Inventar](../elektronik-inventar.md) und soll nicht von langen Teilelieferungen abhängen.
- Für den MVP vorgesehen: ESP32-WROOM-32, SDS011, DHT22, BMP280, kleines I²C-OLED und optional MQ135.
- Der MQ135 ist **kein echter CO₂-Sensor**. Sein Messwert wird weder als CO₂ noch in ppm ausgegeben, sondern nur als relativer Gas-/Luftqualitätstrend.
- Der 3,5-Zoll-Raspberry-Pi-TFT bleibt zunächst außerhalb des MVP. Das kleine OLED ist sparsamer, einfacher anzusteuern und lässt dem ESP32 genügend Ressourcen für WLAN, MQTT und Sensorik.
- Home Assistant, Mosquitto, Prometheus und Grafana sollen auf dem parallel entstehenden Kubernetes-Cluster laufen.

## Links

- 🧰 Inventar: [Elektronik-Inventar](../elektronik-inventar.md)
- 🖼️ Historische Konzeptverdrahtung: [SVG vom 13. August 2026](../assets/history/atmosmesh-concept-wiring-2026-08-13.svg) — nicht als Bauanleitung verwenden
- 💻 Implementierungs-Repository: dieses Repository
- 🌐 GitHub: [konih/atmosmesh](https://github.com/konih/atmosmesh) — private
- ⚠️ Die zuvor im Projektgespräch erzeugte KI-Grafik ist wegen fehlerhafter Leitungsführung verworfen und darf nicht zum Aufbau verwendet werden.

## Historische Roadmap

| Phase | Stories | Ergebnis | Abschlusskriterium |
|---|---|---|---|
| 1. 🔎 Hardware klären | RLS-01 | Konkrete Module, Pins und Versorgung sind bestätigt | Verdrahtung kann ohne Annahmen aufgebaut werden |
| 2. 🧪 Tischprototyp | RLS-02, RLS-03 | Alle Sensoren und das OLED funktionieren einzeln und gemeinsam | Plausible Livewerte laufen mindestens 30 Minuten fehlerfrei |
| 3. 🖥️ Stations-Firmware | RLS-04 | Lokale Anzeige und robuste Messzyklen funktionieren | Display, Fehlerzustände und Neustartverhalten sind demonstriert |
| 4. 📡 Vernetzung | RLS-05 | Messwerte erreichen MQTT in einem stabilen Schema | Daten laufen nach WLAN- oder Broker-Ausfall selbstständig weiter |
| 5. ☸️ Plattform | RLS-06, RLS-07 | Kubernetes-Dienste, Dashboard und Warnungen sind nutzbar | Ende-zu-Ende-Demo vom Sensor bis zum Dashboard ist bestanden |
| 6. 🏠 Dauerbetrieb | RLS-08 | Station ist sicher aufgestellt und langfristig beobachtbar | 48-Stunden-Test ohne manuellen Eingriff ist bestanden |

### Nicht Bestandteil des MVP

- echter CO₂-Wert, solange kein bestätigter NDIR-Sensor vorhanden ist
- automatische Lüftungs- oder Netzspannungssteuerung
- Betrieb des 480×320-Raspberry-Pi-TFT am ESP32
- Akku- oder Batteriebetrieb
- kalibrierte oder zertifizierte Messungen für Gesundheits- oder Sicherheitszwecke
- eigenes PCB und endgültiges Produktgehäuse

## Historische Stories

### RLS-01 — Hardware identifizieren und Verdrahtung freigeben

**Als Bastler möchte ich** die konkreten Modulvarianten und Pinbeschriftungen bestätigen, **damit** wir keine Bauteile durch falsche Spannung oder Pinbelegung beschädigen.

**Umfang**

- ESP32, beide Mini-OLEDs, BMP280, DHT22, SDS011 samt Adapter, MQ135 und verfügbare 5-V-Versorgung fotografieren.
- OLED-Controller beziehungsweise I²C-Adresse soweit möglich bestimmen.
- BMP280-Modulspannung und I²C-Adresse prüfen.
- MQ135-Ausgangsspannung und geplanten Spannungsteiler bestätigen.
- Gemeinsame Pinbelegung als verbindliche Verdrahtungstabelle festhalten.

**Akzeptanzkriterien**

- [ ] 🔍 Jedes verwendete Modul ist auf Vorder- und Rückseite eindeutig dokumentiert.
- [ ] ⚡ Für jedes Modul sind Versorgungsspannung und Logikpegel bekannt.
- [ ] 🧷 Jeder Anschluss ist genau einem ESP32-Pin oder einer Versorgungsschiene zugeordnet.
- [ ] 🛡️ Kein ESP32-GPIO kann direkt mit 5 V beaufschlagt werden.
- [ ] ✅ Die Verdrahtung wurde vor dem Einschalten gemeinsam anhand der Fotos geprüft.

**Unsere Zusammenarbeit**

- Konrad stellt gut lesbare Fotos und gegebenenfalls Multimeter-Messwerte bereit.
- Der Agent identifiziert die Module, erstellt die endgültige Pin-Tabelle und prüft die Verdrahtung auf Risiken.

---

### RLS-02 — I²C-Basis und Mini-OLED in Betrieb nehmen

**Als Betreiber möchte ich** OLED und BMP280 am gemeinsamen I²C-Bus erkennen, **damit** Anzeige und Luftdruckmessung eine verlässliche technische Basis haben.

**Vorgesehene Verdrahtung**

- SDA → GPIO 21
- SCL → GPIO 22
- OLED und BMP280 → 3,3 V und gemeinsame Masse

**Akzeptanzkriterien**

- [ ] 🔎 Ein I²C-Scan erkennt OLED und BMP280 mit stabilen Adressen.
- [ ] 🖥️ Das OLED zeigt einen Testtext ohne Flackern oder Artefakte.
- [ ] 🌡️ Der BMP280 liefert plausible Temperatur- und Luftdruckwerte.
- [ ] 🔁 Nach einem Neustart initialisieren sich beide Module ohne manuellen Eingriff.
- [ ] 📝 Erkannte Adressen und Modulvarianten sind in dieser Notiz dokumentiert.

**Gemeinsamer Checkpoint:** Foto des Displays plus serielle Ausgabe des I²C-Scans prüfen.

---

### RLS-03 — Alle Messsensoren als Tischprototyp integrieren

**Als Betreiber möchte ich** alle vorgesehenen Sensoren gleichzeitig auslesen, **damit** wir elektrische und softwareseitige Konflikte vor der Vernetzung erkennen.

**Vorgesehene Anschlüsse**

- DHT22 DATA → GPIO 27 mit 10-kΩ-Pull-up nach 3,3 V
- SDS011 TX → ESP32 GPIO 16 / RX2
- SDS011 RX → ESP32 GPIO 17 / TX2
- MQ135 AO → Spannungsteiler → ADC GPIO 34
- SDS011 und MQ135 → stabile 5-V-Versorgung
- alle Komponenten → gemeinsame Masse

**Akzeptanzkriterien**

- [ ] 🌡️ Temperatur und Luftfeuchtigkeit werden vom DHT22 gelesen.
- [ ] 🌬️ PM2.5 und PM10 werden über UART vom SDS011 gelesen.
- [ ] 🎈 Luftdruck und BMP280-Temperatur werden über I²C gelesen.
- [ ] 🧪 Der MQ135 liefert nur einen Rohwert und einen relativen Trend; UI und Code nennen ihn nicht CO₂.
- [ ] ⚡ Die Spannung am ESP32-ADC bleibt auch beim höchsten beobachteten MQ135-Ausgang unter 3,3 V.
- [ ] ⏱️ Alle Sensoren laufen gemeinsam mindestens 30 Minuten ohne Absturz oder Busfehler.

**Gemeinsamer Checkpoint:** serielles Messprotokoll auf Plausibilität, Ausreißer und fehlende Werte untersuchen.

---

### RLS-04 — Lokale Anzeige und Stationszustände bauen

**Als Person im Raum möchte ich** die wichtigsten Werte und den Systemzustand ohne Smartphone sehen, **damit** die Station unmittelbar nützlich ist.

**Anzeigekonzept**

- Seite 1: PM2.5, PM10 und grober Luftqualitätsstatus
- Seite 2: Temperatur, Luftfeuchtigkeit und Luftdruck
- Seite 3: WLAN-, MQTT- und Sensorstatus
- automatischer Seitenwechsel; bei Bedarf später Bedienung per Taster oder Encoder

**Akzeptanzkriterien**

- [ ] 👀 Die Hauptwerte sind aus normaler Nähe lesbar.
- [ ] 🚦 Verbindungs- und Sensorfehler sind klar erkennbar und werden nicht als Messwert `0` dargestellt.
- [ ] 🕒 Das Display zeigt das Alter beziehungsweise die Aktualität der Daten nachvollziehbar an.
- [ ] 🔄 Ein Sensorfehler blockiert weder Anzeige noch die übrigen Sensoren dauerhaft.

**Gemeinsamer Checkpoint:** Layout anhand eines Fotos bewerten und einmal gezielt einen Sensor abziehen.

---

### RLS-05 — Messwerte zuverlässig per MQTT veröffentlichen

**Als Home-Automation-Betreiber möchte ich** strukturierte Messwerte empfangen, **damit** Home Assistant, Prometheus und weitere Verbraucher dieselben Daten nutzen können.

**Vorgeschlagenes Topic-Schema**

```text
home/air/wohnzimmer/state
home/air/wohnzimmer/status
home/air/wohnzimmer/availability
```

**Akzeptanzkriterien**

- [ ] 📦 Nachrichten enthalten Geräte-ID, Zeitstempel, Messwerte, Einheiten und Sensorstatus.
- [ ] 🏷️ Messgrößen haben stabile Namen; Gastrend und echtes CO₂ sind semantisch getrennt.
- [ ] 💓 Ein Availability-Topic zeigt online/offline per MQTT Last Will an.
- [ ] 🔁 Nach WLAN- oder Broker-Ausfall verbindet sich die Station mit begrenztem Backoff selbstständig neu.
- [ ] 🧯 Ein Netzwerkausfall blockiert Sensoren und lokale Anzeige nicht.
- [ ] 🔐 WLAN- und MQTT-Zugangsdaten liegen nicht fest im veröffentlichten Quellcode.

**Gemeinsamer Checkpoint:** MQTT-Nachrichten gemeinsam lesen und das Schema vor der Dashboard-Arbeit einfrieren.

---

### RLS-06 — Home-Automation-Dienste im Kubernetes-Cluster bereitstellen

**Als Plattformbetreiber möchte ich** die benötigten Dienste reproduzierbar im Cluster betreiben, **damit** die Station nicht von einem manuell eingerichteten Einzelserver abhängt.

**MVP-Dienste**

- Mosquitto als MQTT-Broker
- Home Assistant für Geräteansicht und Automationen
- Prometheus-kompatibler Pfad für Zeitreihen
- Grafana für langfristige Visualisierung
- persistente Speicherung und gesicherte Konfiguration

**Akzeptanzkriterien**

- [ ] ☸️ Alle benötigten Workloads werden deklarativ aus dem Repository installiert.
- [ ] 💾 Ein Pod-Neustart verliert weder Konfiguration noch relevante Historie.
- [ ] 🔐 Secrets stehen nicht im Klartext in versionierten Manifesten.
- [ ] 🩺 Für die Dienste existieren sinnvolle Readiness- und Liveness-Prüfungen.
- [ ] 📡 Der ESP32 erreicht den Broker über eine stabile Adresse im Heimnetz.
- [ ] ♻️ Eine dokumentierte Neuinstallation reproduziert den Dienstzustand.

**Gemeinsamer Checkpoint:** Deployments, Services, Persistenz und Zugriffspfad anhand der tatsächlichen Clusterumgebung entwerfen.

---

### RLS-07 — Dashboard, Historie und erste Warnung erstellen

**Als Bewohner möchte ich** aktuelle Werte und zeitliche Entwicklungen erkennen, **damit** ich Lüften, Reinigen oder technische Probleme sinnvoll beurteilen kann.

**Akzeptanzkriterien**

- [ ] 🏠 Home Assistant zeigt alle verfügbaren Messwerte und die Geräteverfügbarkeit.
- [ ] 📊 Grafana zeigt mindestens PM2.5, PM10, Temperatur, Feuchtigkeit und Luftdruck über 24 Stunden.
- [ ] 🚨 Eine erste Warnregel für einen anhaltend auffälligen Feinstaubwert ist vorhanden.
- [ ] ⏳ Die Warnung verwendet eine Dauer beziehungsweise Hysterese und reagiert nicht auf einen einzelnen Ausreißer.
- [ ] 📴 Ein offline gegangenes Gerät ist von einem echten Messwert `0` unterscheidbar.
- [ ] 🧪 Ein künstlich ausgelöster Testwert ist Ende zu Ende in MQTT, Historie, Dashboard und Warnzustand sichtbar.

**Gemeinsamer Checkpoint:** Dashboard gemeinsam auf Informationswert reduzieren und Warnschwellen zunächst als experimentell kennzeichnen.

---

### RLS-08 — Station für sicheren Dauerbetrieb härten

**Als Betreiber möchte ich** die Station 48 Stunden unbeaufsichtigt betreiben können, **damit** aus dem Tischaufbau ein verlässliches Home-Automation-Gerät wird.

**Akzeptanzkriterien**

- [ ] 🏠 Leitungen, Platinen und Stromversorgung sind mechanisch gesichert und berührungsgeschützt aufgestellt.
- [ ] 🌬️ SDS011, DHT22 und BMP280 erhalten ausreichenden Luftaustausch und werden nicht durch ESP32- oder Netzteilwärme verfälscht.
- [ ] 🔥 Kabel, Spannungsregler und Module zeigen keine auffällige Erwärmung.
- [ ] 🐕 Ein Watchdog beziehungsweise kontrollierter Neustart fängt festgefahrene Zustände ab.
- [ ] 📈 Neustarts, Verbindungsstatus und Alter des letzten Messwerts sind beobachtbar.
- [ ] ⏱️ Ein 48-Stunden-Test läuft ohne manuellen Eingriff und ohne unerklärte Datenlücken.
- [ ] 📖 Verdrahtung, Firmwarestand, Wiederherstellung und bekannte Einschränkungen sind dokumentiert.

**Gemeinsamer Checkpoint:** Nach dem Dauertest Logs und Messkurven prüfen und erst danach über ein endgültiges Gehäuse entscheiden.

## Optionale Folgestories

### RLS-09 — Echten CO₂-Sensor ergänzen

- Erst starten, wenn ein vorhandenes Modul eindeutig als NDIR-Sensor identifiziert oder ein passendes Modul beschafft wurde.
- Kandidaten: SCD40/SCD41, SCD30, MH-Z19B oder Senseair.
- CO₂ wird erst nach Plausibilitäts- und Frischlufttest in ppm angezeigt.
- Der MQ135-Trend bleibt eine getrennte Messgröße oder wird entfernt.

### RLS-10 — Großes lokales Display evaluieren

- Controller, Pinout und Touch-Controller des 3,5-Zoll-480×320-TFT identifizieren.
- ESP32-Prototyp nur als technische Machbarkeitsprüfung betrachten.
- Bevorzugte Alternative: Raspberry Pi mit Home-Assistant- oder Grafana-Dashboard im Kioskmodus.

## Definition of Done für den MVP

- [ ] ✅ RLS-01 bis RLS-08 sind erfüllt.
- [ ] 📡 Ein realer Messwert ist vom Sensor bis Home Assistant und Grafana nachvollziehbar.
- [ ] 🔄 Strom-, WLAN-, Broker- und Sensorunterbrechungen wurden kontrolliert getestet.
- [ ] 🧪 MQ135-Daten werden nirgends als CO₂-Messung ausgegeben.
- [ ] 📝 Aufbau, Pinbelegung, Konfiguration und Wiederanlauf sind so dokumentiert, dass wir die Station erneut aufbauen können.
- [ ] 🏠 Die Station hat den 48-Stunden-Dauertest bestanden.

## Historischer Plan

1. RLS-01 gemeinsam durchführen und die tatsächliche Hardware bestätigen.
2. RLS-02 und RLS-03 auf dem Tisch aufbauen; jeden Sensor zunächst einzeln testen.
3. RLS-04 und RLS-05 als erste vollständige ESP32-Firmware liefern.
4. RLS-06 an den tatsächlichen Kubernetes-Cluster und dessen Storage-/Ingress-Konzept anpassen.
5. RLS-07 Ende zu Ende demonstrieren.
6. RLS-08 mit provisorischem, aber sicherem Aufbau und 48-Stunden-Beobachtung abschließen.

## Historische Tasks

- [ ] 📸 Für RLS-01 Fotos der konkreten Module und ihrer Rückseiten aufnehmen
- [ ] 🔌 Verfügbare stabile 5-V-Versorgung auswählen und Typenschild dokumentieren
- [ ] 🧷 Endgültige Pin-Tabelle freigeben
- [ ] 🧪 I²C-Testfirmware erstellen und ausführen
- [ ] 🌬️ Sensor-Testfirmware erstellen und ausführen
- [ ] 🖥️ OLED-Layout abstimmen
- [ ] 📡 MQTT-Schema abstimmen
- [ ] ☸️ Cluster-Randbedingungen für Storage, Secrets und Heimnetz-Zugriff erfassen
- [ ] 📊 Dashboard und Warnregel erstellen
- [ ] ⏱️ 48-Stunden-Dauertest durchführen und auswerten

## Notes / Log

- 2026-08-13: Projekt aus dem vorhandenen Elektronik-Inventar geplant; Mini-OLED für den MVP gewählt. MQ135 ausdrücklich nur als Gas-/Luftqualitätstrend eingeordnet. Großer Raspberry-Pi-TFT und echter NDIR-CO₂-Sensor als optionale Folgestories aufgenommen.
- 2026-08-13: Fehler in der ersten KI-generierten Konzeptgrafik erkannt: 5-V- und 3,3-V-Leitungen waren visuell falsch geführt. Grafik verworfen und durch einen deterministischen SVG-Plan ersetzt. ESP32 wird darin separat über USB versorgt; SDS011 und MQ135 erhalten externe 5 V, alle Komponenten teilen nur GND. MQ135-ADC-Teiler auf 15 kΩ/10 kΩ geändert (maximal 2,0 V am ADC bei 5,0 V am AO).
- 2026-08-14: Eigenständiges Git-Repository angelegt. Das Repository ist ab jetzt die Implementierungsquelle mit `AGENTS.md`, Live-Kontext, Architektur, Hardwaregrenzen und einzeln claimbaren Stories RLS-01 bis RLS-10; diese Vault-Notiz bleibt die persönliche Übersicht.
- 2026-08-14: Projekt von „Raumluft-Netzstation“ in **AtmosMesh** umbenannt; alter Name bleibt als Alias erhalten.
