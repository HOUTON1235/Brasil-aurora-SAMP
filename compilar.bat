@echo off
echo Compilando Santa Aurora RP...
echo.

set BASE=%~dp0
set PAWNCC=%BASE%pawno\pawncc.exe
set SOURCE=%BASE%gamemodes\rp.pwn
set OUTPUT=%BASE%gamemodes\rp.amf

"%PAWNCC%" "%SOURCE%" ^
    -o"%OUTPUT%" ^
    -i"%BASE%pawno\include" ^
    -i"%BASE%includes" ^
    -i"%BASE%includes\core" ^
    -i"%BASE%includes\database" ^
    -i"%BASE%includes\players" ^
    -i"%BASE%includes\inventory" ^
    -i"%BASE%includes\economy" ^
    -i"%BASE%includes\jobs" ^
    -i"%BASE%includes\vehicles" ^
    -i"%BASE%includes\houses" ^
    -i"%BASE%includes\businesses" ^
    -i"%BASE%includes\phone" ^
    -i"%BASE%includes\police" ^
    -i"%BASE%includes\hospital" ^
    -i"%BASE%includes\government" ^
    -i"%BASE%includes\factions" ^
    -i"%BASE%includes\admin" ^
    -i"%BASE%includes\ui" ^
    -i"%BASE%includes\logs" ^
    -i"%BASE%includes\utilities" ^
    -r -w203

echo.
if exist "%OUTPUT%" (
    echo [OK] Compilado com sucesso: gamemodes\rp.amf
) else (
    echo [ERRO] Falha na compilacao. Veja os erros acima.
)
echo.
pause
