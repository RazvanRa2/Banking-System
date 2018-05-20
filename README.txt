=== Tema 2 - PC - 2018 - Razvan Radoi - 321CA ===

== Generalitati despre implementarea temei ==

	Intre client si server se realizeaza o conexiune tcp si una udp, pe acelasi port, 
	prin descriptori diferiti. Prin aceste conexiunui, cei doi comunica.

	Fisierul utils.h contine constante atat pentru client cat si pentru server pentru
	a evita codul duplicat.

	Fisierul users.in este folosit pentru a retine datele despre utilizatori.

== Rulare ==

	./selectserver numar_port nume_fisier_utilizatori
	./client ip port

== Limitari ==
	
	Cum acest lucru nu a fost cerut explicit in textul temei, tranzactiile bancare
	nu sunt persistente. Daca serverul se inchide apoi se redeschide, conturile 
	bancare sunt resetate la starea initiala.


== Observatii ==

	Comunicatia client-server se face pe baza conventiei ca atat serverul cat si clientul
	vorbesc "corect". Mai precis, se bazeaza pe faptul ca se dau comenzi corecte, intr-o
	ordine logica. Spre exemplu, un raspuns cu "t" la o intrebare de tipul [y/n] va cauza
	un comportament nedefinit. Totusi, TOATE cazurile descrie in textul temei sunt acoperite.

	Regula de clean din make file sterge by default si fisierele .log

	Cand am testat tema, daca foloseam acelasi port de mai multe ori, cu mai multi clienti, 
	iar apoi incercam sa adaug un client nou, dura ceva mai mult pana cand se conecta
	clientul la server (cate secunde). Pe un port "proaspat" mergea mereu totul bine.
	*** Daca conexiunea nu se realizeaza, va rugam insistati ***

	

