hmLoadTopic({
hmKeywords:"",
hmTitle:"11.1 Purpose of This Chapter",
hmDescription:"This chapter defines the architectural invariants of EMulatR.",
hmPrevLink:"chapter-11---architectural-inv.html",
hmNextLink:"11_2-execution-and-pipeline-in.html",
hmParentLink:"chapter-11---architectural-inv.html",
hmBreadCrumbs:"<a href=\"license-_-attributions.html\">ASA-EMulatR Reference Guide<\/a> &gt; <a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"chapter-11---architectural-inv.html\">Chapter 11 - Architectural Invariants<\/a>",
hmTitlePath:"ASA-EMulatR Reference Guide > Introduction > Architecture Overview > Chapter 11 - Architectural Invariants > 11.1 Purpose of This Chapter",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">11.1 Purpose of This Chapter<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">This chapter defines the architectural invariants of EMulatR.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">An architectural invariant is a rule that must always hold true, is independent of implementation details, cannot be violated by optimization, and defines correctness — not performance. If an invariant is violated, the emulator is architecturally incorrect, even if it appears to work.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">These invariants apply across single-CPU and SMP systems, PAL and non-PAL execution, all pipeline stages, and all device and MMIO interactions.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">This chapter should be treated as a correctness checklist, a validation guide, a regression test oracle, and a contributor contract.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: Every preceding chapter — this chapter is the formal statement of guarantees that Chapters 1–10 collectively enforce.<\/span><\/p>\n\r"
})
