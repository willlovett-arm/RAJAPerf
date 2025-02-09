//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-19, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "LTIMES_NOVIEW.hpp"

#include "RAJA/RAJA.hpp"

#include "common/DataUtils.hpp"

#include <iostream>

namespace rajaperf 
{
namespace apps
{


#define LTIMES_NOVIEW_DATA_SETUP_CPU \
  ResReal_ptr phidat = m_phidat; \
  ResReal_ptr elldat = m_elldat; \
  ResReal_ptr psidat = m_psidat; \
\
  Index_type num_d = m_num_d; \
  Index_type num_z = m_num_z; \
  Index_type num_g = m_num_g; \
  Index_type num_m = m_num_m;


LTIMES_NOVIEW::LTIMES_NOVIEW(const RunParams& params)
  : KernelBase(rajaperf::Apps_LTIMES_NOVIEW, params)
{
  m_num_d_default = 64;
  m_num_z_default = 500;
  m_num_g_default = 32;
  m_num_m_default = 25;

  setDefaultSize(m_num_d_default * m_num_m_default * 
                 m_num_g_default * m_num_z_default);
  setDefaultReps(50);
}

LTIMES_NOVIEW::~LTIMES_NOVIEW() 
{
}

void LTIMES_NOVIEW::setUp(VariantID vid)
{
  m_num_z = run_params.getSizeFactor() * m_num_z_default;
  m_num_g = m_num_g_default;  
  m_num_m = m_num_m_default;  
  m_num_d = m_num_d_default;  

  m_philen = m_num_m * m_num_g * m_num_z;
  m_elllen = m_num_d * m_num_m;
  m_psilen = m_num_d * m_num_g * m_num_z;

  allocAndInitDataConst(m_phidat, int(m_philen), Real_type(0.0), vid);
  allocAndInitData(m_elldat, int(m_elllen), vid);
  allocAndInitData(m_psidat, int(m_psilen), vid);
}

void LTIMES_NOVIEW::runKernel(VariantID vid)
{
  const Index_type run_reps = getRunReps();

  LTIMES_NOVIEW_DATA_SETUP_CPU;
 
  auto ltimesnoview_lam = [=](Index_type d, Index_type z, 
                              Index_type g, Index_type m) {
                                LTIMES_NOVIEW_BODY;
                          };

  switch ( vid ) {

    case Base_Seq : {

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        for (Index_type z = 0; z < num_z; ++z ) {
          for (Index_type g = 0; g < num_g; ++g ) {
            for (Index_type m = 0; m < num_m; ++m ) {
              for (Index_type d = 0; d < num_d; ++d ) {
                LTIMES_NOVIEW_BODY;
              }
            }
          }
        }

      }
      stopTimer();

      break;
    } 

#if defined(RUN_RAJA_SEQ)     
    case RAJA_Seq : {

      using EXEC_POL =
        RAJA::KernelPolicy<
          RAJA::statement::For<1, RAJA::loop_exec,       // z
            RAJA::statement::For<2, RAJA::loop_exec,     // g
              RAJA::statement::For<3, RAJA::loop_exec,   // m
                RAJA::statement::For<0, RAJA::loop_exec, // d
                  RAJA::statement::Lambda<0>
                >
              >
            >
          >
        >;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::kernel<EXEC_POL>( RAJA::make_tuple(RAJA::RangeSegment(0, num_d),
                                                 RAJA::RangeSegment(0, num_z),
                                                 RAJA::RangeSegment(0, num_g),
                                                 RAJA::RangeSegment(0, num_m)),
                                ltimesnoview_lam
                              );

      }
      stopTimer(); 

      break;
    }
#endif // RUN_RAJA_SEQ

#if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
    case Base_OpenMP : {

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        #pragma omp parallel for
        for (Index_type z = 0; z < num_z; ++z ) {
          for (Index_type g = 0; g < num_g; ++g ) {
            for (Index_type m = 0; m < num_m; ++m ) {
              for (Index_type d = 0; d < num_d; ++d ) {
                LTIMES_NOVIEW_BODY;
              }
            }
          }
        }  

      }
      stopTimer();

      break;
    }

    case RAJA_OpenMP : {

      using EXEC_POL =
        RAJA::KernelPolicy<
          RAJA::statement::For<1, RAJA::omp_parallel_for_exec, // z
            RAJA::statement::For<2, RAJA::loop_exec,           // g
              RAJA::statement::For<3, RAJA::loop_exec,         // m
                RAJA::statement::For<0, RAJA::loop_exec,       // d
                  RAJA::statement::Lambda<0>
                >
              >
            >
          >
        >;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::kernel<EXEC_POL>( RAJA::make_tuple(RAJA::RangeSegment(0, num_d),
                                                 RAJA::RangeSegment(0, num_z),
                                                 RAJA::RangeSegment(0, num_g),
                                                 RAJA::RangeSegment(0, num_m)),
                                ltimesnoview_lam
                              );

      }
      stopTimer();

      break;
    }
#endif

#if defined(RAJA_ENABLE_TARGET_OPENMP)
    case Base_OpenMPTarget :
    case RAJA_OpenMPTarget :
    {
      runOpenMPTargetVariant(vid);
      break;
    }
#endif

#if defined(RAJA_ENABLE_CUDA)
    case Base_CUDA :
    case RAJA_CUDA :
    {
      runCudaVariant(vid);
      break;
    }
#endif

    default : {
      std::cout << "\n LTIMES_NOVIEW : Unknown variant id = " << vid << std::endl;
    }

  }
}

void LTIMES_NOVIEW::updateChecksum(VariantID vid)
{
  checksum[vid] += calcChecksum(m_phidat, m_philen);
}

void LTIMES_NOVIEW::tearDown(VariantID vid)
{
  (void) vid;
 
  deallocData(m_phidat);
  deallocData(m_elldat);
  deallocData(m_psidat);
}

} // end namespace apps
} // end namespace rajaperf
