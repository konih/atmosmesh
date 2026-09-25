# Part-name glossary

Canonical spellings for parts and names that speech recognition gets wrong during dictated
inventory sessions. Agents resolve dictated names against this table before writing to
`inventory.md` (see `AGENTS.md`, "Dictated inventory updates"). Add a row whenever the operator
corrects a new mis-hearing.

| Canonical | Heard as (examples) | Notes |
| --- | --- | --- |
| INA226 | "INA two two six", "i n a two two six" | Current/power monitor; common breakout is CJMCU-226 |
| CJMCU-226 | "c j m c u hyphen two two six" | INA226 breakout board marking |
| VEML7700 | "VEM7707", "veml" | Ambient light sensor |
| ENS160 | "ENS one sixty" | Air-quality (TVOC/eCO₂) sensor |
| HC-SR04 | "HCs thirty four a" | Ultrasonic distance sensor |
| SDS011 | "d011v2" (board label) | Nova PM sensor; UART2 only, see `AGENTS.md` |
| MQ135 | | Analog gas sensor; not a CO₂ sensor |
| AtmosMesh | "atmosmash" | This project |
| Dependabot | "depenadbot" | GitHub dependency bot |
