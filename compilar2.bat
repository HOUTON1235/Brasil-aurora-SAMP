@echo off
set BASE=%~dp0
"%BASE%pawno\pawncc.exe" "%BASE%gamemodes\rp.pwn" -o"%BASE%gamemodes\rp.amf" -i"%BASE%pawno\include" -w203 > "%BASE%compilar_output.txt" 2>&1
type "%BASE%compilar_output.txt"
pause
