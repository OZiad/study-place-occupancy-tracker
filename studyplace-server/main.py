from datetime import datetime, timezone

from flask import Flask, jsonify, request

app = Flask(__name__)

last_readings: dict[str, dict] = {}


@app.route("/")
def index():
    return "Study Space Scanner API is running", 200


@app.route("/api/occupancy", methods=["POST"])
def occupancy():
    """
    Expected JSON from ESP32:
    {
        "node_id": "lb8-node-1",
        "free_seats": 2,
        "total_seats": 4
    }
    """
    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "Invalid or missing JSON"}), 400

    node_id = data.get("node_id", "unknown")
    free_seats = data.get("free_seats")
    total_seats = data.get("total_seats")

    if free_seats is None or total_seats is None:
        return jsonify({"error": "free_seats and total_seats required"}), 400

    timestamp = datetime.now(timezone.utc).isoformat()

    last_readings[node_id] = {
        "node_id": node_id,
        "free_seats": free_seats,
        "total_seats": total_seats,
        "timestamp": timestamp,
    }

    print(f"[{timestamp}] Node {node_id}: {free_seats}/{total_seats} seats free")

    return jsonify({"status": "ok", "node_id": node_id}), 200


@app.route("/api/status", methods=["GET"])
def status():
    """
    Simple endpoint for dashboard / debugging.
    Returns last reading from all nodes.
    """
    return jsonify(list(last_readings.values())), 200


if __name__ == "__main__":
    # listen on all interfaces so ESP32 can reach it
    app.run(host="0.0.0.0", port=8080, debug=True)
