hmLoadTopic({
hmKeywords:"",
hmTitle:"12.12 Performance and Instrumentation",
hmDescription:"AlphaCPU tracks performance counters: m_localInstrCount (instructions retired) and m_localCycleCount (cycles elapsed). The architectural cycle counter (CC register) is managed...",
hmPrevLink:"12_11-error-handling.html",
hmNextLink:"12_13-summary.html",
hmParentLink:"alphacpu-core.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphacpu-core.html\">Chapter 12 – AlphaCPU Core<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 12 – AlphaCPU Core > 12.12 Performance and Instrumentation",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">12.12 Performance and Instrumentation<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">AlphaCPU tracks performance counters: m_localInstrCount (instructions retired) and m_localCycleCount (cycles elapsed). The architectural cycle counter (CC register) is managed by incrementCycleCount(), which respects the CC_CTL_ENABLE and CC_CTL_FREEZE_PAL control bits — the cycle counter does not increment during PAL mode when the freeze bit is set.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Instrumentation is controlled by the AXP_INSTRUMENTATION_TRACE compile-time flag. When enabled, EXECTRACE macros emit trace records for PAL entry (EXECTRACE_PAL_ENTRY), PAL exit (EXECTRACE_PAL_EXIT), interrupts (EXECTRACE_INTERRUPT), and register writes. The ExecTrace system records PalEntryReasonTrace (CALL_PAL, FAULT, INTERRUPT, TRAP, MACHINE_CHECK), WriteEntry (register type\/index\/value), and timing information.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">All critical methods are marked AXP_HOT AXP_ALWAYS_INLINE to ensure they remain on the hot path without function call overhead. The AlphaCPU class is alignas(8) for cache-friendly layout.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: coreLib\/ExecTrace.h (PalEntryReasonTrace, WriteEntry).<\/span><\/p>\n\r"
})
