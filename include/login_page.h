#pragma once
const char LOGIN_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='fr'>
<head>
    <meta charset='UTF-8'>
    <title>Accès protégé</title>
    <style>
        body { font-family: Arial, sans-serif; background: #f5f5f5; }
        .container { max-width: 350px; margin: 80px auto; background: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 2px 8px #ccc; }
        h2 { text-align: center; }
        input[type=password] { width: 100%; padding: 10px; margin: 10px 0; }
        button { width: 100%; padding: 10px; background: #1976d2; color: #fff; border: none; border-radius: 4px; }
    </style>
</head>
<body>
    <div class='container'>
        <h2>Accès protégé</h2>
        <form method='get'>
            <input name='code' type='password' placeholder='Mot de passe' required>
            <button type='submit'>Entrer</button>
        </form>
    </div>
</body>
</html>
)=====";