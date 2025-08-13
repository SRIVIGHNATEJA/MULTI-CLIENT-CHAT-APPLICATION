<div align="center">
  <h1>⚡ SocketRelay</h1>
  <p><b>High-Performance, Multithreaded TCP Messaging Router Built from First Principles</b></p>
  
  [![Pure C](https://img.shields.io/badge/Language-Pure%20C-00599C?style=for-the-badge&logo=c&logoColor=white)](#)
  [![POSIX Threads](https://img.shields.io/badge/Concurrency-POSIX%20Threads-4B32C3?style=for-the-badge&logo=linux&logoColor=white)](#)
  [![TCP/IP](https://img.shields.io/badge/Networking-TCP%2FIP-FF4500?style=for-the-badge&logo=wireshark&logoColor=white)](#)
  [![Architecture](https://img.shields.io/badge/Architecture-Thread--Per--Client-2ea44f?style=for-the-badge)](#)
</div>

---

> **The Challenge:** Modern distributed systems often rely on high-level frameworks (e.g., Node.js, gRPC) that obscure the underlying complexity of network I/O, state management, and thread contention, making it difficult to isolate low-level bottlenecks.
> 
> **The Solution:** A custom-built, multi-client concurrent terminal messaging platform that bypasses abstractions. Utilizing low-level Berkeley sockets and POSIX multithreading, SocketRelay achieves direct, full-duplex communication over TCP, demonstrating a rigorous, bottom-up approach to systems engineering.

---

## ⚙️ Core Capabilities

- **Concurrent Execution:** Thread-per-client architecture handles simultaneous connections without blocking.
- **Stateful Routing:** Dynamic edge node registry enables global broadcasts and targeted private messaging.
- **Thread-Safe State:** POSIX mutex locks (`pthread_mutex_t`) protect shared memory from data races.
- **Asynchronous I/O:** Dual-threaded clients asynchronously send and receive TCP streams concurrently.

---

## 🏗️ System Architecture & Orchestration

The platform implements a centralized routing architecture that maintains state and securely directs traffic between edge nodes using a thread-per-client concurrency model.

```mermaid
flowchart LR
    classDef server fill:#f8fafc,stroke:#cbd5e1,stroke-width:1px,color:#0f172a;
    classDef thread fill:#eff6ff,stroke:#93c5fd,stroke-width:1px,color:#1e40af;
    classDef state fill:#fdf4ff,stroke:#f0abfc,stroke-width:1px,color:#86198f;
    classDef client fill:#f0fdf4,stroke:#86efac,stroke-width:1px,color:#166534;
    
    subgraph EdgeLayer ["Edge Nodes (Clients)"]
        C1["Client 1 (Alice)"]:::client
        C2["Client 2 (Bob)"]:::client
    end
    
    subgraph CoreRouter ["SocketRelay Engine (Port 8080)"]
        direction TB
        M["Main Thread<br/>(Event Loop: accept)"]:::server
        
        subgraph WorkerPool ["Worker Thread Pool"]
            T1["Thread 1"]:::thread
            T2["Thread 2"]:::thread
        end
        
        S[("Global Client Registry<br/>& Mutex Lock")]:::state
        
        M -->|Spawns| T1
        M -->|Spawns| T2
        
        T1 <-->|Read / Write| S
        T2 <-->|Read / Write| S
    end
    
    C1 == "TCP Stream" ==> T1
    C2 == "TCP Stream" ==> T2
```

### Full-Duplex Client Operation

To prevent the application from blocking on `stdin` (waiting for user input), the client node separates reading and writing into independent, concurrent execution paths.

```mermaid
sequenceDiagram
    participant User
    participant Main as Client (Main Thread)
    participant BG as Client (Background Thread)
    participant Server as Core Router

    Note over Main, Server: 1. TCP Socket Established
    Main->>BG: pthread_create(receive_handler)
    
    loop 2. Asynchronous Full-Duplex I/O
        par Network Listen
            BG->>Server: recv() (Blocking Wait)
        and User Interaction
            User->>Main: Types input (fgets)
            Main->>Server: send()
        end
    end
    
    Server-->>BG: Inbound Broadcast / DM
    BG-->>User: printf() to stdout
```

---

## 💡 Engineering & Architectural Decisions

In high-performance systems, the "why" matters as much as the "how". Here is the rationale behind the system's design, along with an honest audit of its tradeoffs and hardening paths:

| Component / Challenge | Technical Implementation | Rationale & Tradeoff Analysis |
| :--- | :--- | :--- |
| **Concurrency Model** | **Thread-per-Client (`pthread`)** | Ensures strictly isolated execution contexts per connection. *Tradeoff:* High memory overhead for context switching at scale. An enterprise iteration would migrate to an event-driven, non-blocking I/O model (e.g., `epoll` or `kqueue`). |
| **State Integrity** | **Coarse-Grained Mutex Locking** | Protects the shared client registry from race conditions during simultaneous broadcasts. *Tradeoff:* Creates a lock contention bottleneck under heavy load; optimizing this requires fine-grained locking or lock-free data structures. |
| **Memory Safety** | **Fixed-Size Buffers & Arrays** | Hardcoded `MAX_CLIENTS` allows for predictable memory footprints. *Tradeoff:* Susceptible to bounds overflow if capacity logic fails. Hardening requires dynamic allocation and strict memory lifecycle management. |
| **Stream Framing** | **Raw TCP `recv()`** | Interacts directly with the kernel networking stack for zero-abstraction transmission. *Tradeoff:* TCP is a continuous stream, meaning fragmented payloads can occur. Production readiness requires application-level framing (e.g., length-prefixing). |

---

## 🚀 Quickstart & Reproduction

The system compiles natively on POSIX-compliant environments (Linux / macOS) without external dependencies.

**1. Compile the Core Modules**
```bash
gcc Server.c -o server -lpthread
gcc client.c -o client -lpthread
```

**2. Initialize the Core Router**
```bash
./server
```
*(The engine binds to local port `8080` and begins listening).*

**3. Connect Edge Nodes**
*(Open a new terminal window for each client)*
```bash
./client
```
*(Enter a unique username upon connection to register with the core router).*

---
> 🔐 Licensed under **Apache License 2.0**  
> 📌 Developed by **Sri Vighna Teja** | 2025

<div align="center">
  <i>Engineered to demonstrate first-principles distributed systems architecture, concurrency management, and atomic network programming.</i>
</div>
