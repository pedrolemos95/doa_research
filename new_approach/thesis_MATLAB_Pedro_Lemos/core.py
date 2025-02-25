from flask import Flask, request, jsonify
from collections import defaultdict
import threading
import time

app = Flask(__name__)

# Data structure to store AoA estimates with timestamps
aoa_data = defaultdict(list)

# Lock for thread-safe operations on shared data
data_lock = threading.Lock()

# Time window (in seconds) to consider AoA estimates as coming from the same BLE frame
TIME_WINDOW = 0.1  # 100 ms tolerance

@app.route('/submit_aoa', methods=['POST'])
def submit_aoa():
    data = request.json
    frame_id = data.get('frame_id')
    locator_id = data.get('locator_id')
    azimuth = data.get('azimuth')
    elevation = data.get('elevation')
    timestamp = time.time()

    if not all([frame_id, locator_id, azimuth, elevation]):
        return jsonify({'error': 'Invalid payload'}), 400

    with data_lock:
        aoa_data[frame_id].append({
            'locator_id': locator_id,
            'azimuth': azimuth,
            'elevation': elevation,
            'timestamp': timestamp
        })

    return jsonify({'status': 'AoA estimate received'}), 200

def process_position_estimation():
    while True:
        current_time = time.time()
        with data_lock:
            frames_to_remove = []
            for frame_id, estimates in aoa_data.items():
                # Filter out old estimates
                estimates = [e for e in estimates if current_time - e['timestamp'] <= TIME_WINDOW]
                aoa_data[frame_id] = estimates

                if len(estimates) >= 2:
                    # Call the position estimation function (abstracted)
                    estimate_position(frame_id, estimates)
                    frames_to_remove.append(frame_id)

            # Clean up processed frames
            for frame_id in frames_to_remove:
                del aoa_data[frame_id]

        time.sleep(0.05)  # Process every 50 ms

def estimate_position(frame_id, estimates):
    # Placeholder for actual triangulation logic
    print(f"Estimating position for frame {frame_id} with estimates: {estimates}")

if __name__ == '__main__':
    # Start the position estimation processor thread
    processor_thread = threading.Thread(target=process_position_estimation, daemon=True)
    processor_thread.start()

    # Run the Flask web server
    app.run(host='0.0.0.0', port=5000)
