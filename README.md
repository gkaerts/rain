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

Here's a list of external libraries used:
* [**basis_universal**](https://github.com/BinomialLLC/basis_universal)
* [**enkiTS**](https://github.com/dougbinks/enkiTS)
* [**flatbuffers**](https://github.com/google/flatbuffers)
* [**googletest**](https://github.com/google/googletest)
* [**meshoptimizer**](https://github.com/zeux/meshoptimizer)
* [**MikkTSpace**](https://github.com/mmikk/MikkTSpace)
* [**mio**](https://github.com/vimpunk/mio)
* [**OpenUSD**](https://github.com/PixarAnimationStudios/OpenUSD)
* [**spdlog**](https://github.com/gabime/spdlog)
* [**unordered_dense**](https://github.com/martinus/unordered_dense)
* [**hlslpp**](https://github.com/redorav/hlslpp)
* [**SDL**](https://github.com/libsdl-org/SDL)
