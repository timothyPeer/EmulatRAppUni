hmLoadTopic({
hmKeywords:"",
hmTitle:"13.9 Serialization and Barriers in Pipeline",
hmDescription:"Barrier instructions (MB, WMB, EXCB, TRAPB) do not execute work — they enforce ordering. In EX stage, the grain sets slot.needsMemoryBarrier = true or...",
hmPrevLink:"13_8-flush-semantics.html",
hmNextLink:"13_10-exception-precision.html",
hmParentLink:"alphapipeline-implementation.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphapipeline-implementation.html\">Chapter 13 – AlphaPipeline Implementation<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 13 – AlphaPipeline Implementation > 13.9 Serialization and Barriers in Pipeline",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">13.9 Serialization and Barriers in Pipeline<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">Barrier instructions (MB, WMB, EXCB, TRAPB) do not execute work — they enforce ordering. In EX stage, the grain sets slot.needsMemoryBarrier = true or slot.needsWriteBufferDrain = true. In stage_MEM(), these flags are checked: if the barrier has not completed (memoryBarrierCompleted \/ writeBufferDrained is false), the slot stalls.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">CBox is responsible for completing the barrier (draining write buffers, coordinating with other CPUs for MB). When CBox finishes, it sets the completion flag on the slot. The next tick, stage_MEM() finds the barrier complete and allows the instruction to proceed to WB.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">CALL_PAL is the strongest serialization point — it is detected in stage_WB() and triggers PipelineAction::PAL_CALL, which causes AlphaCPU to flush the entire pipeline and enter PAL mode.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: Chapter 6 – Serialization and Stall Model.<\/span><\/p>\n\r"
})
