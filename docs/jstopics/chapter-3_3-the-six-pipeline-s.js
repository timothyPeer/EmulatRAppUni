hmLoadTopic({
hmKeywords:"",
hmTitle:"3.3 The Six Pipeline Stages",
hmDescription:"The pipeline consists of six fixed stages. Stages execute from oldest to youngest (WB → MEM → EX → IS → DE → IF) within each cycle.",
hmPrevLink:"chapter-3_2-overview-of-the-pi.html",
hmNextLink:"chapter-3_4-fetch-stage-(if).html",
hmParentLink:"chapter-3---pipeline-architect.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"chapter-3---pipeline-architect.html\">Chapter 3 - Pipeline Architecture<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 3 - Pipeline Architecture > 3.3 The Six Pipeline Stages",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">3.3 The Six Pipeline Stages<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">The pipeline consists of six fixed stages. Stages execute from oldest to youngest (WB → MEM → EX → IS → DE → IF) within each cycle.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<h3 class=\"p_Heading3\" style=\"page-break-after: avoid;\"><span class=\"f_Heading3\">Stage Table<\/span><\/h3>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">[0] Fetch (IF) — Instruction fetch, grain resolution, branch prediction, PC advancement<\/p>\n\r<p class=\"p_Normal\">[1] Decode (DE) — Grain validation, execution Box routing, privilege checks<\/p>\n\r<p class=\"p_Normal\">[2] Issue (IS) — Operand readiness, hazard checks, stall enforcement<\/p>\n\r<p class=\"p_Normal\">[3] Execute (EX) — All architectural work: ALU, FP, memory, barriers, PAL<\/p>\n\r<p class=\"p_Normal\">[4] Memory (MEM) — Deferred register commit (forwarding path), stall propagation<\/p>\n\r<p class=\"p_Normal\">[5] Writeback (WB) — Architectural commit: store commit, fault dispatch, retirement<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Invariant: No architectural state becomes visible until an instruction reaches WB. Any instruction that has not reached WB may be discarded without side effects.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: Sections 3.4–3.9 for individual stage detail.<\/span><\/p>\n\r"
})
