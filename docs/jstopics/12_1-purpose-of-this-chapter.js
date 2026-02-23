hmLoadTopic({
hmKeywords:"",
hmTitle:"12.1 Purpose of This Chapter",
hmDescription:"This chapter defines the AlphaCPU core object and its responsibilities within ASA-EmulatR. This is the first chapter of Group 2 (Core Processor Implementation).",
hmPrevLink:"alphacpu-core.html",
hmNextLink:"12_2-alphacpu-as-the-unit-of-e.html",
hmParentLink:"alphacpu-core.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphacpu-core.html\">Chapter 12 – AlphaCPU Core<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 12 – AlphaCPU Core > 12.1 Purpose of This Chapter",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">12.1 Purpose of This Chapter<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">This chapter defines the AlphaCPU core object and its responsibilities within ASA-EmulatR. This is the first chapter of Group 2 (Core Processor Implementation).<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">AlphaCPU is the unit of execution. It owns the processor run loop, pipeline state, architectural registers, per-CPU execution context, and interaction with SMP, interrupts, and PAL.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">This chapter does not redefine architectural rules (those are in Group 1, Chapters 1–11). Instead, it explains how those rules are enforced in code — who owns what and who drives execution.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Implementation: AlphaCPU.h (952 lines, mostly header-only inline methods) and AlphaCPU.cpp (constructor, destructor, and executeLoop() — required by Qt MOC).<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: <a href=\"chapter-3---pipeline-architect.html\" class=\"topiclink\">Chapter 3 - Pipeline Architecture<\/a>; <a href=\"chapter9-smparchitecture.html\" class=\"topiclink\">Chapter 9 - SMP Architecture (ExecutionController creates AlphaCPUs)<\/a>.<\/span><\/p>\n\r"
})
