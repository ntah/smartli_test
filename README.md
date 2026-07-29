# SmartLi BMS Parser

Workspace terpisah untuk pengembangan parser dan monitoring Huawei SmartLi BMS.

## Rencana awal

- Identifikasi model dan versi SmartLi.
- Tentukan jalur komunikasi: Modbus TCP, Modbus RTU, HTTPS, atau SNMP.
- Kumpulkan contoh respons/data mentah perangkat.
- Susun register map dan konversi nilai.
- Buat parser, pengujian, dan penyimpanan data.

## Informasi perangkat yang dibutuhkan

- Model SmartLi.
- Versi firmware.
- IP dan port komunikasi jika menggunakan jaringan.
- Modbus Unit/Slave ID.
- Contoh dump register atau respons mentah.

## External component ESPHome

Struktur komponen:

```text
components/
└── smartli_bms/
    ├── __init__.py
    ├── sensor.py
    ├── smartli_bms.cpp
    └── smartli_bms.h
```

Gunakan `smartli-external-example.yaml` sebagai konfigurasi awal. Komponen
saat ini bersifat read-only dan mengirim command telemetry `0x01`.

Field yang sudah dipublikasikan:

- Arus.
- Tegangan pack dan bus.
- SOC dan SOH.
- Full capacity dan remaining capacity.
- Total charge dan discharge Ah.
- Tegangan Cell 1–15.
- Tegangan cell minimum, maksimum, dan delta.

Arti field selain yang sudah terverifikasi dari log belum dipublikasikan untuk
menghindari pemberian label atau satuan yang salah.
