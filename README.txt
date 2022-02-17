Menan Yasemin 336CC

~~~~~~~~~~~~~~~~ Tema 3 APD ~~~~~~~~~~~~~~~~

In cadrul acestei teme am implementat un program distribuit in MPI, avand
procesele grupate in trei clustere continand un proces coordonator si mai multe
procese worker.

Fiecare proces contine un vector de elemente de tip Processes in care
tine evidenta de topologia proceselor, pe care o afla comunicand cu celelate
procese. Fiecare element de tip Processes contine rangul elementului, vectorul
cu rangurile proceselor worker, numarul de procese worker si, daca este un 
worker, rangul coordonatorului.

Un proces coordonator poate comunica doar cu alte procese coordonator si cu
workerii sai. Singurele procese coordonator sunt cele cu rangul 0, 1 sau 2.

Un proces worker poate comunica doar cu coordonatorul sau.

~~~~~~~~~~~~~~~~ Flow-ul programului ~~~~~~~~~~~~~~~~

Programul poate fi impartit in doua etape: unul de stabilire a topologiei si unul
de realizare a calculelor.


----- Stabilirea topologiei -----

Proces coordonator:
1. Citeste din fisier numarul de procese worker asignate lui, dupa care retine
rangul proceselor.
2. Anunta procesele worker ca le este coordonator.
3. Trimite numarul de procese worker si vectorul cu rangurile worker-ilor la
ceilalti coordonatori si la workerii sai
4. Asteapta sa primeasca de la ceilalti coordonatori informatiile.
5. Odata ce le-a primit, isi actualizeaza topologia.
6. Trimite informatiile mai departe la workerii sai.
7. Printeaza topologia odata ce este completa.

Proces worker:
1. Asteapta sa ii fie asignat un proces coordonator.
2. Primeste rangul coordonatorului si asteapta informatii numai de la acesta.
3. Isi actualizeaza topologia cu informatiile primite.
4. Printeaza topologia odata ce este completa

----- Realizarea calculelor -----

Am incercat sa o implementez dar am probleme la unele teste in legatura cu
impartirea iteratiilor la workeri.

Coordonatorul 0:
1. Genereaza vectorul V.
2. Trimite vectorul V la ceilalti coordonatori.
3. Imparte iteratiile la numarul de workeri.
4. Calculeaza si trimite intervalul de start si end al iteratiilor, dupa care
trimite vectorul V la workerii sai.
5. Asteapta sa primeasca vectorul actualizat de la workeri si de la ceilalti
coordonatori si actualizeaza valorile din intervalul dat.
6. Afiseaza noul vector.

Coordonatorul 1/2:
1. Primeste vectorul V de la coordonatorul 0.
2. Imparte iteratiile la numarul de workeri.
4. Calculeaza si trimite intervalul de start si end al iteratiilor, dupa care
trimite vectorul V la workerii sai.
5. Asteapta sa primeasca vectorul actualizat de la workeri.
6. Trimite fiecare vector actualizat catre coordonatorul 0.

Proces worker:
1. Primeste intervalul pentru realizarea calculelor si vectorul V de la
coordonator.
2. Realizeaza calculele.
3. Trimite noul vector la coordonator.