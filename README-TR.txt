(Türkçe)
/////////////////////////////////////////////////////
katahiromz RisohEditor' ü gururla sunar
/////////////////////////////////////////////////////

RisohEditor Win32 geliştirme yapmanızı sağlayan ücretsiz kaynak(resource) editörüdür.

RC/RES/EXE/DLL dosyalarındaki kaynak(resource) verisini 
düzenleyebilir/çıkarabilir/klonlayabilir/silebilir.

Çalıştırılabilir dosyayı buradan indirebilirsiniz: https://katahiromz.web.fc2.com/re/en

Windows XP/2003/Vista/7/8.1/10 ve ReactOS de çalışır.

Telif hakları(copyrights) ve lisans anlaşmasının detayları için 
"LICENSE.txt" dosyasına bakın.

/////////////////////////////////////////////////////

Soru 1. "Risoh" ne demek?

    Cevap. "Risoh" kelimesi japoncada "ideal" anlamına gelmektedir.
	
Soru 2. edt1, edt2 ve cmb1 nedir?

    Cevap. Bunlar <dlgs.h> dosyasında tanımlanmış standart kontrol ID makrolarıdır.

Soru 3. mcdx nedir?

    Cevap. O benim yapmış olduğum özel mesaj derleyicisdir.
	   Detaylar için mcdx/MESSAGETABLEDX.md dosyasına bakınız.

Soru 4. Visual studio ile derlediğim zaman neden anlamsız karakterler görüyorum/alıyorum?

    Cevap.  MSVC' nin kaynak(resource) derleyicisinin UTF-8 içeren 
 	    kaynak(resource) dosyalarını işleme konusunda bir hatası var. 

            UTF-16 kullanın (fakat UTF-16 GNU windres tarafından desteklenmez).

Soru 5. Kurulumsuz(no installer) ve taşınabilir(portable) versiyon arasındaki fark nedir?

    Cevap. Taşınabilir(portable) versiyon kayıt defterini(registry)' i kullanmaz 
	onun yerine bir ini dosyasını kullanır.

Soru 6. 64-bit dosyaları destekliyor mu?

    Cevap. 64-bit destekleyen Windows sürümünde destekler. Ancak Wow64 emülasyon katmanı
           "C:\Program Files" veya "C:\Windows\system32" dizinlerinden dosya yüklenmesini engelliyor
           bu yüzden 64-bit dosyayınızı yüklemeden önce başka bir dizine kopyalamanız gerekiyor. 
 
/////////////////////////////////////////////////////////////////////
Katayama Hirofumi MZ (katahiromz) [A.N.T.]
Web sitesi (İngilizce): https://katahiromz.web.fc2.com/re/en
Web sitesi (Çince):     https://katahiromz.web.fc2.com/re/ch
Web sitesi (Japonca):   https://katahiromz.web.fc2.com/re/ja
Web sitesi (İtalyanca): https://katahiromz.web.fc2.com/re/it
Web sitesi (Rusça):     https://katahiromz.web.fc2.com/re/ru
Email                   katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
