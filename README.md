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
bersifat read-only dan membaca dua kelompok data pada UART/RS485 yang sama:

- Telemetri baterai biner command `0x01`, mengikuti `update_interval`.
- Data DCDC ASCII `CID1 E5 / CID2 92`, mengikuti
  `dcdc_update_interval` (default 60 detik).

Permintaan pada setiap pack dimasukkan ke satu antrean agar respons tidak
bertumpuk. Contoh YAML sudah memuat sensor DCDC yang berhasil dipetakan dari
log aplikasi Windows.

Satu instance `smartli_bms` menangani seluruh pack pada satu UART. Pack
didaftarkan melalui `packs:` dan dipoll bergiliran agar respons tidak
bertabrakan.

`modbus_address` tetap disimpan manual untuk setiap item `packs` sebagai
referensi saat komponen ini nanti digabungkan dengan konfigurasi lain. Nilai
default 214 di payload DCDC tidak dipakai untuk menentukan alamat. Alarm
Status 1-5 berasal langsung dari field `0x06` telemetri. Component ini tidak
lagi membaca register Modbus Protection Status (`0x103C`) dan Operating
Status (`0x103D`).

PCB barcode dan pack barcode tetap dibaca dari protokol SmartLi dan
ditampilkan sebagai text sensor. Dua text sensor pembanding juga membaca
barcode memakai `modbus_address` manual: `modbus_pcb_barcode` dari register
`0x104D` sebanyak 10 register dan `modbus_pack_barcode` dari register
`0x1065` sebanyak 10 register. Component tidak melakukan pemindaian alamat
Modbus otomatis.

Alarm telemetri diterjemahkan menjadi satu text sensor `status` per pack.
Jika tidak ada bit masalah aktif nilainya `Normal`. Jika lebih dari satu
masalah aktif, keterangannya digabung dengan koma.

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
