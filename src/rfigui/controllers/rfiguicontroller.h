#ifndef RFIGUI_CONTROLLER_H
#define RFIGUI_CONTROLLER_H

#include <sigc++/signal.h>

#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

#include "../../strategy/control/pythonstrategy.h"
#include "../../strategy/control/types.h"

#include "imagecomparisoncontroller.h"

#include <mutex>

class RFIGuiController
{
	public:
		RFIGuiController();
		~RFIGuiController();
		
		void AttachWindow(class RFIGuiWindow* rfiGuiWindow)
		{
			_rfiGuiWindow = rfiGuiWindow;
		}
		
		void AttachStrategyControl(class StrategyController* strategyController)
		{
			_strategyController = strategyController;
		}
		
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
				CheckPolarizations(true);
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
				CheckPolarizations(true);
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
				CheckPolarizations(true);
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
				CheckPolarizations(true);
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
		void PlotPowerTime();
		void PlotPowerTimeComparison();
		void PlotTimeScatter();
		void PlotTimeScatterComparison();
		void PlotSingularValues();
		
		void Open(const std::vector<std::string>& filenames, BaselineIOMode ioMode, bool readUVW, const std::string& dataColumn, bool subtractModel, size_t polCountToRead, bool loadStrategy, bool combineSPW);
		
		void OpenTestSet(unsigned index, bool gaussianTestSets);
		
		bool IsImageLoaded() const;
		
		TimeFrequencyData ActiveData() const;
		TimeFrequencyData OriginalData() const;
		
		TimeFrequencyMetaDataCPtr SelectedMetaData() const;
		
		class PlotManager &PlotManager() { return *_plotManager; }
		
		void ExecutePythonStrategy();
		
 		bool HasImageSet() const { return _imageSet != nullptr; }
 		
		void SetImageSet(std::unique_ptr<rfiStrategy::ImageSet> newImageSet);
		
		void SetImageSetIndex(std::unique_ptr<rfiStrategy::ImageSetIndex> newImageSetIndex);
		
		rfiStrategy::ImageSet &GetImageSet() const { return *_imageSet; }
		
		rfiStrategy::ImageSetIndex &GetImageSetIndex() const { return *_imageSetIndex; }
		
		void LoadCurrentTFData();
		
		std::mutex& IOMutex() { return _ioMutex; }
		
		void SetRevisedData(const TimeFrequencyData& data);
		
		ImageComparisonController& TFController()
		{ return _tfController; }
		
		void LoadSpatial(const std::string& filename);
		
		void LoadSpatialTime(const std::string& filename);
		
		void LoadPaths(const std::vector<std::string>& filenames);
		
		void CheckPolarizations(bool forceSignal = false);
		
		void GetAvailablePolarizations(bool& pp, bool& pq, bool& qp, bool& qq) const;
		
		void InterpolateFlagged();
		
	private:
		void plotMeanSpectrum(bool weight);
		
		bool _showOriginalFlags, _showAlternativeFlags;
		bool _showPP, _showPQ, _showQP, _showQQ;
		
		sigc::signal<void> _signalStateChange;
		class RFIGuiWindow* _rfiGuiWindow;
		class StrategyController* _strategyController;
		class ImageComparisonController _tfController;
		std::unique_ptr<class SpatialMatrixMetaData> _spatialMetaData;
		
		class PlotManager* _plotManager;
		PythonStrategy _pythonStrategy;
		std::unique_ptr<rfiStrategy::ImageSet> _imageSet;
		std::unique_ptr<rfiStrategy::ImageSetIndex> _imageSetIndex;
		std::mutex _ioMutex;
};

#endif
