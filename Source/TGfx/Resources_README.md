# Resources
If you want to use GPU, you have to create at least one shader. And you probably want to get output from the shader with a buffer/texture. Each GPU side data/shader is called *Resource* and this documentation is for all of them. 

# Table of Contents
* [Summary](#Summary)
* [Data Resource Transfers](#Data-Resource-Transfers)
* [Staging Buffer](#Staging-Buffer)
* [Material System](#Material-System)

# Summary
Because there are lots of resource types, here's a short introduction to resource types:
-   *Texture*: All types of images (1-3D, at least one channel). Even though all of them isn't supported for now, every data that GFX APIs accept as image will be supported by the time.
-   *Buffer*: All of GFX API specific buffers will be supported by the time but for now only 2 types of buffer is supported. A shader-side read-only (uniform in Khronos terminology) buffer and its read-write counterpart.
-   *Material*: Collection of rasterization pipeline shaders (vertex, fragment etc.) and stores their input/outputs. You have to create an instance of it to run the pipeline.
-   *Compute*: Stores a compute shader and its input/outputs.
-   *Sampling Type*: While sampling textures, you may want to use a certain type of sampling (limited mip level sampling etc).
-   *RT SlotSet*: Collection of textures references and their access pattern from pixel shaders within the given Draw Pass. Fragment shaders may use attached *RenderTexture*s to perform depth-stencil tests and write pixel calculation results. 
-   *Vertex Attribute Layout*: Vertex shaders uses vertex attributes to access vertex buffers optimally. This object defines all vertex attributes that can be used in the vertex shader.

-   Direct VRAM access isn't supported by most of the GPUs and GFX APIs right now, so you have to use [*Staging Buffer*](#Staging-Buffer). Direct VRAM access will be added as an extension later on.
-   If you want to store and access *Texture*s and *Buffer*s (they are called *Data Resource*) on GPU; you have to create a Staging Buffer, copy data from RAM to it then use a Transfer Pass to copy from Staging Buffer to VRAM.


# Data Resource Transfers
*   *Data Resource*s are generally transfered between VRAM-RAM and TGFX integrates it to the RenderGraph system. You should use a *Transfer Pass* to do any sort of resource transfers.
*   There are 4 types of transfers; from CPU to GPU *Upload*, from GPU to CPU *Download*, GPU to GPU *Copy* and texture layout change *Barrier*.
*   *Upload*: You have to copy data from RAM to staging buffer.
*   *Download*: You have to copy data from staging buffer to RAM.
*   *Copy*: You can copy from one place to another on VRAM or you can copy between VRAM and staging buffer.
*   *Barrier*: You may need to change a texture's layout because accesses'll be faster in new layout.

# Staging Buffer
*   Before following below, you should [read this section](TGFX.md/#VRAM-Allocation-and-Management).
*   Modern GFX APIs uses a temporary buffer called *Staging Buffer* to copy data from RAM to Device Local VRAM.
*   Staging buffer can only be allocated in a SLOW_CPU_GPU or FAST_CPU_GPU memory region and it's done by TGFX backend.

# Material System
*   You have to use Material System to be able to use rasterization pipeline shaders.
*   There are two types of objects: *Material Type* and *Material Instance*.
*   **Material Type**: Stores collection of rasterization pipeline shaders, list of shader input types and rasterization states
*   **Material Instance**: Inherited from a *Material Type*, only contains a list of shader input types.
*   Modern APIs groups shader inputs and call it *Descriptor Set*.
*   You can't use a *Material Type* in any type of rasterization pipeline operation, you can only use Material Instance.
*   *Descriptor Set*s are sorted from global to special. TGFX uses maximum 3 *Descriptor Set* per Material Instance.
*   Global shader inputs doesn't change across different material instances and this *Descriptor Set* is defined as *DescSet0*.
*   Each different material type has its own shader inputs in *DescSet1* and only material instances that are inherited from the same Material Type can access the same shader inputs.
*   Each material instance also has its own shader inputs in *DescSet2* and no other material instance can access them.


# How to write shaders
-   There is no custom shader language for GFX right now, so you have to use languages like SPIR-V, GLSL etc. 
-   For now, only SPIR-V is supported. Run time GLSL->SPIR-V will be supported, you can [check the status here]()
-   If you are using GLSL and Vulkan, you can follow below:
-   Shader Input = Vulkan Descriptor. Descriptor Set 0 is for global buffers and there is at least one global buffer in all applications (if you don't create any global buffer, Vulkan backend will automatically create one). Descriptor Set 1 is for global textures and there is at least one again. When you create a global buffer, you specify its binding index and if you are gonna use it in your shader, you should specify the same value there too!
-   Shader Inputs are in 3 categories: Global, General (Different between material types but same between material instances that are instantinated from the same material type) and Per Instance (Has different values between material instances that are instantinated from the same material type). You can have an array of buffers/textures in a descriptor in Vulkan and this is supported by specifying the number of elements in ShaderInput_Description while linking your material type.
-   If your shader has general shader input, they should use Descriptor Set 1. Then per instance shader inputs should use Descriptor Set 2. If your shader doesn't have any general shader input, then per instance shader inputs should use Descriptor Set 1. While describing shader inputs in ShaderInput_Description, BINDINGPOINT shouldn't exceed the number of shader inputs of the type. That means, you shouldn't use BINDINGPOINT 1 if you have only one shader input of the type (you should use BINDINGPOINT 0).

# ShaderInput Limits
-   Every GPU has a limit for shader inputs of every type, so you shouldn't exceed these limits (Not even get close to them). 
-   If GFX API found that your GPU has Descriptor Indexing support, these limits may grow but this features affects these things on your side:
    1)  You can use NonUniformResourceIndex in your shaders
    2)  SetMaterial_xxx functions' isUsedLastFrame argument means if you accessed the shader input by material type/instance last frame (If DescriptorIndexing is not supported, it means if the material type/instance is used last frame)
    3)  You are able to use SetMaterial_xxx and SetGlobal_xxx functions whenever you want till you don't access them in shaders (Otherwise you have to set all global shader inputs SetGlobal_xxx before finishing constructing RenderGraph and you have to set all general/per instance shader inputs before using them first time).
    4)  You are able to define different texture descriptors -or descriptor arrays- with different data types/dimensions while set and binding indexes are same in shaders. Otherwise each binding index in a descriptor set should be unique to a descriptor -or descriptor array- and you can only access them with one data type/dimension.
    5)  Because maximum descriptor counts are very large, there is no need to allocate hundreds of thousands of descriptor. Vulkan allows unbounded arrays to be in only last binding index, so this is handled at Second Initialization Stage. In GPUSecondStage structure, you already have to fill GlobalBuffers and GlobalTextures descriptions. The ones that has binding index 1 in both descriptions are unbounded arrays and the given ElementCount is maximum allocatable descriptor count (other descriptor should have binding index 0).
