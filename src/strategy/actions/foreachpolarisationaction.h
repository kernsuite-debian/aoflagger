#ifndef RFI_FOREACHPOLARISATION_H
#define RFI_FOREACHPOLARISATION_H 

#include "action.h"

#include "../control/actionblock.h"
#include "../control/artifactset.h"

#include "../../structures/timefrequencydata.h"

namespace rfiStrategy {

	class ForEachPolarisationBlock : public ActionBlock
	{
		public:
			ForEachPolarisationBlock() :
				_onPP(true), _onPQ(true), _onQP(true), _onQQ(true),
				_onStokesI(false), _onStokesQ(false), _onStokesU(false), _onStokesV(false),
				_changeRevised(false)
			{
			}
			virtual ~ForEachPolarisationBlock()
			{
			}
			virtual std::string Description()
			{
				if(selectedPolarizationCount() == 1)
				{
					if(_onPP) return "On XX / RR";
					if(_onPQ) return "On XY / RL";
					if(_onQP) return "On YX / LR";
					if(_onQQ) return "On YY / LL";
					if(_onStokesI) return "On Stokes I";
					if(_onStokesQ) return "On Stokes Q";
					if(_onStokesU) return "On Stokes U";
					if(_onStokesV) return "On Stokes V";
				}
				return "For each polarisation";
			}
			
			virtual void Initialize()
			{ }
			
			virtual ActionType Type() const { return ForEachPolarisationBlockType; }
			
			virtual void Perform(ArtifactSet &artifacts, ProgressListener &progress)
			{
				const TimeFrequencyData
					contaminatedData = artifacts.ContaminatedData(),
					originalData = artifacts.OriginalData();
					
				if(contaminatedData.Polarisations() != originalData.Polarisations())
					throw BadUsageException("Contaminated and original do not have equal polarisation, in for each polarisation block");

				// Since Stokes parameters are (normally) not directly available but
				// need to be calculated from linear/circular polarizations, they are
				// treated differently: the flag mask will be applied to all polarizations,
				// while the image data is not changed.
				if(isStokesSelected())
				{
					performStokesIteration(artifacts, progress);
				}
				
				performDirectPolarizations(artifacts, progress);
			}

			void SetIterateStokesValues(bool iterateStokesValues)
			{
				_onStokesI = iterateStokesValues;
				_onStokesQ = iterateStokesValues;
				_onStokesU = iterateStokesValues;
				_onStokesV = iterateStokesValues;
			}
			
			bool IterateStokesValues() const
			{
				return _onStokesI && _onStokesQ && _onStokesU && _onStokesV;
			}
			
			void SetOnPP(bool onPP) { _onPP = onPP; }
			void SetOnPQ(bool onPQ) { _onPQ = onPQ; }
			void SetOnQP(bool onQP) { _onQP = onQP; }
			void SetOnQQ(bool onQQ) { _onQQ = onQQ; }
			void SetOnStokesI(bool onStokesI) { _onStokesI = onStokesI; }
			void SetOnStokesQ(bool onStokesQ) { _onStokesQ = onStokesQ; }
			void SetOnStokesU(bool onStokesU) { _onStokesU = onStokesU; }
			void SetOnStokesV(bool onStokesV) { _onStokesV = onStokesV; }
			
			bool OnPP() const { return _onPP; }
			bool OnPQ() const { return _onPQ; }
			bool OnQP() const { return _onQP; }
			bool OnQQ() const { return _onQQ; }
			bool OnStokesI() const { return _onStokesI; }
			bool OnStokesQ() const { return _onStokesQ; }
			bool OnStokesU() const { return _onStokesU; }
			bool OnStokesV() const { return _onStokesV; }
		private:
			bool _onPP, _onPQ, _onQP, _onQQ, _onStokesI, _onStokesQ, _onStokesU, _onStokesV;
			bool _changeRevised;
			
			bool isStokesSelected() const
			{
				return _onStokesI || _onStokesQ || _onStokesU || _onStokesV;
			}
			
			bool isDirectPolarizationSelected(PolarizationEnum polarization)
			{
				switch(polarization)
				{
					case Polarization::XX:
					case Polarization::RR:
						return _onPP;
					case Polarization::XY:
					case Polarization::RL:
						return _onPQ;
					case Polarization::YX:
					case Polarization::LR:
						return _onQP;
					case Polarization::YY:
					case Polarization::LL:
						return _onQQ;
					case Polarization::StokesI:
					case Polarization::StokesQ:
					case Polarization::StokesU:
					case Polarization::StokesV:
					case Polarization::Instrumental:
					default:
						return _onPP && _onPQ && _onQP && _onQQ;
				}
			}
			
			int selectedPolarizationCount() const
			{
				int count = 0;
				if(_onPP) ++count;
				if(_onPQ) ++count;
				if(_onQP) ++count;
				if(_onQQ) ++count;
				if(_onStokesI) ++count;
				if(_onStokesQ) ++count;
				if(_onStokesU) ++count;
				if(_onStokesV) ++count;
				return count;
			}

			void setPolarizationData(size_t polarizationIndex, TimeFrequencyData &oldData, TimeFrequencyData &newData)
			{
				oldData.SetPolarizationData(polarizationIndex, newData);
			}
			
			void performDirectPolarizations(ArtifactSet &artifacts, ProgressListener &progress)
			{
				TimeFrequencyData
					oldContaminatedData = artifacts.ContaminatedData(),
					oldRevisedData = artifacts.RevisedData(),
					oldOriginalData = artifacts.OriginalData();
					
				bool changeRevised = (oldRevisedData.Polarisations() == oldContaminatedData.Polarisations());
				unsigned count = oldContaminatedData.PolarisationCount();

				for(unsigned polarizationIndex = 0; polarizationIndex < count; ++polarizationIndex)
				{
					if(isDirectPolarizationSelected(oldContaminatedData.GetPolarisation(polarizationIndex)))
					{
						TimeFrequencyData *newContaminatedData =
							oldContaminatedData.CreateTFDataFromPolarisationIndex(polarizationIndex);
						TimeFrequencyData *newOriginalData =
							oldOriginalData.CreateTFDataFromPolarisationIndex(polarizationIndex);

						artifacts.SetContaminatedData(*newContaminatedData);
						artifacts.SetOriginalData(*newOriginalData);
		
						progress.OnStartTask(*this, polarizationIndex, count, newContaminatedData->Description());
		
						delete newOriginalData;
						delete newContaminatedData;
						
						if(changeRevised)
						{
							TimeFrequencyData *newRevised = oldRevisedData.CreateTFDataFromPolarisationIndex(polarizationIndex);
							artifacts.SetRevisedData(*newRevised);
							delete newRevised;
						}
		
						ActionBlock::Perform(artifacts, progress);

						setPolarizationData(polarizationIndex, oldContaminatedData, artifacts.ContaminatedData());
						setPolarizationData(polarizationIndex, oldOriginalData, artifacts.OriginalData());
						if(changeRevised && _changeRevised)
							setPolarizationData(polarizationIndex, oldRevisedData, artifacts.RevisedData());

						progress.OnEndTask(*this);
					}
				}

				artifacts.SetContaminatedData(oldContaminatedData);
				artifacts.SetRevisedData(oldRevisedData);
				artifacts.SetOriginalData(oldOriginalData);
			}

			void performStokesIteration(ArtifactSet &artifacts, ProgressListener &progress)
			{
				TimeFrequencyData
					oldContaminatedData = artifacts.ContaminatedData(),
					oldRevisedData = artifacts.RevisedData(),
					oldOriginalData = artifacts.OriginalData();

				bool changeRevised = (oldRevisedData.Polarisations() == oldContaminatedData.Polarisations());

				Mask2DPtr mask = Mask2D::CreateSetMaskPtr<false>(oldContaminatedData.ImageWidth(), oldContaminatedData.ImageHeight());

				if(_onStokesI)
				{
					performDerivedPolarisation(artifacts, progress, Polarization::StokesI, oldContaminatedData, oldOriginalData, oldRevisedData, changeRevised, 0, 4);
					mask->Join(artifacts.ContaminatedData().GetSingleMask());
				}

				if(_onStokesQ)
				{
					performDerivedPolarisation(artifacts, progress, Polarization::StokesQ, oldContaminatedData, oldOriginalData, oldRevisedData, changeRevised, 1, 4);
					mask->Join(artifacts.ContaminatedData().GetSingleMask());
				}

				if(_onStokesU)
				{
					performDerivedPolarisation(artifacts, progress, Polarization::StokesU, oldContaminatedData, oldOriginalData, oldRevisedData, changeRevised, 2, 4);
					mask->Join(artifacts.ContaminatedData().GetSingleMask());
				}

				if(_onStokesV)
				{
					performDerivedPolarisation(artifacts, progress, Polarization::StokesV, oldContaminatedData, oldOriginalData, oldRevisedData, changeRevised, 3, 4);
					mask->Join(artifacts.ContaminatedData().GetSingleMask());
				}
				
				oldContaminatedData.SetGlobalMask(mask);
				artifacts.SetContaminatedData(oldContaminatedData);
				artifacts.SetRevisedData(oldRevisedData);
				artifacts.SetOriginalData(oldOriginalData);
			}

			void performDerivedPolarisation(ArtifactSet &artifacts, ProgressListener &progress, PolarizationEnum polarisation, const TimeFrequencyData &oldContaminatedData, const TimeFrequencyData &oldOriginalData, const TimeFrequencyData &oldRevisedData, bool changeRevised, size_t taskNr, size_t taskCount)
			{
				TimeFrequencyData *newContaminatedData =
					oldContaminatedData.CreateTFData(polarisation);
				artifacts.SetContaminatedData(*newContaminatedData);
				progress.OnStartTask(*this, taskNr, taskCount, newContaminatedData->Description());
				delete newContaminatedData;

				TimeFrequencyData *newOriginalData =
					oldOriginalData.CreateTFData(polarisation);
				artifacts.SetOriginalData(*newOriginalData);
				delete newOriginalData;


				if(changeRevised)
				{
					TimeFrequencyData *newRevised = oldRevisedData.CreateTFData(polarisation);
					artifacts.SetRevisedData(*newRevised);
					delete newRevised;
				}

				ActionBlock::Perform(artifacts, progress);

				progress.OnEndTask(*this);
			}
	};

}

#endif // RFI_FOREACHPOLARISATION_H
