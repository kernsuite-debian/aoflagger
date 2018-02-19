#ifndef SYSTEM_H
#define SYSTEM_H

#include <map>
#include <string>

#include "dimension.h"

class System {
	public:
		System() : _includeZeroYAxis(false)
		{
		}

		~System()
		{
			Clear();
		}
		
		bool Empty() const
		{
			return _dimensions.empty();
		}

		void AddToSystem(class Plot2DPointSet &pointSet)
		{
			Dimension *dimension;
			auto iter = _dimensions.find(pointSet.YUnits());
			if(iter == _dimensions.end())
			{
				dimension = &_dimensions.emplace(pointSet.YUnits(), Dimension()).first->second;
			} else {
				dimension = &iter->second;
			}
			dimension->AdjustRanges(pointSet);
		}

		double XRangeMin(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second.XRangeMin();
		}
		double XRangePositiveMin(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second.XRangePositiveMin();
		}
		double XRangeMax(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second.XRangeMax();
		}
		double XRangePositiveMax(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second.XRangePositiveMax();
		}
		double YRangeMin(class Plot2DPointSet &pointSet) const
		{
			const double yMin = _dimensions.find(pointSet.YUnits())->second.YRangeMin();
			if(yMin > 0.0 && _includeZeroYAxis)
				return 0.0;
			else
				return yMin;
		}
		double YRangePositiveMin(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second.YRangePositiveMin();
		}
		double YRangeMax(class Plot2DPointSet &pointSet) const
		{
			const double yMax = _dimensions.find(pointSet.YUnits())->second.YRangeMax();
			if(yMax < 0.0 && _includeZeroYAxis)
				return 0.0;
			else
				return yMax;
		}
		double YRangePositiveMax(class Plot2DPointSet &pointSet) const
		{
			return _dimensions.find(pointSet.YUnits())->second.YRangePositiveMax();
		}
		void Clear()
		{
			_dimensions.clear();
		}
		void SetIncludeZeroYAxis(bool includeZeroYAxis) { _includeZeroYAxis = includeZeroYAxis; }
	private:
		std::map<std::string, Dimension> _dimensions;
		bool _includeZeroYAxis;
};

#endif
