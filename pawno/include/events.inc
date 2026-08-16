// ============================================================
//  Santa Aurora Roleplay ? Sistema de Eventos Internos
//  Modulo : core/events.inc
//  Objetivo: eventos que permitem modulos se comunicarem
//            sem acoplamento direto
// ============================================================

#if defined _SA_EVENTS_INC
    #endinput
#endif
#define _SA_EVENTS_INC

// ------------------------------------------------------------
// Declaracao dos forwards de eventos internos
// Os modulos implementam as versoes "public" para receber
// esses eventos.
// ------------------------------------------------------------

// Jogador carregado do banco com sucesso
forward OnPlayerLoaded(playerid);

// Jogador salvo no banco com sucesso
forward OnPlayerSaved(playerid);

// Dinheiro do jogador mudou
// type: 0=adicionado, 1=removido
forward OnMoneyChanged(playerid, oldValue, newValue, reason[]);

// Inventario mudou
forward OnInventoryChanged(playerid, itemid, delta, reason[]);

// Veiculo criado no mundo
forward OnVehicleCreated(vehicledbid, modelid, ownerid);

// Veiculo destruido/removido do mundo
forward OnVehicleDestroyed(vehicledbid);

// Propriedade comprada
forward OnPropertyBought(propertyid, buyerid, price);

// Transacao em empresa
forward OnBusinessTransaction(businessid, playerid, amount, type);

// Jogador morreu
forward OnPlayerDied(playerid, killerid);

// Jogador foi preso (RP)
forward OnPlayerArrested(playerid, officerid, reason[]);

// Jogador completou o tutorial
forward OnPlayerFinishedTutorial(playerid);

// Jogador entrou numa faccao
forward OnPlayerJoinedFaction(playerid, factionid, rank);

// Jogador saiu de uma faccao
forward OnPlayerLeftFaction(playerid, factionid);

// Jogador mudou de emprego
forward OnPlayerChangedJob(playerid, oldjobid, newjobid);

// ------------------------------------------------------------
// Helper: Disparar evento com protecao
// SA-MP: CallLocalFunction nao retorna erro se nao houver
// implementacao ? apenas dispara para quem declarar public.
// ------------------------------------------------------------
#define FireEvent(%0)  CallLocalFunction(%0)

// Disparar evento com parametros
// Uso: FireEventii("OnPlayerDied", playerid, killerid)
#define FireEvent_ii(%0,%1,%2)      CallLocalFunction(%0,"ii",%1,%2)
#define FireEvent_i(%0,%1)          CallLocalFunction(%0,"i",%1)
#define FireEvent_is(%0,%1,%2)      CallLocalFunction(%0,"is",%1,%2)
#define FireEvent_iii(%0,%1,%2,%3)  CallLocalFunction(%0,"iii",%1,%2,%3)
#define FireEvent_iis(%0,%1,%2,%3)  CallLocalFunction(%0,"iis",%1,%2,%3)
#define FireEvent_iiii(%0,%1,%2,%3,%4) CallLocalFunction(%0,"iiii",%1,%2,%3,%4)

