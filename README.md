# Modern C++20 Electronic Trading Ecosystem

## Description

This project implements a simplified version of an electronic trading ecosystem using modern C++20 techniques. It focuses on the most latency-sensitive components to demonstrate low-latency application development in the context of high-frequency trading systems.

Inspired by and applying knowledge from:
- "Building Low Latency Applications with C++" by Sourav Ghosh
- "Developing High-Frequency Trading Systems" by Sebastien Donadio, Sourav Ghosh, and Romain Rossier

## Key Components

- Order Book Management
- Market Data Processing
- Order Execution Engine
- Low-Latency Networking Module

## Technologies Used

- C++20

## Compatibility

This project is designed to work on both Linux and macOS. However, there are some ongoing issues with socket timestamps and thread affinity under macOS that are currently being addressed.

## Development Environment

- OS: Linux Ubuntu / macOS Sonoma 14.6
- Compiler: GCC 14.1.0
- Build System: CMake 3.23.2
- Build Tool: Ninja 1.10.2

## Prerequisites

- C++20 compliant compiler (e.g., GCC 14.1.0 or later)
- CMake 3.23.2 or higher
- Ninja 1.10.2 or higher

## Building the Project

```bash
git clone https://github.com/yourusername/cpp20-trading-ecosystem.git
cd cpp20-trading-ecosystem
mkdir build && cd build
cmake -G Ninja ..
ninja
```
