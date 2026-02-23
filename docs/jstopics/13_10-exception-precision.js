hmLoadTopic({
hmKeywords:"",
hmTitle:"13.10 Exception Precision",
hmDescription:"Exceptions are detected early (in EX) but delivered late (in WB). When a fault is detected in stage_EX(), slot.faultPending is set to true with the trapCode and faultVA. The...",
hmPrevLink:"13_9-serialization-and-barrier.html",
hmNextLink:"13_11-branch-handling.html",
hmParentLink:"alphapipeline-implementation.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphapipeline-implementation.html\">Chapter 13 – AlphaPipeline Implementation<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 13 – AlphaPipeline Implementation > 13.10 Exception Precision",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">13.10 Exception Precision<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">Exceptions are detected early (in EX) but delivered late (in WB). When a fault is detected in stage_EX(), slot.faultPending is set to true with the trapCode and faultVA. The instruction continues flowing through MEM (where its pending commit is preserved but may be discarded). When the faulting instruction reaches stage_WB(), the fault check fires first — before any store commit or retirement.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">On fault in stage_WB: the m_pending (from a younger instruction) is discarded (PendingCommit{}), PipelineAction::FAULT is set with trapCode\/faultVA\/faultPC, and the slot is invalidated. The fault propagates to AlphaCPU via BoxResult::faultDispatched(), which triggers enterPal().<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">flushYoungerSlots() clears all instructions younger than the faulting instruction — preserving older instructions that have already committed. This guarantees precise exceptions: all prior instructions completed, no later instruction modified architectural state.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: Section 11.2.1 – Precise Execution invariant.<\/span><\/p>\n\r"
})
