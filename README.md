# HFT_sys: Modern C++20 Electronic Trading Ecosystem

## Description

HFTCore is a project that implements a simplified version of an electronic trading ecosystem using modern C++20 techniques. It focuses on the most latency-sensitive components to demonstrate low-latency application development in the context of high-frequency trading systems.

Inspired by and applying knowledge from:
- "Building Low Latency Applications with C++" by Sourav Ghosh
- "Developing High-Frequency Trading Systems" by Sebastien Donadio, Sourav Ghosh, and Romain Rossier

## System Components

HFTCore implements a simplified version of a high-frequency trading ecosystem, including both exchange and trading client components.

### Exchange Components

1. Matching Engine
    - Core component of the electronic trading exchange
    - Matches buy and sell orders efficiently

2. Order Gateway Server
    - Handles incoming order requests
    - Includes protocol encoder and decoder for communication

3. Market Data Encoder and Publisher
    - Encodes market data in a standardized format
    - Publishes real-time market data to subscribers

### Trading Client Components

1. Market Data Consumer and Decoder
    - Receives and decodes market data feeds
    - Processes real-time market information for trading decisions

2. Order Gateway Client
    - Encodes and sends order requests to the exchange
    - Decodes responses from the exchange

3. Trading Engine
    - Implements trading strategies and decision-making logic
    - Analyzes market data and generates trading signals

## Technologies Used

- C++20
- Google Test ( work in progress )
- Google Benchmark ( work in progress )

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
git clone https://github.com/zebiriabderrahim/low_latency_trading_app.git
cd HFT_sys
mkdir build && cd build
cmake ..
```
## Performance Considerations

- Lock-free queues for inter-thread communication
- Cache-friendly data structures
- Careful memory management to minimize allocations
- Custom socket implementations for low-latency networking
- Thread affinity optimization (Linux-specific, work in progress for macOS)

## Known Issues

- Socket timestamp functionality on macOS is currently being worked on
- Thread affinity settings on macOS are under development

## Limitations

This is a simplified version of a real-world trading system. Additional components typically found in practice include:
- Historical data capture
- Connections to clearing brokers
- Backend systems for transaction processing
- Accounting and reconciliation systems
- Backtesting frameworks

## Contributing

feel free to submit a pull request or open an issue for any bugs or feature requests.

## License

This project is licensed under the MIT License - see the LICENSE.md file for details.

## Acknowledgments

- Sourav Ghosh, Sebastien Donadio, and Romain Rossier for their insightful books on low-latency trading systems
- The C++ community for continuous improvements in the language, especially in the areas of performance and concurrency
