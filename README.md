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

## Coming Up

Here's an overview of what I'll be working on next:
* **Render Pass Library**: Simple frame graph style render pass management (although it's technically not a graph!) - **Not started**
* **Vulkan RHI Library**: Vulkan implementation of the RHI interface - **In Progress**
* **Asset Library**: Library for defining, loading, baking and packaging assets - **Not started**
* **Editor**: ImGui-based editor framework for manipulating scenes - **Not started**
* **Rendering Features Library**: Collection of various fun rendering features - **Not started**

## Dependencies
Most dependencies are set up as git submodules (see [contrib/submodules](https://github.com/gkaerts/rain/tree/master/contrib/submodules)), although some are directly included in the repo where a direct build from source is not immediately feasible (see [contrib/third_party](https://github.com/gkaerts/rain/tree/master/contrib/third_party)). I'd like to aggressively cut down on including third party libraries directly wherever possible, so some of these might turn into submodules.

Here's a list of external libraries used:
* [**enkiTS**](https://github.com/dougbinks/enkiTS)
* [**spdlog**](https://github.com/gabime/spdlog)
* [**unordered_dense**](https://github.com/martinus/unordered_dense)
* [**hlslpp**](https://github.com/redorav/hlslpp)
* [**SDL**](https://github.com/libsdl-org/SDL)
