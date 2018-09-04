#include "../../structures/image2d.h"

#include "combinatorialthresholder.h"
#include "thresholdtools.h"

void CombinatorialThresholder::HorizontalVarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold)
{
	size_t width = input->Width()-length+1;
	for(size_t y=0;y<input->Height();++y) {
		for(size_t x=0;x<width;++x) {
			bool flag = true;
			for(size_t i=0;i<length;++i) {
				if(input->Value(x+i, y) < threshold && input->Value(x+i, y) > -threshold) {
					flag = false;
					break;
				}
			}
			if(flag) {
				for(size_t i=0;i<length;++i)
					mask->SetValue(x + i, y, true);
			}
		}
	}
}

void CombinatorialThresholder::VerticalVarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold)
{
	size_t height = input->Height()-length+1; 
	for(size_t y=0;y<height;++y) {
		for(size_t x=0;x<input->Width();++x) {
			bool flag = true;
			for(size_t i=0;i<length;++i) {
				if(input->Value(x, y+i) <= threshold && input->Value(x, y+i) >= -threshold) {
					flag = false;
					break;
				}
			}
			if(flag) {
				for(size_t i=0;i<length;++i)
					mask->SetValue(x, y + i, true);
			}
		}
	}
}

void CombinatorialThresholder::VarThreshold(const Image2D* input, Mask2D* mask, size_t length, num_t threshold)
{
	HorizontalVarThreshold(input, mask, length, threshold);
	VerticalVarThreshold(input, mask, length, threshold);
}
