## TuranLibraries

    C is a nice language but standard libraries doesn't provide enough functionality and other libraries are either hard to integrate or only a single library. TuranLibraries provides these functionalities while being easy to integrate and fast to compile. All libraries designed to work with CMake and Microsoft vcpkg, so it's recommended to have both of these.

    These libraries are divided into 2 categories for now: TAPI and TGFX. TAPI contains sub-libraries like threading, profiling, logging. TGFX is an abstract library that's derived for each graphics API (Vulkan, D3D12 etc.).

    C standard doesn't support some features like threading which C++ supports. Because of that, all libraries and sub-libraries are defined in C++ while declared as C. So user should use a compiler that supports C++.  Some sub-libraries have dependencies to 3rd party libraries such as Vulkan, GLM and these should be already installed to the system by Microsoft vcpkg.

    Examples is a collection of executable projects that is used to demonstrate all TuranLibraries.

## Table of Contents
* [How to Build](#how-to-build)
* [Documentation](#documentation)
* [Roadmap and Issues](#roadmap)

## How to Build

    You need to install external libraries first to use CMake. Each sub-library has different external dependencies, so please read related library's documentation first. Then you should use CMake to build project for your preferred compiler. Compiler-specific codes are avoided mostly, please report if you have compiler specific issue.

## Documentation

-   Some sub-libraries are so complicated that they need their own documentation, so there is no documentation about a specific sub-library. 
-   Documentation is hierarchical: TAPI and TGFX have their own documentation in their folders, then they point out each sub-library's documentation. 
-   If sub-library's documentation is big, it is possible that it also points to another source (web link, pdf etc).
-   Big documentation files (.md) contains **Table of Contents**. Sections that are written in the file but not added in the table are subject to change soon. Generally such sections are written in hurry not to forget.

-   [*TAPI*](TAPI/TAPI.md)
-   [*TGFX*](TGFXCORE/TGFX.md)

## Roadmap

    Github Issues and Milestones are used as roadmap alternative and Github Discussion is used as a forum-discussion portal. Please read below to learn how to create reports: 
-   Each Github Issue should be created with a **Subject** and a **Topic** label, **Status** label will only be added by me.
-   A **Subject** indicates which library/sub-library is the issue belongs to.
-   A **Topic** indicates very abstract intention of the issue.
-   A **Status** indicates what I'm doing about the issue.

5) Turan Engine has a Material system that consists of Material Type and lots of Material Instances of them. So there is nothing like "Bind Shader, Bind Uniform etc.". Material Type is a collection of programmable rasterization pipeline datas such as blending type, depth test type, shaders. Material Instance is a table that stores values of a Material Type's uniforms (Similar to Unreal's Material Instance but there is no optimization for static uniforms etc). 