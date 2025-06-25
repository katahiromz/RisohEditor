(italiano)
/////////////////////////////////////////////////////
RisohEditor di katahiromz
/////////////////////////////////////////////////////

RisohEditor è un editor di risorse gratuito per Windows, 
creato da Katayama Hirofumi MZ.

Link download: https://katahiromz.web.fc2.com/re/en

Funziona con Windows XP/2003/Vista/7/8.1/10 e ReactOS.

Per la standardizzazione degli ID risorse vedi file "Standardize.md".

per dettagli sul copyright e sull'accordo di licenza vedi il 
file "LICENSE.txt".

/////////////////////////////////////////////////////

Domanda 1. Che cos'è "Risoh"?

    Risposta. La parola "Risoh" significa "ideale" in Giapponese.

Domanda 2. Che cosa sono edt1, edt2, cmb1?

    Risposta. Sono gli ID delle macro di controllo definite in <dlgs.h>.

Domanda 3. Che cos'è mcdx?

    Risposta. È uno speciale compilatore messaggio che ho realizzato.
              Per i dettagli vedi mcdx/MESSAGETABLEDX.md.

Domanda 4. Perché ottengo caratteri confusi quando compilo con Visual Studio?

    Risposta. Il compilatore di risorse di MSVC ha un problema nel trattamento 
              dei file risorse codificati in UTF-8.

              Usa UTF-16 (ma UTF-16 non è supportato in windres di GNU).

Domanda 5. Qual è la differenza tra nessuna versione di installazione e portatile?

    Risposta. La versione portatile non utilizza il registro ma un file ini.

Domanda 6. Supporta i 64 bit?

    Risposta. Sì, li supporta.

/////////////////////////////////////////////////////////////////////
// Katayama Hirofumi MZ (katahiromz) [A.N.T.]
// Sito web (inglese):    https://katahiromz.web.fc2.com/re/en
// Sito web (cinese):     https://katahiromz.web.fc2.com/re/ch
// Sito web (giapponese): https://katahiromz.fc2.page/risoheditor/
// Sito web (italiano):   https://katahiromz.web.fc2.com/re/it
// Sito web (russo):      https://katahiromz.web.fc2.com/re/ru
// Sito web (portoghese): https://katahiromz.web.fc2.com/re/pt
// Email:                 katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
