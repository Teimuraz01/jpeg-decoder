# jpeg-decoder
Reads a [picture-name].jpeg or [picture-name].jpg(baseline sequential) file and produces .ppm file with the same picture.
**make** to compile - **make clean** to remove all .o and executables.
To run the program on a picture : ./jpeg2ppm [--idct-slow]  [image].jpg
Option **--idct-slow** runs the program with the naive [idct](https://en.wikipedia.org/wiki/JPEG#Discrete_cosine_transform)  algorithm, by default the idct uses the Van Eijdhoven et Sijstermans algorithm.
