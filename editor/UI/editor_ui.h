#pragma once
typedef struct component_pecf* compHnd_pecf;  // Not a direct pointer to component data
typedef struct componentType_pecf* compTypeHnd_pecf;

// Visualize/Edit the component's values in editor
typedef struct editor_ui_compVisFuncs {
  // Do operations while initializing editor
  void (*editorInitialization)();
  // View components values in entity inspector view
  void (*editorVisualizationTab)(compHnd_pecf handle);
} compVisFuncs_ui_editor;

typedef struct editor_ui {
  void (*register_compVisualization)(compTypeHnd_pecf type, editor_ui_compVisFuncs funcs);
} ui_editor;