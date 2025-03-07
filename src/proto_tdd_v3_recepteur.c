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

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet; /* paquet utilisé par le protocole */
    int fin = 0; /* condition d'arrêt */

    paquet_t pack;  // le paquet d'acquittement

    uint8_t num_paquet_attendu = 0;
    pack.type = ACK;
    int modulo = 8;

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");


    /* tant que le récepteur reçoit des données */
    while ( !fin ) {

        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);

        if (verifier_controle(paquet)) {
            
            if (paquet.num_seq == num_paquet_attendu) {
                // cas où le paquet reçu est en séquence

                /* extraction des donnees du paquet recu */
                for (int i=0; i<paquet.lg_info; i++) {
                    message[i] = paquet.info[i];
                }
                /* remise des données à la couche application */
                fin = vers_application(message, paquet.lg_info);
                
                pack.num_seq = num_paquet_attendu;
                pack.somme_ctrl = generer_controle(pack);
                
                num_paquet_attendu = inc(num_paquet_attendu, modulo);
            } else {
                // cas hors-séquence

                // pas besoin de le bufferiser dans message
                // pas besoin de modifier pack en conséquence

                // utilité du else ?
            }

            vers_reseau(&pack);
            //ici le pack envoyé sera le celui du dernier paquet en séquence reçu
            
        } 
        
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
