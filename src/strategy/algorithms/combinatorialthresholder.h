#ifndef COMBINATORIAL_THRESHOLDER_H
#define COMBINATORIAL_THRESHOLDER_H

#include <cstddef>
#include <cstring>

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"

class CombinatorialThresholder {
public:
	static void VarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
	
	static void HorizontalVarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);
	
	static void VerticalVarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold);

private:
	CombinatorialThresholder() = delete;
};

#endif
