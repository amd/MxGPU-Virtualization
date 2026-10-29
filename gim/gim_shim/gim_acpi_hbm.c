/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <linux/acpi.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/overflow.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include "gim.h"
#include "gim_debug.h"
#include "gim_acpi_hbm.h"

#define GIM_ACPI_MAX_MEM	64
#define GIM_ACPI_MAX_GI		128

struct gim_acpi_mem {
	u32 pd;
	u64 base;
	u64 length;
};

#if !IS_ENABLED(CONFIG_ACPI)

int gim_acpi_discover_hbm(struct pci_dev *pdev, int *pf_pd,
			  uint64_t *hbm_pa, uint64_t *hbm_size)
{
	return -ENOENT;
}

int gim_acpi_lookup_pd(u32 seg, u8 bus, u8 devfn, int *pd)
{
	return -ENOENT;
}

#else

static bool gim_acpi_pci_handle(const u8 handle[16], u32 *seg, u8 *bus, u8 *devfn)
{
	int i;

	/* Bytes past the PCI segment/bus/devfn are zero in the firmware tables.
	 * A non-zero tail is an ACPI HID, not a BDF.
	 */
	for (i = 4; i < 16; i++) {
		if (handle[i])
			return false;
	}

	*seg = handle[0] | ((u32)handle[1] << 8);
	*bus = handle[2];
	*devfn = handle[3];
	return true;
}

static bool gim_acpi_pd_present(const u32 *pds, int nr, u32 pd)
{
	int i;

	for (i = 0; i < nr; i++) {
		if (pds[i] == pd)
			return true;
	}
	return false;
}

/* NPS1 has one SRAT memory record for the selected proximity domain. */
static bool gim_acpi_one_range(struct gim_acpi_mem *mem, int nr, u32 pd,
			       u64 *base, u64 *size)
{
	bool found = false;
	int i;

	for (i = 0; i < nr; i++) {
		if (mem[i].pd != pd || mem[i].length == 0)
			continue;
		if (found) {
			gim_warn("pd %u has another memory range at 0x%llx size 0x%llx, ignoring it\n",
				 pd, mem[i].base, mem[i].length);
			continue;
		}
		*base = mem[i].base;
		*size = mem[i].length;
		found = true;
	}
	return found;
}

static int gim_acpi_read_srat(struct pci_dev *pdev, int *pf_pd,
			      struct gim_acpi_mem *mem, int *nr_mem,
			      u32 *cpu_pd, int *nr_cpu)
{
	struct acpi_table_header *table;
	struct acpi_table_srat *srat;
	u8 *ptr, *end;
	u32 seg = pci_domain_nr(pdev->bus);
	u8 bus = pdev->bus->number;
	u8 devfn = pdev->devfn;
	bool found = false;
	acpi_status status;

	status = acpi_get_table(ACPI_SIG_SRAT, 0, &table);
	if (ACPI_FAILURE(status))
		return -ENOENT;

	srat = (struct acpi_table_srat *)table;
	ptr = (u8 *)srat + sizeof(*srat);
	end = (u8 *)table + table->length;

	while (ptr + sizeof(struct acpi_subtable_header) < end) {
		struct acpi_subtable_header *hdr = (void *)ptr;

		if (hdr->length < sizeof(*hdr) || ptr + hdr->length > end)
			break;

		switch (hdr->type) {
		case ACPI_SRAT_TYPE_MEMORY_AFFINITY: {
			struct acpi_srat_mem_affinity *ma = (void *)hdr;

			if (hdr->length < sizeof(*ma))
				break;
			if (!(ma->flags & ACPI_SRAT_MEM_ENABLED) || !ma->length)
				break;
			if (*nr_mem >= GIM_ACPI_MAX_MEM) {
				gim_warn("SRAT memory affinity table truncated\n");
				break;
			}
			mem[*nr_mem].pd = ma->proximity_domain;
			mem[*nr_mem].base = ma->base_address;
			mem[*nr_mem].length = ma->length;
			(*nr_mem)++;
			break;
		}
		case ACPI_SRAT_TYPE_CPU_AFFINITY: {
			struct acpi_srat_cpu_affinity *cpu = (void *)hdr;
			u32 cpu_pd_id;

			if (hdr->length < sizeof(*cpu))
				break;
			if (!(cpu->flags & ACPI_SRAT_CPU_ENABLED))
				break;
			cpu_pd_id = cpu->proximity_domain_lo |
				    ((u32)cpu->proximity_domain_hi[0] << 8) |
				    ((u32)cpu->proximity_domain_hi[1] << 16) |
				    ((u32)cpu->proximity_domain_hi[2] << 24);
			if (gim_acpi_pd_present(cpu_pd, *nr_cpu, cpu_pd_id))
				break;
			if (*nr_cpu >= GIM_ACPI_MAX_GI) {
				gim_warn("SRAT CPU proximity domains truncated\n");
				break;
			}
			cpu_pd[(*nr_cpu)++] = cpu_pd_id;
			break;
		}
		case ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY: {
			struct acpi_srat_x2apic_cpu_affinity *cpu = (void *)hdr;

			if (hdr->length < sizeof(*cpu))
				break;
			if (!(cpu->flags & ACPI_SRAT_CPU_ENABLED))
				break;
			if (gim_acpi_pd_present(cpu_pd, *nr_cpu, cpu->proximity_domain))
				break;
			if (*nr_cpu >= GIM_ACPI_MAX_GI) {
				gim_warn("SRAT x2APIC proximity domains truncated\n");
				break;
			}
			cpu_pd[(*nr_cpu)++] = cpu->proximity_domain;
			break;
		}
		case ACPI_SRAT_TYPE_GENERIC_AFFINITY: {
			struct acpi_srat_generic_affinity *gi = (void *)hdr;
			u32 hseg;
			u8 hbus, hdevfn;

			if (hdr->length < sizeof(*gi))
				break;
			if (!(gi->flags & ACPI_SRAT_GENERIC_AFFINITY_ENABLED))
				break;

			if (!gim_acpi_pci_handle(gi->device_handle, &hseg, &hbus, &hdevfn))
				break;
			if (hseg == seg && hbus == bus && hdevfn == devfn) {
				*pf_pd = gi->proximity_domain;
				found = true;
			}
			break;
		}
		default:
			break;
		}
		ptr += hdr->length;
	}

	acpi_put_table(table);
	return found ? 0 : -ENOENT;
}

struct gim_acpi_lat_cand {
	u32 pd;
	u64 lat;
	u64 base;
	u64 size;
	u8 type;
	bool have;
	bool tie;
};

/* Access latency selects local HBM. Read, then write, is used only when
 * no stronger type produced a candidate.
 */
static int gim_acpi_lat_rank(u8 type)
{
	switch (type) {
	case ACPI_HMAT_ACCESS_LATENCY:
		return 0;
	case ACPI_HMAT_READ_LATENCY:
		return 1;
	case ACPI_HMAT_WRITE_LATENCY:
		return 2;
	default:
		return 3;
	}
}

/* Actual latency is entry times the matrix entry_base_unit. */
static bool gim_acpi_lat_scale(u16 ent, u64 base_unit, u64 *lat)
{
	if (!ent || ent == 0xffff || !base_unit)
		return false;
	if (check_mul_overflow((u64)ent, base_unit, lat))
		return false;
	return true;
}

static void gim_acpi_lat_update(struct gim_acpi_lat_cand *c, u32 pd, u8 type,
				u64 lat, u64 base, u64 size)
{
	int rank = gim_acpi_lat_rank(type);
	int cur = c->have ? gim_acpi_lat_rank(c->type) : 0;

	if (!c->have || rank < cur || (rank == cur && lat < c->lat)) {
		c->pd = pd;
		c->type = type;
		c->lat = lat;
		c->base = base;
		c->size = size;
		c->have = true;
		c->tie = false;
	} else if (rank == cur && lat == c->lat) {
		c->tie = true;
		if (pd < c->pd) {
			c->pd = pd;
			c->base = base;
			c->size = size;
		}
	}
}

static int gim_acpi_hmat_latency(int pf_pd, struct gim_acpi_mem *mem, int nr_mem,
				 const u32 *cpu_pd, int nr_cpu,
				 u64 *hbm_pa, u64 *hbm_size)
{
	struct acpi_table_header *table;
	struct gim_acpi_lat_cand best = { 0 };
	u8 *ptr, *end;
	acpi_status status;

	status = acpi_get_table(ACPI_SIG_HMAT, 0, &table);
	if (ACPI_FAILURE(status))
		return -ENOENT;

	ptr = (u8 *)table + sizeof(struct acpi_table_hmat);
	end = (u8 *)table + table->length;

	while (ptr + sizeof(struct acpi_hmat_structure) < end) {
		struct acpi_hmat_structure *hs = (void *)ptr;
		struct acpi_hmat_locality *loc;
		u32 ninit, ntgt, init_idx, t;
		u64 bytes;
		u32 *inits, *tgts;
		u16 *ents;

		if (hs->length < sizeof(*hs) || ptr + hs->length > end)
			break;
		if (hs->type != ACPI_HMAT_TYPE_LOCALITY)
			goto next;

		loc = (struct acpi_hmat_locality *)hs;
		if (hs->length < sizeof(*loc))
			goto next;
		if ((loc->flags & 0xf) != ACPI_HMAT_MEMORY)
			goto next;
		if (loc->data_type != ACPI_HMAT_ACCESS_LATENCY &&
		    loc->data_type != ACPI_HMAT_READ_LATENCY &&
		    loc->data_type != ACPI_HMAT_WRITE_LATENCY)
			goto next;

		ninit = loc->number_of_initiator_Pds;
		ntgt = loc->number_of_target_Pds;
		if (!ninit || !ntgt)
			goto next;
		bytes = sizeof(*loc) + (ninit + ntgt) * sizeof(u32) +
			ninit * ntgt * sizeof(u16);
		if ((u64)ninit * ntgt > hs->length || hs->length < bytes)
			goto next;

		inits = (u32 *)((u8 *)loc + sizeof(*loc));
		tgts = inits + ninit;
		ents = (u16 *)(tgts + ntgt);

		for (init_idx = 0; init_idx < ninit; init_idx++) {
			if (inits[init_idx] == (u32)pf_pd)
				break;
		}
		if (init_idx == ninit)
			goto next;

		for (t = 0; t < ntgt; t++) {
			u16 ent = ents[init_idx * ntgt + t];
			u64 base, size, lat;

			if (!gim_acpi_lat_scale(ent, loc->entry_base_unit, &lat))
				continue;
			if (gim_acpi_pd_present(cpu_pd, nr_cpu, tgts[t]))
				continue;
			if (!gim_acpi_one_range(mem, nr_mem, tgts[t], &base, &size))
				continue;
			gim_acpi_lat_update(&best, tgts[t], loc->data_type, lat,
					    base, size);
		}
next:
		ptr += hs->length;
	}

	acpi_put_table(table);
	if (!best.have)
		return -ENOENT;
	if (best.tie)
		gim_warn("multiple HBM ranges at latency %llu type %u, "
			 "using pd %u 0x%llx/0x%llx\n",
			 best.lat, best.type, best.pd, best.base, best.size);
	*hbm_pa = best.base;
	*hbm_size = best.size;
	return 0;
}

int gim_acpi_lookup_pd(u32 seg, u8 bus, u8 devfn, int *pd)
{
	struct acpi_table_header *table;
	u8 *ptr, *end;
	bool found = false;
	acpi_status status;

	if (!pd)
		return -EINVAL;

	status = acpi_get_table(ACPI_SIG_SRAT, 0, &table);
	if (ACPI_FAILURE(status))
		return -ENOENT;

	ptr = (u8 *)table + sizeof(struct acpi_table_srat);
	end = (u8 *)table + table->length;

	while (ptr + sizeof(struct acpi_subtable_header) < end) {
		struct acpi_subtable_header *hdr = (void *)ptr;
		struct acpi_srat_generic_affinity *gi;
		u32 hseg;
		u8 hbus, hdevfn;

		if (hdr->length < sizeof(*hdr) || ptr + hdr->length > end)
			break;
		if (hdr->type != ACPI_SRAT_TYPE_GENERIC_AFFINITY)
			goto next;
		gi = (void *)hdr;
		if (hdr->length < sizeof(*gi))
			goto next;
		if (!(gi->flags & ACPI_SRAT_GENERIC_AFFINITY_ENABLED))
			goto next;
		if (!gim_acpi_pci_handle(gi->device_handle, &hseg, &hbus, &hdevfn))
			goto next;
		if (hseg == seg && hbus == bus && hdevfn == devfn) {
			*pd = gi->proximity_domain;
			found = true;
			break;
		}
next:
		ptr += hdr->length;
	}

	acpi_put_table(table);
	return found ? 0 : -ENOENT;
}

struct gim_acpi_srat_buf {
	struct gim_acpi_mem mem[GIM_ACPI_MAX_MEM];
	u32 cpu_pd[GIM_ACPI_MAX_GI];
};

int gim_acpi_discover_hbm(struct pci_dev *pdev, int *pf_pd,
			  uint64_t *hbm_pa, uint64_t *hbm_size)
{
	struct gim_acpi_srat_buf *buf;
	int nr_mem = 0, nr_cpu = 0;
	int pd = -1;
	u64 pa = 0, size = 0;
	int ret;

	if (!pdev || !pf_pd || !hbm_pa || !hbm_size)
		return -EINVAL;

	*hbm_pa = 0;
	*hbm_size = 0;

	buf = gim_kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = gim_acpi_read_srat(pdev, &pd, buf->mem, &nr_mem, buf->cpu_pd, &nr_cpu);
	if (ret) {
		gim_kfree(buf);
		return ret;
	}

	*pf_pd = pd;
	gim_info("%s: PF proximity domain %d\n", pci_name(pdev), pd);

	if (gim_acpi_hmat_latency(pd, buf->mem, nr_mem, buf->cpu_pd, nr_cpu,
				  &pa, &size)) {
		gim_warn("%s: no HBM range for PF proximity domain %d\n",
			 pci_name(pdev), pd);
		gim_kfree(buf);
		return 0;
	}

	gim_kfree(buf);
	*hbm_pa = pa;
	*hbm_size = size;
	gim_info("%s: HBM pa 0x%llx size 0x%llx\n", pci_name(pdev), pa, size);
	return 0;
}

#endif
