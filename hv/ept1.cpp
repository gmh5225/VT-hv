#include "ept.h"
#include "arch.h"
#include "vcpu.h"
#include "mtrr.h"
#include "mm.h"

namespace hv {

// identity-map the EPT paging structures
void prepare_ept(vcpu_ept_data& ept) {
  memset(&ept, 0, sizeof(ept));

  ept.dummy_page_pfn = MmGetPhysicalAddress(ept.dummy_page).QuadPart >> 12;

  ept.num_used_free_pages = 0;

  for (size_t i = 0; i < ept_free_page_count; ++i)
    ept.free_page_pfns[i] = MmGetPhysicalAddress(&ept.free_pages[i]).QuadPart >> 12;

  ept.hooks.active_list_head = nullptr;
  ept.hooks.free_list_head   = &ept.hooks.buffer[0];

  for (size_t i = 0; i < ept.hooks.capacity - 1; ++i)
    ept.hooks.buffer[i].next = &ept.hooks.buffer[i + 1];

  // the last node points to NULL
  ept.hooks.buffer[ept.hooks.capacity - 1].next = nullptr;

  // setup the first PML4E so that it points to our PDPT
  auto& pml4e             = ept.pml4[0];
  pml4e.flags             = 0;
  pml4e.read_access       = 1;
  pml4e.write_access      = 1;
  pml4e.execute_access    = 1;
  pml4e.accessed          = 0;
  pml4e.user_mode_execute = 1;
  pml4e.page_frame_number = MmGetPhysicalAddress(&ept.pdpt).QuadPart >> 12;

  // MTRR data for setting memory types
  auto const mtrrs = read_mtrr_data();

  // TODO: allocate a PT for the fixed MTRRs region so that we can get
  // more accurate memory typing in that area (as opposed to just
  // mapping the whole PDE as UC).

  for (size_t i = 0; i < ept_pd_count; ++i) {
    // point each PDPTE to the corresponding PD
    auto& pdpte             = ept.pdpt[i];
    pdpte.flags             = 0;
    pdpte.read_access       = 1;
    pdpte.write_access      = 1;
    pdpte.execute_access    = 1;
    pdpte.accessed          = 0;
    pdpte.user_mode_execute = 1;
    pdpte.page_frame_number = MmGetPhysicalAddress(&ept.pds[i]).QuadPart >> 12;

    for (size_t j = 0; j < 512; ++j) {
      // identity-map every GPA to the corresponding HPA
      auto& pde             = ept.pds_2mb[i][j];
      pde.flags             = 0;
      pde.read_access       = 1;
      pde.write_access      = 1;
      pde.execute_access    = 1;
      pde.ignore_pat        = 0;
      pde.large_page        = 1;
      pde.accessed          = 0;
      pde.dirty             = 0;
      pde.user_mode_execute = 1;
      pde.suppress_ve       = 0;
      pde.page_frame_number = (i << 9) + j;
      pde.memory_type       = calc_mtrr_mem_type(mtrrs,
        pde.page_frame_number << 21, 0x1000 << 9);
    }
  }
}

} // namespace hv

