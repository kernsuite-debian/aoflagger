#include "morphology.h"
#include "morphologicalflagger.h"
#include "siroperator.h"

#include "../util/logger.h"

#include <stack>
#include <iostream>

namespace algorithms {

size_t Morphology::BROADBAND_SEGMENT = 1, Morphology::LINE_SEGMENT = 2,
       Morphology::BLOB_SEGMENT = 3;

void Morphology::SegmentByMaxLength(const Mask2D* mask,
                                    SegmentedImagePtr output) {
  int** lengthWidthValues = new int*[mask->Height()];
  for (size_t y = 0; y < mask->Height(); ++y)
    lengthWidthValues[y] = new int[mask->Width()];

  calculateOpenings(mask, lengthWidthValues);

  for (size_t y = 0; y < mask->Height(); ++y) {
    for (size_t x = 0; x < mask->Width(); ++x) output->SetValue(x, y, 0);
  }
  for (size_t y = 0; y < mask->Height(); ++y) {
    for (size_t x = 0; x < mask->Width(); ++x) {
      if (mask->Value(x, y) && output->Value(x, y) == 0) {
        floodFill(mask, output, lengthWidthValues, x, y,
                  output->NewSegmentValue());
      }
    }
  }

  for (size_t y = 0; y < mask->Height(); ++y) delete[] lengthWidthValues[y];
  delete[] lengthWidthValues;
}

void Morphology::SegmentByLengthRatio(const Mask2D* mask,
                                      SegmentedImagePtr output) {
  const Mask2DPtr maskCopy(new Mask2D(*mask));

  Mask2DPtr matrices[3];
  for (size_t i = 0; i < 3; ++i)
    matrices[i] = Mask2D::CreateUnsetMaskPtr(mask->Width(), mask->Height());

  int **hCounts = new int *[mask->Height()],
      **vCounts = new int *[mask->Height()];
  for (size_t y = 0; y < mask->Height(); ++y) {
    hCounts[y] = new int[mask->Width()];
    vCounts[y] = new int[mask->Width()];
  }

  // Calculate convolved counts
  calculateHorizontalCounts(maskCopy.get(), hCounts);
  calculateVerticalCounts(maskCopy.get(), vCounts);

  calculateOpenings(maskCopy.get(), matrices, hCounts, vCounts);

  for (size_t y = 0; y < mask->Height(); ++y) {
    for (size_t x = 0; x < mask->Width(); ++x) output->SetValue(x, y, 0);
  }
  MorphologicalFlagger::DilateFlags(matrices[0].get(), _hLineEnlarging, 0);
  MorphologicalFlagger::DilateFlags(matrices[2].get(), 0, _vLineEnlarging);
  SIROperator::OperateHorizontally(*matrices[0], _hDensityEnlargeRatio);
  SIROperator::OperateVertically(*matrices[2], _vDensityEnlargeRatio);

  // Calculate counts again with new matrices
  calculateHorizontalCounts(matrices[0].get(), hCounts);
  calculateVerticalCounts(matrices[2].get(), vCounts);

  for (size_t z = 0; z < 3; z += 2) {
    for (size_t y = 0; y < mask->Height(); ++y) {
      for (size_t x = 0; x < mask->Width(); ++x) {
        if (matrices[z]->Value(x, y) && output->Value(x, y) == 0) {
          floodFill(mask, output, matrices, x, y, z, output->NewSegmentValue(),
                    hCounts, vCounts);
        }
      }
    }
  }

  for (size_t y = 0; y < mask->Height(); ++y) {
    delete[] hCounts[y];
    delete[] vCounts[y];
  }
  delete[] hCounts;
  delete[] vCounts;
}

void Morphology::calculateHorizontalCounts(const Mask2D* mask, int** values) {
  for (size_t y = 0; y < mask->Height(); ++y) {
    size_t length = 0;

    for (size_t x = 0; x < mask->Width(); ++x) {
      if (mask->Value(x, y)) {
        ++length;
      } else if (length > 0) {
        for (size_t i = x - length; i < x; ++i) {
          values[y][i] = length;
        }
        length = 0;
        values[y][x] = 0;
      } else {
        values[y][x] = 0;
      }
    }
    for (size_t i = mask->Width() - length; i < mask->Width(); ++i) {
      values[y][i] = length;
    }
  }
}

void Morphology::calculateVerticalCounts(const Mask2D* mask, int** values) {
  for (size_t x = 0; x < mask->Width(); ++x) {
    size_t length = 0;

    for (size_t y = 0; y < mask->Height(); ++y) {
      if (mask->Value(x, y)) {
        ++length;
      } else if (length > 0) {
        for (size_t i = y - length; i < y; ++i) {
          values[i][x] = length;
        }
        length = 0;
        values[y][x] = 0;
      } else {
        values[y][x] = 0;
      }
    }
    for (size_t i = mask->Height() - length; i < mask->Height(); ++i) {
      values[i][x] = length;
    }
  }
}

void Morphology::calculateOpenings(const Mask2D* mask, int** values) {
  for (size_t y = 0; y < mask->Height(); ++y) {
    size_t length = 0;

    for (size_t x = 0; x < mask->Width(); ++x) {
      if (mask->Value(x, y)) {
        ++length;
      } else if (length > 0) {
        for (size_t i = x - length; i < x; ++i) {
          values[y][i] = length;
        }
        length = 0;
        values[y][x] = 0;
      } else {
        values[y][x] = 0;
      }
    }
    if (length > 0) {
      for (size_t i = mask->Width() - length; i < mask->Width(); ++i) {
        values[y][i] = length;
      }
    }
  }

  for (size_t x = 0; x < mask->Width(); ++x) {
    size_t length = 0;

    for (size_t y = 0; y < mask->Height(); ++y) {
      if (mask->Value(x, y)) {
        ++length;
      } else if (length > 0) {
        for (size_t i = y - length; i < y; ++i) {
          if (values[i][x] < (int)length) values[i][x] = -(int)length;
        }
        length = 0;
      }
    }
    if (length > 0) {
      for (size_t i = mask->Height() - length; i < mask->Height(); ++i) {
        if (values[i][x] < (int)length) values[i][x] = -(int)length;
      }
    }
  }
}

void Morphology::calculateOpenings(const Mask2D* mask, Mask2DPtr* values,
                                   int** hCounts, int** vCounts) {
  // const int zThreshold = 5;

  for (size_t y = 0; y < mask->Height(); ++y) {
    for (size_t x = 0; x < mask->Width(); ++x) {
      const bool v = mask->Value(x, y);
      values[0]->SetValue(x, y, v && (hCounts[y][x] > vCounts[y][x]));
      values[1]->SetValue(x, y, v && false);
      // values[1]->SetValue(x, y, v && (abs(hCounts[y][x] - vCounts[y][x]) <
      // zThreshold));
      values[2]->SetValue(x, y, v && (hCounts[y][x] <= vCounts[y][x]));
    }
  }
}

struct MorphologyPoint2D {
  size_t x, y;
};
struct MorphologyPoint3D {
  size_t x, y, z;
};

void Morphology::floodFill(const Mask2D* mask, SegmentedImagePtr output,
                           const int* const* lengthWidthValues, size_t x,
                           size_t y, size_t value) {
  std::stack<MorphologyPoint2D> points;
  MorphologyPoint2D startPoint;
  startPoint.x = x;
  startPoint.y = y;
  points.push(startPoint);
  do {
    const MorphologyPoint2D p = points.top();
    points.pop();
    output->SetValue(p.x, p.y, value);
    const int z = lengthWidthValues[p.y][p.x];
    if (p.x > 0 && output->Value(p.x - 1, p.y) == 0 &&
        mask->Value(p.x - 1, p.y)) {
      const int zl = lengthWidthValues[p.y][p.x - 1];
      if ((zl > 0 && z > 0) || (zl < 0 && z < 0)) {
        MorphologyPoint2D newP;
        newP.x = p.x - 1;
        newP.y = p.y;
        points.push(newP);
      }
    }
    if (p.x < mask->Width() - 1 && output->Value(p.x + 1, p.y) == 0 &&
        mask->Value(p.x + 1, p.y)) {
      const int zr = lengthWidthValues[p.y][p.x + 1];
      if ((zr > 0 && z > 0) || (zr < 0 && z < 0)) {
        MorphologyPoint2D newP;
        newP.x = p.x + 1;
        newP.y = p.y;
        points.push(newP);
      }
    }
    if (p.y > 0 && output->Value(p.x, p.y - 1) == 0 &&
        mask->Value(p.x, p.y - 1)) {
      const int zt = lengthWidthValues[p.y - 1][p.x];
      if ((zt > 0 && z > 0) || (zt < 0 && z < 0)) {
        MorphologyPoint2D newP;
        newP.x = p.x;
        newP.y = p.y - 1;
        points.push(newP);
      }
    }
    if (p.y < mask->Height() - 1 && output->Value(p.x, p.y + 1) == 0 &&
        mask->Value(p.x, p.y + 1)) {
      const int zb = lengthWidthValues[p.y + 1][p.x];
      if ((zb > 0 && z > 0) || (zb < 0 && z < 0)) {
        MorphologyPoint2D newP;
        newP.x = p.x;
        newP.y = p.y + 1;
        points.push(newP);
      }
    }
  } while (points.size() != 0);
}

void Morphology::floodFill(const Mask2D* mask, SegmentedImagePtr output,
                           Mask2DPtr* matrices, size_t x, size_t y, size_t z,
                           size_t value, int** hCounts, int** vCounts) {
  std::stack<MorphologyPoint3D> points;
  MorphologyPoint3D startPoint;
  startPoint.x = x;
  startPoint.y = y;
  startPoint.z = z;
  points.push(startPoint);
  do {
    const MorphologyPoint3D p = points.top();
    points.pop();
    if (mask->Value(p.x, p.y)) {
      if (output->Value(p.x, p.y) == 0) {
        output->SetValue(p.x, p.y, value);
      } else {
        // now we need to decide whether to change this sample to the new
        // segment or not
        if (hCounts[p.y][p.x] < vCounts[p.y][p.x] && p.z == 2)
          output->SetValue(p.x, p.y, value);
      }
    }
    const Mask2DPtr matrix = matrices[p.z];
    matrix->SetValue(p.x, p.y, false);
    if ((p.z == 0 || p.z == 2) && matrices[1]->Value(p.x, p.y)) {
      MorphologyPoint3D newP;
      newP.x = p.x;
      newP.y = p.y;
      newP.z = 1;
      points.push(newP);
    }
    if (p.x > 0 && matrix->Value(p.x - 1, p.y)) {
      MorphologyPoint3D newP;
      newP.x = p.x - 1;
      newP.y = p.y;
      newP.z = p.z;
      points.push(newP);
    }
    if (p.x < mask->Width() - 1 && matrix->Value(p.x + 1, p.y)) {
      MorphologyPoint3D newP;
      newP.x = p.x + 1;
      newP.y = p.y;
      newP.z = p.z;
      points.push(newP);
    }
    if (p.y > 0 && matrix->Value(p.x, p.y - 1)) {
      MorphologyPoint3D newP;
      newP.x = p.x;
      newP.y = p.y - 1;
      newP.z = p.z;
      points.push(newP);
    }
    if (p.y < mask->Height() - 1 && matrix->Value(p.x, p.y + 1)) {
      MorphologyPoint3D newP;
      newP.x = p.x;
      newP.y = p.y + 1;
      newP.z = p.z;
      points.push(newP);
    }
  } while (points.size() != 0);
}

void Morphology::Cluster(SegmentedImagePtr segmentedImage) {
  std::map<size_t, SegmentInfo> segments = createSegmentMap(segmentedImage);
  Logger::Debug << "Segments before clustering: " << segments.size();

  for (std::map<size_t, SegmentInfo>::iterator i = segments.begin();
       i != segments.end(); ++i) {
    SegmentInfo& info1 = i->second;
    for (std::map<size_t, SegmentInfo>::iterator j = segments.begin();
         j != segments.end(); ++j) {
      if (info1.segment != j->second.segment && !(i->second.mark) &&
          !(j->second.mark)) {
        SegmentInfo& info2 = j->second;
        const size_t hDist = info1.HorizontalDistance(info2);
        const size_t vDist = info1.VerticalDistance(info2);

        // The MERGE criteria
        bool cluster = false;
        // int minDist = hDist > vDist ? vDist : hDist;
        const int maxDist = hDist > vDist ? hDist : vDist;
        // int maxCount = info1.count > info2.count ? info1.count : info2.count;
        const int minCount =
            info1.count > info2.count ? info2.count : info1.count;
        const int maxWidth =
            info1.width > info2.width ? info1.width : info2.width;
        const int maxHeight =
            info1.height > info2.height ? info1.height : info2.height;
        const int minHeight =
            info1.height > info2.height ? info2.height : info1.height;
        // int lDist = abs((int) info1.left - (int) info2.left);
        // int rDist = abs((int) info1.right - (int) info2.right);
        // int tDist = abs((int) info1.top - (int) info2.top);
        // int bDist = abs((int) info1.bottom - (int) info2.bottom);
        const int widthDist = abs((int)info1.width - (int)info2.width);
        const int heightDist = abs((int)info1.height - (int)info2.height);
        // double x1Mean = (double) info1.xTotal / info1.count;
        // double x2Mean = (double) info2.xTotal / info2.count;
        // double xMeanDist = fabs(x1Mean - x2Mean);
        const double y1Mean = (double)info1.yTotal / info1.count;
        const double y2Mean = (double)info2.yTotal / info2.count;
        const double yMeanDist = fabs(y1Mean - y2Mean);

        bool remove1 = false, remove2 = false;

        // Cluster large segments with very small segments that are close
        // together (probably noise from the continuous transmitter)
        bool noiseH1 = maxDist <= 1 && info2.count > (info1.count * 20) &&
                       info2.width > info1.width * 8 && info1.height < 16 &&
                       info1.width < segmentedImage->Width() / 10,
             noiseH2 = maxDist <= 1 && info1.count > (info2.count * 20) &&
                       info1.width > info2.width * 8 && info2.height < 16 &&
                       info2.width < segmentedImage->Width() / 10;
        cluster = cluster || noiseH1 || noiseH2;
        remove1 = remove1 || noiseH1;
        remove2 = remove2 || noiseH2;

        bool noiseV1 = maxDist <= 1 && info2.count > (info1.count * 20) &&
                       info2.height > info1.height * 8 && info1.height < 16 &&
                       info1.width < segmentedImage->Width() / 10,
             noiseV2 = maxDist <= 1 && info1.count > (info2.count * 20) &&
                       info1.height > info2.height * 8 && info2.height < 16 &&
                       info2.width < segmentedImage->Width() / 10;
        cluster = cluster || noiseV1 || noiseV2;
        remove1 = remove1 || noiseV1;
        remove2 = remove2 || noiseV2;

        // Cluster same-shaped segments that are in the same channels
        cluster =
            cluster ||
            (vDist == 0 && yMeanDist * 8 <= (maxHeight + minHeight) &&
             widthDist <= (maxWidth / 4 + 2) &&
             heightDist <= (maxHeight / 4 + 2) && maxDist < minCount * 32);

        if (cluster) {
          const size_t oldSegment = info2.segment;
          segmentedImage->MergeSegments(info1.segment, oldSegment);
          for (std::map<size_t, SegmentInfo>::iterator i = segments.begin();
               i != segments.end(); ++i) {
            SegmentInfo& info = i->second;
            if (info.segment == oldSegment) info.segment = info1.segment;
          }
        }
        if (remove1) info1.mark = true;
        if (remove2) info2.mark = true;
      }
    }
  }
}

std::map<size_t, Morphology::SegmentInfo> Morphology::createSegmentMap(
    SegmentedImageCPtr segmentedImage) const {
  std::map<size_t, SegmentInfo> segments;
  for (size_t y = 0; y < segmentedImage->Height(); ++y) {
    for (size_t x = 0; x < segmentedImage->Width(); ++x) {
      const size_t segmentValue = segmentedImage->Value(x, y);
      if (segmentValue != 0) {
        if (segments.count(segmentValue) == 0) {
          SegmentInfo segment;
          segment.segment = segmentValue;
          segment.left = x;
          segment.right = x + 1;
          segment.top = y;
          segment.bottom = y + 1;
          segment.AddPoint(x, y);
          segments.insert(
              std::map<size_t, SegmentInfo>::value_type(segmentValue, segment));
        } else {
          SegmentInfo& segment = segments.find(segmentValue)->second;
          segment.AddPoint(x, y);
        }
      }
    }
  }

  for (std::map<size_t, SegmentInfo>::iterator i = segments.begin();
       i != segments.end(); ++i) {
    SegmentInfo& info = i->second;
    info.width = info.right - info.left;
    info.height = info.bottom - info.top;
  }
  return segments;
}

void Morphology::RemoveSmallSegments(SegmentedImagePtr segmentedImage,
                                     size_t thresholdLevel) {
  std::map<size_t, SegmentInfo> segments = createSegmentMap(segmentedImage);
  size_t removedSegments = 0;

  for (std::map<size_t, SegmentInfo>::iterator i = segments.begin();
       i != segments.end(); ++i) {
    const SegmentInfo& segment = i->second;
    if (segment.count <= thresholdLevel) {
      ++removedSegments;
      segmentedImage->RemoveSegment(segment.segment, segment.left,
                                    segment.right, segment.top, segment.bottom);
    }
  }
  Logger::Debug << "Removed " << removedSegments << " segments of size "
                << thresholdLevel << " or smaller.\n";
}

void Morphology::Classify(SegmentedImagePtr segmentedImage) {
  std::map<size_t, SegmentInfo> segments = createSegmentMap(segmentedImage);

  for (std::map<size_t, SegmentInfo>::iterator i = segments.begin();
       i != segments.end(); ++i) {
    const SegmentInfo& info = i->second;
    if (info.width > info.height * 10)
      segmentedImage->MergeSegments(LINE_SEGMENT, info.segment);
    else if (info.height > info.width * 10)
      segmentedImage->MergeSegments(BROADBAND_SEGMENT, info.segment);
    else
      segmentedImage->MergeSegments(BLOB_SEGMENT, info.segment);
  }
}

}  // namespace algorithms
