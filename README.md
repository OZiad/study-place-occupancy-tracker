# Study Place Occupancy Tracker (SPOT)

Small end-to-end system that tracks free seats in a study area using an ESP32 node, a distance sensor on a servo, a Python API, and a web dashboard.

The goal: stop walking around campus hunting for free seats. Nodes scan a few desks, send occupancy to a server, and the dashboard shows how many seats are free at each node.

---

## Features

- ESP32/TTGO node with servo-mounted distance sensor
- Scans multiple seats in a sweep and counts free vs occupied
- Periodic uploads to a backend API (HTTP)
- Stores history of readings
- Web dashboard that shows latest occupancy per node
- Simple calibration mode on the node so it can be moved to new rooms

---

## Repo Structure

```text
esp32/                  # Firmware for the ESP32 / TTGO node
studyplace-server/      # Backend API + storage
studyplace-frontend/    # Web dashboard
design/                 # Diagrams and state machines
