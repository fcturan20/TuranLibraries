# C++ Naming Convention

## General Principles

* Names should describe intent rather than implementation.
* Avoid abbreviations unless universally accepted (CPU, GPU, URL) or defined as a plugin.
* Prefer full words.
* Use singular nouns for single objects and plural nouns for collections.

## Namespaces

* snake_case
* Example:

  * graphics
  * asset_import

## Classes / Structs

* PascalCase
* Types represent nouns.
* In C headers, always typedef struct with the same name
* Example:

  * RenderPipeline
  * TextureCache

## Interfaces

* Prefix with I.
* Example:

  * IRenderer
  * IAssetLoader

## Enums

* Enum type name is PascalCase.
* Enum values use UPPER_CASE with type name in it.
* Example:
  enum class TextureFormat
  {
  TEXTURE_FORMAT_RGBA8,
  TEXTURE_FORMAT_BC7
  };

## Functions and Function Pointers Inside Class/Structs

* PascalCase.
* Functions represent actions.
* Example:

  * LoadTexture()
  * RebuildAccelerationStructure()

## Boolean Functions and Function Pointers Inside Class/Structs

* Start with Is, Has, Can, Should.
* Example:

  * IsVisible()
  * HasPendingRequests()

## Member Variables and Static Member Variables

* Don't prefix.
* PascalCase.
* Example:

  * RenderTarget
  * FrameIndex

## Global Variables

* Allow only inside .cpp/.c files
* Prefix with g
* PascalCase
* Example:

    * gContext
    * gStandartTexture

## Constants

* Allow only inside .cpp/.c files
* Prefix with k.
* PascalCase.
* Example:

  * kMaxFramesInFlight
  * kDefaultTimeoutMs

## Parameters

* snake_case.
* Example:
  void Upload(Buffer& destination_buffer);

## Local Variables

* camelCase.
* Example:
  uint32_t frameIndex;

## Smart Pointers

* Name by ownership target.
* Example:
  unique_ptr<Texture> texture;
  shared_ptr<Device> device;

## Acronyms

* Treat acronyms as words.
* Example:
  XmlParser
  HttpRequest
  GpuDevice

## Files

* Match primary type or plugin name.
* Example:
  TextureCache.h
  TextureCache.cpp

## Templates

* Use descriptive names.
* Example:
  TValue
  TAllocator

## Macros

* Uppercase
* Example:
    #define TCORE_PLUGIN_DEFINE
    #define TCORE_END_C_LINKAGE

Avoid:

* Hungarian notation.
* Single-letter names except loop indices.
* Abbreviations like tex, cfg, mgr unless domain-standard.
