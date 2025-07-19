#pragma once
const char LOGIN_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang='fr'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>🔒 RFID Scanner - Accès protégé</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            background: #f5f5f5; 
            margin: 0; 
            padding: 20px; 
            min-height: 100vh; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
        }
        .container { 
            max-width: 400px; 
            width: 100%; 
            background: #fff; 
            padding: 40px 30px; 
            border-radius: 12px; 
            box-shadow: 0 4px 16px rgba(0,0,0,0.1); 
            box-sizing: border-box;
        }
        h2 { 
            text-align: center; 
            color: #333; 
            margin-bottom: 30px; 
            font-size: 24px;
        }
        input[type=password] { 
            width: 100%; 
            padding: 15px; 
            margin: 15px 0; 
            border: 2px solid #ddd; 
            border-radius: 8px; 
            font-size: 16px; 
            box-sizing: border-box;
            transition: border-color 0.3s;
        }
        input[type=password]:focus {
            outline: none;
            border-color: #1976d2;
        }
        button { 
            width: 100%; 
            padding: 15px; 
            background: #1976d2; 
            color: #fff; 
            border: none; 
            border-radius: 8px; 
            font-size: 16px; 
            cursor: pointer;
            transition: background-color 0.3s;
        }
        button:hover {
            background: #1565c0;
        }
        button:active {
            transform: translateY(1px);
        }
        
        /* Responsive Design */
        @media (max-width: 768px) {
            body { padding: 15px; }
            .container { 
                max-width: 90vw; 
                padding: 30px 20px; 
            }
            h2 { 
                font-size: 22px; 
                margin-bottom: 25px; 
            }
            input[type=password], button { 
                padding: 12px; 
                font-size: 16px; 
            }
        }
        
        @media (max-width: 480px) {
            body { padding: 10px; }
            .container { 
                padding: 25px 15px; 
            }
            h2 { 
                font-size: 20px; 
                margin-bottom: 20px; 
            }
            input[type=password], button { 
                padding: 14px; 
                font-size: 16px; 
                margin: 10px 0;
            }
        }
        
        @media (max-width: 320px) {
            .container { 
                padding: 20px 10px; 
            }
            h2 { 
                font-size: 18px; 
            }
        }
    </style>
</head>
<body>
    <div class='container'>
        <h2>🔒 Accès protégé</h2>
        <p style='text-align: center; color: #666; margin-bottom: 25px;'>Veuillez entrer le code d'accès</p>
        <form method='get'>
            <input name='code' type='password' required autocomplete='current-password'>
            <button type='submit'>🚀 Entrer</button>
        </form>
    </div>
</body>
</html>
)=====";