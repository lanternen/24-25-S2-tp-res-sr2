/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
* TRANSFERT DE DONNEES  v0                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"
#include <unistd.h>
#include <stdlib.h>

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{

    /* --------------------------------------------------
       fonctionnalité pour rentrer la taille de la fenêtre
       --------------------------------------------------- */

    // si argc == 1 la fenetre est 'par défaut', sinon, elle est de taille argv[1]
    if (argc > 2) {
        perror ("Trop d'arguments\n");
        exit(2);
    }

    int taille_fenetre;
    if (argc == 2) {
        int inter = atoi(argv[1]);
        // pour respecter la règle de SR en w / 2 <= n 
        if (taille_fenetre_correcte_SR(inter)){
           taille_fenetre = inter;
        } else {
           perror("taille de fenetre trop grande, tdd3.1 emetteur\n");
           exit(3);
        }
    } else {
       taille_fenetre = 4;
    }


    /* --------------------------
        Déclaration des variables
      ---------------------------- */

    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet; /* paquet utilisé par le protocole */
    int fin = 0; /* condition d'arrêt */

    paquet_t pack;  // le paquet d'acquittement

    int borne_inf = 0;
    int duree_type = 250;
    paquet_t buffer_paquets[SEQ_NUM_SIZE];  // paquets bufferisés le temps de les acquitter
    int booleens_ack[SEQ_NUM_SIZE] = {0};   // les paquets dont il faut envoyer l'aqcuittement


    /*----------------------------------------------
                T R A N S M I S S I O N
    ------------------------------------------------*/

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");


    /* tant que le récepteur reçoit des données */
    while ( !fin ) {

        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);

        if (verifier_controle(paquet)) {
            
            if (dans_fenetre(borne_inf, paquet.num_seq, taille_fenetre)) {
                
                //on prépare l'acquittement
                pack.type = ACK;
                pack.num_seq = paquet.num_seq;
                pack.lg_info = 0;
                pack.somme_ctrl = generer_controle(pack);
                
                buffer_paquets[paquet.num_seq] = paquet; // on le bufferise

                // si le paquet est hors-séquence
                if (paquet.num_seq != borne_inf) {
                    booleens_ack[paquet.num_seq] = 1;   // on note que l'ack est prêt
                } else {    // == paquet reçu en séquence
                    do {
                        /* extraction des donnees du paquet recu */
                        for (int i=0; i<buffer_paquets[borne_inf].lg_info; i++) {
                            message[i] = buffer_paquets[borne_inf].info[i];
                        }
                        /* remise des données à la couche application */
                        fin = vers_application(message, buffer_paquets[borne_inf].lg_info);
                        booleens_ack[borne_inf] = 0;    //on "note" que l'acquittement est "fait"
                        borne_inf = inc(borne_inf, SEQ_NUM_SIZE);
                    } while (booleens_ack[borne_inf] != 0);
                    /* il faut rentrer au moins une fois dans la boucle (d'où le 'do {...} while')
                    afin d'évacuer le paquet que l'on vient de recevoir (qui est en séquence).
                    Ensuite, on s'assure de mettre à jour borne_inf, en remontant les paquets
                    potentiellement déjà acquittés.
                    */

                }

            } //pas besoin de else, comme dans les versions précédentes

            vers_reseau(&pack);
            //ici le pack envoyé sera le celui du dernier paquet en séquence reçu
            
        } 
        
    }

    /* tant que je reçois encore des paquets, j'envoie les ack */
    depart_temporisateur(duree_type);
    int evt = attendre();
    while (evt == -1) {
        arret_temporisateur(); //on arrête le timer lancé juste avant la boucle
        de_reseau(&paquet); // on regarde ce qu'on a reçu
        pack.type = ACK;            // on renvoie l'acquittement (qui s'est probablement perdu la fois d'avant)
        pack.num_seq = paquet.num_seq;
        pack.lg_info = 0;
        pack.somme_ctrl = generer_controle(pack);
        evt = attendre();
    }
    // le mérite revient aux explications de l'encadrant de TP

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
