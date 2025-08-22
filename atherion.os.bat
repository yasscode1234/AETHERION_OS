@echo off
cd /d "C:\Users\mimouna\AETHERION_OS"

echo 🔄 Ajout des fichiers modifiés...
git add .

echo.
set /p commitMessage="📝 Entrez le message du commit : "
git commit -m "%commitMessage%"

echo.
echo 🚀 Envoi vers la branche principale (main) sur GitHub...
git push origin main --force

echo.
echo ✅ Mise à jour terminée sur https://github.com/yasscode1234/AETHERION_OS !
pause
