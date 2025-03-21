/*************************************************************
* proto_tdd_v0 -  émetteur                                   *
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

#include <stdlib.h>

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */
    // paquet_t paquet; /* paquet utilisé par le protocole */


    // si argc == 1 la fenetre est 'par défaut', sinon, elle est de taille argv[1]
    if (argc > 2) {
        perror ("Trop d'arguments\n");
        exit(2);
    }

    paquet_t pack; // le packet d'acquittement
    int curseur = 0;
    int borne_inf = 0;
    paquet_t tab_p[SEQ_NUM_SIZE];  // cf algo 3 TD
    int evt;
    int duree_type = 250;   // à définir selon la durée de temporisateur souhaitée
    int booleens_ack[SEQ_NUM_SIZE] = {0};


    /* --------------------------------------------------
       fonctionnalité pour rentrer la taille de la fenêtre
       --------------------------------------------------- */
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

    /*-------------------------
    | début de la transmission |
     ------------------------ */

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);



    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 ) {

        if ((dans_fenetre(borne_inf, curseur, taille_fenetre)) && (taille_msg > 0)) {

            /* construction paquet */
            for (int i=0; i<taille_msg; i++) {
                tab_p[curseur].info[i] = message[i];
            }
            tab_p[curseur].lg_info = taille_msg;
            tab_p[curseur].type = DATA;
            tab_p[curseur].num_seq = curseur;
            tab_p[curseur].somme_ctrl = generer_controle(tab_p[curseur]);

            // printf("le paquet envoyé a le numéro de séquence : \t%d \n", tab_p[curseur].num_seq);
            /* remise à la couche reseau */
            vers_reseau(&tab_p[curseur]);

            //lancer le temporisateur correspondant au paquet envoyé
            depart_temporisateur_num(curseur, duree_type);
        
            // incrément du curseur
            curseur = inc(curseur, SEQ_NUM_SIZE);

            /* lecture de donnees provenant de la couche application */
            de_application(message, &taille_msg);

        } else {
            // si on a plus de paquets à envoyer
            evt = attendre();
            // si un ack est reçu
            if (evt == -1) {
                de_reseau(&pack);

                if (verifier_controle(pack) && dans_fenetre(borne_inf, pack.num_seq, taille_fenetre)) {
                    //décalage fenêtre
                    printf("Ack reçu : n°%d\n", pack.num_seq);
                    
                    if (borne_inf == pack.num_seq) {
                        //  && l'acquittement reçu correspond au début de notre fenêtre
                        borne_inf = inc(pack.num_seq, SEQ_NUM_SIZE);

                        //on arrête le temporaisateur correspondant
                        arret_temporisateur_num(pack.num_seq);

                        //on remet l'acquittement à 'faux' dans le tableau d'acquittements
                        booleens_ack[pack.num_seq] = 0;

                        // tant que les paquets sont acquittés, on peut incrémenter borne_inf
                        while (booleens_ack[borne_inf] != 0) {
                            borne_inf = inc(borne_inf, SEQ_NUM_SIZE);
                            booleens_ack[borne_inf] = 0;
                        }

                    } else {
                        // on met à vrai dans le tableau des acquittements
                        booleens_ack[pack.num_seq] = 1;

                        // on arrête le temporaisateur correspondant
                        arret_temporisateur_num(pack.num_seq);
                    }

                  
                }
            } else {
                // Timer expire
                // evt prend la valeur du temporisateur qui a expiré
                //on lance le temporisateur du message à renvoyé
                depart_temporisateur_num(evt, duree_type);
                //le message a renvoyé est à l'indice evt du tableau
                vers_reseau(&tab_p[evt]);
                    
            }

        }                 

            
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
