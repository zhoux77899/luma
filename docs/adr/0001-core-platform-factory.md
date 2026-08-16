# Core stays platform-independent via a Cardputer factory TU

Luma Core must build and be tested without Cardputer or Arduino headers. Firmware still uses the RFC `Luma luma; luma.begin();` entry point, and that global is constructed before `M5Cardputer.begin()`, so adapters are inert in their constructors and touch hardware only from `Luma::begin()`. The default constructor lives only in `src/luma/platform/cardputer/luma-factory.cpp` and binds function-local static adapters so construction does not depend on translation-unit init order. Native tests construct Luma with injected fakes.
