/* Datagram sending/receiving
 *
 * (c) Copyright 2005, ITB CompuPhase
 * This file is provided as is (no warranties).
 */
#if defined _datagram_included
  #endinput
#endif
#define _datagram_included
#pragma library DGram

native sendstring(message[], destination[]="");
native sendpacket(packet[], size, destination[]="");

native listenport(port);

forward @receivestring(message[], source[]);
forward @receivepacket(packet[], size, source[]);
