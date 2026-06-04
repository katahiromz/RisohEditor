(Polish)
/////////////////////////////////////////////////////
RisohEditor przez katahiromz
/////////////////////////////////////////////////////

RisohEditor jest to darmowy edytor zasobów do programowania Win32.
Może edytować/wyodrębniać/klonować/usuwać dane zasobów
w plikach RC/RES/EXE/DLL.

Pobierz binarny: https://katahiromz.web.fc2.com/re/en

Działa w Windows XP/2003/Vista/7/8.1/10 oraz ReactOS.

Zobacz "LICENSE.txt" dla szczegółów praw autorskich i 
umowy licencyjnej.

/////////////////////////////////////////////////////

Pytanie 1. Co to jest „Risoh”?

    Odpowiedź. Słowo „Risoh” po japońsku oznacza „idealny”.

Pytanie 2. Co to są edt1, edt2, cmb1?

    Odpowiedź. Są to standardowe makra ID kontroli zdefiniowane w <dlgs.h>.

Pytanie 3. Co to jest mcdx?

    Odpowiedź. To specjalny kompilator wiadomości, który utworzyłem.
            Zobacz mcdx/MESSAGETABLEDX.md dla szczegółów.

Pytanie 4. Dlaczego podczas kompilacji z Visual Studio pojawiły się zniekształcone znaki?

    Odpowiedź. rc.exe poprawnie obsługuje UTF-16, ale przed Visual Studio 2022
            ładowanie pliku UTF-8 spowoduje śmieci w danych wyjściowych.

            Proszę użyj programu Visual Studio 2022 lub nowszego.

Pytanie 5. Jaka jest różnica pomiędzy wersją bez instalatora a wersją przenośną?

    Odpowiedź. Wersja przenośna nie korzysta z rejestru, ale z pliku ini.

Pytanie 6. Czy obsługuje wersję 64-bit?

    Odpowiedź. Tak, to prawda.

/////////////////////////////////////////////////////////////////////
Katayama Hirofumi MZ (katahiromz) [A.N.T.]
Webpage (English):    https://katahiromz.web.fc2.com/re/en
Webpage (Chinese):    https://katahiromz.web.fc2.com/re/ch
Webpage (Japanese):   https://katahiromz.fc2.page/risoheditor/
Webpage (Italian):    https://katahiromz.web.fc2.com/re/it
Webpage (Russian):    https://katahiromz.web.fc2.com/re/ru
Webpage (Portuguese): https://katahiromz.web.fc2.com/re/pt
Email                 katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
