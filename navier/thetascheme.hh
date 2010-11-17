#ifndef METADATA_HH
#define METADATA_HH

#include <dune/stokes/discretestokesmodelinterface.hh>
#include <dune/stokes/stokespass.hh>
#include <dune/navier/fractionaltimeprovider.hh>
#include <dune/navier/stokestraits.hh>
#include <dune/navier/testdata.hh>
#include <dune/navier/exactsolution.hh>
#include <dune/fem/misc/mpimanager.hh>
#include <dune/stuff/datawriter.hh>
#include <dune/stuff/functions.hh>
#include <dune/stuff/tuple.hh>
#include <dune/stuff/customprojection.hh>
#include <dune/stuff/error.hh>
#include <dune/common/collectivecommunication.hh>
#include <cmath>
#include <boost/scoped_ptr.hpp>

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
															typename DiscreteStokesFunctionWrapperType::DiscreteVelocityFunctionType >
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
			typedef Dune::DiscreteStokesModelDefault< OseenModelTraits >
				OseenModelType;
			typedef Dune::StokesPass< OseenModelType,StokesStartPassType, 0 >
				OseenpassType;
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
				typedef Stuff::TupleSerializer<	typename Traits::DiscreteStokesFunctionWrapperType,
											typename Traits::DiscreteStokesFunctionWrapperType,
											ExactSolutionType,
											typename Traits::DiscreteStokesFunctionWrapperType>
					TupleSerializerType1;
				typedef typename TupleSerializerType1::TupleType
					OutputTupleType1;
				typedef TimeAwareDataWriter<	typename Traits::TimeProviderType,
												typename Traits::GridPartType::GridType,
												OutputTupleType1 >
					DataWriterType1;
				typedef Stuff::TupleSerializer<	typename Traits::DiscreteStokesFunctionWrapperType >
					TupleSerializerType2;
				typedef typename TupleSerializerType2::TupleType
					OutputTupleType2;
				typedef TimeAwareDataWriter<	typename Traits::TimeProviderType,
												typename Traits::GridPartType::GridType,
												OutputTupleType2 >
					DataWriterType2;
				typedef typename Traits::DiscreteStokesFunctionWrapperType::DiscreteVelocityFunctionType
					DiscreteVelocityFunctionType;
				typedef typename Traits::DiscreteStokesFunctionWrapperType::DiscretePressureFunctionType
					DiscretePressureFunctionType;

				mutable typename Traits::GridPartType gridPart_;
		public:
				const double theta_;
				const double operator_weight_alpha_;
				const double operator_weight_beta_;

				//! used cause we cannot use function calls and whatnot to define static constants
				struct Defaults {
					Defaults()
						: theta ( 1 - std::pow( 2.0, -1/2.0 ) ),
						operator_weight_alpha ( ( 1-2*theta ) / ( 1-theta ) ),
						operator_weight_beta ( 1 - operator_weight_alpha )
					{}
					const double theta;
					const double operator_weight_alpha;
					const double operator_weight_beta;
				};

		protected:
				CommunicatorType& communicator_;
				typename Traits::TimeProviderType timeprovider_;
				typename Traits::DiscreteStokesFunctionSpaceWrapperType functionSpaceWrapper_;
				mutable typename Traits::DiscreteStokesFunctionWrapperType currentFunctions_;
				mutable typename Traits::DiscreteStokesFunctionWrapperType nextFunctions_;
				typename Traits::DiscreteStokesFunctionWrapperType errorFunctions_;
				ExactSolutionType exactSolution_;
				mutable typename Traits::DiscreteStokesFunctionWrapperType dummyFunctions_;
				mutable typename Traits::DiscreteStokesFunctionWrapperType updateFunctions_;
				mutable typename Traits::DiscreteStokesFunctionWrapperType rhsFunctions_;
				DataWriterType1 dataWriter1_;
				DataWriterType2 dataWriter2_;
				const typename Traits::StokesPassType::DiscreteSigmaFunctionSpaceType sigma_space_;
				mutable typename Traits::StokesPassType::RhsDatacontainer rhsDatacontainer_;

			public:
				const double viscosity_;
				const double d_t_;
				const double reynolds_;
				const double beta_qout_re_;
				double current_max_gridwidth_;

			public:
				ThetaScheme( typename Traits::GridPartType gridPart,
							 const double theta				= Defaults().theta,
							 CommunicatorType comm			= Dune::MPIManager::helper().getCommunicator(),
							 double operator_weight_alpha	= Defaults().operator_weight_alpha,
							 double operator_weight_beta	= Defaults().operator_weight_beta
						)
					: gridPart_( gridPart ),
					theta_(theta),
					operator_weight_alpha_( operator_weight_alpha ),
					operator_weight_beta_( operator_weight_beta ),
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
					dummyFunctions_("dummy",
									functionSpaceWrapper_,
									gridPart_ ),
					updateFunctions_("updates",
									  functionSpaceWrapper_,
									  gridPart_ ),
					rhsFunctions_("rhs-adapter",
								 functionSpaceWrapper_,
								 gridPart_ ),
					dataWriter1_( timeprovider_,
								 gridPart_.grid(),
								 TupleSerializerType1::getTuple(
										 currentFunctions_,
										 errorFunctions_,
										 exactSolution_,
										 dummyFunctions_)
								),
					dataWriter2_( timeprovider_,
								 gridPart_.grid(),
								 TupleSerializerType2::getTuple(
										 updateFunctions_,
										 rhsFunctions_)
								),
					sigma_space_( gridPart_ ),
					rhsDatacontainer_( currentFunctions_.discreteVelocity().space(), sigma_space_ ),
					viscosity_( Parameters().getParam( "viscosity", 1.0 ) ),
					d_t_( timeprovider_.deltaT() ),
					reynolds_( 1.0 / viscosity_ ),
					beta_qout_re_( operator_weight_beta_ / reynolds_ ),
					current_max_gridwidth_( Dune::GridWidth::calcGridWidth( gridPart_ ) )
				{}

				void nextStep( const int step, RunInfo& info )
				{
					current_max_gridwidth_ = Dune::GridWidth::calcGridWidth( gridPart_ );
					currentFunctions_.assign( nextFunctions_ );
					exactSolution_.project();

					//error calc
					if ( Parameters().getParam( "calculate_errors", true ) ) {
						errorFunctions_.discretePressure().assign( exactSolution_.discretePressure() );
						errorFunctions_.discretePressure() -= currentFunctions_.discretePressure();
						errorFunctions_.discreteVelocity().assign( exactSolution_.discreteVelocity() );
						errorFunctions_.discreteVelocity() -= currentFunctions_.discreteVelocity();

						double meanPressure_exact = Stuff::integralAndVolume( exactSolution_.exactPressure(), currentFunctions_.discretePressure().space() ).first;
						double meanPressure_discrete = Stuff::integralAndVolume( currentFunctions_.discretePressure(), currentFunctions_.discretePressure().space() ).first;

						Dune::L2Norm< typename Traits::GridPartType > l2_Error( gridPart_ );

						if ( Parameters().getParam( "error_scaling", false ) ) {
								const double scale		= 1 / std::sqrt( viscosity_ );
								errorFunctions_.discretePressure() *= scale;
								errorFunctions_.discreteVelocity() *= scale;
						}

						const double l2_error_pressure_				= l2_Error.norm( errorFunctions_.discretePressure() );
						const double l2_error_velocity_				= l2_Error.norm( errorFunctions_.discreteVelocity() );
						const double relative_l2_error_pressure_	= l2_error_pressure_ / l2_Error.norm( exactSolution_.discretePressure() );
						const double relative_l2_error_velocity_	= l2_error_velocity_ / l2_Error.norm( exactSolution_.discreteVelocity() );
						std::vector<double> error_vector;
						error_vector.push_back( l2_error_velocity_ );
						error_vector.push_back( l2_error_pressure_ );


						Logger().Info().Resume();
						Logger().Info() << boost::format ("L2-Error Pressure (abs|rel): %e | %e \n") % l2_error_pressure_ % relative_l2_error_pressure_
										<< boost::format ("L2-Error Velocity (abs|rel): %e | %e \n") % l2_error_velocity_ % relative_l2_error_velocity_
										<< "Mean pressure (exact|discrete): " << meanPressure_exact << " | " << meanPressure_discrete << std::endl;
						const double max_l2_error = 1e4;
						if ( l2_error_velocity_ > max_l2_error )
							DUNE_THROW(MathError, "Aborted, L2 error above " << max_l2_error );
						info.L2Errors		= error_vector;
					}
					//end error calc

					if (step == 3) {
						typedef Dune::StabilizationCoefficients::ValueType
							Pair;
						Dune::StabilizationCoefficients stabil_coeff = Dune::StabilizationCoefficients::getDefaultStabilizationCoefficients();

						info.codim0			= gridPart_.grid().size( 0 );
						info.grid_width		= current_max_gridwidth_;
						info.run_time		= profiler().GetTiming( "Timestep" );
						info.delta_t		= timeprovider_.deltaT();
						info.current_time	= timeprovider_.time();
						info.viscosity		= viscosity_;
						info.reynolds		= reynolds_;

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

					Logger().Info() << boost::format("current time (substep %d ): %f\n") % step % timeprovider_.subTime();

					if ( step == 3 || !Parameters().getParam( "write_fulltimestep_only", false ) )
						writeData();
					timeprovider_.nextFractional();
				}

				RunInfo stokesStep() const
				{
					const bool scale_equations = Parameters().getParam( "scale_equations", false );
					const double delta_t_factor = theta_ * d_t_;
					double stokes_alpha,scale_factor,stokes_viscosity;
					const double stokes_alpha_unscaled = 1 / delta_t_factor;
					if ( scale_equations ) {
						stokes_alpha = 1;
						scale_factor = delta_t_factor;
						stokes_viscosity = operator_weight_alpha_ * delta_t_factor / reynolds_;
					}
					else {
						stokes_alpha = stokes_alpha_unscaled;
						scale_factor = 1;
						stokes_viscosity = operator_weight_alpha_ / reynolds_;
					}

					if ( Parameters().getParam( "silent_stokes", true ) )
						Logger().Suspend( Logging::LogStream::default_suspend_priority + 1 );

					const bool first_stokes_step = timeprovider_.timeStep() <= 1;
					const typename Traits::AnalyticalForceType force ( viscosity_,
																 currentFunctions_.discreteVelocity().space() );

					boost::scoped_ptr< typename Traits::StokesAnalyticalForceAdapterType >
							ptr_stokesForce_vanilla ( first_stokes_step
												? new typename Traits::StokesAnalyticalForceAdapterType ( timeprovider_,
																										  currentFunctions_.discreteVelocity(),
																										  force,
																										  beta_qout_re_,
																										  stokes_alpha_unscaled )
												: new typename Traits::StokesAnalyticalForceAdapterType ( timeprovider_,
																										  currentFunctions_.discreteVelocity(),
																										  force,
																										  beta_qout_re_,
																										  stokes_alpha_unscaled,
																										  rhsDatacontainer_ )
											);

					typedef Stuff::L2Error<typename Traits::GridPartType>
							L2ErrorType;
					L2ErrorType l2Error( gridPart_ );

					// CHEAT (projecting the anaylitcal evals into the container filled by last pass
					const bool do_cheat = Parameters().getParam( "rhs_cheat", false ) && !first_stokes_step ;
//					if ( do_cheat ) //do cheat rhs assembly unconditionally, below we'll choose according to do_cheat which rhs to put into the model
					{
						typedef typename DiscreteVelocityFunctionType::FunctionSpaceType::FunctionSpaceType
							VelocityFunctionSpaceType;
						VelocityFunctionSpaceType continousVelocitySpace_;
						typedef TESTING_NS::VelocityConvection<	VelocityFunctionSpaceType,
																typename Traits::TimeProviderType >
							VelocityConvection;
						VelocityConvection velocity_convection( timeprovider_, continousVelocitySpace_ );
						Dune::BetterL2Projection //we need evals from the _previous_ (t_0) step
							::project( timeprovider_.previousSubTime(), velocity_convection, rhsDatacontainer_.convection );
//						// ----
						typedef TESTING_NS::VelocityLaplace<	VelocityFunctionSpaceType,
																					typename Traits::TimeProviderType >
								VelocityLaplace;
						VelocityLaplace velocity_laplace( timeprovider_, continousVelocitySpace_ );
						Dune::BetterL2Projection //this seems currently inconsequential to the produced error
							::project( timeprovider_.previousSubTime(), velocity_laplace, rhsDatacontainer_.velocity_laplace );

//						typename L2ErrorType::Errors errors_convection = l2Error.get(	exactSolution_.discreteVelocity() ,
//																			currentFunctions_.discreteVelocity(),
//																			dummyFunctions_.discreteVelocity() );
//						std::cerr << "BLAH " << errors_convection.str();

						currentFunctions_.discreteVelocity().assign( exactSolution_.discreteVelocity() );
					}// END CHEAT

					boost::scoped_ptr< typename Traits::StokesAnalyticalForceAdapterType >
							ptr_stokesForce ( first_stokes_step
												? new typename Traits::StokesAnalyticalForceAdapterType ( timeprovider_,
																										  currentFunctions_.discreteVelocity(),
																										  force,
																										  beta_qout_re_,
																										  stokes_alpha_unscaled )
												: new typename Traits::StokesAnalyticalForceAdapterType ( timeprovider_,
																										  currentFunctions_.discreteVelocity(),
																										  force,
																										  beta_qout_re_,
																										  stokes_alpha_unscaled,
																										  rhsDatacontainer_ )
											);
					*ptr_stokesForce *= scale_factor;
					*ptr_stokesForce_vanilla *= scale_factor;
					rhsFunctions_.discreteVelocity().assign( do_cheat ? *ptr_stokesForce : *ptr_stokesForce_vanilla );

					typename L2ErrorType::Errors errors_rhs = l2Error.get(	static_cast<typename Traits::StokesAnalyticalForceAdapterType::BaseType>(*ptr_stokesForce),
																		static_cast<typename Traits::StokesAnalyticalForceAdapterType::BaseType>(*ptr_stokesForce_vanilla),
																		dummyFunctions_.discreteVelocity() );
					std::cerr << "RHS " << errors_rhs.str();

					Dune::StabilizationCoefficients stab_coeff = Dune::StabilizationCoefficients::getDefaultStabilizationCoefficients();

					if ( Parameters().getParam( "stab_coeff_visc_scale", true ) ) {
						stab_coeff.Factor( "D11", ( 1 / stokes_viscosity ) );
						stab_coeff.Factor( "C11", stokes_viscosity );
					}
					else {
						stab_coeff.FactorFromParams("D11");
						stab_coeff.FactorFromParams("C11");
					}
					stab_coeff.FactorFromParams("D12");
					stab_coeff.FactorFromParams("C12");
					stab_coeff.Add( "E12", 0.5 );
					stab_coeff.print( Logger().Info() );

					typename Traits::AnalyticalDirichletDataType stokesDirichletData =
							Traits::StokesModelTraits::AnalyticalDirichletDataTraitsImplementation
											::getInstance( timeprovider_,
														   functionSpaceWrapper_ );
					double meanGD
							= Stuff::boundaryIntegral( stokesDirichletData, currentFunctions_.discreteVelocity().space() );
					Logger().Info() << boost::format("Dirichlet boundary integral %e") % meanGD
									<< std::endl;

					typename Traits::StokesModelType
							stokesModel(stab_coeff,
										do_cheat ? *ptr_stokesForce : *ptr_stokesForce_vanilla,
										stokesDirichletData,
										stokes_viscosity ,
										stokes_alpha, scale_factor, scale_factor );
					typename Traits::StokesStartPassType stokesStartPass;
					typename Traits::StokesPassType stokesPass( stokesStartPass,
											stokesModel,
											gridPart_,
											functionSpaceWrapper_,
											currentFunctions_.discreteVelocity(),
											false );
					typename Traits::StokesModelTraits::SigmaFunctionSpaceType
							continousVelocityGradientSpace_;
					typedef TESTING_NS::VelocityGradient<	typename Traits::StokesModelTraits::SigmaFunctionSpaceType,
															typename Traits::TimeProviderType >
						VelocityGradient;
					VelocityGradient velocity_gradient( timeprovider_, continousVelocityGradientSpace_ );

					stokesPass.apply( currentFunctions_, nextFunctions_, &rhsDatacontainer_ );
					setUpdateFunctions();
					RunInfo info;
					stokesPass.getRuninfo( info );
					if ( Parameters().getParam( "silent_stokes", true ) )
						Logger().Resume( Logging::LogStream::default_suspend_priority + 1 );
					return info;
				}

				void Init()
				{
//					typename Traits::DiscreteStokesFunctionWrapperType::DiscreteVelocityFunctionType::RangeType meanVelocity
//							= Stuff::meanValue( currentFunctions_.discreteVelocity(), currentFunctions_.discreteVelocity().space() );
//					const double typicalVelocity = meanVelocity.two_norm();
//					Stuff::printFieldVector( meanVelocity, "meanVelocity", Logger().Info() );
//					Logger().Info() << "typicalVelocity " << typicalVelocity << std::endl;

					timeprovider_.init( d_t_ );
					//initial flow field at t = 0
					exactSolution_.project();
					currentFunctions_.assign( exactSolution_ );
					nextFunctions_.assign( exactSolution_ );
					writeData();
					//set current time to t_0 + theta
					timeprovider_.nextFractional();
				}

				RunInfoVector run()
				{
					RunInfoVector runInfoVector;
					Init();

					for( ;timeprovider_.time() < timeprovider_.endTime(); )
					{
						RunInfo info_dummy;
						profiler().StartTiming( "Timestep" );
						//stokes step A
						stokesStep();
						nextStep( 1, info_dummy );

						//Nonlinear step
						oseenStep();
						nextStep( 2, info_dummy );

						//stokes step B
						RunInfo info;
						info = stokesStep();
						profiler().StopTiming( "Timestep" );
						nextStep( 3, info );

						runInfoVector.push_back( info );
					}
					return runInfoVector;
				}

				void oseenStep()
				{
					const bool scale_equations = Parameters().getParam( "scale_equations", false );
					const double delta_t_factor = ( 1. - 2. * theta_ ) * d_t_;
					double oseen_alpha, oseen_viscosity,scale_factor;
					const double oseen_alpha_unscaled = 1 / delta_t_factor;
					if ( scale_equations ) {
						oseen_alpha = 1;
						scale_factor = delta_t_factor;
						oseen_viscosity = beta_qout_re_ * scale_factor;
					}
					else {
						oseen_alpha = oseen_alpha_unscaled;
						oseen_viscosity = beta_qout_re_;
						scale_factor = 1;
					}

					const typename Traits::AnalyticalForceType force ( viscosity_,
																 currentFunctions_.discreteVelocity().space() );

					// CHEAT (projecting the anaylitcal evals into the container filled by last pass
					if ( Parameters().getParam( "rhs_cheat", false ) ) {
						typedef typename DiscreteVelocityFunctionType::FunctionSpaceType::FunctionSpaceType
							VelocityFunctionSpaceType;
						VelocityFunctionSpaceType continousVelocitySpace_;

						typedef TESTING_NS::PressureGradient<	VelocityFunctionSpaceType,
																typename Traits::TimeProviderType >
							PressureGradient;
						PressureGradient pressure_gradient( timeprovider_, continousVelocitySpace_ );
						Dune::BetterL2Projection //we need evals from the _previous_ (t_0) step
							::project( timeprovider_.previousSubTime(), pressure_gradient, rhsDatacontainer_.pressure_gradient );
						// ----
						typedef TESTING_NS::VelocityLaplace<	VelocityFunctionSpaceType,
																					typename Traits::TimeProviderType >
								VelocityLaplace;
						VelocityLaplace velocity_laplace( timeprovider_, continousVelocitySpace_ );
						Dune::BetterL2Projection
							::project( timeprovider_.previousSubTime(), velocity_laplace, rhsDatacontainer_.velocity_laplace );
						currentFunctions_.discreteVelocity().assign( exactSolution_.discreteVelocity() );
					}// END CHEAT

					typename Traits::NonlinearForceAdapterFunctionType nonlinearForce( timeprovider_,
																	  currentFunctions_.discreteVelocity(),
																	  force,
																	  operator_weight_alpha_ / reynolds_,
																	  oseen_alpha_unscaled,
																	  rhsDatacontainer_ );
					nonlinearForce *= scale_factor;
					rhsFunctions_.discreteVelocity().assign( nonlinearForce );
					unsigned int oseen_iterations = Parameters().getParam( "oseen_iterations", (unsigned int)(1) );
					assert( oseen_iterations > 0 );
					for( unsigned int i = 0; i<oseen_iterations; ++i )
					{
						oseenStepSingle( nonlinearForce, oseen_viscosity, oseen_alpha, scale_factor );
						setUpdateFunctions();
						currentFunctions_.assign( nextFunctions_ );
					}
					dummyFunctions_.discreteVelocity().assign( nextFunctions_.discreteVelocity() );
					dummyFunctions_.discreteVelocity() -= exactSolution_.discreteVelocity();
				}

				void oseenStepSingle(	const typename Traits::NonlinearForceAdapterFunctionType& nonlinearForce,
										const double oseen_viscosity,
										const double oseen_alpha,
										const double scale_factor )
				{
					typename Traits::StokesStartPassType stokesStartPass;

					typename Traits::AnalyticalDirichletDataType stokesDirichletData =
							Traits::StokesModelTraits::AnalyticalDirichletDataTraitsImplementation
											::getInstance( timeprovider_,
														   functionSpaceWrapper_ );
					Dune::StabilizationCoefficients stab_coeff = Dune::StabilizationCoefficients::getDefaultStabilizationCoefficients();
					if ( Parameters().getParam( "stab_coeff_visc_scale", true ) ) {
						stab_coeff.Factor( "D11", ( 1 / oseen_viscosity )  );
						stab_coeff.Factor( "C11", oseen_viscosity );
					}
					else {
						stab_coeff.FactorFromParams("D11");
						stab_coeff.FactorFromParams("C11");
					}
					stab_coeff.FactorFromParams("D12");
					stab_coeff.FactorFromParams("C12");
					stab_coeff.Add( "E12", 0.5 );

					stab_coeff.print( Logger().Info() );

					typename Traits::OseenModelType
							stokesModel(stab_coeff,
										nonlinearForce,
										stokesDirichletData,
										oseen_viscosity,
										oseen_alpha,
										scale_factor, scale_factor);
					typename Traits::OseenpassType oseenPass( stokesStartPass,
											stokesModel,
											gridPart_,
											functionSpaceWrapper_,
											currentFunctions_.discreteVelocity(),
											true );
					oseenPass.apply( currentFunctions_, nextFunctions_, &rhsDatacontainer_ );

				}

				void setUpdateFunctions() const
				{
					updateFunctions_.assign( nextFunctions_);
					updateFunctions_ -= currentFunctions_ ;
				}

				void writeData()
				{
					dataWriter1_.write();
					dataWriter2_.write();
				}

				typename Traits::StokesPassType::RhsDatacontainer& rhsDatacontainer()
				{
					return rhsDatacontainer_;
				}

				const ExactSolutionType& exactSolution() const
				{
					return exactSolution_;
				}

				const typename Traits::DiscreteStokesFunctionWrapperType& currentFunctions() const
				{
					return currentFunctions_;
				}

				const typename Traits::TimeProviderType& timeprovider() const
				{
					return timeprovider_;
				}

		};
	}//end namespace NavierStokes
}//end namespace Dune

#endif // METADATA_HH
