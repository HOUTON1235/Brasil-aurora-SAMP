// ============================================================
//  Santa Aurora Roleplay — Sistema de Eventos Internos
//  Módulo : core/events.inc
//  Objetivo: eventos que permitem módulos se comunicarem
//            sem acoplamento direto
// ============================================================

#if defined _SA_EVENTS_INC
    #endinput
#endif
#define _SA_EVENTS_INC

// ------------------------------------------------------------
// Declaração dos forwards de eventos internos
// Os módulos implementam as versões "public" para receber
// esses eventos.
// ------------------------------------------------------------

// Jogador carregado do banco com sucesso
forward OnPlayerLoaded(playerid);

// Jogador salvo no banco com sucesso
forward OnPlayerSaved(playerid);

// Dinheiro do jogador mudou
// type: 0=adicionado, 1=removido
forward OnMoneyChanged(playerid, oldValue, newValue, reason[]);

// Inventário mudou
forward OnInventoryChanged(playerid, itemid, delta, reason[]);

// Veículo criado no mundo
forward OnVehicleCreated(vehicledbid, modelid, ownerid);

// Veículo destruído/removido do mundo
forward OnVehicleDestroyed(vehicledbid);

// Propriedade comprada
forward OnPropertyBought(propertyid, buyerid, price);

// Transação em empresa
forward OnBusinessTransaction(businessid, playerid, amount, type);

// Jogador morreu
forward OnPlayerDied(playerid, killerid);

// Jogador foi preso (RP)
forward OnPlayerArrested(playerid, officerid, reason[]);

// Jogador completou o tutorial
forward OnPlayerFinishedTutorial(playerid);

// Jogador entrou numa facção
forward OnPlayerJoinedFaction(playerid, factionid, rank);

// Jogador saiu de uma facção
forward OnPlayerLeftFaction(playerid, factionid);

// Jogador mudou de emprego
forward OnPlayerChangedJob(playerid, oldjobid, newjobid);

// ------------------------------------------------------------
// Helper: Disparar evento com proteção
// SA-MP: CallLocalFunction não retorna erro se não houver
// implementação — apenas dispara para quem declarar public.
// ------------------------------------------------------------
#define FireEvent(%0)  CallLocalFunction(%0)

// Disparar evento com parâmetros
// Uso: FireEventii("OnPlayerDied", playerid, killerid)
#define FireEvent_ii(%0,%1,%2)      CallLocalFunction(%0,"ii",%1,%2)
#define FireEvent_i(%0,%1)          CallLocalFunction(%0,"i",%1)
#define FireEvent_is(%0,%1,%2)      CallLocalFunction(%0,"is",%1,%2)
#define FireEvent_iii(%0,%1,%2,%3)  CallLocalFunction(%0,"iii",%1,%2,%3)
#define FireEvent_iis(%0,%1,%2,%3)  CallLocalFunction(%0,"iis",%1,%2,%3)
#define FireEvent_iiii(%0,%1,%2,%3,%4) CallLocalFunction(%0,"iiii",%1,%2,%3,%4)

