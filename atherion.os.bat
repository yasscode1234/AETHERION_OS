@echo off
cd /d "C:\Users\mimouna\AETHERION_OS"

echo 🔧 Configuration Git locale...
git config user.name "yasscode1234"
git config user.email "yelka5626@outlook.fr"

echo 🔄 Ajout des fichiers modifiés...
git add .

echo.
set /p commitMessage="📝 Entrez le message du commit : "
git commit -m "%commitMessage%"

echo.
echo 🚀 Envoi vers GitHub (branche main)...
git push origin main --force

echo.
echo ✅ Fichiers mis à jour sur GitHub !
pause
