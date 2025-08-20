#ifndef WEB_PAGES_H
#define WEB_PAGES_H

const char* getDataCollectionHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ML Data Collection</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }
        .card { background: #f9f9f9; padding: 15px; margin: 10px 0; border-radius: 5px; }
        .status { font-size: 18px; font-weight: bold; }
        .collecting { color: #ff6b6b; }
        .idle { color: #51cf66; }
        button { padding: 10px 20px; margin: 5px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        .btn-primary { background: #4dabf7; color: white; }
        .btn-success { background: #51cf66; color: white; }
        .btn-danger { background: #ff6b6b; color: white; }
        .btn-secondary { background: #868e96; color: white; }
        input, select { padding: 8px; margin: 5px; border: 1px solid #ccc; border-radius: 3px; }
        .data-area { height: 200px; overflow-y: scroll; background: #f8f9fa; padding: 10px; font-family: monospace; font-size: 12px; border: 1px solid #ccc; }
        .progress { width: 100%; height: 20px; background: #e9ecef; border-radius: 10px; overflow: hidden; }
        .progress-bar { height: 100%; background: #4dabf7; transition: width 0.3s; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ML Finger Detection - Data Collection</h1>
        
        <div class="card">
            <div class="status" id="status">Status: Idle</div>
            <div id="progress-container" style="margin: 10px 0;">
                <div>Progress: <span id="progress-text">0/0</span></div>
                <div class="progress">
                    <div class="progress-bar" id="progress-bar" style="width: 0%"></div>
                </div>
            </div>
        </div>
        
        <div class="card">
            <h3>Camera View</h3>
            <div style="text-align: center;">
                <img id="cameraView" src="data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMzIwIiBoZWlnaHQ9IjI0MCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cmVjdCB3aWR0aD0iMTAwJSIgaGVpZ2h0PSIxMDAlIiBmaWxsPSIjZGRkIi8+PHRleHQgeD0iNTAlIiB5PSI1MCUiIGZvbnQtZmFtaWx5PSJBcmlhbCIgZm9udC1zaXplPSIxNCIgZmlsbD0iIzk5OSIgdGV4dC1hbmNob3I9Im1pZGRsZSIgZHk9Ii4zZW0iPkNhbWVyYSBMb2FkaW5nLi4uPC90ZXh0Pjwvc3ZnPg==" style="max-width: 320px; border: 2px solid #ccc; border-radius: 5px;" alt="Camera feed will appear here">
                <br><br>
                <button class="btn-secondary" onclick="refreshCamera()">Refresh Camera</button>
                <label>
                    <input type="checkbox" id="autoRefresh" checked> Auto-refresh (2s)
                </label>
            </div>
        </div>
        
        <div class="card">
            <h3>Collection Settings</h3>
            <label>Finger Count: 
                <select id="fingers">
                    <option value="0">0 Fingers</option>
                    <option value="1">1 Finger</option>
                    <option value="2">2 Fingers</option>
                    <option value="3">3 Fingers</option>
                    <option value="4">4 Fingers</option>
                    <option value="5">5 Fingers</option>
                </select>
            </label><br>
            
            <label>Samples per finger count: 
                <input type="number" id="samplesPerCount" value="20" min="1" max="100">
            </label><br>
            
            <label>Delay between samples (ms): 
                <input type="number" id="delay" value="3000" min="1000" max="10000">
            </label><br>
            
            <label>
                <input type="checkbox" id="autoMode"> Auto mode (advance finger count automatically)
            </label>
        </div>
        
        <div class="card">
            <h3>Collection Controls</h3>
            <button class="btn-primary" onclick="updateSettings()">Update Settings</button>
            <button class="btn-success" onclick="startCollection()">Start Collection</button>
            <button class="btn-secondary" onclick="collectSample()">Collect Single Sample</button>
            <button class="btn-danger" onclick="stopCollection()">Stop Collection</button>
        </div>
        
        <div class="card">
            <h3>Collected Data</h3>
            <button class="btn-secondary" onclick="viewData()">View Data</button>
            <button class="btn-secondary" onclick="downloadData()">Download CSV</button>
            <button class="btn-danger" onclick="clearData()">Clear Data</button>
            <div class="data-area" id="dataArea">No data collected yet...</div>
        </div>
    </div>

    <script>
        let statusInterval;
        
        function startStatusUpdates() {
            statusInterval = setInterval(updateStatus, 1000);
        }
        
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    const status = document.getElementById('status');
                    const progressText = document.getElementById('progress-text');
                    const progressBar = document.getElementById('progress-bar');
                    
                    if (data.isCollecting) {
                        status.innerHTML = `Status: <span class="collecting">Collecting ${data.currentFingers} fingers</span>`;
                    } else {
                        status.innerHTML = `Status: <span class="idle">Idle</span>`;
                    }
                    
                    progressText.textContent = `${data.currentSample}/${data.samplesPerFingerCount} (Total: ${data.totalSamples})`;
                    
                    const progressPercent = data.samplesPerFingerCount > 0 ? 
                        (data.currentSample / data.samplesPerFingerCount) * 100 : 0;
                    progressBar.style.width = progressPercent + '%';
                });
        }
        
        function updateSettings() {
            const fingers = document.getElementById('fingers').value;
            const samplesPerCount = document.getElementById('samplesPerCount').value;
            const delay = document.getElementById('delay').value;
            const autoMode = document.getElementById('autoMode').checked;
            
            const formData = new FormData();
            formData.append('fingers', fingers);
            formData.append('samplesPerCount', samplesPerCount);
            formData.append('delay', delay);
            formData.append('autoMode', autoMode);
            
            fetch('/api/settings', {
                method: 'POST',
                body: formData
            });
        }
        
        function startCollection() {
            fetch('/api/start', { method: 'POST' });
        }
        
        function stopCollection() {
            fetch('/api/stop', { method: 'POST' });
        }
        
        function collectSample() {
            fetch('/api/collect', { method: 'POST' });
        }
        
        function viewData() {
            fetch('/api/data')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('dataArea').textContent = data || 'No data collected yet...';
                });
        }
        
        function downloadData() {
            fetch('/api/data')
                .then(response => response.text())
                .then(data => {
                    const blob = new Blob([data], { type: 'text/csv' });
                    const url = window.URL.createObjectURL(blob);
                    const a = document.createElement('a');
                    a.href = url;
                    a.download = 'finger_detection_data.csv';
                    a.click();
                });
        }
        
        function clearData() {
            fetch('/api/cleardata', { method: 'POST' })
                .then(() => {
                    document.getElementById('dataArea').textContent = 'No data collected yet...';
                });
        }
        
        function refreshCamera() {
            const img = document.getElementById('cameraView');
            img.src = '/api/camera/capture?' + new Date().getTime();
        }
        
        function startCameraAutoRefresh() {
            setInterval(() => {
                const autoRefresh = document.getElementById('autoRefresh');
                if (autoRefresh && autoRefresh.checked) {
                    refreshCamera();
                }
            }, 2000);
        }
        
        startStatusUpdates();
        startCameraAutoRefresh();
        setTimeout(refreshCamera, 1000);
    </script>
</body>
</html>
)rawliteral";
}

#endif