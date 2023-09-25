#include "ept.h"
#include "arch.h"
#include "vcpu.h"
#include "mtrr.h"
#include "mm.h"

namespace hv {

// set the memory type in every EPT paging structure to the specified value
void set_ept_memory_type(vcpu_ept_data& ept, uint8_t const memory_type) {
  for (size_t i = 0; i < ept_pd_count; ++i) {
    for (size_t j = 0; j < 512; ++j) {
      auto& pde = ept.pds_2mb[i][j];

      // 2MB large page
      if (pde.large_page)
        pde.memory_type = memory_type;
      // PDE points to a PT
      else {
        auto const pt = reinterpret_cast<ept_pte*>(host_physical_memory_base
          + (ept.pds[i][j].page_frame_number << 12));

        // update the memory type for every PTE
        for (size_t k = 0; k < 512; ++k)
          pt[k].memory_type = memory_type;
      }
    }
  }
}

} // namespace hv

