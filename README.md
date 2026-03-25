# Cost-Based Cache Replacement Policy for ndnSIM

## Project Overview

This repository provides the implementation of the **Cost-Based Cache (CBC) replacement policy** for **Information-Centric Networking (ICN)** using the **ndnSIM 2.9 simulator**.

The CBC strategy aims to improve caching efficiency by considering multiple factors when making cache replacement decisions. Unlike traditional caching policies such as **FIFO** and **LRU**, which rely on simple heuristics, the CBC approach incorporates a **cost-based decision mechanism** to optimize cache utilization and improve overall network performance.

The performance of the proposed CBC policy is evaluated through simulation experiments using different network topologies and cache configurations.

# Simulator Environment

The simulation experiments were conducted using **ndnSIM 2.9**, an open-source network simulator built on **NS-3** and designed for the **Named Data Networking (NDN)** architecture.

### System Specifications

* **Processor:** Intel Core i5-9400F CPU @ 2.90 GHz
* **Memory:** 8 GB RAM
* **Operating System:** Ubuntu 20.04 LTS
* **Network Simulator:** ndnSIM 2.9 (NS-3 based)

# Network Topology Scenarios

Two network topology scenarios were used to evaluate the effectiveness of the CBC replacement policy.

## Scenario 1: 5×5 Grid Topology

This topology represents a structured and controlled network environment.

* Total nodes: **25**
* Content producers: **2 nodes (Node 0 and Node 20)**
* Consumer nodes: **5 nodes**
* Remaining nodes act as routers with caching capability.

This topology allows controlled experimentation to observe caching behaviour and data dissemination patterns.

## Scenario 2: Abilene Topology

The Abilene topology represents a **real-world backbone network structure**.

* Total nodes: **11**
* Nodes represent major routers from the original **Abilene Internet2 network**
* Each node functions as **both producer and consumer**
* Creates a dynamic and heterogeneous traffic environment.

This scenario is used to evaluate the CBC strategy under more realistic network conditions.

# Cache Replacement Strategies

The CBC replacement policy is evaluated against several existing caching strategies commonly used in ICN research.

### Compared Strategies

* **FIFO** – First In First Out
* **LRU** – Least Recently Used
* **LRFU** – Least Recently/Frequently Used
* **PFR** – Popularity Freshness Replacement
* **CBC** – Proposed Cost-Based Cache Policy

# Performance Metrics

The performance evaluation focuses on three key metrics.

### Cache Hit Ratio (CHR)

Measures the proportion of requests successfully served from the cache. Higher CHR indicates more efficient cache utilization.

### Cache Miss Ratio (CMR)

Represents the percentage of requests that cannot be satisfied by the cache and must be forwarded to the producer. Lower CMR indicates better caching performance.

### Average Retrieval Delay

Measures the average time required for requested content to be delivered to the consumer. Lower retrieval delay indicates faster content delivery.

# Repository Structure

The repository is organized as follows:

CBC-Replacement-Policy-ndnSIM/
│
├── examples/
│   Simulation scenario scripts used to run ndnSIM experiments.
│   These files define network topologies, traffic generation,
│   and caching configurations.
│
├── helper/
│   Helper modules and utility functions used to support
│   the simulation environment.
│
├── table/
│   Implementation of the CBC cache replacement mechanism
│   and related data structures.
│
└── README.md
    Project documentation.

# Running the Simulation

### 1. Install ndnSIM

Follow the official ndnSIM installation guide:

[https://ndnsim.net/current/getting-started.html](https://ndnsim.net/current/getting-started.html)

Ensure the following are installed:

* NS-3
* ndnSIM 2.9
* Ubuntu 20.04 (recommended)

### 2. Clone the Repository

```bash
git clone https://github.com/ainaacollege-bit/CBC-Replacement-Policy-ndnSIM.git
cd CBC-Replacement-Policy-ndnSIM
```

### 3. Compile the Simulator

From the ndnSIM directory:

```bash
./waf configure
./waf
```

### 4. Run Example Simulation

Example scenario scripts are located in:

```
examples/
```

Run a simulation using:

```bash
./waf --run examples/<simulation-file>
```

Replace `<simulation-file>` with the appropriate simulation script.

# Research Contribution

This project contributes to the research area of **cache management in Information-Centric Networking (ICN)** by introducing a **cost-based cache replacement mechanism** designed to improve:

* cache efficiency
* content retrieval performance
* overall network resource utilization

The proposed approach evaluates multiple factors when selecting cache replacement candidates, enabling more intelligent caching decisions compared to traditional policies.

# License

This project is intended for **academic and research purposes**.

