#include<linux/module.h>
#include<linux/sched/signal.h>
#include<linux/skbuff.h>


//globals counters for contig/non-contig
unsigned long m_ContigPages = 0, m_NonContigPages = 0;

// converts virtual address space to physical
unsigned long virt2phys(struct mm_struct *mm, unsigned long vpage) {
  unsigned long physical_page_addr;
  pgd_t *pgd;
  p4d_t *p4d;
  pud_t *pud;
  pmd_t *pmd;
  pte_t *pte;
  struct page *page;
  pgd = pgd_offset(mm, vpage);
  if(pgd_none(*pgd) || pgd_bad(*pgd)) {
      return 0;
  }
  p4d = p4d_offset(pgd, vpage);
  if(p4d_none(*p4d) || p4d_bad(*p4d)) {
      return 0;
  }
  pud = pud_offset(p4d, vpage);
  if(pud_none(*pud) || pud_bad(*pud)) {
      return 0;
  }
  pmd = pmd_offset(pud, vpage);
  if(pmd_none(*pmd) || pmd_bad(*pmd)) {
      return 0;
  }
  if(!(pte = pte_offset_map(pmd,vpage))){
      return 0;
  }
  if(!(page = pte_page(*pte))){
      return 0;
  }
  physical_page_addr = page_to_phys(page);
  pte_unmap(pte);
  if(physical_page_addr == 70368744173568) {
      return 0;
  }
  return physical_page_addr;
}

int proc_count(void) {
  //local variables
  int i = 0, totalPages = 0, counter = 0;
  struct task_struct *thechild; 
  struct vm_area_struct *vma = 0;
  unsigned long vpage;

  // prints start of proc file
  for_each_process(thechild) {
    int contig = 0, noncontig = 0;
    if (thechild->pid > 650) {
      if(thechild->mm && thechild->mm->mmap) {
        unsigned long prev_page;
        for(vma = thechild->mm->mmap; vma; vma = vma->vm_next) {
          for(vpage = vma->vm_start; vpage < vma->vm_end; vpage += PAGE_SIZE) {
            unsigned long physical_page_addr = virt2phys (thechild->mm, vpage);
            //check if address is allocated
            if (physical_page_addr != 0) {
              // Check for contig/non-contig pages
              if (PAGE_SIZE + prev_page == physical_page_addr) {
                contig++;
              } else {
                noncontig++;
              }
              prev_page = physical_page_addr;
              counter++;
            }
          }
        }
      }
      totalPages += counter; 
      if (counter > 0) {
        printk("%d,%s,%d,%d,%d\n",thechild->pid, thechild->comm, contig, noncontig, counter);
        i++;
      }
    }
    m_ContigPages += contig;
    m_NonContigPages += noncontig;
    counter = 0;
  }
  printk(KERN_INFO "TOTALS,,%lu,%lu,%d\n", m_ContigPages, m_NonContigPages, totalPages);
  return i;
}

int proc_init (void) {
  printk("PROCESS REPORT:\n");
	printk(KERN_INFO "proc_id,proc_name,contig_pages,noncontig_pages,total_pages\n");
  proc_count();
  return 0;
}


void proc_cleanup(void) {
  printk(KERN_INFO "procReport: performing cleanup of module\n");
}

MODULE_LICENSE("GPL");
module_init(proc_init);
module_exit(proc_cleanup);

