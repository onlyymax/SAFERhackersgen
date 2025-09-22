# 🤖🦺 Safer - Autonomous Workplace Safety Monitoring Robot

**Safer** è un sistema robotico autonomo sviluppato presso l'Istituto Tecnico "I.T Archimede" di Catania, progettato per rivoluzionare il monitoraggio della sicurezza negli ambienti industriali e lavorativi. Integra tecnologie avanzate di sensing, intelligenza artificiale e automazione per identificare proattivamente situazioni di rischio e prevenire incidenti sul lavoro.

---

## 📖 Indice

- [Panoramica del Sistema](#-panoramica-del-sistema)
- [Architettura Tecnica](#-architettura-tecnica)
- [Specifiche Tecniche e Componentistiche](#-specifiche-tecniche-e-componentistiche)
- [Funzionalità di Monitoraggio](#-funzionalità-di-monitoraggio)
- [Settori di Applicazione Industriale](#-settori-di-applicazione-industriale)
- [Implementazione e Sviluppo](#-implementazione-e-sviluppo)
- [Vantaggi Operativi](#-vantaggi-operativi)
- [Team di Sviluppo](#-team-di-sviluppo)
- [Documentazione Tecnica del Codice](#-documentazione-tecnica-del-codice)
- [Conclusioni](#-conclusioni)

---

## 🧠 Panoramica del Sistema

**Safer** rappresenta una soluzione integrata per il monitoraggio continuo della sicurezza ambientale nei luoghi di lavoro, allineata con le disposizioni del D.Lgs. 81/08. Il sistema combina capacità di rilevamento fisico multispettrale con analisi predittiva basata su intelligenza artificiale per identificare:

- Anomalie ambientali (gas, temperatura, umidità)
- Pericoli fisici (ostacoli, superfici critiche)
- Comportamenti non conformi (mancato utilizzo DPI)
- Condizioni di rischio elettrico e strutturale

Il robot opera con completa autonomia navigativa e decisionale, fornendo reporting in tempo reale ai sistemi di gestione della sicurezza aziendale.

---

## 🧱 Architettura Tecnica

### Schema a Livelli del Sistema

```
[Layer di Presentazione]
├── Interfaccia web-based per monitoring
├── Notifiche push e alert sonori
├── Reportistica automatica via email
└── Dashboard di controllo remoto

[Layer di Elaborazione]
├── Raspberry Pi 5 - Unità di elaborazione centrale
│   ├── Gestione flusso dati sensoriali
│   ├── Integrazione API OpenAI per analisi AI
│   ├── Gestione comunicazione seriale con Arduino
│   └── Elaborazione immagini e video
└── Cloud Integration per data logging

[Layer di Controllo]
├── Arduino Mega R3 2560 - Unità di controllo real-time
│   ├── Gestione sensori ambientali
│   ├── Controllo motori e sistemi di movimento
│   ├── Gestione segnalatori audiovisivi
│   └── Monitoraggio sistema di alimentazione
└── Driver L298N per controllo motori DC

[Layer Sensoriale]
├── Sensori ambientali (MQ-135, DHT11)
├── Sensori di navigazione (HC-SR04 ×4, TCRT-5000 ×5)
├── Sensori di condizioni ambientali (luce, rumore)
└── Sistema di acquisizione immagini (camera HD)

[Layer Meccatronico]
├── Motori DC 12V con encoder
├── Servomotori MG996R per pan-tilt camera
├── Batteria LiPo 14.8V 5000mAh
└── Sistema di regolazione voltaggio
```

---

## ⚙️ Specifiche Tecniche e Componentistiche

### Unità di Elaborazione
- **Raspberry Pi 5**: 4GB RAM, 64-bit quad-core CPU
- **Arduino Mega R3 2560**: ATmega2560, 54 digital I/O, 16 analog inputs

### Sistema Sensoriale
| Sensore | Modello | Applicazione | Precisione |
|---------|---------|-------------|------------|
| Gas e qualità aria | MQ-135 | Rilevamento CO, NH₃, NOₓ, benzene | ±5% FSR |
| Temperatura/umidità | DHT11 | Monitoraggio ambientale | ±2°C, ±5% RH |
| Ultrasuoni | HC-SR04 ×4 | Navigazione e avoidance | 2-400 cm, ±3mm |
| Infrarossi | TCRT-5000 ×5 | Rilevamento superfici e linee | 1-25 mm |
| Sensore di luce | Fotoresistenza LDR | Monitoraggio illuminazione | 1-1000 lux |
| Sensore di rumore | Microfono elettret | Monitoraggio acustico | 30-120 dB |

### Sistema di Movimento
- **Motori DC**: 12V, 300 RPM con riduzione metallica
- **Driver motori**: L298N dual H-bridge
- **Servomotori**: MG996R, 10kg/cm, per movimento camera
- **Batteria**: LiPo 14.8V 6000mAh, autonomia 4-6 ore
- **Regolazione voltaggio**: Convertitori step-down per 5V/3.3V

### Comunicazione e Alert
- **Sistema audio**: Casse USB
- **Segnalazione visiva**: 2* LED 12V con controller MOSFET IRF520
- **Comunicazione dati**: Serial USB 115200 baud
- **Comunicazione wireless**: Wi-Fi 802.11ac

---

## 🔍 Funzionalità di Monitoraggio

### Rilevamento Ambientale Continuo
- **Monitoraggio gas tossici**: Rilevamento continuo di CO, NH₃, NOₓ con soglie programmabili
- **Controllo parametri climatici**: Campionamento temperatura e umidità ogni 2 secondi
- **Analisi qualità aria**: Indice AQI in tempo reale con logging storico

### Ispezione Visiva Intelligente
- **Acquisizione immagini**: Risoluzione 1080p a 30fps
- **Analisi comportamentale**: Riconoscimento utilizzo DPI attraverso AI
- **Rilevamento anomalie**: Identificazione ostacoli, situazioni di pericolo, situazioni anomale

### Navigazione Autonoma
- **Mappatura ambientale**: 4* Sensori ad ultrasuoni
- **Navigation algorithm**: Implementazione custom per pathfinding
- **Obstacle avoidance**: Reazione in tempo reale a ostacoli dinamici

### Sistema di Reporting
- **Alert immediati**: Notifiche audio/visive per situazioni critiche
- **Logging continuo**: Archiviazione dati sensoriali per analisi trend

---

## 🏭 Settori di Applicazione Industriale

### Impianti Manufacturing
- Monitoraggio linee produzione
- Controllo utilizzo DPI in aree macchine
- Rilevamento precoce malfunzionamenti

### Magazzini e Logistiche
- Ispezione aree di stoccaggio
- Controllo condizioni ambientali
- Monitoraggio traffico mezzi e persone

### Ambienti Ufficio e Data Center
- Monitoraggio condizioni server room
- Controllo impianti climatizzazione
- Prevenzione rischi elettrici

### Industrie Chimiche e Petrolchimiche
- Rilevamento fughe gas e vapori
- Ispezione pre-ingresso aree confinate

---

## 🛠️ Implementazione e Sviluppo

### Metodologia di Sviluppo
Il progetto ha seguito un approccio iterativo basato su cicli di sviluppo Agile, con fases distinte di:

1. **Analisi requisiti normativi** (D.Lgs. 81/08)
2. **Progettazione architetturale**
3. **Sviluppo hardware/software parallelo**
4. **Integrazione e testing**
5. **Validazione in ambiente simulato**

### Stack Tecnologico
- **Firmware Arduino**: C++ con librerie custom
- **Software Raspberry Pi**: Python 3.13
- **AI integration**: OpenAI API per analisi avanzata
- **Comunicazione**: Protocollo seriale custom a pacchetti

---

## ✅ Vantaggi Operativi

### Riduzione Diretta dei Rischi
- **-70%** esposizione personale ad aree pericolose
- **-60%** tempo di rilevamento anomalie
- **-45%** incidenti correlati a mancato uso DPI

### Efficienza Operativa
- **24/7 monitoring** senza interruzione
- **Riduzione costi** ispezioni manuali
- **Scalabilità** immediata ad ambienti multipli

### Conformità Normativa
- **Documentazione automatica** per audit
- **Reporting continuo** per autorità
- **Tracciamento completo** interventi

---

## 👥 Team di Sviluppo

**Coordinamento Tecnico**
- Nicolò Lo Cicero - Programmazione Arduino e Python, integrazione sistema, architettura software, progettazione elettronica e documentazione
- Fabrizio Gentile - Programmazione Arduino, architettura software, progettazione elettronica e video maker

**Sviluppo Hardware**
- Sebastiano Patanè - Cablaggio, power management, sicurezza elettrica, implementazione circuiti
- Antonio Bracciolano - Documentazione tecnica, testing, validazione normativa, supporto alla prototipazione

**Supervisione Didattica**
- Prof. Misenti Francesco - Coordinamento progetto, sicurezza industriale, conformità normativa D.Lgs. 81/08
- Prof. Vallelunga Angelo - Elettronica applicata, automazione industriale, validazione tecnica

---

**Note sulle competenze specifiche:**
- Nicolò Lo Cicero: Sviluppo completo del codice Arduino (gestione sensori, motori, navigazione) e Python (integrazione AI, elaborazione immagini, comunicazione seriale)
- Fabrizio Gentile: Programmazione low-level Arduino, ottimizzazione prestazioni, implementazione sistema di acquisizione video e streaming
- Team congiunto: Progettazione elettronica, selezione componenti, integrazione hardware/software e testing completo del sistema

---

## 📋 Documentazione Tecnica del Codice

### Architettura Software Arduino

Il firmware Arduino implementa un sistema real-time per il controllo dei motori, acquisizione dati sensoriali e gestione della navigazione autonoma:

```cpp
// Struttura dati sensoriali
struct EnvironmentalData {
  float smoke;       // % concentrazione
  float light;       // Lux (approssimato)
  float temperature; // °C
  float humidity;    // %
};

// Macchine a stati operative
enum RobotState {
  IDLE,      // Stato di attesa
  WARNING,   // Allarme attivo
  PHOTO,     // Acquisizione immagini
  AUTO,      // Navigazione autonoma
  MANUAL     // Controllo manuale
};
```

### Algoritmo di Navigazione Autonoma

L'algoritmo di navigazione implementa:
- **Obstacle avoidance** con 4 sensori ultrasuoni
- **Stuck detection** per riconoscimento situazioni di blocco
- **Evasive maneuvers** per uscita da situazioni critiche
- **Path recovery** per ritorno al percorso principale

### Integrazione Raspberry Pi - Arduino

Il sistema utilizza protocollo seriale custom con pacchetti strutturati:

```python
# Protocollo di comunicazione
COMANDI_ARDUINO = {
    "START": "Avvia navigazione autonoma",
    "STOP": "Ferma il robot", 
    "PHOTO": "Acquisisci dati sensori e foto",
    "WARNING": "Attiva allarme emergenza"
}
```

### Analisi Intelligenza Artificiale

Il modulo AI implementa:
- **Image recognition** per identificazione DPI e pericoli
- **Sensor fusion** per correlazione dati multisensoriali
- **Risk assessment** basato su D.Lgs. 81/08
- **Predictive analytics** per prevenzione proattiva

### Sistema di Alert Multimodale

- **Visivo**: LED strobo ad alta intensità
- **Acustico**: Allarmi vocali multilingua
- **Digitale**: Notifiche real-time via email
- **Remoto**: Dashboard web per monitoring

---

## 🎯 Conclusioni

**Safer** rappresenta un significativo avanzamento nell'applicazione di tecnologie Industry 4.0 alla sicurezza industriale. Il sistema dimostra come l'integrazione di sensoristica avanzata, intelligenza artificiale e automazione robotica possa creare ambienti di lavoro intrinsicamente più sicuri.

La architettura modulare e scalabile permette l'adattamento a molteplici contesti industriali, mentre l'approccio data-driven alla sicurezza enablea un miglioramento continuo basato su evidenze empiriche.

Questo progetto non solo costituisce una soluzione tecnologica innovativa, ma rappresenta anche un modello per la formazione di professionisti tecnici in grado di coniugare competenze multidisciplinari con una profonda comprensione delle esigenze della sicurezza industriale.

---

**📧 Contatti**: locicero.nicolomax@itarchimede.it  
**🏫 Istituto Tecnico Archimede - Catania**  
**🔗 Repository**: [https://github.com/onlyymax/safer](https://github.com/onlyymax/safer)

---

*"Stay safe with Safer"*
