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
    paquet_t paquet; /* paquet utilisé par le protocole */

    paquet_t pack; // le packet d'acquittement

    
    uint8_t num_next_paquet = 0;    // on initialise un numéro de séquence (cf algo 2, TD)
    int duree_type = 200;   // à définir selon la durée de temporisateur souhaitée

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);

    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 ) {

        /* construction paquet */
        for (int i=0; i<taille_msg; i++) {
            paquet.info[i] = message[i];
        }
        paquet.lg_info = taille_msg;
        paquet.type = DATA;

        paquet.num_seq = num_next_paquet;   // le paquet prend son numéro de séquence (cf algo 2, TD)

        // générer contrôle 
        paquet.somme_ctrl = generer_controle(paquet);
        
        
        /* remise à la couche reseau */
        vers_reseau(&paquet);

        // on lance le temporisateur
        depart_temporisateur(duree_type);
        int evt = attendre();       // attendre renvoie un n° de timer s'il y en a un qui expire
                                    // donc si un timer expire, le paquet est envoyé à nouveau


                                    
        //int limite = 20;    // arbitraire, pour éviter boucles infinies
        /*
             ^
            / \
           / ! \
          /_____\
        */
        //à remettre si j'ai à nouveau des problèmes de boucle infinies pour l'émetteur
        // lorsque le message a été envoyé en entier (bloquage sur le dernier acquittement ?)

        while (evt != -1) {      // si evt == -1, on a reçu un paquet, possiblement l'acquittement
                                // donc on sort de la boucle
            
            // if (limite == 0) {
            //     perror("pas d'acquittement reçus, proto_tdd_v2_emetteur\n");
            //     exit(1);
            // }
                                
            vers_reseau(&paquet);
            arret_temporisateur();
            depart_temporisateur(duree_type);
            evt = attendre();
            //limite--;
        } 
       
        de_reseau(&pack);   // peut-être boucle à régler ici pour gérer erreurs

        arret_temporisateur();
        num_next_paquet = inc(num_next_paquet, 2);

        /* lecture des donnees suivantes de la couche application */
        de_application(message, &taille_msg);
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
