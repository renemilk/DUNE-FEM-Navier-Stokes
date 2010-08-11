#ifndef METADATA_HH
#define METADATA_HH

#include <dune/stokes/discretestokesmodelinterface.hh>
#include <dune/stokes/stokespass.hh>
#include <dune/navier/fractionaltimeprovider.hh>
#include <dune/navier/stokestraits.hh>
#include <dune/navier/exactsolution.hh>
#include <dune/navier/nonlinear/models.hh>
#include <dune/navier/oseen/oseenpass.hh>
#include <dune/fem/misc/mpimanager.hh>
#include <dune/stuff/datawriter.hh>
#include <dune/stuff/functions.hh>
#include <dune/stuff/customprojection.hh>
#include <dune/common/collectivecommunication.hh>
#include <cmath>

namespace Dune {
	namespace NavierStokes {
		template <	class CommunicatorImp,
					class GridPartImp,
					template < class > class AnalyticalForceImp,
					template < class > class AnalyticalDirichletDataImp,
					template < class,class > class ExactPressureImp,
					template < class,class > class ExactVelocityImp,
					int gridDim, int sigmaOrder, int velocityOrder = sigmaOrder, int pressureOrder = sigmaOrder >
		struct ThetaSchemeTraits {
			typedef ThetaSchemeTraits<CommunicatorImp,
										GridPartImp,
										AnalyticalForceImp,
										AnalyticalDirichletDataImp,
										ExactPressureImp,
										ExactVelocityImp,
										gridDim, sigmaOrder, velocityOrder , pressureOrder >
				ThisType;
			typedef GridPartImp
				GridPartType;
			typedef FractionalTimeProvider<CommunicatorImp>
				TimeProviderType;

			typedef StokesStep::DiscreteStokesModelTraits<
						TimeProviderType,
						GridPartType,
						AnalyticalForceImp,
						AnalyticalDirichletDataImp,
						gridDim,
						sigmaOrder,
						velocityOrder,
						pressureOrder >
					StokesModelTraits;
			typedef typename StokesModelTraits::PressureFunctionSpaceType
				PressureFunctionSpaceType;
			typedef typename StokesModelTraits::VelocityFunctionSpaceType
				VelocityFunctionSpaceType;
			typedef Dune::DiscreteStokesModelDefault< StokesModelTraits >
				StokesModelType;
			typedef typename StokesModelTraits::DiscreteStokesFunctionSpaceWrapperType
				DiscreteStokesFunctionSpaceWrapperType;

			typedef typename StokesModelTraits::DiscreteStokesFunctionWrapperType
				DiscreteStokesFunctionWrapperType;
			typedef typename StokesModelTraits::AnalyticalForceFunctionType
				AnalyticalForceType;
			typedef typename StokesModelTraits::AnalyticalForceAdapterType
				StokesAnalyticalForceAdapterType;
			typedef typename StokesModelTraits::AnalyticalDirichletDataType
				AnalyticalDirichletDataType;

			typedef Dune::StartPass< DiscreteStokesFunctionWrapperType, -1 >
				StokesStartPassType;
			typedef Dune::StokesPass< StokesModelType, StokesStartPassType, 0 >
				StokesPassType;

			typedef CommunicatorImp
				CommunicatorType;

			typedef ExactPressureImp< typename StokesModelTraits::PressureFunctionSpaceType,
									  TimeProviderType >
				ExactPressureType;
			typedef ExactVelocityImp< typename StokesModelTraits::VelocityFunctionSpaceType,
									  TimeProviderType >
				ExactVelocityType;

			typedef ExactSolution<ThisType>
				ExactSolutionType;

			typedef NonlinearStep::ForceAdapterFunction<	TimeProviderType,
															AnalyticalForceType,
															typename DiscreteStokesFunctionWrapperType::DiscreteVelocityFunctionType,
															typename DiscreteStokesFunctionWrapperType::DiscretePressureFunctionType,
															typename StokesModelTraits::DiscreteSigmaFunctionType>
				NonlinearForceAdapterFunctionType;
			typedef NonlinearStep::DiscreteStokesModelTraits<
						TimeProviderType,
						GridPartType,
						NonlinearForceAdapterFunctionType,
						typename ExactSolutionType::DiscreteVelocityFunctionType,
						AnalyticalDirichletDataImp,
						gridDim,
						sigmaOrder,
						velocityOrder,
						pressureOrder >
				OseenModelTraits;
		};

		template < class T1, class T2, class T3 >
		struct TupleSerializer {
			typedef Dune::Tuple<	const typename T1::DiscreteVelocityFunctionType*,
									const typename T1::DiscretePressureFunctionType*,
									const typename T2::DiscreteVelocityFunctionType*,
									const typename T2::DiscretePressureFunctionType*,
									const typename T3::DiscreteVelocityFunctionType*,
									const typename T3::DiscretePressureFunctionType* >
				TupleType;

			static TupleType& getTuple( T1& t1,
										T2& t2,
										T3& t3 )
			{
				static TupleType t( &(t1.discreteVelocity()),
									&(t1.discretePressure()),
									&(t2.discreteVelocity()),
									&(t2.discretePressure()),
									&(t3.discreteVelocity()),
									&(t3.discretePressure()));
				return t;
			}
		};

		template < class TraitsImp >
		class ThetaScheme {
			protected:
				typedef TraitsImp
					Traits;
				typedef typename Traits::CommunicatorType
					CommunicatorType;
				typedef typename Traits::ExactSolutionType
					ExactSolutionType;
				typedef TupleSerializer<	typename Traits::DiscreteStokesFunctionWrapperType,
											typename Traits::DiscreteStokesFunctionWrapperType,
											ExactSolutionType >
					TupleSerializerType;
				typedef typename TupleSerializerType::TupleType
					OutputTupleType;
				typedef TimeAwareDataWriter<	typename Traits::TimeProviderType,
												typename Traits::GridPartType::GridType,
												OutputTupleType >
					DataWriterType;
				typedef typename Traits::DiscreteStokesFunctionWrapperType::DiscreteVelocityFunctionType
					DiscreteVelocityFunctionType;
				typedef typename Traits::DiscreteStokesFunctionWrapperType::DiscretePressureFunctionType
					DiscretePressureFunctionType;

				typename Traits::GridPartType gridPart_;
				const double theta_;
				const double operator_weight_alpha_;
				const double operator_weight_beta_;
				CommunicatorType& communicator_;
				typename Traits::TimeProviderType timeprovider_;
				typename Traits::DiscreteStokesFunctionSpaceWrapperType functionSpaceWrapper_;
				typename Traits::DiscreteStokesFunctionWrapperType currentFunctions_;
				typename Traits::DiscreteStokesFunctionWrapperType nextFunctions_;
				typename Traits::DiscreteStokesFunctionWrapperType errorFunctions_;
				ExactSolutionType exactSolution_;
				DataWriterType dataWriter_;

			public:
				ThetaScheme( typename Traits::GridPartType gridPart,
							 const double theta = 1 - std::pow( 2.0, -1/2.0 ),
							 CommunicatorType comm = Dune::MPIManager::helper().getCommunicator()
						)
					: gridPart_( gridPart ),
					theta_(theta),
					operator_weight_alpha_( ( 1-2*theta_ ) / ( 1-theta_ ) ),
					operator_weight_beta_( 1 - operator_weight_alpha_ ),
					communicator_( comm ),
					timeprovider_( theta_,operator_weight_alpha_,operator_weight_beta_, communicator_ ),
					functionSpaceWrapper_( gridPart_ ),
					currentFunctions_(  "current_",
										functionSpaceWrapper_,
										gridPart_ ),
					nextFunctions_(  "next_",
									functionSpaceWrapper_,
									gridPart_ ),
					errorFunctions_(  "error_",
									functionSpaceWrapper_,
									gridPart_ ),
					exactSolution_( timeprovider_,
									gridPart_,
									functionSpaceWrapper_ ),
					dataWriter_( timeprovider_,
								 gridPart_.grid(),
								 TupleSerializerType::getTuple(
										 currentFunctions_,
										 errorFunctions_,
										 exactSolution_ )
								)
				{}

				void nextStep( const int step, RunInfo& info )
				{
					currentFunctions_.assign( nextFunctions_ );
					nextFunctions_.clear();

					//error calc
					exactSolution_.project();
					errorFunctions_.discretePressure().assign( exactSolution_.discretePressure() );
					errorFunctions_.discretePressure() -= currentFunctions_.discretePressure();
					errorFunctions_.discreteVelocity().assign( exactSolution_.discreteVelocity() );
					errorFunctions_.discreteVelocity() -= currentFunctions_.discreteVelocity();

					double meanPressure_exact = Stuff::integralAndVolume( exactSolution_.exactPressure(), currentFunctions_.discretePressure().space() ).first;
					double meanPressure_discrete = Stuff::integralAndVolume( currentFunctions_.discretePressure(), currentFunctions_.discretePressure().space() ).first;

					Dune::L2Norm< typename Traits::GridPartType > l2_Error( gridPart_ );

					const double l2_error_pressure_				= l2_Error.norm( errorFunctions_.discretePressure() );
					const double l2_error_velocity_				= l2_Error.norm( errorFunctions_.discreteVelocity() );
					const double relative_l2_error_pressure_	= l2_error_pressure_ / l2_Error.norm( exactSolution_.discretePressure() );
					const double relative_l2_error_velocity_	= l2_error_velocity_ / l2_Error.norm( exactSolution_.discreteVelocity() );
					std::vector<double> error_vector;
					error_vector.push_back( l2_error_velocity_ );
					error_vector.push_back( l2_error_pressure_ );


					Logger().Info().Resume();
					Logger().Info() << "L2-Error Pressure (abs|rel): " << std::setw(8) << l2_error_pressure_ << " | " << relative_l2_error_pressure_ << "\n"
									<< "L2-Error Velocity (abs|rel): " << std::setw(8) << l2_error_velocity_ << " | " << relative_l2_error_velocity_ << "\n"
									<< "Mean pressure (exact|discrete): " << meanPressure_exact << " | " << meanPressure_discrete << std::endl;
					const double max_l2_error = 1e4;
					if ( l2_error_velocity_ > max_l2_error )
						DUNE_THROW(MathError, "Aborted, L2 error above " << max_l2_error );
					//end error calc

					if (step == 3) {
						typedef Dune::StabilizationCoefficients::ValueType
							Pair;
						Dune::StabilizationCoefficients stabil_coeff = Dune::StabilizationCoefficients::getDefaultStabilizationCoefficients();

						info.codim0			= gridPart_.grid().size( 0 );
						info.grid_width		= Dune::GridWidth::calcGridWidth( gridPart_ );
						info.run_time		= profiler().GetTiming( "Timestep" );
						info.delta_t		= timeprovider_.deltaT();
						info.current_time	= timeprovider_.time();
						info.L2Errors		= error_vector;

						info.c11			= Pair( stabil_coeff.Power( "C11" ), stabil_coeff.Factor( "C11" ) );
						info.c12			= Pair( stabil_coeff.Power( "C12" ), stabil_coeff.Factor( "C12" ) );
						info.d11			= Pair( stabil_coeff.Power( "D11" ), stabil_coeff.Factor( "D11" ) );
						info.d12			= Pair( stabil_coeff.Power( "D12" ), stabil_coeff.Factor( "D12" ) );
						info.bfg			= Parameters().getParam( "do-bfg", true );
						info.gridname		= gridPart_.grid().name();
						info.refine_level	= Parameters().getParam( "minref", 0 );

						info.polorder_pressure	= Traits::StokesModelTraits::pressureSpaceOrder;
						info.polorder_sigma		= Traits::StokesModelTraits::sigmaSpaceOrder;
						info.polorder_velocity	= Traits::StokesModelTraits::velocitySpaceOrder;

						info.solver_accuracy		= Parameters().getParam( "absLimit", 1e-4 );
						info.inner_solver_accuracy	= Parameters().getParam( "inner_absLimit", 1e-4 );
						info.bfg_tau				= Parameters().getParam( "bfg-tau", 0.1 );

						info.problemIdentifier = TESTCASE_NAME;
					}

					std::cout << "current time (substep " << step << "): " << timeprovider_.subTime() << std::endl;

					dataWriter_.write();
					timeprovider_.nextFractional();
				}

				RunInfo stokesStep( const double viscosity,
								 const double stokes_viscosity,
								 const double quasi_stokes_alpha,
								 const double beta_qout_re )
				{
					Logger().Suspend( Logging::LogStream::default_suspend_priority + 1 );
					const typename Traits::AnalyticalForceType force ( viscosity,
																 currentFunctions_.discreteVelocity().space() );
					typename Traits::StokesAnalyticalForceAdapterType stokesForce( timeprovider_,
																				   currentFunctions_.discreteVelocity(),
																				   force,
																				   beta_qout_re,
																				   quasi_stokes_alpha );
					typename Traits::AnalyticalDirichletDataType stokesDirichletData =
							Traits::StokesModelTraits::AnalyticalDirichletDataTraitsImplementation
											::getInstance( timeprovider_,
														   functionSpaceWrapper_ );
					double meanGD
							= Stuff::boundaryIntegral( stokesDirichletData, currentFunctions_.discreteVelocity().space() );
					typename Traits::StokesModelType
							stokesModel( Dune::StabilizationCoefficients::getDefaultStabilizationCoefficients() ,
										stokesForce,
										stokesDirichletData,
										stokes_viscosity,
										quasi_stokes_alpha );
					typename Traits::StokesStartPassType stokesStartPass;
					typename Traits::StokesPassType stokesPass( stokesStartPass,
											stokesModel,
											gridPart_,
											functionSpaceWrapper_ );
					stokesPass.apply( currentFunctions_, nextFunctions_ );
					RunInfo info;
					stokesPass.getRuninfo( info );
					Logger().Resume( Logging::LogStream::default_suspend_priority + 1 );
					Logger().Info() << "Dirichlet boundary integral " << meanGD << std::endl;
					return info;
				}

				RunInfoVector run()
				{
					RunInfoVector runInfoVector;

					//initial flow field at t = 0
					currentFunctions_.assign( exactSolution_ );

					typename Traits::DiscreteStokesFunctionWrapperType::DiscreteVelocityFunctionType::RangeType meanVelocity
							= Stuff::meanValue( currentFunctions_.discreteVelocity(), currentFunctions_.discreteVelocity().space() );
					const double typicalVelocity = meanVelocity.two_norm();
					Stuff::printFieldVector( meanVelocity, "meanVelocity", Logger().Info() );
					Logger().Info() << "typicalVelocity " << typicalVelocity << std::endl;
					const double typicalLength = 1.0;

					const double grid_width = Dune::GridWidth::calcGridWidth( gridPart_ );
					double dt_new = grid_width * grid_width * 2;
					//constants
					const double viscosity				= Parameters().getParam( "viscosity", 1.0 );
//					const double viscosity				= 1 / ( 3 * M_PI);
					const double d_t					= timeprovider_.deltaT();
					const double quasi_stokes_alpha		= 1 / ( theta_ * d_t );
//					const double reynolds				= typicalVelocity * typicalLength / viscosity;
					const double reynolds				= 1.0 / viscosity;
					const double stokes_viscosity		= operator_weight_alpha_ / reynolds;
					const double beta_qout_re			= operator_weight_beta_ / reynolds;
					const int verbose					= 1;

					for( timeprovider_.init( d_t ); timeprovider_.time() < timeprovider_.endTime(); )
					{
						RunInfo info_dummy;
						profiler().StartTiming( "Timestep" );
						std::cout << "current time (substep " << 0 << "): " << timeprovider_.subTime() << std::endl;
						//stokes step A
						if( Parameters().getParam( "enable_stokesA", true ) )
							stokesStep( viscosity, stokes_viscosity, quasi_stokes_alpha, beta_qout_re );
						else {
							exactSolution_.project();
							nextFunctions_.assign( exactSolution_ );
						}
						nextStep( 1, info_dummy );
						//Nonlinear step
						if( Parameters().getParam( "enable_oseen", true ) )
							oseenStep( viscosity, d_t, reynolds );
						else {
							exactSolution_.project();
							nextFunctions_.assign( exactSolution_ );
						}

						nextStep( 2, info_dummy );
						//stokes step B
						RunInfo info;
						if( Parameters().getParam( "enable_stokesB", true ) )
							info = stokesStep( viscosity, stokes_viscosity, quasi_stokes_alpha, beta_qout_re  );
						else {
							exactSolution_.project();
							nextFunctions_.assign( exactSolution_ );
						}

						profiler().StopTiming( "Timestep" );
						nextStep( 3, info );
						runInfoVector.push_back( info );
					}
					return runInfoVector;
				}

				void oseenStep( const double viscosity,
								const double d_t,
								const double reynolds )
				{
					const double oseen_alpha = 1 / ( ( 1 - 2 * theta_ ) * d_t );
					const double oseen_viscosity = operator_weight_beta_ / reynolds;

					const typename Traits::AnalyticalForceType force ( viscosity,
																 currentFunctions_.discreteVelocity().space() );

					typename Traits::NonlinearForceAdapterFunctionType nonlinearForce( timeprovider_,
																	  currentFunctions_.discreteVelocity(),
																	  currentFunctions_.discretePressure(),
																	  force,
																	  operator_weight_alpha_ / reynolds,
																	  oseen_alpha );

					typedef Dune::DiscreteStokesModelDefault< typename Traits::OseenModelTraits >
						OseenModelType;
					typedef NonlinearStep::OseenPass< OseenModelType,typename Traits::StokesStartPassType >
							OseenpassType;
					typename Traits::StokesStartPassType stokesStartPass;

					typename Traits::AnalyticalDirichletDataType stokesDirichletData =
							Traits::StokesModelTraits::AnalyticalDirichletDataTraitsImplementation
											::getInstance( timeprovider_,
														   functionSpaceWrapper_ );
					OseenModelType
							stokesModel( Dune::StabilizationCoefficients::getDefaultStabilizationCoefficients() ,
										nonlinearForce,
										stokesDirichletData,
										oseen_viscosity,
										oseen_alpha );
					OseenpassType oseenPass( stokesStartPass,
											stokesModel,
											gridPart_,
											functionSpaceWrapper_,
											currentFunctions_.discreteVelocity() );
					oseenPass.apply( currentFunctions_, nextFunctions_ );
				}

		};
	}//end namespace NavierStokes
}//end namespace Dune

#endif // METADATA_HH
