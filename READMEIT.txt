(italiano)
/////////////////////////////////////////////////////
RisohEditor di katahiromz
/////////////////////////////////////////////////////

RisohEditor è un editor di risorse gratuito per Windows, 
creato da Katayama Hirofumi MZ.

Link download: https://katahiromz.web.fc2.com/re/en

Funziona con Windows XP/2003/Vista/7/8.1/10 e ReactOS.

Per la standardizzazione degli ID risorse vedi file "Standardize.md".

per dettagli sul copyrights e sull'accordo di licenza vedi il 
file "LICENSE.txt".

NOTA: Ti consigliamo di non installare RisohEditor in "C:\Program Files" o 
"C:\Program Files (x86)". Questo poichè il compilatore di 
risorse (windres) non accetta il carattere spazio nel percorso 
del file. 

/////////////////////////////////////////////////////

Domanda 1. Che cos'è "Risoh"?

    Ripsosta. La parola "Risoh" significa "ideale" in Giapponese.

Domanda 2. Che cosa sono edt1, edt2, cmb1?

    Risposta. Sono gli ID delle macro di controllo definite in <dlgs.h>.

Domanda 3. Non trovo i miei ID risorsa nell' [Elenco ID risorse] 
            Window.

    Risposta. Per prima cosa, configura le impostazioni [Associazione ID] 
            nel menu [File] -> [Associazione ID]. Va poi caricato il necessario 
            file "resource.h" per ottenere i nomi corretti degli ID risorse.

Domanda 4. Che cos'è mcdx?

    Risposta. È uno speciale compilatore messaggio che ho realizzato.
              Per i dettagli vedi mcdx/MESSAGETABLEDX.md.

Domanda 5. Perchè ottengo caratteri confusi quando compilo con Visual Studio?

    Risposta. Il compilatore di risorse di MSVC ha un problema nel trattamento 
              dei file risosre codificati in UTF-8.

              Usa UTF-16 (ma UTF-16 non è supportato in windres di GNU).

Domanda 6. Qual è la differenza tra nessuna versione di installazione e portatile?

    Risposta. La versione portatile non utilizza il registro ma un file ini.

/////////////////////////////////////////////////////
// CRONOLOGIA VERSIONI PROGRAMMA

- v. 0.6 (31.08.2017)
  Prima versione.
- v. 5.0.8 (12.09.2018)
  Migliorate icone della barra strumenti.
  Corretti tipi di file durante l'estrazione.
  Scrittura riferimento sorgente per facilitare il rilevamento UTF-8.
  Corrette icone vista struttura.
  Divisi modelli risorse RT_DIALOG e RT_MENU nelle varie lingue.
- v. 5.0.9 (27.09.2018)
  Migliorato output di resource.h.
  Migliorata icona del file vista struttura.
  Migliorata l'importazione.
- v. 5.1.0 (08.10.2018)
  Migliorate finestre di dialogo "Inserimento stringa" (multilinea).
  Corretto comportamento Ctrl + A.
  Controllo blocco dei file durante il salvataggio di un file.
  Correzione ID risorsa sensibili al maiuscolo/minuscolo.
  Supporto codifica degli elementi delle risorse.
  Corretta chiusura anomala nella finestra di dialogo "Lingue".
- v. 5.1.1 (08.11.2018)
  Accetta spazio nella casella di testo della didascalia.
  Accetta spazio nella casella di testo della lingua.
- v. 5.1.2 (08.12.2018)
  Accetta lo spazio nella casella di testo del nome della classe della finestra.
  Accetta il valore stringa letterale C nella casella di testo Nome classe finestra.
  Migliorato sistema modelli.
  Abilitato modello manifest.
  Corretto aggiornamento dell'interfaccia utente della barra strumenti.
  Visualizza correttamente i tipi di ID in "Elenco ID risorsa".
  Ordina correttamente le lingue in output.
- v. 5.1.3 (01.01.2019)
  Corretto "Elenco ID risorsa".
- v. 5.1.4 (13.01.2019)
  Aggiunto programma di installazione inglese.
  Aggiunto programma di installazione italiano.
  Aggiunta traduzione italiana.
  Aggiunto alcuni programma installazione diverse lingue.
- v. 5.1.5 (27.01.2019)
  Non usa lo stile esteso WS_EX_MDICHILD.
  Aggiunto supporto ReactOS.
- v. 5.1.6 (24.02.2019)
  Migliorata traduzione italiana.
  Modificata la GUI.
  Correzione "Clona in una nuova lingua".
- v. 5.1.7 (20.03.2019)
  Aggiunti gli stili PBS_MARQUEE e PBS_SMOOTHREVERSE.
  Corretto l'errore del processo di compilazione.
- v. 5.1.8 (14.05.2019)
  Aggiunto il controllo della ricompilazione al momento della clonazione.
  Corretta la selezione dopo la clonazione.
  Corretta situazione in caso di errore di compilazione della tabella delle stringhe e della tabella dei messaggi.
- v. 5.1.9 (14.07.2019)
  Supporto UTF-16 per in/out sorgenti.
- v. 5.2.0 (26.07.2019)
  Corretta gestione valori DIALOG STYLE (WS_CAPTION è del valore predefinito di DIALOG STYLE).
  Risolto un bug per cui l'applicazione cancellava inaspettatamente il file quando apriva un file EXE compresso senza espanderlo.
- v. 5.2.1 (04.08.2019)
  Viene selezionato "Inglese (Stati Uniti)" se si immette "En" per il nome della lingua.
  Aggiunta funzione "Costante query".
  Corretta funzione "Comprimi tutto".
  WS_POPUPWINDOW | WS_BORDER deve essere WS_POPUPWINDOW | WS_CAPTION.
  Supporto RT_FONT.
  Migliorata finestra di dialogo "Aggiungi risorsa".
- v. 5.2.2 (14.08.2019)
  Aggiornata la traduzione italiana.
  Supporto tipi di risorse XML, XSLT, SCHEMA e REGISTRY.
  Migliorata interpretazione in caso di mancata corrispondenza della lingua di RT_ICON / RT_CURSOR e RT_GROUP_ICON / RT_GROUP_CURSOR.
  Migliorata interpretazione in caso di mancata corrispondenza della lingua di RT_DLGINIT e RT_DIALOG.
- v. 5.2.3 (14.09.2019)
  Corretti STILE AUTORADIOBUTTON.
- v. 5.2.4 (15.09.2019)
  Aggiunto supporto XP.
- v. 5.2.5 (19.09.2019)
  Risolto un bug WS_CHILDWINDOW nella casella di riepilogo dello stile della finestra di dialogo.
  Modificato il nome file del programma/versione (in RisohEditor-X.X.X.exe e RisohEditor-X.X.X.zip).
  Aggiunto TRANSLATORS.txt.
  Resi visibili i controlli disegnati dal proprietario.
- v. 5.2.6 (23.09.2019)
  Migliorata finestra di dialogo selezione lingua.
  Aggiunta la traduzione in russo.
  Visualizzazione messaggio di errore se il percorso di installazione contiene degli spazi.
  Aggiunta opzione --use-temp-file nel richiamo di windres per risolvere il problema popen.
- v. 5.2.7 (20.10.2019)
  Nuovo miglioramento finestra di dialogo selezione lingue.
  Corrette posizioni dei segni degli elementi della finestra di dialogo.
- v. 5.2.8 (30.01.2020)
  Abilitata visualizzazione controlli indefiniti.
  Aggiunta opzione "Usa BEGIN / END".
  Risolto un bug per cui la DLL non poteva essere salvata con un altro nome.
- v. 5.2.9 (01.02.2020)
  Non era stato attivato il supporto per XP. Ora è attivo.
  Corretta elaborazione salvataggio file.
  Ora è possibile salvare file EXE/DLL senza proprietà utente dell'eseguibile.
  Corretto come effettuare backup.
- v. 5.3.0 (06.02.2020)
  Aggiunto ES_AUTOHSCROLL in alcuni riquadri testo.
  Corretto l'errore logico di sovrascrittura file.
- v. 5.3.1 (23.02.2020)
  Corretta e migliorata lettura/visualizzazione menu.
  Migliorata gestione IDC_STATIC.
  Migliorata finestra "Elenco ID risorse".
- v. 5.3.2 (02.03.2020)
  Abilitato salvataggio con 'Ctrl+S' senza richiesta del percorso.
  Resa non volatile l'opzione "Destinazione file RC come UTF-16".
  Portata in primo piano finestra "Elenco ID risorse".
- v. 5.3.3 (03.03.2020)
  Ridotta dimensione del file da 9 MB a 3MB.
- v. 5.3.4 (19.03.2020)
  Modificato nella barra strumenti il comportamento del pulsante 'Salva'.
  Introdotta automazione usando il linguaggio programmazione EGA.
  Aggiunta conferma salvataggio in caso di modifiche del file.
- v.5.3.5 (26.03.2020)
  Migliorata traduzione lingua italiana.
  Correzione modulo salvataggio configurazione.
- v.5.3.6 (15.04.2020)
  Evitato loop infino mediante modifica della variabile loop a 32bit.
  Supporto dati Delphi DFM.
  Migliorato menu contestuale.
- v.5.3.7 (28.04.2020)
  Aggiornata flag modifica file quando si trascina un file.
  Migliorata gestione nome file durante estrazione.
  Rafforzato EGA.
  Migliorata traduzione lingua russa.
- v.5.3.8 (22.05.2020)
  Aggiunto elemento "Apri manuale EGA" nel menu "Automazione"
  Controllo rigoroso flag modifica file.
- v.5.3.9 (01.06.2020)
  Aggiunta funzione "sostituisci-font-finestra".
  Spostati alcuni funzioni nel menu "Modifica".
  Aggiunto controllo valore-zero del nome risorsa e tipo risorsa.
  Aggiunto stile DS_CENTER al modello finestra dialogo.
  Abilitati tasti funzione F1, F3, F5 e F6.
  Aggiunta versione portatile.
  Potenziata funzione ricerca.
- v.5.4.0 (YY.XX.2020)
    - Fixed "Query Constant" dialog.
    - Fixed "Encoding of Resource Item" dialog.
    - Improved "ID Association" dialog.
    - Improved "Predefined Macros" dialog.
    - Improved "Configuration" dialog.
    - Fixed RT_ACCELERATOR output (unsigned 16-bit).
    - Removed "Store into res folder" option.

/////////////////////////////////////////////////////////////////////
// Katayama Hirofumi MZ (katahiromz) [A.N.T.]
// Webpage (English):  https://katahiromz.web.fc2.com/re/en
// Webpage (Chinese):  https://katahiromz.web.fc2.com/re/ch
// Webpage (Japanese): https://katahiromz.web.fc2.com/re/ja
// Webpage (Italian):  https://katahiromz.web.fc2.com/re/it
// Webpage (Russian):  https://katahiromz.web.fc2.com/re/ru
// Email               katayama.hirofumi.mz@gmail.com
/////////////////////////////////////////////////////////////////////
