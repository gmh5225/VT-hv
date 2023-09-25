#include "ept.h"
#include "arch.h"
#include "vcpu.h"
#include "mtrr.h"
#include "mm.h"

namespace hv {

// update the memory types in the EPT paging structures based on the MTRRs.
// this function should only be called from root-mode during vmx-operation.
void update_ept_memory_type(vcpu_ept_data& ept) {
  // TODO: completely virtualize the guest MTRRs
  auto const mtrrs = read_mtrr_data();

  for (size_t i = 0; i < ept_pd_count; ++i) {
    for (size_t j = 0; j < 512; ++j) {
      auto& pde = ept.pds_2mb[i][j];

      // 2MB large page
      if (pde.large_page) {
        // update the memory type for this PDE
        pde.memory_type = calc_mtrr_mem_type(mtrrs,
          pde.page_frame_number << 21, 0x1000 << 9);
      }
      // PDE points to a PT
      else {
        auto const pt = reinterpret_cast<ept_pte*>(host_physical_memory_base
          + (ept.pds[i][j].page_frame_number << 12));

        // update the memory type for every PTE
        for (size_t k = 0; k < 512; ++k) {
          pt[k].memory_type = calc_mtrr_mem_type(mtrrs,
            pt[k].page_frame_number << 12, 0x1000);
        }
      }
    }
  }
}

} // namespace hv

