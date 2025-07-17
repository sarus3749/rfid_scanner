#pragma once

const char WEB_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>RFID Scanner</title>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 840px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; margin-bottom: 30px; }
        .tabs { display: flex; border-bottom: 2px solid #e0e0e0; margin-bottom: 20px; }
        .tab { padding: 12px 32px; cursor: pointer; background: #f7f7f7; border: none; outline: none; font-size: 18px; color: #333; border-radius: 10px 10px 0 0; margin-right: 2px; }
        .tab.active { background: #fff; border-bottom: 2px solid #fff; font-weight: bold; }
        .tab-content { display: none; }
        .tab-content.active { display: block; }
        .status { background: #e8f5e8; padding: 15px; border-radius: 5px; margin: 15px 0; }
        .button { padding: 12px 24px; margin: 8px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        .button:hover { background: #45a049; }
        .button.danger { background: #f44336; }
        .button.danger:hover { background: #da190b; }
        .info { background: #e3f2fd; padding: 15px; border-radius: 5px; margin: 15px 0; }
        input[type=text], input[type=password] { padding: 10px; width: 220px; border: 1px solid #ddd; border-radius: 4px; }
        .upload-form { background: #fff3cd; padding: 15px; border-radius: 5px; margin: 15px 0; }
        .terminal { background: #111; color: #0f0; font-family: monospace; padding: 15px; border-radius: 5px; min-height: 220px; max-height: 320px; overflow-y: auto; margin: 15px 0; }
        @media (max-width: 900px) { .container { max-width: 98vw; } }
    </style>
</head>
<body>
    <div class='container'>
        <div class='header'>
            <h1>🔧 RFID Scanner</h1>
        </div>
        <div class='tabs'>
            <button class='tab active' onclick='showTab(0)'>État</button>
            <button class='tab' onclick='showTab(1)'>RFID</button>
            <button class='tab' onclick='showTab(2)'>Configuration</button>
            <button class='tab' onclick='showTab(3)'>Terminal API</button>
        </div>
        <div class='tab-content active' id='tab-etat'>
            <div class='status'>
                <h3>📊 État du système</h3>
                <p><strong>Mode:</strong> <span id='mode'>Chargement...</span></p>
                <p><strong>Mémoire libre:</strong> <span id='memory'>Chargement...</span></p>
                <p><strong>Uptime:</strong> <span id='uptime'>Chargement...</span></p>
                <p><strong>Signal WiFi:</strong> <span id='rssi'>Chargement...</span></p>
            </div>
            <div id='cardInfo' class='status' style='display:none'>
                <h3>💳 Dernière carte détectée</h3>
                <p id='cardDetails'>Aucune carte</p>
            </div>
        </div>
        <div class='tab-content' id='tab-rfid'>
            <div class='info'>
                <h3>🎛️ Commandes RFID</h3>
                <button class='button' onclick='sendCommand("READ")'>📖 Mode Lecture</button>
                <button class='button' onclick='sendCommand("STOP")'>⏹️ Arrêter</button>
                <button class='button' onclick='sendCommand("INFO")'>ℹ️ Informations</button>
                <br><br>
                <input type='text' id='writeData' placeholder='Données à écrire'>
                <button class='button' onclick='writeData()'>✏️ Écrire</button>
            </div>
            <div class='info'>
                <h3>⏱️ Délai entre scans RFID</h3>
                <input type='number' id='scanDelay' min='500' step='100' style='width:120px'> ms <span style='color:#888'>(min 500 ms)</span>
                <button class='button' onclick='saveScanDelay()'>💾 Enregistrer</button>
                <span id='scanDelayStatus'></span>
            </div>
        </div>
        <div class='tab-content' id='tab-config'>
            <div class='info'>
                <h3>🌐 Configuration API</h3>
                <input type='text' id='apiUrl' placeholder='URL API' style='width:350px'>
                <button class='button' onclick='saveApiUrl()'>💾 Enregistrer URL</button>
                <span id='apiUrlStatus'></span>
            </div>
            <div class='info'>
                <h3>🔑 Configuration WiFi</h3>
                <input type='text' id='wifiSsid' placeholder='SSID' style='width:180px'>
                <input type='password' id='wifiPass' placeholder='Mot de passe' style='width:180px'>
                <button class='button' onclick='saveWifiConfig()'>💾 Enregistrer WiFi</button>
                <span id='wifiStatus'></span>
            </div>
            <div class='info'>
                <h3>🔊 Test du buzzer</h3>
                <button class='button' onclick='buzzerTest()'>Tester le buzzer</button>
                <label for='buzzerTimes'>Nombre de bips :</label>
                <input type='number' id='buzzerTimes' value='1' min='1' max='10' style='width:40px;'>
                <label for='buzzerDuration'>Durée (ms) :</label>
                <input type='number' id='buzzerDuration' value='100' min='10' max='1000' style='width:60px;'>
                <span id='buzzerResult'></span>
            </div>
            <div class='info'>
                <h3>🔧 Actions système</h3>
                <button class='button danger' onclick='restartESP()'>🔄 Redémarrer</button>
            </div>
            <div class='upload-form'>
                <h3>🔄 Mise à jour du firmware</h3>
                <p><strong>⚠️ Attention:</strong> La mise à jour interrompra temporairement les opérations RFID.</p>
                <form method='POST' action='/update' enctype='multipart/form-data'>
                    <input type='file' name='update' accept='.bin'>
                    <br><br>
                    <input type='submit' value='📤 Téléverser' class='button'>
                </form>
            </div>
            <div class='info'>
                <h3>🔒 Code d'accès à l'interface web</h3>
                <p>Code actuel : <span id='webCode'>Chargement...</span></p>
                <form id='webCodeForm' onsubmit='return changeWebCode();'>
                    <input type='text' id='newWebCode' placeholder='Nouveau code' maxlength='16' required>
                    <button class='button' type='submit'>💾 Modifier le code</button>
                    <span id='webCodeStatus'></span>
                </form>
            </div>
        </div>
        <div class='tab-content' id='tab-apilog'>
            <div class='terminal' id='apiTerminal'></div>
        </div>
    </div>
    <script>
        function showTab(idx) {
            var tabs = document.getElementsByClassName('tab');
            var contents = document.getElementsByClassName('tab-content');
            for (var i = 0; i < tabs.length; i++) {
                tabs[i].classList.remove('active');
                contents[i].classList.remove('active');
            }
            tabs[idx].classList.add('active');
            contents[idx].classList.add('active');
        }
        function sendCommand(cmd) {
            fetch('/api/command?cmd=' + cmd)
                .then(response => response.text())
                .then(data => {
                    alert('Commande envoyée: ' + cmd);
                    updateStatus();
                });
        }
        function writeData() {
            const data = document.getElementById('writeData').value;
            if (data) {
                fetch('/api/write?data=' + encodeURIComponent(data))
                    .then(response => response.text())
                    .then(data => {
                        alert('Mode écriture activé: ' + data);
                        updateStatus();
                    });
            } else {
                alert('Veuillez entrer des données à écrire');
            }
        }
        function formatUptime(seconds) {
            const d = Math.floor(seconds / 86400);
            const h = Math.floor((seconds % 86400) / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            const s = seconds % 60;
            let str = '';
            if (d > 0) str += d + 'j ';
            str += h + 'h ' + m + 'm ' + s + 's';
            return str;
        }
        function getCurrentWebCode() {
            return document.getElementById('webCode').textContent;
        }
        function updateCardInfo() {
            const code = getCurrentWebCode();
            fetch('/api/lastcard?code=' + encodeURIComponent(code))
                .then(response => response.text())
                .then(data => {
                    document.getElementById('cardDetails').innerHTML = data;
                    document.getElementById('cardInfo').style.display = (data && data !== 'Aucune carte') ? '' : 'none';
                });
        }
        function updateStatus() {
            const code = getCurrentWebCode();
            fetch('/api/status?code=' + encodeURIComponent(code))
                .then(response => response.json())
                .then(data => {
                    document.getElementById('mode').textContent = data.mode;
                    document.getElementById('memory').textContent = data.memory + ' bytes';
                    document.getElementById('uptime').textContent = formatUptime(data.uptime);
                    document.getElementById('rssi').textContent = data.rssi + ' dBm';
                });
        }
        function updateApiTerminal() {
            fetch('/api/apilog')
                .then(response => response.json())
                .then(data => {
                    let html = '';
                    data.forEach(entry => {
                        html += '[' + entry.t + 's] UID=' + entry.uid + '  HTTP=' + entry.code + '<br>URL: ' + entry.url + '<br>';
                    });
                    document.getElementById('apiTerminal').innerHTML = html || '<i>Aucun envoi enregistré</i>';
                });
        }
        function buzzerTest() {
            const times = document.getElementById('buzzerTimes').value;
            const duration = document.getElementById('buzzerDuration').value;
            fetch(`/api/buzzer?times=${times}&duration=${duration}`)
                .then(r => r.text())
                .then(txt => document.getElementById('buzzerResult').textContent = txt)
                .catch(() => document.getElementById('buzzerResult').textContent = 'Erreur');
        }
        function loadApiUrl() {
            fetch('/api/apiurl')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('apiUrl').value = data;
                });
        }
        function saveApiUrl() {
            const url = document.getElementById('apiUrl').value;
            fetch('/api/setapiurl', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'url=' + encodeURIComponent(url)
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('apiUrlStatus').textContent = 'URL enregistrée!';
                setTimeout(()=>{document.getElementById('apiUrlStatus').textContent='';}, 2000);
            });
        }
        function loadWifiConfig() {
            fetch('/api/wificonfig')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('wifiSsid').value = data.ssid;
                    document.getElementById('wifiPass').value = data.pass;
                });
        }
        function saveWifiConfig() {
            const ssid = document.getElementById('wifiSsid').value;
            const pass = document.getElementById('wifiPass').value;
            fetch('/api/setwificonfig', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass)
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('wifiStatus').textContent = 'WiFi enregistré!';
                setTimeout(()=>{document.getElementById('wifiStatus').textContent='';}, 2000);
            });
        }
        function loadScanDelay() {
            fetch('/api/scandelay')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('scanDelay').value = data;
                });
        }
        function saveScanDelay() {
            const delay = document.getElementById('scanDelay').value;
            fetch('/api/scandelay', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'delay=' + encodeURIComponent(delay)
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('scanDelayStatus').textContent = 'Délai enregistré!';
                setTimeout(()=>{document.getElementById('scanDelayStatus').textContent='';}, 2000);
            });
        }
        function loadWebCode() {
            fetch('/api/webcode')
                .then(response => response.text())
                .then(code => {
                    document.getElementById('webCode').textContent = code;
                });
        }
        function changeWebCode() {
            const newCode = document.getElementById('newWebCode').value;
            if (!newCode) return false;
            fetch('/api/webcode', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'code=' + encodeURIComponent(newCode)
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('webCodeStatus').textContent = 'Code modifié !';
                loadWebCode();
                setTimeout(()=>{document.getElementById('webCodeStatus').textContent='';}, 2000);
            });
            return false;
        }
        loadApiUrl();
        loadWifiConfig();
        loadScanDelay();
        loadWebCode();
        setInterval(updateStatus, 5000);
        setInterval(updateCardInfo, 2000);
        setInterval(updateApiTerminal, 2000);
        updateStatus();
        updateCardInfo();
        updateApiTerminal();
    </script>
</body>
</html>
)rawliteral";