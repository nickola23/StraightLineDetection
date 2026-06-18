# Parallel Line Detection using Hough Transform

A high-performance C++ system for detecting straight lines in digital images using the Hough Transform, parallelized with **Intel Threading Building Blocks (TBB)** and organized as a TBB flow graph pipeline.

---

## Overview

The Hough Transform maps edge pixels from image space into a polar parameter space (ρ, θ), where every pixel "votes" for all lines passing through it:

```
ρ = x·cos(θ) + y·sin(θ)
```

Local maxima in the resulting accumulator correspond to dominant lines in the image. For a 768×477 image this means 60,000+ edge pixels × 180 angle values = **10+ million operations** in the transform phase alone — making parallelization essential.

**Achieved speedups: 1.59× – 6.83×** depending on image complexity.

---

## Pipeline Architecture

Processing is organized as a **TBB flow graph** with five sequential nodes, each receiving and forwarding a `PipelineData` structure:

```
continue_msg
    │
    ▼
┌─────────────┐
│  loadNode   │  Load image + parallel grayscale conversion
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  edgeNode   │  Parallel Sobel edge detection (blocked_range2d)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  houghNode  │  Parallel accumulator computation (combinable)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  lineNode   │  Parallel local maxima detection (concurrent_vector)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  sinkNode   │  Collect results
└─────────────┘
```

### Parallelism strategy per phase

| Phase | TBB mechanism | Notes |
|---|---|---|
| Grayscale conversion | `parallel_for` + `blocked_range<int>` | Pixels fully independent |
| Sobel edge detection | `parallel_for` + `blocked_range2d<int>` | Reads input, writes separate output — no race |
| Hough accumulator | `combinable<vector<int>>` | Each thread gets private accumulator; merged at end |
| Line detection | `concurrent_vector` | Safe parallel write during local maxima scan |

---

## Results

### Test images

| Image | Dimensions | Edge pixels | Serial (ms) | Parallel (ms) | Speedup |
|---|---|---|---|---|---|
| `building1.png` | 768×477 | 62,134 | 303.30 | 62.59 | **4.85×** |
| `digital4.jpg` | 3840×2556 | 640,020 | 4064.13 | 595.18 | **6.83×** |
| `road1.png` | 704×337 | 13,414 | 89.93 | 36.58 | **2.46×** |
| `horizont1.png` | 731×434 | 3,270 | 59.89 | 37.58 | **1.59×** |

> Speedup scales with computational load — images with more edge pixels benefit most from parallelization. Small images see reduced gains because TBB thread management overhead becomes relatively significant.

### Scalability analysis (`building1.png`, serial baseline: 310.22 ms)

| Threads | Hough (ms) | Total (ms) | Speedup |
|---|---|---|---|
| 1 | 246.43 | 309.52 | 1.00× |
| 2 | 130.19 | 163.05 | 1.90× |
| 4 | 72.10 | 89.61 | 3.46× |
| 8 | 50.74 | 61.13 | **5.07×** |

Scaling is nearly linear up to 4 cores, with slight saturation at 8 threads due to the accumulator merging step in the `combinable` phase.

---

## Class Structure

```
src/
├── Image              
├── PipelineConfig     
├── PerformanceTimer   
├── ImageLoader       
├── EdgeDetector       
├── HoughTransform     
├── LineDetector       
├── ResultWriter       
└── Pipeline          
```

### Key design decisions

**`HoughTransform::computeParallel`** — uses `tbb::combinable<vector<int>>` so each thread accumulates into a private copy with zero contention, then merges by summation. This eliminates mutexes entirely.

**`LineDetector::findLines`** — parallel local maxima detection with `tbb::concurrent_vector` replaces a naive O(n²) sort-and-suppress approach. On buildings images (27,000+ candidates) this cuts line detection from **~390 ms → ~2 ms** (197× speedup in this phase alone).

**`Pipeline::run`** — one `Pipeline` object is created outside the image loop and reused, avoiding graph re-initialization overhead per image.

---

## Configuration

All parameters are set through `PipelineConfig`:

| Parameter | Description | Default |
|---|---|---|
| `inputPath` | Folder with input images | input |
| `outputPath` | Root output folder | output |
| `edgeThreshold` | Sobel gradient magnitude threshold (0–255) | 50 |
| `houghThreshold` | Minimum votes for a line | 185 |
| `thetaStepDeg` | Angular resolution in degrees | 1.0 |
| `rhoStep` | Distance resolution in pixels | 1.0 |
| `maxLines` | Maximum lines to detect | 50 |
| `nmsRadius` | Non-maximum suppression window radius | 10 |
| `numThreads` | Thread count (0 = TBB automatic) | 0 |

---

## Output

For each input image, the program creates a subfolder `<outputPath>/<image_name>/` containing:

```
output/
└── building1/
    ├── gray.png           # Grayscale conversion result
    ├── edges.png          # Binary edge map after Sobel + threshold
    ├── result.png         # Original image with detected lines drawn in red
    ├── accumulator.png    # Hough parameter space visualization
    └── report.txt         # Detected lines (ρ, θ, votes) + timing + speedup
```

The accumulator visualization maps vote counts to grayscale intensity — bright spots correspond to strong lines. Sinusoidal patterns in the accumulator correspond to individual edge points in the image.

The `report.txt` includes:
- Number of detected lines
- Execution time per phase (grayscale, edge detection, Hough, line detection)
- Speedup vs. the serial reference version

---

## Performance

The program automatically runs three passes for each image:

1. **Serial reference** — all phases executed sequentially, no TBB calls
2. **Scalability analysis** — parallel pipeline repeated with 1, 2, 4, and 8 threads via `tbb::global_control`
3. **Full parallel pipeline** — TBB flow graph with automatic thread count

This produces a fair, consistent speedup measurement without any algorithmic differences between serial and parallel paths.

---

## Requirements

- C++17
- Intel oneAPI TBB (Threading Building Blocks)
- Supported image formats: PNG, JPG/JPEG, BMP, PPM
