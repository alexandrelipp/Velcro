#pragma once

struct HierarchyComponent {
    int level = -1;
    int parent = -1;
    int firstChild = -1;
    int nextSibling = -1;
};

struct MeshComponent {
    uint32_t firstVertexIndex = 0; ///< Index in the index buffer of the first vertex
    uint32_t indexCount = 0;       ///< Number of indices
};
