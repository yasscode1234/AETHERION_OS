@echo off
cd /d "C:\Users\mimouna\AETHERION_OS"

set /p branchName="🪄 Nom de la nouvelle branche : "
git checkout -b %branchName%

git config user.name "yasscode1234"
git config user.email "yelka5626@outlook.fr"

echo 🔄 Ajout des fichiers...
git add .

echo.
set /p commitMessage="📝 Message du commit : "
git commit -m "%commitMessage%"

echo.
echo 🚀 Push vers GitHub sur la branche %branchName%...
git push origin %branchName%

echo ✅ Branche '%branchName%' créée et poussée sur GitHub !
pause
