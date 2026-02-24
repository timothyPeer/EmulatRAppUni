hmLoadTopic({
hmKeywords:"",
hmTitle:"7.11 Traps and TRAPB",
hmDescription:"Arithmetic traps are detected during execution but delivered after instruction retirement. This deferred delivery model means that without TRAPB, the precise instruction that...",
hmPrevLink:"chapter-7_9-interrupt-handling.html",
hmNextLink:"chapter-11-interaction-with-se.html",
hmParentLink:"chapter-7---interrupt-and-ipi-.html",
hmBreadCrumbs:"<a href=\"license-_-attributions.html\">ASA-EMulatR Reference Guide<\/a> &gt; <a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"chapter-7---interrupt-and-ipi-.html\">Chapter 7 - Exceptions, Faults, and Interrupts<\/a>",
hmTitlePath:"ASA-EMulatR Reference Guide > Introduction > Architecture Overview > Chapter 7 - Exceptions, Faults, and Interrupts > 7.11 Traps and TRAPB",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">7.11 Traps and TRAPB<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">Arithmetic traps are detected during execution but delivered after instruction retirement. This deferred delivery model means that without TRAPB, the precise instruction that caused the trap may be ambiguous.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">TRAPB ensures precise arithmetic trap ordering by requiring all prior traps to be resolved before later instructions execute. The FaultDispatcher provides hasPendingArithmeticTraps() which tests the FLAG_ARITHMETIC_TRAP bit, and clearArithmeticTrap() which selectively clears arithmetic events while leaving other pending events intact.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">The TRAPB → FaultDispatcher interaction: when TRAPB stalls in the pipeline (via CBox::executeTRAPB with slot.mustComplete = true), it cannot release until hasPendingArithmeticTraps() returns false — meaning all prior arithmetic traps have been delivered and cleared.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: <a href=\"chapter-6_10---trapb---trap-ba.html\" class=\"topiclink\">6.10 TRAPB - Trap Barrier<\/a>; faultLib\/FaultDispatcher.h (hasPendingArithmeticTraps, clearArithmeticTrap).<\/span><\/p>\n\r"
})
