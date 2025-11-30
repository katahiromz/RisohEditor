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

     Jawaban. rc.exe dengan benar mendukung UTF-16, tetapi sebelum Visual Studio 2022,
              memuat file UTF-8 akan mengakibatkan sampah dalam data keluaran.
              Gunakan UTF-16 (tetapi, UTF-16 tidak didukung dalam GNU windres).

Pertanyaan 5. Apa perbedaan dari versi "no installer" dan portabel?

     Jawaban. Versi portabel tidak menggunakan registri, tetapi mengunakan berkas "*.ini".

Pertanyaan 6. Apakah mendukung 64-bit?

     Jawaban. Ya, mendukung.

/////////////////////////////////////////////////////////////////////
Katayama Hirofumi MZ (katahiromz) [A.N.T.]
Laman web (Inggris): https://katahiromz.web.fc2.com/re/en
Laman web (Cina):    https://katahiromz.web.fc2.com/re/ch
Laman web (Jepang):  https://katahiromz.fc2.page/risoheditor/
Laman web (Italia):  https://katahiromz.web.fc2.com/re/it
Laman web (Rusia):   https://katahiromz.web.fc2.com/re/ru
Email                katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
