hmLoadTopic({
hmKeywords:"",
hmTitle:"8.8 PAL and Memory Ordering",
hmDescription:"PAL code is implicitly serialized. Entry implies a full barrier (equivalent to MB + EXCB), exit implies a barrier. This ensures that exception handlers see stable memory,...",
hmPrevLink:"chapter-8_7-pal-and-interrupts.html",
hmNextLink:"chapter-8_9-hw_rei---exiting-p.html",
hmParentLink:"chapter-8---pal-and-privleged-.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"chapter-8---pal-and-privleged-.html\">Chapter 8 - PAL and Privileged Boundary<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 8 - PAL and Privileged Boundary > 8.8 PAL and Memory Ordering",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">8.8 PAL and Memory Ordering<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">PAL code is implicitly serialized. Entry implies a full barrier (equivalent to MB + EXCB), exit implies a barrier. This ensures that exception handlers see stable memory, context switches are consistent, and device state is coherent.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Specifically, on PAL entry: write buffers are drained via CBox::issueMemoryBarrier(MemoryBarrierKind::PAL), the global barrier coordinator synchronizes with other CPUs (in SMP), and the CBox state transitions from SERIALIZING back to RUNNING only after all acknowledgments are received.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Certain PAL operations explicitly drain buffers (e.g., MB instruction within PAL code, DRAINA). Memory ordering is guaranteed across PAL boundaries — this is a stronger guarantee than any single barrier instruction provides to non-PAL code.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: <a href=\"chapter-6_11-call_pal-as-a-ser.html\" class=\"topiclink\">6.11 CALL_PAL as a Serialization Point<\/a>.<\/span><\/p>\n\r"
})
