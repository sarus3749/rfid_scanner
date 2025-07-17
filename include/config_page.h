#pragma once
const char CONFIG_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='fr'>
<head>
    <meta charset='UTF-8'>
    <title>Configuration RFID Scanner</title>
    <style>
        body { font-family: Arial, sans-serif; background: #f5f5f5; }
        .container { max-width: 400px; margin: 60px auto; background: #fff; padding: 30px; border-radius: 8px; box-shadow: 0 2px 8px #ccc; }
        h2 { text-align: center; }
        label { display: block; margin-top: 15px; }
        input[type=text], input[type=password] { width: 100%; padding: 10px; margin: 10px 0; }
        button { width: 100%; padding: 10px; background: #388e3c; color: #fff; border: none; border-radius: 4px; }
        .msg { color: green; text-align: center; }
    </style>
</head>
<body>
    <div class='container'>
        <h2>Configuration RFID Scanner</h2>
        <form method='post'>
            <label for='code'>Nouveau code d'accès :</label>
            <input name='code' type='password' id='code' placeholder='Nouveau code' required>
            <button type='submit'>Enregistrer</button>
        </form>
        <div class='msg'>%MSG%</div>
    </div>
</body>
</html>
)=====";