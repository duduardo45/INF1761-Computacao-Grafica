#version 410 core

// No output color is needed for a depth-only pass.
// However, some drivers optimize better if you are explicit about empty main.

void main()
{
    // Intentionally empty.
    // The Z-buffer (depth) write happens automatically by the fixed-function pipeline.
}