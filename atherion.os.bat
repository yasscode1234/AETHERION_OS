@echo off
cd /d "%~dp0"

echo Mise à jour du dépôt Git...
git add .

echo.
set /p commitMessage="Entrez le message du commit : "
git commit -m "%commitMessage%"

echo.
echo Envoi sur GitHub...
git push origin main --force

echo.
echo ✅ Mise à jour terminée !
pause
