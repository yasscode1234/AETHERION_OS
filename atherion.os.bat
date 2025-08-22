@echo off
cd /d "%~dp0"
git add .
git commit -m "Ajout/Mise à jour des fichiers AETHERION OS"
git push origin main --force
pause
