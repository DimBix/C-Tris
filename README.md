# TEST DA EFFETTUARE
### accensione server 
- senza argomenti 
- con tempo non valido
- con troppi agomenti 
- con troppi pochi argomenti
- più di un carattere per player
- esecuzione normale
---
### accensione client
- server spento
- senza argomenti
- troppi argomenti
- client normale
- client con bot
- cercare di avviare un client quando ci sono gia 2 giocatori collegati
- cercare di avviare client con bot quando 1 giocatore è gia collegato
---
### esecuzione server
- premere ctrl+c e aspettare che il timer scada (sia in attesa di giocatori che durante la partita), verificare che il ctrl+c non rovini il gioco (skip turni, blocco fine turno)
- premere ctrl+c e verificare che il programma di chiuda correttamente ed elimini semafori e memoria condivisa, nel caso di partita in corso o giocatori collegati verificare anche che questi vengano scollegati correttamente
- a partita finita verificare che il server comunichi il risultato ai client, li termini e si termini correttamente
- controllare che il server si comporti correttamente quando un client si disconnette, comunicare all'altro la vittoria e terminarlo
---
### esecuzione client
- controllare corretto ignoramento di input non corretti
- controllare che l'input inserito durante il NON turno venga ignorato e non considerato durante il turno
- verificare la corretta uscita dalla partita e relativa comunicazione al server
- controllare corretti messaggi di errore nel caso in cui il server si chiuda, sia durante l'attesa di giocatori che durante la partita
