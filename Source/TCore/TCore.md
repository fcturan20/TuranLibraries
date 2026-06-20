# TCore

Core libraries to provide the best programming experience while starting a new project.

## Coding Guidelines
### Naming Convention

Prefixes for a system; `T` for system's display name, `TC` for system's struct name, `TS` for system's variable name.
Plugin header files (APIs) should be written in C but C++ wrapper sections are also allowed. C++ Wrapper sections should be in `namespace TCore` and shouldn't define any other namepsace. Classes/structs inside source files (.cpp/.c) should be in `namespace TCore{ namespace <Plugin>{}}` to improve code navigation and avoid name collisions.

## Plugins

### TCore
TCore is a plugin manager center. It also provides a plugin SDK which all other plugins uses to implement themselves.

### TECS
TECS is a Entity-Component-System manager. It stores functions as Systems, dynamic data structures as Components and objects as Entities. Use it for dynamic object systems.