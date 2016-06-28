#include "rfiguicontroller.h"

#include "../../strategy/actions/strategy.h"

#include "../../strategy/algorithms/fringestoppingfitter.h"
#include "../../strategy/algorithms/mitigationtester.h"
#include "../../strategy/algorithms/svdmitigater.h"

#include "../../strategy/plots/rfiplots.h"
#include "../../strategy/control/defaultstrategy.h"

#include "../../strategy/imagesets/imageset.h"
#include "../../strategy/imagesets/msimageset.h"

#include "../../quality/histogramcollection.h"

#include "../../util/multiplot.h"

#include "../plot/plotmanager.h"

#include "../rfiguiwindow.h"

#include <gtkmm/messagedialog.h>

RFIGuiController::RFIGuiController(RFIGuiWindow& rfiGuiWindow, StrategyController* strategyController) :
	_rfiGuiWindow(rfiGuiWindow), _strategyController(strategyController)
{
	_plotManager = new class PlotManager();
}

RFIGuiController::~RFIGuiController()
{
	delete _plotManager;
}

bool RFIGuiController::IsImageLoaded() const
{
	return _rfiGuiWindow.GetTimeFrequencyWidget().HasImage();
}

TimeFrequencyData RFIGuiController::ActiveData() const
{
	return _rfiGuiWindow.GetTimeFrequencyWidget().GetActiveData();
}

TimeFrequencyData RFIGuiController::OriginalData() const
{
	return _rfiGuiWindow.GetTimeFrequencyWidget().OriginalData();
}

TimeFrequencyData RFIGuiController::RevisedData() const
{
	return _rfiGuiWindow.GetTimeFrequencyWidget().RevisedData();
}

TimeFrequencyData RFIGuiController::ContaminatedData() const
{
	return _rfiGuiWindow.GetTimeFrequencyWidget().ContaminatedData();
}

TimeFrequencyMetaDataCPtr RFIGuiController::SelectedMetaData() const
{
	return _rfiGuiWindow.GetTimeFrequencyWidget().GetSelectedMetaData();
}

void RFIGuiController::plotMeanSpectrum(bool weight)
{
	if(IsImageLoaded())
	{
		std::string title = weight ? "Sum spectrum" : "Mean spectrum";
		Plot2D &plot = _plotManager->NewPlot2D(title);

		TimeFrequencyData data = ActiveData();
		Mask2DCPtr mask =
			Mask2D::CreateSetMaskPtr<false>(data.ImageWidth(), data.ImageHeight());
		Plot2DPointSet &beforeSet = plot.StartLine("Without flagging");
		if(weight)
			RFIPlots::MakeMeanSpectrumPlot<true>(beforeSet, data, mask, SelectedMetaData());
		else
			RFIPlots::MakeMeanSpectrumPlot<false>(beforeSet, data, mask, SelectedMetaData());

		mask = Mask2D::CreateCopy(data.GetSingleMask());
		if(!mask->AllFalse())
		{
			Plot2DPointSet &afterSet = plot.StartLine("Flagged");
			if(weight)
				RFIPlots::MakeMeanSpectrumPlot<true>(afterSet, data, mask, SelectedMetaData());
			else
				RFIPlots::MakeMeanSpectrumPlot<false>(afterSet, data, mask, SelectedMetaData());
		}
		
		_plotManager->Update();
	}
}

void RFIGuiController::PlotDist()
{
	if(IsImageLoaded())
	{
		Plot2D &plot = _plotManager->NewPlot2D("Distribution");

		TimeFrequencyData activeData = ActiveData();
		Image2DCPtr image = activeData.GetSingleImage();
		Mask2DPtr mask =
			Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
		Plot2DPointSet &totalSet = plot.StartLine("Total");
		RFIPlots::MakeDistPlot(totalSet, image, mask);

		Plot2DPointSet &uncontaminatedSet = plot.StartLine("Uncontaminated");
		mask = Mask2D::CreateCopy(activeData.GetSingleMask());
		RFIPlots::MakeDistPlot(uncontaminatedSet, image, mask);

		mask->Invert();
		Plot2DPointSet &rfiSet = plot.StartLine("RFI");
		RFIPlots::MakeDistPlot(rfiSet, image, mask);

		_plotManager->Update();
	}
}

void RFIGuiController::PlotLogLogDist()
{
	if(IsImageLoaded())
	{
		TimeFrequencyData activeData = ActiveData();
		HistogramCollection histograms(activeData.PolarisationCount());
		for(unsigned p=0;p!=activeData.PolarisationCount();++p)
		{
			TimeFrequencyData *polData = activeData.CreateTFDataFromPolarisationIndex(p);
			Image2DCPtr image = polData->GetSingleImage();
			Mask2DCPtr mask = Mask2D::CreateCopy(polData->GetSingleMask());
			histograms.Add(0, 1, p, image, mask);
		}
		_rfiGuiWindow.ShowHistogram(histograms);
	}
}

void RFIGuiController::PlotPowerSpectrum()
{
	if(IsImageLoaded())
	{
		Plot2D &plot = _plotManager->NewPlot2D("Power spectrum");
		plot.SetLogarithmicYAxis(true);

		TimeFrequencyData data = ActiveData();
		Image2DCPtr image = data.GetSingleImage();
		Mask2DPtr mask =
			Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
		Plot2DPointSet &beforeSet = plot.StartLine("Before");
		RFIPlots::MakePowerSpectrumPlot(beforeSet, image, mask, SelectedMetaData());

		mask = Mask2D::CreateCopy(data.GetSingleMask());
		if(!mask->AllFalse())
		{
			Plot2DPointSet &afterSet = plot.StartLine("After");
			RFIPlots::MakePowerSpectrumPlot(afterSet, image, mask, SelectedMetaData());
		}
		
		_plotManager->Update();
	}
}

void RFIGuiController::PlotPowerSpectrumComparison()
{
	if(IsImageLoaded())
	{
		Plot2D &plot = _plotManager->NewPlot2D("Power spectrum comparison");

		TimeFrequencyData data = OriginalData();
		Image2DCPtr image = data.GetSingleImage();
		Mask2DCPtr mask = data.GetSingleMask();
		Plot2DPointSet &originalSet = plot.StartLine("Original");
		RFIPlots::MakePowerSpectrumPlot(originalSet, image, mask, SelectedMetaData());

		data = ContaminatedData();
		image = data.GetSingleImage();
		mask = data.GetSingleMask();
		Plot2DPointSet &alternativeSet = plot.StartLine("Alternative");
		RFIPlots::MakePowerSpectrumPlot(alternativeSet, image, mask, SelectedMetaData());
	
		_plotManager->Update();
	}
}

void RFIGuiController::PlotPowerRMS()
{
	if(IsImageLoaded())
	{
		Plot2D &plot = _plotManager->NewPlot2D("Spectrum RMS");
		plot.SetLogarithmicYAxis(true);

		TimeFrequencyData activeData = ActiveData();
		Image2DCPtr image = activeData.GetSingleImage();
		Mask2DPtr mask =
			Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
		Plot2DPointSet &beforeSet = plot.StartLine("Before");
		RFIPlots::MakeRMSSpectrumPlot(beforeSet, image, mask);

		mask = Mask2D::CreateCopy(activeData.GetSingleMask());
		if(!mask->AllFalse())
		{
			Plot2DPointSet &afterSet = plot.StartLine("After");
			RFIPlots::MakeRMSSpectrumPlot(afterSet, image, mask);
	
			//mask->Invert();
			//Plot2DPointSet &rfiSet = plot.StartLine("RFI");
			//RFIPlots::MakeRMSSpectrumPlot(rfiSet, _timeFrequencyWidget.Image(), mask);
		}

		_plotManager->Update();
	}
}

void RFIGuiController::PlotPowerSNR()
{
	Image2DCPtr
		image = ActiveData().GetSingleImage(),
		model = RevisedData().GetSingleImage();
	if(IsImageLoaded())
	{
		Plot2D &plot = _plotManager->NewPlot2D("SNR spectrum");
		plot.SetLogarithmicYAxis(true);

		Mask2DPtr mask =
			Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
		Plot2DPointSet &totalPlot = plot.StartLine("Total");
		RFIPlots::MakeSNRSpectrumPlot(totalPlot, image, model, mask);

		mask = Mask2D::CreateCopy(ActiveData().GetSingleMask());
		if(!mask->AllFalse())
		{
			Plot2DPointSet &uncontaminatedPlot = plot.StartLine("Uncontaminated");
			RFIPlots::MakeSNRSpectrumPlot(uncontaminatedPlot, image, model, mask);
	
			mask->Invert();
			Plot2DPointSet &rfiPlot = plot.StartLine("RFI");
			RFIPlots::MakeSNRSpectrumPlot(rfiPlot, image, model, mask);
		}

		_plotManager->Update();
	}
}

void RFIGuiController::PlotPowerTime()
{
	if(IsImageLoaded())
	{
		Plot2D &plot = _plotManager->NewPlot2D("Power over time");
		plot.SetLogarithmicYAxis(true);

		TimeFrequencyData activeData = ActiveData();
		Image2DCPtr image = activeData.GetSingleImage();
		Mask2DPtr mask =
			Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
		Plot2DPointSet &totalPlot = plot.StartLine("Total");
		RFIPlots::MakePowerTimePlot(totalPlot, image, mask, SelectedMetaData());

		mask = Mask2D::CreateCopy(activeData.GetSingleMask());
		if(!mask->AllFalse())
		{
			Plot2DPointSet &uncontaminatedPlot = plot.StartLine("Uncontaminated");
			RFIPlots::MakePowerTimePlot(uncontaminatedPlot, image, mask, SelectedMetaData());
	
			mask->Invert();
			Plot2DPointSet &rfiPlot = plot.StartLine("RFI");
			RFIPlots::MakePowerTimePlot(rfiPlot, image, mask, SelectedMetaData());
		}

		_plotManager->Update();
	}
}

void RFIGuiController::PlotPowerTimeComparison()
{
	if(IsImageLoaded())
	{
		Plot2D &plot = _plotManager->NewPlot2D("Time comparison");

		TimeFrequencyData data = OriginalData();
		Mask2DCPtr mask = data.GetSingleMask();
		Image2DCPtr image = data.GetSingleImage();
		Plot2DPointSet &originalPlot = plot.StartLine("Original");
		RFIPlots::MakePowerTimePlot(originalPlot, image, mask, SelectedMetaData());

		data = ContaminatedData();
		mask = data.GetSingleMask();
		image = data.GetSingleImage();
		Plot2DPointSet &alternativePlot = plot.StartLine("Original");
		plot.StartLine("Alternative");
		RFIPlots::MakePowerTimePlot(alternativePlot, image, mask, SelectedMetaData());

		_plotManager->Update();
	}
}

void RFIGuiController::PlotTimeScatter()
{
	if(IsImageLoaded())
	{
		MultiPlot plot(_plotManager->NewPlot2D("Time scatter"), 4);
		RFIPlots::MakeScatterPlot(plot, ActiveData(), SelectedMetaData());
		plot.Finish();
		_plotManager->Update();
	}
}

void RFIGuiController::PlotTimeScatterComparison()
{
	if(IsImageLoaded())
	{
		MultiPlot plot(_plotManager->NewPlot2D("Time scatter comparison"), 8);
		RFIPlots::MakeScatterPlot(plot, OriginalData(), SelectedMetaData(), 0);
		RFIPlots::MakeScatterPlot(plot, ContaminatedData(), SelectedMetaData(), 4);
		plot.Finish();
		_plotManager->Update();
	}
}

void RFIGuiController::PlotSingularValues()
{
	if(IsImageLoaded())
	{
		Plot2D &plot = _plotManager->NewPlot2D("Singular values");

		SVDMitigater::CreateSingularValueGraph(ActiveData(), plot);
		_plotManager->Update();
	}
}

void RFIGuiController::Open(const std::string& filename, BaselineIOMode ioMode, bool readUVW, const std::string& dataColumn, bool subtractModel, size_t polCountToRead, bool loadBaseline, bool loadStrategy)
{
	std::cout << "Opening " << filename << std::endl;
	try
	{
		rfiStrategy::ImageSet *imageSet = rfiStrategy::ImageSet::Create(filename, ioMode);
		if(dynamic_cast<rfiStrategy::MSImageSet*>(imageSet) != 0)
		{
			rfiStrategy::MSImageSet *msImageSet = static_cast<rfiStrategy::MSImageSet*>(imageSet);
			msImageSet->SetSubtractModel(subtractModel);
			msImageSet->SetDataColumnName(dataColumn);
	
			if(polCountToRead == 1)
				msImageSet->SetReadStokesI();
			else if(polCountToRead == 2)
				msImageSet->SetReadDipoleAutoPolarisations();
			else
				msImageSet->SetReadAllPolarisations();
	
			msImageSet->SetReadUVW(readUVW);
		}
		imageSet->Initialize();
		
		if(loadStrategy)
		{
			rfiStrategy::DefaultStrategy::TelescopeId telescopeId;
			unsigned flags;
			double frequency, timeResolution, frequencyResolution;
			rfiStrategy::DefaultStrategy::DetermineSettings(*imageSet, telescopeId, flags, frequency, timeResolution, frequencyResolution);
			_strategyController->Strategy().RemoveAll();
			rfiStrategy::DefaultStrategy::LoadStrategy(
				_strategyController->Strategy(),
				telescopeId,
				flags | rfiStrategy::DefaultStrategy::FLAG_GUI_FRIENDLY,
				frequency,
				timeResolution,
				frequencyResolution
			);
			_strategyController->NotifyChange();
		}
	
		_rfiGuiWindow.SetImageSet(imageSet, loadBaseline);
	} catch(std::exception &e)
	{
		Gtk::MessageDialog dialog(_rfiGuiWindow, e.what(), false, Gtk::MESSAGE_ERROR);
		dialog.run();
	}
}

void RFIGuiController::OpenTestSet(unsigned index, bool gaussianTestSets)
{
	unsigned width = 1024*16, height = 1024;
	if(IsImageLoaded())
	{
		TimeFrequencyData activeData = ActiveData();
		width = activeData.ImageWidth();
		height = activeData.ImageHeight();
	}
	Mask2DPtr rfi = Mask2D::CreateSetMaskPtr<false>(width, height);
	Image2DPtr testSetReal(MitigationTester::CreateTestSet(index, rfi, width, height, gaussianTestSets));
	Image2DPtr testSetImaginary(MitigationTester::CreateTestSet(2, rfi, width, height, gaussianTestSets));
	TimeFrequencyData data(SinglePolarisation, testSetReal, testSetImaginary);
	data.SetGlobalMask(rfi);
	
	_rfiGuiWindow.GetTimeFrequencyWidget().SetNewData(data, SelectedMetaData());
	_rfiGuiWindow.GetTimeFrequencyWidget().Update();
}
