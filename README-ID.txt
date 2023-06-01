(Indonesian)
/////////////////////////////////////////////////////
RisohEditor oleh katahiromz
/////////////////////////////////////////////////////

RisohEditor adalah pengedit sumber daya gratis untuk pengembangan win32.
Ini bisa mengedit/ekstrak/klon/hapus data sumber daya dalam
berkas RC/RES/EXE/DLL.

Pengunduhan binary: https://katahiromz.web.fc2.com/re/en

Bekerja pada Windows XP/2003/Vista/7/8.1/10 dan ReactOS.

Lihat "LICENSE.txt" untuk rincian hak cipta dan
lisensi persetujuan.

/////////////////////////////////////////////////////

Pertanyaan 1. Apa itu "Risoh"?

     Jawaban. Dalam Bahasa Jepang, "Risoh" berarti "ideal".

Pertanyaan 2. Apa itu edt1, edt2, cmb1?

     Jawaban. Semua itu adalah makro ID kontrol standar yang disebut dalam <dlgs.h>.

Pertanyaan 3. Apa itu mcdx?

     Jawaban. itu adalah pesan spesial pada penyusun yang saya buat.
              Lihat mcdx/MESSAGETABLEDX.md untuk rincian.

Pertanyaan 4. Mengapa saya mendapat karakter acak ketika menyusun dengan Visual Studio?

     Jawaban. Penyusun Sumber daya pada MSVC memiliki bug dalam menangani
              berkas sumber daya UTF-8.

              Gunakan UTF-16 (tetapi, UTF-16 tidak didukung dalam GNU windres).

Pertanyaan 5. Apa perbedaan dari versi "no installer" dan portabel?

     Jawaban. Versi portabel tidak menggunakan registri, tetapi mengunakan berkas "*.ini".

Pertanyaan 6. Apakah berkas 64-bit didukung?

    Jawaban. Ya, pada Windows 64-bit. Bagaimanapun layar emulasi WoW64 mencegah
             pemuatan dari "C:\Program Files" atau "C:\Windows\system32".
             Kamu harus menyalin berkas 64-bit di tempat lain sebelum memuatnya.

/////////////////////////////////////////////////////////////////////
Katayama Hirofumi MZ (katahiromz) [A.N.T.]
Laman web (Inggris): https://katahiromz.web.fc2.com/re/en
Laman web (Cina):    https://katahiromz.web.fc2.com/re/ch
Laman web (Jepang):  https://katahiromz.web.fc2.com/re/ja
Laman web (Italia):  https://katahiromz.web.fc2.com/re/it
Laman web (Rusia):   https://katahiromz.web.fc2.com/re/ru
Email                katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
