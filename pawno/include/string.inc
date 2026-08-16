/* String functions
 *
 * (c) Copyright 2005, ITB CompuPhase
 * This file is provided as is (no warranties).
 */
#if defined _string_included
  #endinput
#endif
#define _string_included
#pragma library String

native strlen(string[]);
native strpack(dest[], source[], maxlength=sizeof dest);
native strunpack(dest[], source[], maxlength=sizeof dest);
native strcat(dest[], source[], maxlength=sizeof dest);

native strmid(dest[], source[], start, end, maxlength=sizeof dest);
native bool: strins(string[], substr[], pos, maxlength=sizeof string);
native bool: strdel(string[], start, end);

native strcmp(string1[], string2[], bool:ignorecase=false, length=cellmax);
native strfind(string[], sub[], bool:ignorecase=false, pos=0);

native strval(string[]);
native valstr(dest[], value, bool:pack=false);
native bool: ispacked(string[]);

native uudecode(dest[], source[], maxlength=sizeof dest);
native uuencode(dest[], source[], numbytes, maxlength=sizeof dest);
native memcpy(dest[], source[], index=0, numbytes, maxlength=sizeof dest);
