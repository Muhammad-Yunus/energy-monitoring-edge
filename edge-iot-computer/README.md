# Panduan Instalasi & Deployment Flow Node-RED (BeagleBone Black)

Dokumen ini berisi langkah-langkah lengkap untuk menyiapkan lingkungan sistem, menginstal dependensi aplikasi & library Node-RED, serta melakukan deployment flow monitoring pada perangkat **BeagleBone Black** (menggunakan default user `debian`).

## 1. Instalasi Dependensi Sistem (OS)

Pastikan sistem diperbarui dan paket yang dibutuhkan (seperti `ffmpeg` untuk perekaman video RTSP dan `sqlite3`/`build-essential` untuk kompilasi database lokal) sudah terpasang.

```
sudo apt-get update
sudo apt-get install -y ffmpeg sqlite3 build-essential python3

```

## 2. Persiapan Direktori & Hak Akses

Flow ini membutuhkan direktori `/data` untuk menyimpan rekaman video `.mp4` dan direktori `/sqlite/energy-monitoring` untuk menyimpan file database SQLite.

```
# Buat folder penyimpanan video dan atur kepemilikan ke user debian
sudo mkdir -p /data
sudo chown -R debian:debian /data

# Buat folder penyimpanan database SQLite dan atur kepemilikan ke user debian
sudo mkdir -p /sqlite/energy-monitoring
sudo chown -R debian:debian /sqlite/energy-monitoring

```

## 3. Instalasi Library / Palette Node-RED

Masuk ke direktori kerja Node-RED dan instal ketiga modul tambahan yang dibutuhkan oleh flow:

* `@flowfuse/node-red-dashboard` (Dashboard 2.0 UI)

* `node-red-node-sqlite` (Koneksi database SQLite)

* `node-red-contrib-condition-monitoring` (Deteksi anomali sensor arus)

```
cd ~/.node-red
npm install @flowfuse/node-red-dashboard@1.35.0 node-red-node-sqlite@2.0.1 node-red-contrib-condition-monitoring@0.3.3

```

*(Catatan: Proses kompilasi `node-red-node-sqlite` pada BeagleBone Black mungkin memakan waktu beberapa menit).*

## 4. Konfigurasi `httpStatic` pada `settings.js`

Agar Dashboard Node-RED dapat memutar rekaman video `.mp4` secara langsung dari folder `/data/`, aktifkan dan arahkan parameter `httpStatic` di file konfigurasi Node-RED.

1. Buka file konfigurasi:

   ```
   nano ~/.node-red/settings.js
   
   ```

2. Cari baris `httpStatic` (biasanya masih berupa komentar `//httpStatic: ...`), hapus tanda komentarnya, dan ubah menjadi:

   ```
   httpStatic: '/data/',
   
   ```

3. Simpan perubahan (`Ctrl + O`, lalu `Enter`) dan keluar dari editor (`Ctrl + X`).

## 5. Restart Layanan Node-RED

Terapkan semua perubahan konfigurasi dan library baru dengan me-restart service Node-RED:

```
sudo systemctl restart nodered

```

Untuk memastikan service berjalan normal tanpa error:

```
sudo systemctl status nodered

```

## 6. Upload / Import Flows JSON

Setelah Node-RED kembali aktif, lakukan langkah berikut untuk mengunggah flow:

1. Buka editor Node-RED melalui browser di alamat:
   `http://<IP_BEAGLEBONE_BLACK>:1880`

2. Klik ikon **Menu (☰)** di pojok kanan atas, lalu pilih **Import** (atau tekan `Ctrl + I`).

3. Pilih file `nodered-flows.json` (atau paste isi JSON-nya), pilih opsi **Import to: new flow** (atau *replace*), lalu klik tombol **Import**.

4. Klik tombol **Deploy** di pojok kanan atas untuk mengaktifkan seluruh flow.

## 7. Inisialisasi Awal (Wajib Dilakukan Pertama Kali)

Agar database dan konfigurasi sesuai dengan perangkat Anda:

1. **Buat Tabel Database SQLite:**

   * Buka tab **Insert DB** di editor Node-RED.

   * Klik tombol pada node inject **`CREATE TABLE SENSOR LOGS`** satu kali untuk membuat tabel `sensor_logs` dan indeksnya secara otomatis.

2. **Sesuaikan Environment Variables (Opsional):**

   * Buka menu **Global Configuration** / **Environment Variables** pada pengaturan flow untuk menyesuaikan kredensial dan alamat IP jika berbeda:

     * `API_KEY` (Isi dengan API key yang digunakan)

     * `RTSP_USER`, `RTSP_PASS`, `RTSP_HOST`, `RTSP_PORT`

     * `EM_API_BASE` & `EM_API_KEY`

     * `EM_VIDEO_PATH` (Pastikan bernilai `/data/`)

3. **Akses Dashboard:**

   * Buka tampilan antarmuka monitoring di alamat:
     `http://<IP_BEAGLEBONE_BLACK>:1880/dashboard/event-viewer`