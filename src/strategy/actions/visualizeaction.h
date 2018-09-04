#ifndef STRATEGY_VISUALIZE_ACTION_H
#define STRATEGY_VISUALIZE_ACTION_H

#include "action.h"

namespace rfiStrategy {
	
class VisualizeAction : public Action {
public:
	enum Source { FromOriginal, FromRevised, FromContaminated };
	
	VisualizeAction() : _label("Intermediate"), _source(FromContaminated), _sortingIndex(0)
	{ }
	
	virtual std::string Description() final override
	{
		switch(_source)
		{
		default:
		case FromOriginal:
			return "Visualize original in UI";
		case FromRevised:
			return "Visualize fit in UI";
		case FromContaminated:
			return "Visualize residual in UI";
		}
	}
	
	virtual void Perform(class ArtifactSet& artifacts, class ProgressListener& listener) final override;
	
	virtual ActionType Type() const final override { return VisualizeActionType; }
	
	const std::string& Label() const { return _label; }
	void SetLabel(const std::string& label) { _label = label; }
	void SetLabel(std::string&& label) { _label = std::move(label); }
	
	enum Source Source() const { return _source; }
	void SetSource(enum Source source) { _source = source; }
	
	size_t SortingIndex() const { return _sortingIndex; }
	void SetSortingIndex(size_t index) { _sortingIndex = index; }
	
private:
	std::string _label;
	enum Source _source;
	size_t _sortingIndex;
};

}

#endif // STRATEGY_VISUALIZE_ACTION_H
