
## Bewegung

Eine Motor hat drei States
- Gestoppt
- Läuft Rechts
- Läuft Links
3^1 = 3 = {
    0, // Aus
    1, // drehe nach Recht
    2 // drehe nach Links
    }

Zwei Motoren haben: 3^2 = 9 = {
    0, // Beide Aus,
    1, // Motor1 Aus,
    2, // Motor1 Rechts,
    3, // Motor1 Links,
    4, // Motor2 Aus,
    5, // Motor2 Rechts,
    6, // Motor2 Links,
    7, // Beide nach Rechts,
    8, // Beide nach Links
    }

## Geschwindigkeit
1. beide Motoren können eine Feste Geschwindigkeit haben
    nichts zu tun
2. beide Motoren können drei Stufig, gleiche Geschwindigkeit haben
    steigend/fallend = 2
    auswählbares = 3
3. beide Motoren können zehn Stufig, gleiche Geschwindigkeit haben
    steigend/fallend = 2
    auswählbares = 10
4. beide Motoren können drei Stufig, unterschiedlich Geschwindigkeit haben
    steigend/fallend = 4
    auswählbares = 6
5. beide Motoren können zehn Stufig, unterschiedlich Geschwindigkeit haben
    steigend/fallend = 4
    auswählbares = 20

**Auswahl**: 2

## Statusausgabe
später

## MQTT Implementierung
nach mein bisherigen Wissensstand kann der Mbed nur zu ein Mqtt-Topic 
abonieren (updates erhalten).
Es können zwar theoratisch zu mehrere Topics publiziert werden aber mit 
publizieren klappt es nicht immer vernunftig. Es kann möglicherweise mit 
bessere Verwaltung von Yield (prüfung nach einkommende Nachrichten) besser 
implementiert werden, aber es ist derzeit nicht erreicht.

### Idee
Es ist zu prüfen ob auch die updates von Kinder Topics vermittelt werden

Beispiel Struktur von einer Mqtt Topic-Baum
- Topic 1
    - Topic 1.1
        - Topic 1.1.1
        - Topic 1.1.2
    - Topic 1.2
    - Topic 1.3
- Topic 2
    - Topic 2.1
    - Topic 2.2
    - Topic 2.3

In der obigen Beispiel, habe ich bisher nur ein Topic aboniert und desen updates 
beachtet.

### Wildcard Topic abos
In allgemein Mqtt system, kann ein Client/Node auch zu ein sogenannte Wildcard Topic 
abonieren. Z.b. an Topic 1#, heißt, alle Topics die unter Topic 1 kommen.
Es ist zu prüfen ob es auch mit Mbed realisierbar ist.

## Note: Information in ein erhaltenen Mqtt Nachricht
- Payload       void, muss auf char konvertieren
- PayloadLength int
- QOS           enum {0,1,2}
- Duplicate     bool
- Id            ?
- Retained      bool

