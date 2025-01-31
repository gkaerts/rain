# Rain: **R**enders **A**ll **I** **N**eed

## Overview
**Rain** is a modern high performance real-time rendering engine written in C++ for x64 Windows. It is designed for experimentation with different rendering techniques as well as general rendering engine architecture.

The engine started its life as a different project personal rendering playground project. I decided to do a large cleanup and refactoring pass and make it publicly available here under a new name. I'm slowly working on porting things over until I've met general feature parity, although certain features which I'm not keen on supporting anymore will be left out.

Additionally I'd like to integrate my TBDR-specific Vulkan rendering engine into Rain as well at some point, but it'd require some code bifurcation. Maybe something to do at a later point in time

## Libraries
* [**Common**](https://github.com/gkaerts/rain/tree/master/libs/common): Platform and compiler defines, memory management, logging, threading and math
* [**RHI**](https://github.com/gkaerts/rain/tree/master/libs/rhi): Low-level rendering hardware interface
* [**D3D12 RHI**](https://github.com/gkaerts/rain/tree/master/libs/rhi_d3d12): D3D12 implementation of the RHI interface
* [**Application**](https://github.com/gkaerts/rain/tree/master/libs/application): Event processing, window management
* [**Render Graph**](https://github.com/gkaerts/rain/tree/master/libs/render_graph): Simple frame graph style render pass management
* [**Asset**](https://github.com/gkaerts/rain/tree/master/libs/asset): Library for defining and processing asset types. Will be extended to include support for asset tooling.
* [**Data**](https://github.com/gkaerts/rain/tree/master/libs/data): Data/asset schema definitions and their accompanying asset loading code
* [**usd**](https://github.com/gkaerts/rain/tree/master/libs/usd): OpenUSD plugins

## Coming Up

Here's an overview of what I'll be working on next:
* **Vulkan RHI Library**: Vulkan implementation of the RHI interface - **In Progress**
* **Editor**: ImGui-based editor framework for manipulating scenes - **Not started**
* **Rendering Features Library**: Collection of various fun rendering features - **Not started**

## Dependencies
Some dependencies are set up as git submodules (see [contrib/submodules](https://github.com/gkaerts/rain/tree/master/contrib/submodules)), while others are downloaded during configuration using a custom premake module.

OpenUSD is its own beast and will require a one-time command to be run to download and build the library. This currently makes the first time setup take a long amount of time, and I'd like to provide the option in the future to disable the inclusion of OpenUSD in case no tinkering with the asset import pipeline is required.

Here's a list of external libraries used:
* [**basis_universal**](https://github.com/BinomialLLC/basis_universal) - Texture asset compression and runtime transcoding
* [**enkiTS**](https://github.com/dougbinks/enkiTS) - Task scheduling
* [**googletest**](https://github.com/google/googletest) - Unit testing framework
* [**meshoptimizer**](https://github.com/zeux/meshoptimizer) - Mesh asset optimization and meshlet generation
* [**MikkTSpace**](https://github.com/mmikk/MikkTSpace) - Tangent vector generation for mesh assets
* [**mio**](https://github.com/vimpunk/mio) - Cross-platform memory mapped IO (mostly for loading assets from disk)
* [**OpenUSD**](https://github.com/PixarAnimationStudios/OpenUSD) - Scene and asset description + composition
* [**spdlog**](https://github.com/gabime/spdlog) - Runtime logging
* [**unordered_dense**](https://github.com/martinus/unordered_dense) - Fast hashmap/set implementation
* [**hlslpp**](https://github.com/redorav/hlslpp) - HLSL-style math library
* [**SDL**](https://github.com/libsdl-org/SDL) - Windowing and input management
