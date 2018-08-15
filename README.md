# Graphics Engine

This repo documents the design of a graphics co-processor, along
with some tools (assembler, debugger, simulator) and microcode.
It was designed by Lawrence Kesteloot and Rob Wheeler in March 1995
for the University of North Carolina CS 265 (Computer Architecture)
final project.

See the [final write-up](doc/ge.pdf) for the details.

From the introduction of the write-up:

> This document outlines the design of the Graphics Engine (GE), which
> has an optimized instruction set for transforming and rasterizing 3-D
> graphics primitives for low-end machines.  The GE typically is
> included in home or arcade video game machines.  A general-purpose CPU
> and the GE share memory for communication: The CPU calculates the
> game dynamics and sets up a graphics data structure (a display list of
> 3-D primitives) that the GE traverses and rasterizes in a pipeline
> arrangement.  The GE has a RISC core with a back-end specialized
> for rasterizing triangles.
> 
> A typical application is a game that uses 3-D graphics along with
> sound and 2-D sprites (the latter two being supplied by other
> hardware).  The application uses the GE to transform, light, and shade
> the generated geometry.  A typical game might require up to
> 3,000 triangles per scene to achieve rendering detail comparable to
> current 2-D games.  Thus, at 30 frames a second, roughly 100,000
> triangles per second must be transformed and half of these must be lit
> and rasterized.  (Over half of the triangles will be removed by
> backface and frustum culling.)
> 
> The GE instruction set is optimized for the following tasks:
> 
> * traversing a hierarchical display list;
> * transforming coordinates;
> * calculating plane equations; and
> * rasterizing polygons.
> 
> Only minimal 3-D features are supported--clipping,
> diffuse lighting, Gouraud shading, screen-door transparency, and
> Z-buffering.  The GE has no anti-aliasing, texturing, or blending
> support.
> 
> This is a very specialized CPU, not only because it is optimized
> for graphics, but because it is designed for low-end, inexpensive
> graphics.  It does not provide the flexibility of high-end graphics
> workstations, and the game programmer is expected to abide by
> restrictions, such as keeping coordinates within the range of
> fixed-point values.  When necessary, we simplified the hardware at the
> expense of a heavier burden on the programmer.  Most game programmers
> are used to programming in assembler and having no programming support
> whatsoever, so these are not unusual nor unexpected demands.

# License

Copyright 1995 Lawrence Kesteloot and Rob Wheeler

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
