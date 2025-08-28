@echo off
title Terminal para Comp-Graf (MinGW)
echo Configurando o ambiente para MinGW...

REM Adiciona o MinGW ao PATH
SET PATH=C:\msys64\ucrt64\bin;%PATH%

echo Ambiente pronto!
echo.

REM Inicia uma nova instancia do Command Prompt com este PATH
cmd.exe