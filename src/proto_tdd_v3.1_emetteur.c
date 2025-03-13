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
    int curseur = 0;    // on initialise un numéro de séquence (cf algo 2, TD)
    int borne_inf = 0;
    paquet_t tab_p[SEQ_NUM_SIZE];  // cf algo 3 TD
    int evt;
    int duree_type = 300;   // à définir selon la durée de temporisateur souhaitée
    //int modulo = 16;     // à modifier selon le modulo souhaité
    /* en fait, SEQ_NUM_SIZE joue déjà le rôle de modulo */

    /* ------------------------------------------------
    fonctionnalité pour rentrer la taille de la fenêtre
    --------------------------------------------------- */
    int taille_fenetre;
    if (argc == 2) {
        int inter = atoi(argv[1]);
        // pour respecter la règle de go-back-N en w < n - 1 
        if (taille_fenetre_correcte(inter)){
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
        
        if (dans_fenetre(borne_inf, curseur, taille_fenetre)) {

            /* construction paquet */
            for (int i=0; i<taille_msg; i++) {
                tab_p[curseur].info[i] = message[i];
            }
            // possibilité de faire ?
            // tab_p[curseur].info = message;
            // quelque chose comme ça
            tab_p[curseur].lg_info = taille_msg;
            tab_p[curseur].type = DATA;
            tab_p[curseur].num_seq = curseur;   // le paquet prend son numéro de séquence (cf algo 2, TD)
            // générer contrôle 
            tab_p[curseur].somme_ctrl = generer_controle(tab_p[curseur]);

            printf("le paquet envoyé a le numéro de séquence : \t%d \n", tab_p[curseur].num_seq);
            /* remise à la couche reseau */
            vers_reseau(&tab_p[curseur]);

            if (borne_inf == curseur) {
                depart_temporisateur(duree_type);
            }
            // incrément du curseur
            curseur = inc(curseur, SEQ_NUM_SIZE);

            /* lecture de donnees provenant de la couche application */
            de_application(message, &taille_msg);
        } else {
            evt = attendre();
            if (evt == -1) {
                de_reseau(&pack);
                if (verifier_controle(pack) && dans_fenetre(borne_inf, pack.num_seq, taille_fenetre)) {
                    //décalage fenêtre
                    borne_inf = inc(pack.num_seq, SEQ_NUM_SIZE);
                    if (borne_inf == curseur) {
                        // Tous les paquets sont acquittés
                        arret_temporisateur();
                    }
                }
            } else {
                // Timer expire
                int i = borne_inf;
                depart_temporisateur(duree_type);
                while (i != curseur) {
                    vers_reseau(&tab_p[i]);
                    i = inc(i, SEQ_NUM_SIZE);
                }
            }

        }                 
            
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
