# OpenCL Resizer

### Goal
Goal of this project is to explore performance of OpenCL and its applications on image processing.

### What it does
 It's a high performance image resizing utility. It supports JPEG images and 3 sampling methods:
 * nearest neighbor
 * linear interpolation
 * cubic interpolation
 
 ### How fast is it
 For high resolution images (2k+) it is faster than OpenCV.
 It's not particularly fast for low resolution images due to overhead of data transfer between CPU and GPU.
 
### How does it work
It resizes images on GPU (when found) rather than CPU. Detailed explanation can be found on my blog http://vaclavsamec.com/blog_resizer.
 
### How to use it
Call `resizer` with these parameters:

| Parameter | Description | Default |
| --- | --- | --- |
| `-input` | input directory of JPEG images | required
| `-output` | output directory of resized images | required
| `-quality` | quality of output JPEG image in range <1, 100> | 95
| `-algo` | sampling algorithm, use one of these [`nn`, `linear` `cubic`] | cubic
| `-ratio` | ratio of resizing | 1.0

##### Example
`resizer -output ./testImages/output -input ./testImages/input -quality 95 -algo cubic -ratio 0.5`

### How to build it

You need C++ compiler and following libraries:
* OpenCL
* Libjpeg Turbo
* CMake

See CMakeList.txt for further adjustments of libraries path.
