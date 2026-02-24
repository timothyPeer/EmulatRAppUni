hmLoadTopic({
hmKeywords:"",
hmTitle:"13.12 LL\/SC and Determinism",
hmDescription:"The pipeline enforces LL\/SC semantics: speculation is allowed between LDL_L and STL_C, but reservations are cleared on exceptions, interrupts, barriers, PAL entry, and pipeline...",
hmPrevLink:"13_11-branch-handling.html",
hmNextLink:"13_13-summary.html",
hmParentLink:"alphapipeline-implementation.html",
hmBreadCrumbs:"<a href=\"license-_-attributions.html\">ASA-EMulatR Reference Guide<\/a> &gt; <a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphapipeline-implementation.html\">Chapter 13 – AlphaPipeline Implementation<\/a>",
hmTitlePath:"ASA-EMulatR Reference Guide > Introduction > Architecture Overview > Chapter 13 – AlphaPipeline Implementation > 13.12 LL\/SC and Determinism",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">13.12 LL\/SC and Determinism<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">The pipeline enforces LL\/SC semantics: speculation is allowed between LDL_L and STL_C, but reservations are cleared on exceptions, interrupts, barriers, PAL entry, and pipeline flush. STL_C retires atomically in stage_WB — the store-conditional check (reservation valid?) and the store commit happen in the same WB pass. No reservation survives a pipeline flush.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Store commits in stage_WB always call m_reservationManager→breakReservationsOnCacheLine(slot.pa), invalidating any reservation on the written cache line across all CPUs.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">The pipeline guarantees determinism: in-order retirement (m_instructionsRetired increments monotonically), one architectural commit per cycle, no speculative side effects (stores only commit in WB, registers only write in MEM via committed pending), and no privilege leakage. All behavior is replayable and debuggable.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: Section 5.11 – LL\/SC; Section 11.4 – LL\/SC Invariants.<\/span><\/p>\n\r"
})
