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

Permintaan DCDC dikirim satu detik setelah telemetri agar kedua respons tidak
bertumpuk. Contoh YAML sudah memuat seluruh sensor DCDC yang berhasil
dipetakan dari log aplikasi Windows.

Satu instance `smartli_bms` menangani seluruh pack pada satu UART. Pack
didaftarkan melalui `packs:` dan dipoll bergiliran agar respons tidak
bertabrakan.

Alamat Modbus ditemukan otomatis:

1. Component membaca PCB barcode pack dengan command `0x42`.
2. Alamat Modbus 214-221 dipindai melalui register `0x104D` sebanyak
   10 register.
3. Barcode dinormalisasi dan dicocokkan.
4. Alamat yang cocok dipakai untuk membaca alarm, protection, dan operating
   status pada register `0x1037` sampai `0x103D`.

Nilai alamat default 214 yang terdapat di payload DCDC tidak digunakan
sebagai hasil discovery. Sensor `dcdc_modbus_address` baru dipublikasikan
setelah PCB barcode cocok atau `modbus_address` diisi manual.

`modbus_address` tetap dapat diisi pada salah satu item `packs` sebagai
override/fallback. Ini diperlukan jika firmware tertentu tidak menyediakan
barcode yang dapat dicocokkan.

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
