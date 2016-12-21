#ifndef RFIGUI_CONTROLLER_H
#define RFIGUI_CONTROLLER_H

#include <sigc++/signal.h>

#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

class RFIGuiController
{
	public:
		RFIGuiController(class RFIGuiWindow &rfiGuiWindow, class StrategyController* strategyController);
		~RFIGuiController();
		
		bool AreOriginalFlagsShown() const { return _showOriginalFlags; }
		void SetShowOriginalFlags(bool showFlags) {
			if(_showOriginalFlags != showFlags)
			{
				_showOriginalFlags = showFlags;
				_signalStateChange();
			}
		}
		
		bool AreAlternativeFlagsShown() const { return _showAlternativeFlags; }
		void SetShowAlternativeFlags(bool showFlags) {
			if(_showAlternativeFlags != showFlags)
			{
				_showAlternativeFlags = showFlags;
				_signalStateChange();
			}
		}
		
		bool IsPPShown() const { return _showPP; }
		bool IsPQShown() const { return _showPQ; }
		bool IsQPShown() const { return _showQP; }
		bool IsQQShown() const { return _showQQ; }
		void SetShowPP(bool showPP)
		{
			if(showPP != _showPP)
			{
				if(showPP || _showPQ || _showQP || _showQQ)
				{
					_showPP = showPP;
					if(_showPP)
					{
						_showPQ = false;
						_showQP = false;
					}
				}
				_signalStateChange();
			}
		}
		void SetShowPQ(bool showPQ)
		{
			if(showPQ != _showPQ)
			{
				if(showPQ || _showPP || _showQP || _showQQ)
				{
					_showPQ = showPQ;
					if(_showPQ)
					{
						_showPP = false;
						_showQQ = false;
					}
				}
				_signalStateChange();
			}
		}
		void SetShowQP(bool showQP)
		{
			if(showQP != _showQP)
			{
				if(showQP || _showPP || _showPQ || _showQQ)
				{
					_showQP = showQP;
					if(_showQP)
					{
						_showPP = false;
						_showQQ = false;
					}
				}
				_signalStateChange();
			}
		}
		void SetShowQQ(bool showQQ)
		{
			if(showQQ != _showQQ)
			{
				if(showQQ || _showPP || _showPQ || _showQP)
				{
					_showQQ = showQQ;
					if(_showQQ)
					{
						_showPQ = false;
						_showQP = false;
					}
				}
				_signalStateChange();
			}
		}
		sigc::signal<void> &SignalStateChange()
		{ return _signalStateChange; }
		
		void PlotDist();
		void PlotLogLogDist();
		void PlotComplexPlane();
		void PlotMeanSpectrum() { plotMeanSpectrum(false); }
		void PlotSumSpectrum() { plotMeanSpectrum(true); }
		void PlotPowerSpectrum();
		void PlotPowerSpectrumComparison();
		void PlotFrequencyScatter();
		void PlotPowerRMS();
		void PlotPowerSNR();
		void PlotPowerTime();
		void PlotPowerTimeComparison();
		void PlotTimeScatter();
		void PlotTimeScatterComparison();
		void PlotSingularValues();
		
		void Open(const std::string& filename, BaselineIOMode ioMode, bool readUVW, const std::string& dataColumn, bool subtractModel, size_t polCountToRead, bool loadBaseline, bool loadStrategy);
		void OpenTestSet(unsigned index, bool gaussianTestSets);
		
		bool IsImageLoaded() const;
		
		TimeFrequencyData ActiveData() const;
		TimeFrequencyData OriginalData() const;
		TimeFrequencyData RevisedData() const;
		TimeFrequencyData ContaminatedData() const;
		
		TimeFrequencyMetaDataCPtr SelectedMetaData() const;
		
		class PlotManager &PlotManager() { return *_plotManager; }
		
	private:
		void plotMeanSpectrum(bool weight);
		
		bool _showOriginalFlags, _showAlternativeFlags;
		bool _showPP, _showPQ, _showQP, _showQQ;
		
		sigc::signal<void> _signalStateChange;
		class RFIGuiWindow &_rfiGuiWindow;
		class StrategyController* _strategyController;
		
		class PlotManager *_plotManager;
};

#endif
