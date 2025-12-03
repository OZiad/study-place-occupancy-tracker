import { useEffect, useState } from "react";

interface NodeStatus {
  node_id: string;
  free_seats: number;
  total_seats: number;
  timestamp: string;
}

function App() {
  const [nodes, setNodes] = useState<NodeStatus[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchStatus = async () => {
    try {
      setError(null);
      const res = await fetch("/api/status");
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data: NodeStatus[] = await res.json();
      setNodes(data);
      setLoading(false);
    } catch (err) {
      console.error(err);
      setError("Failed to load status.");
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 5000);
    return () => clearInterval(interval);
  }, []);

  // aggregate stats
  const totalNodes = nodes.length;
  const totalSeats = nodes.reduce((sum, n) => sum + (n.total_seats || 0), 0);
  const totalFree = nodes.reduce((sum, n) => sum + (n.free_seats || 0), 0);
  const totalUsed = totalSeats - totalFree;
  const overallOcc =
    totalSeats > 0 ? Math.round((totalUsed / totalSeats) * 100) : 0;

  return (
    <div className="app">
      <header className="app-header">
        <h1>Study Space Scanner</h1>
        <p>Live occupancy from all scanner nodes.</p>
      </header>

      <main>
        {loading && <p className="status-text">Loading…</p>}
        {error && <p className="status-text error">{error}</p>}

        {!loading && !error && (
          <>
            {nodes.length === 0 ? (
              <p className="status-text">No readings yet. Waiting for nodes…</p>
            ) : (
              <>
                {/* overall stats */}
                <section className="stats-grid">
                  <div className="stat-card">
                    <span className="stat-label">Nodes Online</span>
                    <span className="stat-value">{totalNodes}</span>
                  </div>
                  <div className="stat-card">
                    <span className="stat-label">Total Seats</span>
                    <span className="stat-value">{totalSeats}</span>
                  </div>
                  <div className="stat-card">
                    <span className="stat-label">Free Seats</span>
                    <span className="stat-value">{totalFree}</span>
                  </div>
                  <div className="stat-card stat-occupancy">
                    <span className="stat-label">Overall Occupancy</span>
                    <span className="stat-value">{overallOcc}%</span>
                    <div className="progress">
                      <div
                        className="progress-bar"
                        style={{ width: `${overallOcc}%` }}
                      />
                    </div>
                  </div>
                </section>

                {/* per-node cards */}
                <section className="nodes-grid">
                  {nodes.map((node) => {
                    const used =
                      (node.total_seats || 0) - (node.free_seats || 0);
                    const occ =
                      node.total_seats > 0
                        ? Math.round((used / node.total_seats) * 100)
                        : 0;
                    const date = new Date(node.timestamp);
                    const lastUpdate = isNaN(date.getTime())
                      ? "n/a"
                      : date.toLocaleTimeString();

                    const isFull = node.free_seats === 0;

                    return (
                      <article
                        key={node.node_id}
                        className={`node-card ${isFull ? "full" : "available"}`}
                      >
                        <div className="node-header">
                          <h2>{node.node_id}</h2>
                          <span className="node-pill">
                            {used}/{node.total_seats} used
                          </span>
                        </div>
                        <p className="node-free">
                          Free seats:{" "}
                          <strong>{node.free_seats ?? "unknown"}</strong>
                        </p>
                        <p className="node-occupancy">{occ}% occupancy</p>
                        <p className="node-timestamp">
                          Last update: <span>{lastUpdate}</span>
                        </p>
                      </article>
                    );
                  })}
                </section>
              </>
            )}
          </>
        )}
      </main>
    </div>
  );
}

export default App;
