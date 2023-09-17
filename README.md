Pentru implementarea temei, am petrecut in jur de 8 ore + timpul pentru a ma documenta despre protocolul TCP din videoclipurile
oferite in laborator. Am realizat complet toate cerintele temei, codul pentru crearea server-ului folosind socketii UDP si TCP
a fost preluat din laboaratoarele 6 si 8, pentru clientul TCP din laboratorul 8. M-am gandit sa folosesc API-ul cu select la file
descriptori(API gasit pe internet). Server-ul primeste mesaje de la clientii 
TCP, trecandu-le in baza de date(in structura Client) topic-urile la care acestia au dat subscribe. Cand se primeste un mesaj
de la un client UDP, se trimite mesajul catre toti clientii care sunt abonati la acest topic(se verifica baza de date). De asemenea,
cand un client da unsubscribe, se foloseste o simpla eliminare din vector(se trece la final si se scade dimensiunea, daca vom
adauga un element nou se va scrie peste el). Clientii nu sunt stersi din vector dupa ce se deconecteaza, ci dupa deconectare se trece
parametrul de connected pe 0 si file descriptor-ul pe -1, urmand ca la urmatoarea conectare sa ii fie atribuit un nou file descriptor.
Am realizat store and forward folosind "buffere" de tipul Message stocate in fiecare structura de tip client, verificand daca un client
este abonat cu SF = 1(avem un vector de sf pentru fiecare topic la care este acesta abonat, am avut grija si la eliminare sa ramana
SF-urile corecte) sa se stocheze mesajul. Structura de tip Message contine informatiile dorite la afisare, IP-ul si port-ul clientului
UDP si buffer-ul propriu zis, verificarea si afisarea pentru fiecare tip fiind un simplu lucru cu memoria si trecerea din network
order in sistem order. Tin sa mentionez ca pe langa teste am facut si foarte multe verificari manuale cu subscribe-uri si unsubcribe-uri.
Singura dificultate intampinata in executia temei este ambiguitatea oferita de checker, care imi spunea ca nu mi se deconecteaza clientul
si la toate testele de dupa primeam timeout doar pentru ca am uitat sa pun '.' in mesajul de deconectare. A trebuit sa inspectez
codul checker-ului ca sa imi dau seama de aceasta eroare. De asemenea, nu am implementat fluxul de octeti deoarece nu am avut nici
o problema cand am rulat sugestiile de pe forum, imi ruleaza cum trebuie.
De asemenea, trebuie sa mentionez ca parsez mesajul in client folosind structura din server_helper, deoarece si daca am lucra cu 
un buffer de tipul char* oricum ar trebui sa stim la ce indici se afla datele ca sa ii dam unpack, este mult mai usor asa si nu
as vedea de ce ar fi mai buna modalitatea cealalta.
