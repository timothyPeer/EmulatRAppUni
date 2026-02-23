hmLoadTopic({
hmKeywords:"",
hmTitle:"15.1 Memory Is Shared, Not Owned",
hmDescription:"ASA EmulatR implements a single shared physical address space, called GuestMemory, accessible by all CPUs. There is one unified physical address space with no per-CPU private...",
hmPrevLink:"chapter-15---memory-system-imp.html",
hmNextLink:"15_2-guestmemory-vs-safememory.html",
hmParentLink:"chapter-15---memory-system-imp.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"chapter-15---memory-system-imp.html\">Chapter 15 – Memory System Implementation Details<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 15 – Memory System Implementation Details > 15.1 Memory Is Shared, Not Owned",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">15.1 Memory Is Shared, Not Owned<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">ASA EmulatR implements a single shared physical address space, called <span class=\"f_CodeExample\">GuestMemory<\/span>, accessible by all CPUs. There is one unified physical address space with no per-CPU private memory. All CPUs observe the same backing store, and SMP correctness is enforced by coherency rules (barrier coordination and reservation invalidation), not by memory ownership.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">The <span class=\"f_CodeExample\">GuestMemory<\/span> instance is created as a process-wide singleton via <span class=\"f_CodeExample\">global_GuestMemory()<\/span> (thread-safe initialization in <span class=\"f_CodeExample\">global_GuestMemory.cpp<\/span>). Every CPU, every DMA path, and every device access routes through this single instance. The class is declared <span style=\"font-weight: bold;\">final<\/span>, non-copyable, and non-movable.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\"><span style=\"font-weight: bold;\">Invariant:<\/span> GuestMemory is global and authoritative. No subsystem may access physical memory without routing through GuestMemory.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: memoryLib\/global_GuestMemory.h – Singleton accessor; memoryLib\/GuestMemory.h – Class declaration.<\/span><\/p>\n\r"
})
